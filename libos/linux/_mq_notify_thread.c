/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 Keith Packard
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "local-mqueue.h"
#include "local-timer.h"
#include <pthread.h>
#include <string.h>

/*
 * Linux delivers SIGEV_THREAD message queue notifications over a netlink
 * socket: the kernel sends the cookie passed in sigev_value, with its
 * last byte set to say whether a message arrived or the registration
 * was removed. A helper thread waits on the socket and calls the
 * function.
 */
#define LINUX_SIGEV_THREAD      2
#define LINUX_AF_NETLINK        16
#define LINUX_SOCK_RAW          3
#define LINUX_NETLINK_ROUTE     0
#define LINUX_MSG_WAITALL       0x100
#define LINUX_MSG_NOSIGNAL      0x4000
#define LINUX_NOTIFY_COOKIE_LEN 32
#define LINUX_NOTIFY_WOKENUP    1

static const char _mq_cookie[LINUX_NOTIFY_COOKIE_LEN];

struct _mq_notify_args {
    pthread_barrier_t      barrier;
    int                    sock;
    int                    registered;
    const struct sigevent *sev;
};

static void *
_mq_notify_wait(void *p)
{
    struct _mq_notify_args *args = p;
    char                    buf[LINUX_NOTIFY_COOKIE_LEN];
    void                    (*function)(union sigval);
    union sigval            value;
    int                     sock, registered;
    long                    n;

    /* Wait for mq_notify to register, then copy what it left for us */
    pthread_barrier_wait(&args->barrier);
    sock = args->sock;
    registered = args->registered;
    function = args->sev->sigev_notify_function;
    value = args->sev->sigev_value;
    pthread_barrier_wait(&args->barrier);

    if (registered) {
        n = _syscall_raw(LINUX_SYS_recvfrom, sock, buf, sizeof(buf),
                         LINUX_MSG_NOSIGNAL | LINUX_MSG_WAITALL, NULL, NULL);
        (void)_syscall_raw(LINUX_SYS_close, sock);
        if (n == (long)sizeof(buf) && buf[sizeof(buf) - 1] == LINUX_NOTIFY_WOKENUP)
            function(value);
    } else {
        (void)_syscall_raw(LINUX_SYS_close, sock);
    }
    return NULL;
}

int
_mq_notify_thread(mqd_t mqdes, const struct sigevent *sev)
{
    struct _mq_notify_args   args = { .sev = sev };
    struct __kernel_sigevent kev;
    pthread_attr_t           attr;
    pthread_t                thread;
    long                     ret;

    ret = _syscall_raw(LINUX_SYS_socket, LINUX_AF_NETLINK, LINUX_SOCK_RAW | LINUX_O_CLOEXEC,
                       LINUX_NETLINK_ROUTE);
    if (ret < 0) {
        errno = _syscall_errno(ret);
        return -1;
    }
    args.sock = (int)ret;

    if (sev->sigev_notify_attributes)
        attr = *(const pthread_attr_t *)sev->sigev_notify_attributes;
    else
        pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_barrier_init(&args.barrier, NULL, 2);
    if (pthread_create(&thread, &attr, _mq_notify_wait, &args) != 0) {
        (void)_syscall_raw(LINUX_SYS_close, args.sock);
        pthread_barrier_destroy(&args.barrier);
        errno = EAGAIN;
        return -1;
    }

    memset(&kev, 0, sizeof(kev));
    kev.sigev_notify = LINUX_SIGEV_THREAD;
    kev.sigev_signo = args.sock;
    kev.sigev_value.sival_ptr = (void *)_mq_cookie;
    ret = _syscall_raw(LINUX_SYS_mq_notify, mqdes, &kev);
    args.registered = ret == 0;

    /* Let the helper copy its arguments, then let it go */
    pthread_barrier_wait(&args.barrier);
    pthread_barrier_wait(&args.barrier);
    pthread_barrier_destroy(&args.barrier);

    if (ret < 0) {
        errno = _syscall_errno(ret);
        return -1;
    }
    return 0;
}
