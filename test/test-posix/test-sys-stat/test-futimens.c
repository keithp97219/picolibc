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

#define _POSIX_C_SOURCE 202405L
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "FUTIMENS.TXT"
#endif

/* futimens sets times past 2038, honoring UTIME_OMIT and UTIME_NOW */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(TEST_FILE_NAME);              \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    struct stat     st;
    struct timespec times[2];
    time_t          now;
    int             fd, fd2;

    (void)unlink(TEST_FILE_NAME);
    fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "open");

    /* A modification time past 2038, leaving the access time alone */
    check(fstat(fd, &st) == 0, "fstat before futimens");
    times[0].tv_sec = 0;
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = (time_t)2208988800LL;
    times[1].tv_nsec = 500;
    if (sizeof(time_t) >= 8) {
        struct timespec atime = st.st_atim;

        check(futimens(fd, times) == 0, "futimens");
        check(fstat(fd, &st) == 0, "fstat after futimens");
        check(st.st_mtim.tv_sec == times[1].tv_sec && st.st_mtim.tv_nsec == 500,
              "modification time in 2040");
        check(st.st_atim.tv_sec == atime.tv_sec && st.st_atim.tv_nsec == atime.tv_nsec,
              "UTIME_OMIT kept the access time");
    }
    times[0].tv_nsec = UTIME_NOW;
    times[1].tv_nsec = UTIME_NOW;
    check(futimens(fd, times) == 0, "futimens UTIME_NOW");
    check(futimens(fd, NULL) == 0, "futimens NULL");

    /* NULL sets both times to now */
    times[0].tv_sec = 1000000000;
    times[0].tv_nsec = 0;
    times[1] = times[0];
    check(futimens(fd, times) == 0, "futimens past");
    now = time(NULL);
    check(futimens(fd, NULL) == 0 && fstat(fd, &st) == 0, "futimens NULL again");
    check(st.st_atim.tv_sec >= now - 60 && st.st_atim.tv_sec <= now + 60
              && st.st_mtim.tv_sec >= now - 60 && st.st_mtim.tv_sec <= now + 60,
          "NULL is now");

    /* Invalid nanoseconds fail and change nothing */
    times[0].tv_sec = 1000000000;
    times[0].tv_nsec = -1;
    times[1].tv_sec = 1000000000;
    times[1].tv_nsec = 0;
    errno = 0;
    check(futimens(fd, times) == -1 && errno == EINVAL, "EINVAL");
    check(fstat(fd, &st) == 0 && st.st_mtim.tv_sec >= now - 60, "nothing changed");

    /* The owner can use a read-only descriptor */
    fd2 = open(TEST_FILE_NAME, O_RDONLY);
    check(fd2 >= 0, "open read-only");
    times[0].tv_nsec = 0;
    check(futimens(fd2, times) == 0 && fstat(fd2, &st) == 0 && st.st_mtim.tv_sec == 1000000000,
          "futimens read-only descriptor");
    check(close(fd2) == 0, "close read-only");

    check(close(fd) == 0, "close");
    errno = 0;
    check(futimens(fd, times) == -1 && errno == EBADF, "EBADF");
    check(unlink(TEST_FILE_NAME) == 0, "unlink");

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
