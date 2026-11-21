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

/* mq_setattr and mq_getattr */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if _POSIX_MESSAGE_PASSING > 0

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
    struct mq_attr attr = { 0 }, set, old, got;
    mqd_t          q, other;

    snprintf(name, sizeof(name), "/picolibc-test-mq-setattr-%ld", (long)getpid());
    (void)mq_unlink(name);

    attr.mq_maxmsg = 3;
    attr.mq_msgsize = 16;
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");
    other = mq_open(name, O_RDWR);
    check(other != (mqd_t)-1, "mq_open existing");
    check(mq_send(q, "a", 2, 0) == 0 && mq_send(q, "b", 2, 0) == 0, "send two");

    /* Only mq_flags changes; omqstat gets what mq_getattr would have */
    check(mq_getattr(q, &got) == 0, "mq_getattr");
    set.mq_flags = O_NONBLOCK;
    set.mq_maxmsg = 100;
    set.mq_msgsize = 100;
    set.mq_curmsgs = 100;
    memset(&old, 0xa5, sizeof(old));
    check(mq_setattr(q, &set, &old) == 0, "mq_setattr");
    check(old.mq_flags == got.mq_flags && old.mq_maxmsg == got.mq_maxmsg
              && old.mq_msgsize == got.mq_msgsize && old.mq_curmsgs == got.mq_curmsgs,
          "previous attributes");
    check(old.mq_flags == 0 && old.mq_maxmsg == 3 && old.mq_msgsize == 16 && old.mq_curmsgs == 2,
          "previous attribute values");
    check(mq_getattr(q, &got) == 0, "mq_getattr after mq_setattr");
    check(got.mq_flags == O_NONBLOCK && got.mq_maxmsg == 3 && got.mq_msgsize == 16
              && got.mq_curmsgs == 2,
          "only mq_flags changed");

    /* The flags belong to the open description */
    check(mq_getattr(other, &got) == 0 && got.mq_flags == 0, "other descriptor unchanged");

    set.mq_flags = 0;
    check(mq_setattr(q, &set, NULL) == 0, "mq_setattr without omqstat");
    check(mq_getattr(q, &got) == 0 && got.mq_flags == 0, "O_NONBLOCK cleared");

    errno = 0;
    check(mq_setattr((mqd_t)-1, &set, NULL) == -1 && errno == EBADF, "mq_setattr of -1");
    errno = 0;
    check(mq_getattr((mqd_t)-1, &got) == -1 && errno == EBADF, "mq_getattr of -1");

    check(mq_close(other) == 0 && mq_close(q) == 0, "mq_close");
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
