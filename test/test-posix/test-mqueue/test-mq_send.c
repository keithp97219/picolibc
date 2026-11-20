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

/* mq_send */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                 \
    || ((defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING > 0)                    \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0) \
        && (defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0))

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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
    unsigned       prio;
    long           prio_max;
    mqd_t          q, ro, nb;
    pid_t          pid;
    int            status;

    snprintf(name, sizeof(name), "/picolibc-test-mq-send-%ld", (long)getpid());
    (void)mq_unlink(name);

    attr.mq_maxmsg = 4;
    attr.mq_msgsize = sizeof(buf);
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");

    /* Higher priorities first, equal priorities in the order sent */
    check(mq_send(q, "a", 2, 1) == 0, "send a");
    check(mq_send(q, "b", 2, 2) == 0, "send b");
    check(mq_send(q, "c", 2, 1) == 0, "send c");
    check(mq_send(q, "d", 2, 2) == 0, "send d");
    for (const char *expect = "bdac"; *expect; expect++) {
        check(mq_receive(q, buf, sizeof(buf), &prio) == 2 && buf[0] == *expect, "message order");
        check(prio == (*expect == 'b' || *expect == 'd' ? 2U : 1U), "message priority");
    }

    /* Priorities run from zero to {MQ_PRIO_MAX} - 1 */
    prio_max = sysconf(_SC_MQ_PRIO_MAX);
    check(prio_max >= _POSIX_MQ_PRIO_MAX, "sysconf(_SC_MQ_PRIO_MAX)");
    check(mq_send(q, "e", 2, (unsigned)(prio_max - 1)) == 0, "send at the highest priority");
    errno = 0;
    check(mq_send(q, "f", 2, (unsigned)prio_max) == -1 && errno == EINVAL,
          "send above the highest priority");
    check(mq_receive(q, buf, sizeof(buf), &prio) == 2 && prio == (unsigned)(prio_max - 1),
          "receive at the highest priority");

    /* Messages may be as long as mq_msgsize, and empty */
    check(mq_send(q, "0123456789abcdef", sizeof(buf), 0) == 0, "send mq_msgsize bytes");
    errno = 0;
    check(mq_send(q, "0123456789abcdefg", sizeof(buf) + 1, 0) == -1 && errno == EMSGSIZE,
          "send more than mq_msgsize bytes");
    check(mq_send(q, "", 0, 0) == 0, "send an empty message");
    check(mq_receive(q, buf, sizeof(buf), NULL) == (ssize_t)sizeof(buf),
          "receive mq_msgsize bytes");
    check(mq_receive(q, buf, sizeof(buf), NULL) == 0, "receive an empty message");

    /* Sending needs a descriptor open for writing */
    ro = mq_open(name, O_RDONLY);
    check(ro != (mqd_t)-1, "mq_open O_RDONLY");
    errno = 0;
    check(mq_send(ro, "g", 2, 0) == -1 && errno == EBADF, "send on a read-only descriptor");
    check(mq_close(ro) == 0, "mq_close read-only");

    /* A full non-blocking queue refuses the message */
    nb = mq_open(name, O_WRONLY | O_NONBLOCK);
    check(nb != (mqd_t)-1, "mq_open O_NONBLOCK");
    for (int i = 0; i < 4; i++)
        check(mq_send(nb, "h", 2, 0) == 0, "fill the queue");
    errno = 0;
    check(mq_send(nb, "i", 2, 9) == -1 && errno == EAGAIN, "send to a full non-blocking queue");
    check(mq_close(nb) == 0, "mq_close non-blocking");

    /* A blocked send is interrupted by a signal */
    check(interrupt_soon() == 0, "sigaction SIGALRM");
    errno = 0;
    check(mq_send(q, "j", 2, 9) == -1 && errno == EINTR, "send interrupted by a signal");
    check(mq_getattr(q, &got) == 0 && got.mq_curmsgs == 4, "failed sends queue nothing");

    /* A blocked send finishes once another process makes room */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        pause_ms(100);
        _exit(mq_receive(q, buf, sizeof(buf), NULL) == 2 ? 0 : 1);
    }
    check(mq_send(q, "k", 2, 9) == 0, "send after room is made");
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child receive");
    check(mq_receive(q, buf, sizeof(buf), &prio) == 2 && strcmp(buf, "k") == 0 && prio == 9,
          "receive the blocked message");

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
