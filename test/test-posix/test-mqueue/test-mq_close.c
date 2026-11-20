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

/* mq_close */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || (defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING > 0)

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
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
    struct sigevent sev;
    mqd_t           q, other;
    int             fds[2];

    snprintf(name, sizeof(name), "/picolibc-test-mq-close-%ld", (long)getpid());
    (void)mq_unlink(name);

    q = mq_open(name, O_RDWR | O_CREAT | O_EXCL, 0600, NULL);
    check(q != (mqd_t)-1, "mq_open create");
    other = mq_open(name, O_RDWR);
    check(other != (mqd_t)-1, "mq_open existing");

    errno = 0;
    check(mq_close((mqd_t)-1) == -1 && errno == EBADF, "mq_close of -1");

#ifdef __PICOLIBC__
    /* glibc closes any file descriptor passed to mq_close */
    check(pipe(fds) == 0, "pipe");
    errno = 0;
    check(mq_close((mqd_t)fds[0]) == -1 && errno == EBADF, "mq_close of a pipe");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "pipe still open");
#else
    (void)fds;
#endif

    /* Closing the descriptor that registered for notification removes it */
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_NONE;
    if (mq_notify(q, &sev) == 0) {
        errno = 0;
        check(mq_notify(other, &sev) == -1 && errno == EBUSY, "queue already registered");
        check(mq_close(q) == 0, "mq_close registered descriptor");
        check(mq_notify(other, &sev) == 0, "registration after mq_close");
        check(mq_notify(other, NULL) == 0, "remove registration");
    } else {
        check(errno == ENOSYS, "mq_notify SIGEV_NONE");
        printf("mq_notify not supported here, notification not tested\n");
        check(mq_close(q) == 0, "mq_close");
    }

    check(mq_close(other) == 0, "mq_close other");
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
