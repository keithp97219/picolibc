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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_ADVISORY_INFO && TEST_POSIX_MAPPED_FILES && TEST_POSIX_FILE_SYSTEM

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "../is-qemu.h"

/* posix_madvise: advice never changes what the memory holds; errors are returned */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-posix_madvise.txt"
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

static const struct {
    const char *name;
    int         advice;
} advices[] = {
    { "POSIX_MADV_NORMAL",     POSIX_MADV_NORMAL     },
    { "POSIX_MADV_SEQUENTIAL", POSIX_MADV_SEQUENTIAL },
    { "POSIX_MADV_RANDOM",     POSIX_MADV_RANDOM     },
    { "POSIX_MADV_WILLNEED",   POSIX_MADV_WILLNEED   },
    { "POSIX_MADV_DONTNEED",   POSIX_MADV_DONTNEED   },
};

#define NADVICE (sizeof(advices) / sizeof(advices[0]))

/* Every byte of p[0..len) is c */
static bool
all(const char *p, size_t len, char c)
{
    for (size_t i = 0; i < len; i++)
        if (p[i] != c)
            return false;
    return true;
}

int
main(void)
{
    long   ps = sysconf(_SC_PAGESIZE);
    bool   qemu = is_qemu();
    size_t page;
    char  *anon, *priv;
    int    fd, bad, ret;

    check(ps > 0, "_SC_PAGESIZE");
    page = (size_t)ps;
    snprintf(file, sizeof(file), "%s-%ld", TEST_FILE_NAME, (long)getpid());

    anon = mmap(NULL, 4 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(anon != MAP_FAILED, "mmap anonymous");

    (void)unlink(file);
    fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0, "create");
    for (int i = 0; i < 4; i++)
        check(write(fd, anon, page) == (ssize_t)page, "write");
    priv = mmap(NULL, 4 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    check(priv != MAP_FAILED, "mmap file");
    check(close(fd) == 0, "close");

    /* Each piece of advice leaves the contents alone, including private changes */
    for (size_t i = 0; i < NADVICE; i++) {
        char c = (char)('a' + i);

        printf("%s\n", advices[i].name);
        memset(anon, c, 4 * page);
        memset(priv, c, 4 * page);
        check(posix_madvise(anon, 4 * page, advices[i].advice) == 0, "anonymous memory");
        check(posix_madvise(priv, 4 * page, advices[i].advice) == 0, "file mapping");
        check(posix_madvise(anon + page, 1, advices[i].advice) == 0, "partial page");
        check(all(anon, 4 * page, c), "anonymous contents kept");
        check(all(priv, 4 * page, c), "private file contents kept");
    }

    /* Errors are returned, not reported with errno */
    for (bad = 0; bad == POSIX_MADV_NORMAL || bad == POSIX_MADV_SEQUENTIAL
         || bad == POSIX_MADV_RANDOM || bad == POSIX_MADV_WILLNEED || bad == POSIX_MADV_DONTNEED;
         bad++)
        ;
    check(posix_madvise(anon, page, bad) == EINVAL, "unknown advice");
    check(posix_madvise(anon, page, -1) == EINVAL, "negative advice");
    check(posix_madvise(anon + 1, page, POSIX_MADV_NORMAL) == EINVAL, "address not page aligned");

    check(munmap(anon + page, page) == 0, "munmap middle page");
    for (size_t i = 0; i < NADVICE; i++) {
        /* glibc 2.43 ignores POSIX_MADV_DONTNEED without looking at the range */
#ifdef __GLIBC__
        if (advices[i].advice == POSIX_MADV_DONTNEED)
            continue;
#endif
        printf("%s on a hole\n", advices[i].name);
        ret = posix_madvise(anon, 3 * page, advices[i].advice);
        if (qemu && ret == 0) {
            printf("qemu accepted advice for a hole, skipping\n");
            continue;
        }
        check(ret == ENOMEM, "range with a hole");
        check(all(anon, page, 'a' + (char)(NADVICE - 1)), "contents before the hole kept");
    }

    check(munmap(anon, 4 * page) == 0, "munmap");
    check(munmap(priv, 4 * page) == 0, "munmap");
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
