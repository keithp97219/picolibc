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
/* utimes is an XSI interface */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                         \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>

/* utimes sets file access and modification times */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

int
main(void)
{
    struct timeval tv[2] = {
        { 1000000000, 500000 },
        { 1100000000, 250000 }
    };
    struct stat st;
    int         fd;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create file");
    check(utimes("file", tv) == 0, "utimes");
    check(stat("file", &st) == 0, "stat");
    check(st.st_atim.tv_sec == 1000000000 && st.st_atim.tv_nsec == 500000000, "atime");
    check(st.st_mtim.tv_sec == 1100000000 && st.st_mtim.tv_nsec == 250000000, "mtime");
    check(utimes("file", NULL) == 0 && stat("file", &st) == 0 && st.st_mtim.tv_sec > 1100000000,
          "now");
    check(utimes("missing", tv) == -1 && errno == ENOENT, "ENOENT");
    check(utimes("", tv) == -1 && errno == ENOENT, "empty path");
    check(utimes("file/", tv) == -1 && errno == ENOTDIR, "trailing slash on a file");

    /* EINVAL for microseconds out of range; the times are unchanged */
    {
        /*
         * 4294968 microseconds is 704 nanoseconds once multiplied in a
         * 32-bit long; 32-bit glibc makes that mistake
         */
        static const suseconds_t bad[] = { -1, 1000000,
#ifndef __GLIBC__
                                           4294968
#endif
        };
        struct timeval btv[2];
        unsigned       i;

        check(utimes("file", tv) == 0, "utimes");
        for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
            btv[0] = tv[0];
            btv[1] = tv[1];
            btv[i & 1].tv_usec = bad[i];
            errno = 0;
            check(utimes("file", btv) == -1 && errno == EINVAL, "EINVAL for tv_usec");
        }
        check(stat("file", &st) == 0 && st.st_atim.tv_sec == 1000000000
                  && st.st_mtim.tv_sec == 1100000000,
              "times unchanged after EINVAL");
    }
    check(unlink("file") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
