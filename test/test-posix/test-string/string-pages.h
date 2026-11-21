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

/*
 * Shared by the page tests: arenas of one usable page with an
 * inaccessible page on each side, so that a function reading or
 * writing past either end of the page faults, and the report naming
 * the call that faulted.
 *
 * Include it after the feature-test macros and "../posix-groups.h".
 */

#ifndef _STRING_PAGES_H_
#define _STRING_PAGES_H_

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _POSIX_MAPPED_FILES > 0 && _POSIX_MEMORY_PROTECTION > 0
#include <sys/mman.h>
#define HAVE_GUARD_PAGES 1
#else
#define HAVE_GUARD_PAGES 0
#endif

#ifdef __PICOLIBC_POSIX_SIGNALS
#include <signal.h>
#endif

#define N_ARENAS 3

static unsigned char *arena[N_ARENAS];
static size_t         page_size;

/* What is being run, for the fault report */
static char           current[200];
static int            failures;

#define fail(...)                          \
    do {                                   \
        printf("%s: ", current);           \
        printf(__VA_ARGS__);               \
        printf("\n");                      \
        if (++failures > 50) {             \
            printf("too many failures\n"); \
            exit(1);                       \
        }                                  \
    } while (0)

#define now(...) snprintf(current, sizeof(current), __VA_ARGS__)

#ifdef __PICOLIBC_POSIX_SIGNALS
static void
on_fault(int sig)
{
    static const char msg[] = "fault (outside the bytes the call may touch): ";
    size_t            len = 0;

    (void)sig;
    while (current[len])
        len++;
    (void)!write(1, msg, sizeof(msg) - 1);
    (void)!write(1, current, len);
    (void)!write(1, "\n", 1);
    _exit(1);
}
#endif

#if !HAVE_GUARD_PAGES
#define FALLBACK_PAGE 4096
static unsigned char fallback[N_ARENAS][3 * FALLBACK_PAGE];
#endif

static inline void
setup_arenas(void)
{
    int i;

#if HAVE_GUARD_PAGES
    long ps = sysconf(_SC_PAGESIZE);

    if (ps <= 0) {
        printf("no page size\n");
        exit(1);
    }
    page_size = (size_t)ps;
    for (i = 0; i < N_ARENAS; i++) {
        unsigned char *m
            = mmap(NULL, 3 * page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m == MAP_FAILED) {
            printf("mmap: %s\n", strerror(errno));
            exit(1);
        }
        if (mprotect(m, page_size, PROT_NONE) != 0
            || mprotect(m + 2 * page_size, page_size, PROT_NONE) != 0) {
            printf("mprotect: %s\n", strerror(errno));
            exit(1);
        }
        arena[i] = m + page_size;
    }
#else
    page_size = FALLBACK_PAGE;
    for (i = 0; i < N_ARENAS; i++)
        arena[i] = fallback[i] + FALLBACK_PAGE;
    printf("no guard pages; only the contents are checked\n");
#endif

#ifdef __PICOLIBC_POSIX_SIGNALS
    {
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = on_fault;
        sigemptyset(&sa.sa_mask);
        (void)sigaction(SIGSEGV, &sa, NULL);
#ifdef SIGBUS
        (void)sigaction(SIGBUS, &sa, NULL);
#endif
    }
#endif
}

/* len bytes ending at the arena's top guard page */
static inline unsigned char *
at_end(int a, size_t len)
{
    return arena[a] + page_size - len;
}

/* Starting at the arena's bottom guard page */
static inline unsigned char *
at_start(int a)
{
    return arena[a];
}

/*
 * Bytes for the contents: never zero, and a good share with the high
 * bit set, so that signed char comparisons go wrong
 */
static inline unsigned char
pattern(size_t i)
{
    static const unsigned char bytes[] = {
        'a', 0x80, 'B', 0xff, '7', 0xc3, 'z', 0x01, 0x7f, 'Q', 0xa5, 0xfe, '_', 0x81, 'm', 0x20,
    };
    return bytes[(i * 7 + i / 16) % sizeof(bytes)];
}

static inline void
fill(unsigned char *p, size_t len, size_t seed)
{
    size_t i;

    for (i = 0; i < len; i++)
        p[i] = pattern(i + seed);
}

/* Bytes the compiler can't turn back into library calls */
static inline int
same(const volatile unsigned char *a, const volatile unsigned char *b, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

static inline int
all(const volatile unsigned char *a, unsigned char c, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
        if (a[i] != c)
            return 0;
    return 1;
}

static inline void
vset(volatile unsigned char *a, unsigned char c, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
        a[i] = c;
}

static inline void
vcopy(volatile unsigned char *d, const volatile unsigned char *s, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
        d[i] = s[i];
}

static inline int
sign(int v)
{
    return (v > 0) - (v < 0);
}

/* Lengths: everything up to 64, then sizes around word and page boundaries */
static const size_t extra_lengths[]
    = { 65, 79, 80, 95, 96, 100, 127, 128, 129, 255, 256, 257, 511, 1000 };

#define N_EXTRA   (sizeof(extra_lengths) / sizeof(extra_lengths[0]))
#define N_LENGTHS (65 + N_EXTRA)

static inline size_t
length(unsigned i)
{
    return i < 65 ? i : extra_lengths[i - 65];
}

/* errno must not change on valid input */
#define ERRNO_MARK EDOM

static inline void
check_errno(void)
{
    if (errno != ERRNO_MARK)
        fail("errno changed to %d", errno);
    errno = ERRNO_MARK;
}

#endif /* _STRING_PAGES_H_ */
