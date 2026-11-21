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

/* mq_timedreceive deadlines */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if _POSIX_MESSAGE_PASSING > 0 && _POSIX_TIMEOUTS > 0

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int
before(const struct timespec *a, const struct timespec *b)
{
    return a->tv_sec < b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec < b->tv_nsec);
}

int
main(void)
{
    struct mq_attr  attr = { 0 };
    struct timespec past = { 0, 0 }, before_epoch = { -1, 0 }, deadline, now;
    struct timespec negative_nsec = { 0, -1 }, large_nsec = { 0, 1000000000L };
    char            buf[16];
    mqd_t           q;

    snprintf(name, sizeof(name), "/picolibc-test-mq-timedreceive-%ld", (long)getpid());
    (void)mq_unlink(name);

    attr.mq_maxmsg = 2;
    attr.mq_msgsize = sizeof(buf);
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");

    /* An empty queue times out at once when the deadline has passed */
    errno = 0;
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &past) == -1 && errno == ETIMEDOUT,
          "empty queue and a passed deadline");

    /* The timeout doesn't expire before the CLOCK_REALTIME deadline */
    check(clock_gettime(CLOCK_REALTIME, &deadline) == 0, "clock_gettime");
    deadline.tv_nsec += 50000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    errno = 0;
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &deadline) == -1 && errno == ETIMEDOUT,
          "empty queue and a future deadline");
    check(clock_gettime(CLOCK_REALTIME, &now) == 0, "clock_gettime");
    check(!before(&now, &deadline), "timed out after the deadline");

    /* Blocking with a bad tv_nsec fails */
    errno = 0;
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &negative_nsec) == -1 && errno == EINVAL,
          "empty queue and a negative tv_nsec");
    errno = 0;
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &large_nsec) == -1 && errno == EINVAL,
          "empty queue and a tv_nsec of one second");

    /* With a message waiting, a deadline already passed doesn't matter */
    check(mq_send(q, "a", 2, 0) == 0, "send a");
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &past) == 2 && strcmp(buf, "a") == 0,
          "message waiting and a passed deadline");

#ifndef __GLIBC__
    /*
     * glibc hands the deadline to Linux, which rejects these before
     * looking at the queue
     */

    /* A deadline before the epoch has passed */
    errno = 0;
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &before_epoch) == -1 && errno == ETIMEDOUT,
          "empty queue and a deadline before the epoch");
    check(mq_send(q, "b", 2, 0) == 0, "send b");
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &before_epoch) == 2 && strcmp(buf, "b") == 0,
          "message waiting and a deadline before the epoch");

    /* With O_NONBLOCK set, mq_timedreceive is mq_receive */
    attr.mq_flags = O_NONBLOCK;
    check(mq_setattr(q, &attr, NULL) == 0, "mq_setattr O_NONBLOCK");
    errno = 0;
    check(mq_timedreceive(q, buf, sizeof(buf), NULL, &negative_nsec) == -1 && errno == EAGAIN,
          "empty non-blocking queue and a negative tv_nsec");
#else
    (void)before_epoch;
#endif

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
