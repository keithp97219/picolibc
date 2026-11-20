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

/* mq_receive */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                 \
    || ((defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING > 0)                    \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0) \
        && (defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0))

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)mq_unlink(name);                     \
            exit(1);                                   \
        }                                              \
    } while (0)

static char name[64];

static void
on_alarm(int sig)
{
    (void)sig;
}

/* Interrupt the next blocking call with SIGALRM */
static int
interrupt_soon(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    if (sigaction(SIGALRM, &sa, NULL) != 0)
        return -1;
    alarm(1);
    return 0;
}

static void
pause_ms(long ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };

    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
        ;
}

int
main(void)
{
    struct mq_attr attr = { 0 }, got;
    char           buf[16];
    mqd_t          q, wo, nb;
    pid_t          pid;
    int            status;

    snprintf(name, sizeof(name), "/picolibc-test-mq-receive-%ld", (long)getpid());
    (void)mq_unlink(name);

    attr.mq_maxmsg = 4;
    attr.mq_msgsize = sizeof(buf);
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");

    /* A buffer smaller than mq_msgsize leaves the message queued */
    check(mq_send(q, "abc", 3, 0) == 0, "send abc");
    errno = 0;
    check(mq_receive(q, buf, sizeof(buf) - 1, NULL) == -1 && errno == EMSGSIZE,
          "receive into a buffer smaller than mq_msgsize");
    check(mq_getattr(q, &got) == 0 && got.mq_curmsgs == 1, "message still queued");

    /* Receiving needs a descriptor open for reading */
    wo = mq_open(name, O_WRONLY);
    check(wo != (mqd_t)-1, "mq_open O_WRONLY");
    errno = 0;
    check(mq_receive(wo, buf, sizeof(buf), NULL) == -1 && errno == EBADF,
          "receive on a write-only descriptor");
    check(mq_close(wo) == 0, "mq_close write-only");

    /* The result is the message length; msg_prio may be NULL */
    memset(buf, 0, sizeof(buf));
    check(mq_receive(q, buf, sizeof(buf), NULL) == 3 && memcmp(buf, "abc", 3) == 0, "receive abc");

    /* An empty non-blocking queue has nothing to receive */
    nb = mq_open(name, O_RDONLY | O_NONBLOCK);
    check(nb != (mqd_t)-1, "mq_open O_NONBLOCK");
    errno = 0;
    check(mq_receive(nb, buf, sizeof(buf), NULL) == -1 && errno == EAGAIN,
          "receive from an empty non-blocking queue");
    check(mq_close(nb) == 0, "mq_close non-blocking");

    /* A blocked receive is interrupted by a signal */
    check(interrupt_soon() == 0, "sigaction SIGALRM");
    errno = 0;
    check(mq_receive(q, buf, sizeof(buf), NULL) == -1 && errno == EINTR,
          "receive interrupted by a signal");

    /* A blocked receive gets a message sent by another process */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        pause_ms(100);
        _exit(mq_send(q, "late", 5, 3) == 0 ? 0 : 1);
    }
    check(mq_receive(q, buf, sizeof(buf), NULL) == 5 && strcmp(buf, "late") == 0,
          "receive the late message");
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child send");

    check(mq_close(q) == 0, "mq_close");
    check(mq_unlink(name) == 0, "mq_unlink");

    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
