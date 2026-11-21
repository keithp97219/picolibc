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
#if _POSIX_MAPPED_FILES > 0 && _POSIX_SYNCHRONIZED_IO > 0 && defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

/* msync: flags, what gets written, timestamps and errors */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-msync.txt"
#endif

static char file[256];

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            if (file[0])                               \
                (void)unlink(file);                    \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    long            ps = sysconf(_SC_PAGESIZE);
    size_t          page;
    struct timespec old[2];
    struct stat     st;
    char           *p;
    char            buf[8];
    int             fd, bad;

    check(ps > 0, "_SC_PAGESIZE");
    page = (size_t)ps;
    snprintf(file, sizeof(file), "%s-%ld", TEST_FILE_NAME, (long)getpid());

    (void)unlink(file);
    fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0, "create");
    check(write(fd, "abcdefgh", 8) == 8, "write");

    /* Writing through the mapping marks the times for update by msync */
    check(clock_gettime(CLOCK_REALTIME, &old[0]) == 0, "clock_gettime");
    old[0].tv_sec -= 3600;
    old[1] = old[0];
    check(futimens(fd, old) == 0, "futimens");
    p = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    check(p != MAP_FAILED, "mmap");
    p[0] = 'A';
    check(msync(p, page, MS_ASYNC) == 0, "MS_ASYNC");
    check(msync(p, 8, MS_SYNC) == 0, "MS_SYNC");
    check(fstat(fd, &st) == 0, "fstat");
    check(st.st_mtime > old[0].tv_sec && st.st_ctime > old[0].tv_sec, "times updated");
    check(pread(fd, buf, 8, 0) == 8 && memcmp(buf, "Abcdefgh", 8) == 0, "written");
    check(msync(p, page, MS_SYNC | MS_INVALIDATE) == 0, "MS_INVALIDATE");
    check(pwrite(fd, "B", 1, 1) == 1, "pwrite");
    check(p[1] == 'B', "mapping sees the file");

    /* Errors */
    for (bad = 1; bad & (MS_ASYNC | MS_SYNC | MS_INVALIDATE); bad <<= 1)
        ;
    errno = 0;
    check(msync(p, page, MS_SYNC | bad) == -1 && errno == EINVAL, "unknown flag");
    errno = 0;
    check(msync(p, page, MS_ASYNC | MS_SYNC) == -1 && errno == EINVAL, "MS_ASYNC and MS_SYNC");
    errno = 0;
    check(msync(p + 1, page, MS_SYNC) == -1 && errno == EINVAL, "address not page aligned");

    /* MS_INVALIDATE of locked pages, where this process may lock memory */
    if (mlock(p, page) == 0) {
        errno = 0;
        check(msync(p, page, MS_SYNC | MS_INVALIDATE) == -1 && errno == EBUSY,
              "MS_INVALIDATE of locked pages");
        check(munlock(p, page) == 0, "munlock");
    } else {
        printf("can't lock memory (errno %d), skipping EBUSY\n", errno);
    }
    check(munmap(p, page) == 0, "munmap");
    errno = 0;
    check(msync(p, page, MS_SYNC) == -1 && errno == ENOMEM, "unmapped");

    /* MAP_PRIVATE changes are never written */
    p = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    check(p != MAP_FAILED, "mmap MAP_PRIVATE");
    p[2] = 'C';
    check(msync(p, page, MS_SYNC) == 0, "msync MAP_PRIVATE");
    check(pread(fd, buf, 8, 0) == 8 && memcmp(buf, "ABcdefgh", 8) == 0, "private not written");
    check(munmap(p, page) == 0, "munmap");

    check(close(fd) == 0, "close");
    (void)unlink(file);
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
