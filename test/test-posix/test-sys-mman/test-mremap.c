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

#define _GNU_SOURCE
#include <unistd.h>
#include "../test-flags.h"

#include <stdio.h>

/*
 * Linux's mremap (glibc and musl have it; SQLite uses it on Linux
 * whenever _GNU_SOURCE is defined): grow in place or move, keeping
 * the contents, shrink, move to a fixed address, and the errors
 */

#if defined(__linux__) && TEST_POSIX_MAPPED_FILES && TEST_POSIX_MEMORY_PROTECTION

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
filled(const char *p, size_t len, char c)
{
    for (size_t i = 0; i < len; i++)
        if (p[i] != c)
            return 0;
    return 1;
}

int
main(void)
{
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    char  *p, *q, *r, *hole;

    p = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap");
    memset(p, 'a', page);

    /* Growing may move the mapping; the contents come along, the rest is zero */
    q = mremap(p, page, 4 * page, MREMAP_MAYMOVE);
    check(q != MAP_FAILED, "mremap grow");
    check(filled(q, page, 'a'), "contents kept");
    check(filled(q + page, 3 * page, 0), "new pages zero");
    memset(q + page, 'b', 3 * page);

    /* Shrinking stays in place */
    r = mremap(q, 4 * page, 2 * page, 0);
    check(r == q, "mremap shrink");
    check(filled(r, page, 'a') && filled(r + page, page, 'b'), "contents after shrink");

    /* Moving to a fixed address replaces what is there */
    hole = mmap(NULL, 2 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(hole != MAP_FAILED, "mmap destination");
    q = mremap(r, 2 * page, 2 * page, MREMAP_MAYMOVE | MREMAP_FIXED, hole);
    check(q == hole, "mremap MREMAP_FIXED address");
    check(filled(q, page, 'a') && filled(q + page, page, 'b'), "contents after move");

    /* MREMAP_FIXED without MREMAP_MAYMOVE, and unknown flags, are EINVAL */
    errno = 0;
    check(mremap(q, 2 * page, 2 * page, MREMAP_FIXED, (void *)0) == MAP_FAILED && errno == EINVAL,
          "MREMAP_FIXED alone");
    errno = 0;
    check(mremap(q, 2 * page, 2 * page, 0x40000000) == MAP_FAILED && errno == EINVAL,
          "unknown flag");
    /* A misaligned old address is EINVAL */
    errno = 0;
    check(mremap(q + 1, page, 2 * page, MREMAP_MAYMOVE) == MAP_FAILED && errno == EINVAL,
          "misaligned address");

    check(munmap(q, 2 * page) == 0, "munmap");
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no mremap, skipping\n");
    return 77;
}

#endif
