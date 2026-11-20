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

/* mq_timedsend deadlines */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                              \
    || ((defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING > 0) \
        && (defined(_POSIX_TIMEOUTS) && _POSIX_TIMEOUTS > 0))

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

int
main(void)
{
    struct mq_attr  attr = { 0 }, got;
    struct timespec past = { 0, 0 }, before_epoch = { -1, 0 };
    struct timespec negative_nsec = { 0, -1 }, large_nsec = { 0, 1000000000L };
    char            buf[16];
    mqd_t           q;

    snprintf(name, sizeof(name), "/picolibc-test-mq-timedsend-%ld", (long)getpid());
    (void)mq_unlink(name);

    attr.mq_maxmsg = 1;
    attr.mq_msgsize = sizeof(buf);
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");

    /* With room in the queue, a deadline already passed doesn't matter */
    check(mq_timedsend(q, "a", 2, 0, &past) == 0, "room and a passed deadline");

    /* A full queue times out at once when the deadline has passed */
    errno = 0;
    check(mq_timedsend(q, "b", 2, 0, &past) == -1 && errno == ETIMEDOUT,
          "full queue and a passed deadline");

    /* Blocking with a bad tv_nsec fails */
    errno = 0;
    check(mq_timedsend(q, "b", 2, 0, &negative_nsec) == -1 && errno == EINVAL,
          "full queue and a negative tv_nsec");
    errno = 0;
    check(mq_timedsend(q, "b", 2, 0, &large_nsec) == -1 && errno == EINVAL,
          "full queue and a tv_nsec of one second");

    /* None of the failures queued anything */
    check(mq_getattr(q, &got) == 0 && got.mq_curmsgs == 1, "one message queued");

#ifndef __GLIBC__
    /*
     * glibc hands the deadline to Linux, which rejects these before
     * looking at the queue
     */

    /* A deadline before the epoch has passed */
    errno = 0;
    check(mq_timedsend(q, "b", 2, 0, &before_epoch) == -1 && errno == ETIMEDOUT,
          "full queue and a deadline before the epoch");

    /* With O_NONBLOCK set, mq_timedsend is mq_send */
    attr.mq_flags = O_NONBLOCK;
    check(mq_setattr(q, &attr, NULL) == 0, "mq_setattr O_NONBLOCK");
    errno = 0;
    check(mq_timedsend(q, "b", 2, 0, &negative_nsec) == -1 && errno == EAGAIN,
          "full non-blocking queue and a negative tv_nsec");
    attr.mq_flags = 0;
    check(mq_setattr(q, &attr, NULL) == 0, "mq_setattr blocking");

    check(mq_receive(q, buf, sizeof(buf), NULL) == 2 && strcmp(buf, "a") == 0, "receive a");
    check(mq_timedsend(q, "c", 2, 0, &before_epoch) == 0, "room and a deadline before the epoch");
#else
    (void)before_epoch;
    check(mq_receive(q, buf, sizeof(buf), NULL) == 2 && strcmp(buf, "a") == 0, "receive a");
    check(mq_send(q, "c", 2, 0) == 0, "send c");
#endif
    check(mq_receive(q, buf, sizeof(buf), NULL) == 2 && strcmp(buf, "c") == 0, "receive c");

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
