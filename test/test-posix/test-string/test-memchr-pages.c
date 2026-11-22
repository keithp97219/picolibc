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
/* memccpy is XSI */
#define _XOPEN_SOURCE 800
/* glibc declares memmem only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"
#include "string-pages.h"

/*
 * memchr, memcmp, memcpy, memmove, memset, memccpy and memmem, for
 * every alignment of the operands, lengths 0 to 64 and some larger
 * ones, bytes with the high bit set, overlapping memmove, and operands
 * that end at, or start at, an inaccessible page. The optimized
 * versions (assembly on aarch64 and arm) are where these go wrong.
 *
 * memchr "shall behave as if it reads the bytes sequentially and stops
 * as soon as a matching byte is found", so a match before the guard
 * page with a length that runs past it must not fault.
 */

/* Through pointers, so that the library functions are what runs */
static void *(* volatile p_memchr)(const void *, int, size_t) = memchr;
static int   (  *volatile p_memcmp)(const void *, const void *, size_t) = memcmp;
static void *(* volatile p_memcpy)(void *, const void *, size_t) = memcpy;
static void *(* volatile p_memmove)(void *, const void *, size_t) = memmove;
static void *(* volatile p_memset)(void *, int, size_t) = memset;
static void *(* volatile p_memccpy)(void *, const void *, int, size_t) = memccpy;
static void *(* volatile p_memmem)(const void *, size_t, const void *, size_t) = memmem;

/* Bytes the pattern never uses */
#define MARK1  0x90
#define MARK2  0x00

#define MAXLEN 1000
#define SLOP   80

static unsigned char buf_a[MAXLEN + 2 * SLOP];
static unsigned char buf_b[MAXLEN + 2 * SLOP];
static unsigned char move_buf[3 * MAXLEN + 4 * SLOP];
static unsigned char move_want[3 * MAXLEN + 4 * SLOP];

/* Positions to try within a buffer of len bytes */
static int
position(size_t len, unsigned i, size_t *pos)
{
    if (len <= 64) {
        if (i >= len)
            return 0;
        *pos = i;
        return 1;
    }
    switch (i) {
    case 0:
        *pos = 0;
        return 1;
    case 1:
        *pos = 1;
        return 1;
    case 2:
        *pos = len / 2;
        return 1;
    case 3:
        *pos = len - 2;
        return 1;
    case 4:
        *pos = len - 1;
        return 1;
    }
    return 0;
}

static void
one_memchr(const char *where, unsigned char *p, size_t len, int guard_after, int every)
{
    unsigned i;
    size_t   k;

    now("memchr %s len %zu absent", where, len);
    if (p_memchr(p, MARK1, len) != NULL)
        fail("found a byte that isn't there");
    now("memchr %s len %zu, n 0", where, len);
    if (p_memchr(p + len, 'a', 0) != NULL)
        fail("found something in no bytes");
    for (i = 0; position(len, i, &k); i++) {
        /* Every position, or only near the start, middle and end */
        if (!every && len > 4 && k > 1 && k != len / 2 && k < len - 2)
            continue;
        static const int marks[] = { MARK1, MARK2 };
        unsigned         m;
        unsigned char    save = p[k];

        for (m = 0; m < 2; m++) {
            int mark = marks[m];

            p[k] = (unsigned char)mark;
            now("memchr %s len %zu, %#x at %zu", where, len, mark, k);
            if (p_memchr(p, mark, len) != p + k)
                fail("wrong result");
            if (p_memchr(p, mark, k + 1) != p + k)
                fail("wrong result with n just covering it");
            if (p_memchr(p, mark, k) != NULL)
                fail("looked past n");
            /* c is converted to unsigned char */
            if (p_memchr(p, mark + 256, len) != p + k)
                fail("c + 256 isn't converted to unsigned char");
            if (p_memchr(p, (signed char)mark, len) != p + k)
                fail("negative c isn't converted to unsigned char");
            if (guard_after) {
                now("memchr %s len %zu, %#x at %zu, n past the page", where, len, mark, k);
                if (p_memchr(p, mark, len + page_size) != p + k)
                    fail("wrong result");
                now("memchr %s len %zu, %#x at %zu, n SIZE_MAX", where, len, mark, k);
                if (p_memchr(p, mark, SIZE_MAX) != p + k)
                    fail("wrong result");
            }
        }
        p[k] = save;
    }
    check_errno();
}

static void
test_memchr(void)
{
    unsigned li, off;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        fill(at_end(0, len), len, len);
        one_memchr("at the end of a page", at_end(0, len), len, 1, 1);
        fill(at_start(0), len, len);
        one_memchr("at the start of a page", at_start(0), len, 0, 1);
    }
    for (off = 0; off < 32; off++)
        for (li = 0; li < 65; li++) {
            char where[32];

            snprintf(where, sizeof(where), "offset %u", off);
            fill(buf_a + off, li, off);
            one_memchr(where, buf_a + off, li, 0, 0);
        }
}

static void
one_memcmp(const char *where, unsigned char *a, unsigned char *b, size_t len, int every)
{
    unsigned i;
    size_t   k;

    now("memcmp %s len %zu equal", where, len);
    fill(a, len, 3);
    fill(b, len, 3);
    if (p_memcmp(a, b, len) != 0)
        fail("equal bytes compare unequal");
    for (i = 0; position(len, i, &k); i++) {
        /* Every position, or only near the start, middle and end */
        if (!every && len > 4 && k > 1 && k != len / 2 && k < len - 2)
            continue;
        static const unsigned char flips[] = { 0x80, 0x01, 0xff };
        unsigned                   f;

        for (f = 0; f < sizeof(flips); f++) {
            unsigned char save = b[k];
            int           want;

            b[k] ^= flips[f];
            want = sign((int)a[k] - (int)b[k]);
            now("memcmp %s len %zu, %#x and %#x at %zu", where, len, a[k], b[k], k);
            if (sign(p_memcmp(a, b, len)) != want)
                fail("wrong sign (bytes are unsigned char)");
            if (sign(p_memcmp(b, a, len)) != -want)
                fail("wrong sign, swapped");
            if (p_memcmp(a, b, k) != 0)
                fail("compared past n");
            b[k] = save;
        }
    }
    check_errno();
}

static void
test_memcmp(void)
{
    unsigned li, oa, ob;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        one_memcmp("at the ends of pages", at_end(0, len), at_end(1, len), len, 1);
        one_memcmp("at the starts of pages", at_start(0), at_start(1), len, 1);
        one_memcmp("at an end and a start", at_end(0, len), at_start(1), len, 1);
    }
    for (oa = 0; oa < 16; oa++)
        for (ob = 0; ob < 16; ob++)
            for (li = 0; li < 65; li++) {
                char where[32];

                snprintf(where, sizeof(where), "offsets %u and %u", oa, ob);
                one_memcmp(where, buf_a + oa, buf_b + ob, li, 0);
            }
}

/* Copy len bytes from src to dst, with canaries in the dst_size bytes around */
static void
one_memcpy(const char *where, unsigned char *dst, unsigned char *src, size_t len,
           unsigned char *dst_lo, size_t dst_size)
{
    now("memcpy %s len %zu", where, len);
    vset(dst_lo, 0xee, dst_size);
    fill(src, len, len + 5);
    if (p_memcpy(dst, src, len) != dst)
        fail("wrong return value");
    if (!same(dst, src, len))
        fail("wrong contents");
    if (!all(dst_lo, 0xee, (size_t)(dst - dst_lo))
        || !all(dst + len, 0xee, dst_size - (size_t)(dst - dst_lo) - len))
        fail("wrote outside the destination");
    check_errno();
}

static void
test_memcpy(void)
{
    unsigned li, os, od;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        one_memcpy("at the ends of pages", at_end(1, len), at_end(0, len), len, at_start(1),
                   page_size);
        one_memcpy("at the starts of pages", at_start(1), at_start(0), len, at_start(1), page_size);
    }
    for (os = 0; os < 16; os++)
        for (od = 0; od < 16; od++)
            for (li = 0; li < N_LENGTHS; li++) {
                char where[32];

                snprintf(where, sizeof(where), "offsets %u and %u", od, os);
                /* Canaries in the 32 bytes on each side */
                one_memcpy(where, buf_a + SLOP + od, buf_b + SLOP + os, length(li),
                           buf_a + SLOP - 32, 32 + od + length(li) + 32);
            }
}

/* Move len bytes in move_buf from src to dst, which may overlap */
static void
one_memmove(size_t src, size_t dst, size_t len)
{
    /* The bytes around both operands */
    size_t lo = (src < dst ? src : dst) - 16;
    size_t hi = (src > dst ? src : dst) + len + 16;
    size_t i;

    now("memmove len %zu from %zu to %zu", len, src, dst);
    for (i = lo; i < hi; i++)
        move_buf[i] = pattern(i * 3 + len);
    vcopy(move_want + lo, move_buf + lo, hi - lo);
    for (i = 0; i < len; i++)
        move_want[dst + i] = move_buf[src + i];
    if (p_memmove(move_buf + dst, move_buf + src, len) != move_buf + dst)
        fail("wrong return value");
    if (!same(move_buf + lo, move_want + lo, hi - lo))
        fail("wrong contents");
    check_errno();
}

/* Overlapping moves at a guard page */
static void
one_memmove_page(const char *where, unsigned char *dst, unsigned char *src, size_t len)
{
    unsigned char want[MAXLEN + SLOP];

    now("memmove %s len %zu by %d", where, len, (int)(dst - src));
    fill(src, len, len);
    vcopy(want, src, len);
    if (p_memmove(dst, src, len) != dst)
        fail("wrong return value");
    if (!same(dst, want, len))
        fail("wrong contents");
    check_errno();
}

static void
test_memmove(void)
{
    unsigned li, base;
    int      delta;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        int    max_delta = 40;

        for (delta = -max_delta; delta <= max_delta; delta++) {
            size_t src = MAXLEN / 2 + 2 * SLOP;

            if (len <= 64) {
                for (base = 0; base < 8; base++)
                    one_memmove(src + base, (size_t)((long)(src + base) + delta), len);
            } else {
                one_memmove(src, (size_t)((long)src + delta), len);
            }
        }
        /* Moves by exactly the length and one less */
        if (len > 0) {
            size_t src = MAXLEN + 2 * SLOP;

            one_memmove(src, src + len, len);
            one_memmove(src, src - len, len);
            one_memmove(src, src + len - 1, len);
            one_memmove(src, src - len + 1, len);
        }
        for (delta = 1; delta <= 17; delta++) {
            size_t d = (size_t)delta;

            /* Upwards, ending at the guard page; then downwards from it */
            one_memmove_page("up to the end of a page", at_end(0, len), at_end(0, len + d), len);
            one_memmove_page("down from the end of a page", at_end(0, len + d), at_end(0, len),
                             len);
            /* Downwards to the start of a page; then upwards from it */
            one_memmove_page("down to the start of a page", at_start(0), at_start(0) + d, len);
            one_memmove_page("up from the start of a page", at_start(0) + d, at_start(0), len);
        }
        /* Between pages, without overlap */
        one_memmove_page("between pages", at_end(1, len), at_start(0), len);
        one_memmove_page("between pages", at_start(1), at_end(0, len), len);
    }
}

static void
one_memset(const char *where, unsigned char *dst, int c, size_t len, unsigned char *dst_lo,
           size_t dst_size)
{
    unsigned char canary = (unsigned char)c == 0xee ? 0x11 : 0xee;

    now("memset %s len %zu c %d", where, len, c);
    vset(dst_lo, canary, dst_size);
    if (p_memset(dst, c, len) != dst)
        fail("wrong return value");
    if (!all(dst, (unsigned char)c, len))
        fail("wrong contents (c converted to unsigned char)");
    if (!all(dst_lo, canary, (size_t)(dst - dst_lo))
        || !all(dst + len, canary, dst_size - (size_t)(dst - dst_lo) - len))
        fail("wrote outside the destination");
    check_errno();
}

static void
test_memset(void)
{
    static const int values[] = { 0, 0x5a, 0x80, 0xff, 0xee, 0x1a5, -1, -128 };
    unsigned         li, off, v;

    for (v = 0; v < sizeof(values) / sizeof(values[0]); v++) {
        int c = values[v];

        for (li = 0; li < N_LENGTHS; li++) {
            size_t len = length(li);

            one_memset("at the end of a page", at_end(0, len), c, len, at_start(0), page_size);
            one_memset("at the start of a page", at_start(0), c, len, at_start(0), page_size);
            for (off = 0; off < 16; off++) {
                char where[32];

                snprintf(where, sizeof(where), "offset %u", off);
                one_memset(where, buf_a + SLOP + off, c, len, buf_a + SLOP - 32,
                           32 + off + len + 32);
            }
        }
    }
}

/* memccpy from src, which holds mark at k (or nowhere if k >= len) */
static void
one_memccpy(const char *where, unsigned char *dst, unsigned char *src, size_t len, size_t k, int c)
{
    size_t         copied = k < len ? k + 1 : len;
    void          *want = k < len ? dst + k + 1 : NULL;
    unsigned char *r;

    now("memccpy %s len %zu c %d at %zu", where, len, c, k);
    vset(dst, 0xee, len);
    r = p_memccpy(dst, src, c, len);
    if (r != want)
        fail("returned %p, not %p", (void *)r, want);
    if (!same(dst, src, copied))
        fail("wrong contents");
    if (!all(dst + copied, 0xee, len - copied))
        fail("copied past c");
    check_errno();
}

static void
memccpy_all(const char *where, unsigned char *dst, unsigned char *src, size_t len, int every)
{
    unsigned i;
    size_t   k;

    fill(src, len, len);
    one_memccpy(where, dst, src, len, len, MARK1);
    /* c at every position, or only near the start, middle and end */
    for (i = 0; position(len, i, &k); i++) {
        if (!every && len > 4 && k > 1 && k != len / 2 && k < len - 2)
            continue;
        unsigned char save = src[k];

        src[k] = MARK1;
        one_memccpy(where, dst, src, len, k, MARK1);
        /* c is converted to unsigned char */
        one_memccpy(where, dst, src, len, k, MARK1 + 256);
        one_memccpy(where, dst, src, len, k, (signed char)MARK1);
        /* n stops it first */
        one_memccpy(where, dst, src, k, k, MARK1);
        src[k] = MARK2;
        one_memccpy(where, dst, src, len, k, MARK2);
        src[k] = save;
    }
}

static void
test_memccpy(void)
{
    unsigned li, os, od;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        memccpy_all("at the ends of pages", at_end(1, len), at_end(0, len), len, 1);
        memccpy_all("at the starts of pages", at_start(1), at_start(0), len, 1);
    }
    for (os = 0; os < 16; os++)
        for (od = 0; od < 16; od++)
            for (li = 0; li < 65; li++) {
                char where[32];

                snprintf(where, sizeof(where), "offsets %u and %u", od, os);
                memccpy_all(where, buf_a + od, buf_b + os, li, 0);
            }
}

static const unsigned char *
ref_memmem(const volatile unsigned char *h, size_t hl, const volatile unsigned char *n, size_t nl)
{
    size_t i, j;

    if (nl == 0)
        return (const unsigned char *)h;
    for (i = 0; i + nl <= hl; i++) {
        for (j = 0; j < nl; j++)
            if (h[i + j] != n[j])
                break;
        if (j == nl)
            return (const unsigned char *)h + i;
    }
    return NULL;
}

static uint32_t rand_state = 1;

static unsigned
next_rand(void)
{
    rand_state = rand_state * 1103515245 + 12345;
    return (rand_state >> 16) & 0x7fff;
}

static void
one_memmem(const char *where, const unsigned char *h, size_t hl, const unsigned char *n, size_t nl)
{
    const unsigned char *want = ref_memmem(h, hl, n, nl);
    const unsigned char *got;

    now("memmem %s haystack %zu needle %zu", where, hl, nl);
    got = p_memmem(h, hl, n, nl);
    if (got != want)
        fail("found %td, not %td", got ? got - h : -1, want ? want - h : -1);
    check_errno();
}

static void
test_memmem(void)
{
    static const size_t needles[]
        = { 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 64, 255, 256, 257, 300 };
    static const unsigned char alphabet[] = { 'a', 0x80, 0x00, 0xff };
    unsigned                   li, ni, round;

    /* An empty needle gives the haystack, even an empty one */
    now("memmem empty needle");
    if (p_memmem(at_end(0, 0), 0, at_end(1, 0), 0) != at_end(0, 0))
        fail("empty needle in an empty haystack");
    if (p_memmem(at_end(0, 5), 5, at_end(1, 0), 0) != at_end(0, 5))
        fail("empty needle");
    /* A haystack shorter than the needle gives NULL, even when it is a prefix */
    now("memmem short haystack");
    memcpy(at_end(0, 3), "abc", 3);
    memcpy(at_end(1, 4), "abcd", 4);
    if (p_memmem(at_end(0, 3), 3, at_end(1, 4), 4) != NULL)
        fail("haystack shorter than the needle");
    memcpy(at_end(1, 3), "abc", 3);
    if (p_memmem(at_end(0, 3), 2, at_end(1, 3), 3) != NULL)
        fail("haystack shorter than the needle, haystack bytes past haystacklen");
    check_errno();

    for (li = 0; li < N_LENGTHS; li++) {
        size_t hl = length(li);

        for (ni = 0; ni < sizeof(needles) / sizeof(needles[0]); ni++) {
            size_t nl = needles[ni];

            if (nl > hl + 1)
                continue;
            for (round = 0; round < 6; round++) {
                unsigned char *h = at_end(0, hl);
                unsigned char *n = at_end(1, nl);
                size_t         i, pos;
                /* Small alphabets give repeats and near misses */
                unsigned       alpha = round < 3 ? 2 : 4;

                for (i = 0; i < hl; i++)
                    h[i] = alphabet[next_rand() % alpha];
                if (nl <= hl) {
                    pos = round % 3 == 0 ? hl - nl : next_rand() % (hl - nl + 1);
                    vcopy(n, h + pos, nl);
                    if (round % 2)
                        n[next_rand() % nl] ^= 0x80;
                } else {
                    for (i = 0; i < nl; i++)
                        n[i] = alphabet[next_rand() % alpha];
                }
                one_memmem("at the ends of pages", h, hl, n, nl);
                vcopy(at_start(0), h, hl);
                one_memmem("at the starts of pages", at_start(0), hl, n, nl);
            }
        }
    }
}

int
main(void)
{
    setup_arenas();
    errno = ERRNO_MARK;
    test_memchr();
    test_memcmp();
    test_memcpy();
    test_memmove();
    test_memset();
    test_memccpy();
    test_memmem();
    if (failures)
        return 1;
    printf("success\n");
    return 0;
}
