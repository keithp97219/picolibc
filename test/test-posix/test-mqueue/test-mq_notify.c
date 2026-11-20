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

/* POSIX message queues */

#define _DEFAULT_SOURCE
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                 \
    || ((defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING > 0)                    \
        && (defined(_POSIX_TIMEOUTS) && _POSIX_TIMEOUTS > 0)                               \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0) \
        && (defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0))

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)mq_unlink(name);                     \
            exit(1);                                   \
        }                                              \
    } while (0)

static char         name[64];
static volatile int got_signal, got_thread, thread_value;

static void
on_usr1(int sig, siginfo_t *info, void *context)
{
    (void)context;
    if (sig == SIGUSR1 && info->si_code == SI_MESGQ && info->si_value.sival_int == 42)
        got_signal = 1;
}

static void
notified(union sigval value)
{
    thread_value = value.sival_int;
    got_thread = 1;
}

static struct timespec
after_ms(long ms)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += ms * 1000000L;
    ts.tv_sec += ts.tv_nsec / 1000000000L;
    ts.tv_nsec %= 1000000000L;
    return ts;
}

static int
wait_for(volatile int *flag)
{
    struct timespec ten_ms = { 0, 10000000L };

    for (int i = 0; i < 300 && !*flag; i++)
        nanosleep(&ten_ms, NULL);
    return *flag;
}

/* Give a notification that shouldn't come time to arrive */
static void
settle(void)
{
    struct timespec hundred_ms = { 0, 100000000L };

    nanosleep(&hundred_ms, NULL);
}

int
main(void)
{
    struct mq_attr   attr = { 0 }, got;
    struct sigevent  sev, sig_sev;
    struct sigaction sa;
    struct timespec  deadline;
    char             buf[64];
    unsigned         prio;
    mqd_t            q;
    pid_t            pid;
    int              status;

    snprintf(name, sizeof(name), "/picolibc-test-mq-%ld", (long)getpid());
    (void)mq_unlink(name);

    errno = 0;
    check(mq_open("no-slash", O_RDWR | O_CREAT, 0600, NULL) == (mqd_t)-1 && errno == EINVAL,
          "name without a leading slash");

    attr.mq_maxmsg = 4;
    attr.mq_msgsize = 64;
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");
    errno = 0;
    check(mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr) == (mqd_t)-1 && errno == EEXIST,
          "exclusive create of an existing queue");

    check(mq_getattr(q, &got) == 0, "mq_getattr");
    check(got.mq_maxmsg == 4 && got.mq_msgsize == 64 && got.mq_curmsgs == 0 && got.mq_flags == 0,
          "attributes");

    /* Messages come out highest priority first */
    check(mq_send(q, "one", 4, 1) == 0, "send one");
    check(mq_send(q, "five", 5, 5) == 0, "send five");
    check(mq_send(q, "three", 6, 3) == 0, "send three");
    check(mq_getattr(q, &got) == 0 && got.mq_curmsgs == 3, "three queued");
    check(mq_receive(q, buf, sizeof(buf), &prio) == 5 && prio == 5 && strcmp(buf, "five") == 0,
          "receive five");
    check(mq_receive(q, buf, sizeof(buf), &prio) == 6 && prio == 3 && strcmp(buf, "three") == 0,
          "receive three");
    check(mq_receive(q, buf, sizeof(buf), &prio) == 4 && prio == 1 && strcmp(buf, "one") == 0,
          "receive one");
    errno = 0;
    check(mq_receive(q, buf, 8, NULL) == -1 && errno == EMSGSIZE, "buffer smaller than mq_msgsize");

    /* Non-blocking mode */
    attr.mq_flags = O_NONBLOCK;
    check(mq_setattr(q, &attr, &got) == 0 && got.mq_flags == 0, "mq_setattr O_NONBLOCK");
    check(mq_getattr(q, &got) == 0 && got.mq_flags == O_NONBLOCK, "O_NONBLOCK set");
    errno = 0;
    check(mq_receive(q, buf, sizeof(buf), NULL) == -1 && errno == EAGAIN, "empty, non-blocking");
    attr.mq_flags = 0;
    check(mq_setattr(q, &attr, NULL) == 0, "mq_setattr blocking");

    /* Timeouts */
    deadline = after_ms(50);
    errno = 0;
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &deadline) == -1 && errno == ETIMEDOUT,
          "timed receive on an empty queue");
    for (int i = 0; i < 4; i++)
        check(mq_send(q, "x", 2, 0) == 0, "fill the queue");
    deadline = after_ms(50);
    errno = 0;
    check(mq_timedsend(q, "x", 2, 0, &deadline) == -1 && errno == ETIMEDOUT,
          "timed send to a full queue");
    for (int i = 0; i < 4; i++)
        check(mq_receive(q, buf, sizeof(buf), NULL) == 2, "drain the queue");

    /* A deadline past 2038 must reach the kernel intact */
    if (sizeof(time_t) >= 8) {
        deadline.tv_sec = (time_t)2208988800LL;
        deadline.tv_nsec = 0;
        check(mq_send(q, "late", 5, 0) == 0, "send late");
        check(mq_timedreceive(q, buf, sizeof(buf), NULL, &deadline) == 5,
              "timed receive with a deadline in 2040");
    }

    /* Notification, where the system has it (qemu-user doesn't) */
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_NONE;
    if (mq_notify(q, &sev) == 0) {
        check(mq_notify(q, NULL) == 0, "remove the registration");

        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = on_usr1;
        sa.sa_flags = SA_SIGINFO;
        check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGUSR1;
        sev.sigev_value.sival_int = 42;
        check(mq_notify(q, &sev) == 0, "mq_notify SIGEV_SIGNAL");
        errno = 0;
        check(mq_notify(q, &sev) == -1 && errno == EBUSY, "second registration");
        check(mq_send(q, "ping", 5, 0) == 0, "send for the signal");
        check(wait_for(&got_signal), "signal notification");
        check(mq_receive(q, buf, sizeof(buf), NULL) == 5, "receive after the signal");

        memset(&sev, 0, sizeof(sev));
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = notified;
        sev.sigev_value.sival_int = 7;
        check(mq_notify(q, &sev) == 0, "mq_notify SIGEV_THREAD");
        check(mq_send(q, "ping", 5, 0) == 0, "send for the thread");
        check(wait_for(&got_thread) && thread_value == 7, "thread notification");
        check(mq_receive(q, buf, sizeof(buf), NULL) == 5, "receive after the thread");

        memset(&sig_sev, 0, sizeof(sig_sev));
        sig_sev.sigev_notify = SIGEV_SIGNAL;
        sig_sev.sigev_signo = SIGUSR1;
        sig_sev.sigev_value.sival_int = 42;

        errno = 0;
        check(mq_notify((mqd_t)-1, &sig_sev) == -1 && errno == EBADF, "mq_notify of -1");

        /* Only a message arriving at an empty queue is announced */
        got_signal = 0;
        check(mq_send(q, "full", 5, 0) == 0, "send to an empty queue");
        check(mq_notify(q, &sig_sev) == 0, "register on a non-empty queue");
        check(mq_send(q, "more", 5, 0) == 0, "send to a non-empty queue");
        settle();
        check(!got_signal, "no signal for a non-empty queue");
        check(mq_receive(q, buf, sizeof(buf), NULL) == 5, "receive full");
        check(mq_receive(q, buf, sizeof(buf), NULL) == 5, "receive more");
        check(mq_send(q, "ping", 5, 0) == 0, "send to the emptied queue");
        check(wait_for(&got_signal), "signal for the emptied queue");
        check(mq_receive(q, buf, sizeof(buf), NULL) == 5, "receive ping");

        /* Another process can't register while this one is */
        check(mq_notify(q, &sig_sev) == 0, "register again");
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0)
            _exit(mq_notify(q, &sig_sev) == -1 && errno == EBUSY ? 0 : 1);
        check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
              "registration from another process");

        /* A blocked receiver takes the message; the registration stays */
        got_signal = 0;
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0) {
            settle();
            settle();
            _exit(mq_send(q, "wake", 5, 0) == 0 ? 0 : 1);
        }
        check(mq_receive(q, buf, sizeof(buf), NULL) == 5 && strcmp(buf, "wake") == 0,
              "blocked receive");
        check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
              "child send");
        settle();
        check(!got_signal, "no signal with a blocked receiver");
        errno = 0;
        check(mq_notify(q, &sig_sev) == -1 && errno == EBUSY, "registration remains");
        check(mq_notify(q, NULL) == 0, "remove the registration");
        check(mq_send(q, "quiet", 6, 0) == 0, "send after removal");
        settle();
        check(!got_signal, "no signal after removal");
        check(mq_receive(q, buf, sizeof(buf), NULL) == 6, "receive quiet");
    } else {
        check(errno == ENOSYS, "mq_notify SIGEV_NONE");
        printf("mq_notify not supported here, notification not tested\n");
    }

    check(mq_close(q) == 0, "mq_close");
    check(mq_unlink(name) == 0, "mq_unlink");
    errno = 0;
    check(mq_open(name, O_RDONLY) == (mqd_t)-1 && errno == ENOENT, "open after unlink");
    errno = 0;
    check(mq_unlink(name) == -1 && errno == ENOENT, "unlink twice");

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
