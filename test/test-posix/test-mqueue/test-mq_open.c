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

/* mq_open */

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
    struct mq_attr attr = { 0 }, other = { 0 }, got;
    char           buf[16];
    mqd_t          q, again, nb, ro;

    snprintf(name, sizeof(name), "/picolibc-test-mq-open-%ld", (long)getpid());
    (void)mq_unlink(name);

    errno = 0;
    check(mq_open(name, O_RDWR) == (mqd_t)-1 && errno == ENOENT, "open without O_CREAT");

    /* Both sizes must be positive */
    attr.mq_maxmsg = 0;
    attr.mq_msgsize = sizeof(buf);
    errno = 0;
    check(mq_open(name, O_RDWR | O_CREAT, 0600, &attr) == (mqd_t)-1 && errno == EINVAL,
          "mq_maxmsg of zero");
    attr.mq_maxmsg = -1;
    errno = 0;
    check(mq_open(name, O_RDWR | O_CREAT, 0600, &attr) == (mqd_t)-1 && errno == EINVAL,
          "negative mq_maxmsg");
    attr.mq_maxmsg = 2;
    attr.mq_msgsize = 0;
    errno = 0;
    check(mq_open(name, O_RDWR | O_CREAT, 0600, &attr) == (mqd_t)-1 && errno == EINVAL,
          "mq_msgsize of zero");
    attr.mq_msgsize = -1;
    errno = 0;
    check(mq_open(name, O_RDWR | O_CREAT, 0600, &attr) == (mqd_t)-1 && errno == EINVAL,
          "negative mq_msgsize");

    attr.mq_msgsize = sizeof(buf);
    q = mq_open(name, O_RDWR | O_CREAT, 0600, &attr);
    check(q != (mqd_t)-1, "mq_open create");
    check(mq_send(q, "a", 2, 0) == 0, "send a");

    /* O_CREAT without O_EXCL opens the existing queue, messages and attributes intact */
    other.mq_maxmsg = 3;
    other.mq_msgsize = sizeof(buf) * 2;
    again = mq_open(name, O_RDWR | O_CREAT, 0600, &other);
    check(again != (mqd_t)-1, "mq_open O_CREAT of an existing queue");
    check(mq_getattr(again, &got) == 0, "mq_getattr");
    check(got.mq_maxmsg == 2 && got.mq_msgsize == (long)sizeof(buf) && got.mq_curmsgs == 1,
          "attributes of the existing queue");
    check(mq_receive(again, buf, sizeof(buf), NULL) == 2 && strcmp(buf, "a") == 0, "receive a");

    /* O_NONBLOCK belongs to each open description */
    nb = mq_open(name, O_RDWR | O_NONBLOCK);
    check(nb != (mqd_t)-1, "mq_open O_NONBLOCK");
    check(mq_getattr(nb, &got) == 0 && got.mq_flags == O_NONBLOCK, "O_NONBLOCK from mq_open");
    check(mq_getattr(q, &got) == 0 && got.mq_flags == 0, "other descriptor still blocking");
    check(mq_close(nb) == 0 && mq_close(again) == 0 && mq_close(q) == 0, "mq_close");
    check(mq_unlink(name) == 0, "mq_unlink");

    /* Default attributes */
    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, NULL);
    check(q != (mqd_t)-1, "mq_open with default attributes");
    check(mq_getattr(q, &got) == 0 && got.mq_maxmsg > 0 && got.mq_msgsize > 0 && got.mq_curmsgs == 0
              && got.mq_flags == 0,
          "default attributes");
    check(mq_close(q) == 0, "mq_close");
    check(mq_unlink(name) == 0, "mq_unlink");

    /* The mode limits access, except for a privileged process */
    q = mq_open(name, O_RDONLY | O_CREAT | O_EXCL, 0400, &attr);
    check(q != (mqd_t)-1, "mq_open read-only mode");
    ro = mq_open(name, O_RDONLY);
    check(ro != (mqd_t)-1, "mq_open O_RDONLY");
    if (geteuid() != 0) {
        errno = 0;
        check(mq_open(name, O_WRONLY) == (mqd_t)-1 && errno == EACCES, "mq_open O_WRONLY");
        errno = 0;
        check(mq_open(name, O_RDWR) == (mqd_t)-1 && errno == EACCES, "mq_open O_RDWR");
    }
    check(mq_close(ro) == 0 && mq_close(q) == 0, "mq_close");
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
