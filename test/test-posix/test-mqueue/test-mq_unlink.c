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

/* mq_unlink */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || (defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING > 0)

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    struct mq_attr attr = { 0 }, got;
    char           buf[16];
    mqd_t          q, fresh;

    snprintf(name, sizeof(name), "/picolibc-test-mq-unlink-%ld", (long)getpid());
    (void)mq_unlink(name);

    errno = 0;
    check(mq_unlink(name) == -1 && errno == ENOENT, "unlink a missing queue");

    attr.mq_maxmsg = 2;
    attr.mq_msgsize = sizeof(buf);
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");
    check(mq_send(q, "a", 2, 0) == 0, "send a");
    check(mq_unlink(name) == 0, "mq_unlink");

    /* The name is gone */
    errno = 0;
    check(mq_open(name, O_RDWR) == (mqd_t)-1 && errno == ENOENT, "open after unlink");

    /* Reusing the name makes a new, empty queue */
    fresh = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(fresh != (mqd_t)-1, "mq_open create after unlink");
    check(mq_getattr(fresh, &got) == 0 && got.mq_curmsgs == 0, "new queue is empty");
    check(mq_close(fresh) == 0, "mq_close new queue");
    check(mq_unlink(name) == 0, "mq_unlink new queue");

    /* The unlinked queue lives on while it is open */
    check(mq_getattr(q, &got) == 0 && got.mq_curmsgs == 1, "unlinked queue keeps its message");
    check(mq_send(q, "b", 2, 0) == 0, "send b");
    check(mq_receive(q, buf, sizeof(buf), NULL) == 2 && strcmp(buf, "a") == 0, "receive a");
    check(mq_receive(q, buf, sizeof(buf), NULL) == 2 && strcmp(buf, "b") == 0, "receive b");
    check(mq_close(q) == 0, "mq_close");

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
