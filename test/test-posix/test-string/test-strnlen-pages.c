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
/* glibc declares strlcpy and strlcat only with _DEFAULT_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"
#include "string-pages.h"
#include <strings.h>

/*
 * The string functions for every alignment of the operands, lengths 0
 * to 64 and some larger ones, bytes with the high bit set, and
 * operands that end at, or start at, an inaccessible page. The
 * optimized versions (assembly on aarch64 and arm) are where these go
 * wrong.
 *
 * Several functions take arrays that needn't be null-terminated:
 * strnlen "shall never examine more than maxlen bytes", strndup "shall
 * not examine more than size bytes", strncmp, strncpy, stpncpy and
 * strncat stop after n bytes, and strlcat looks at no more than
 * dstsize bytes of dst. Those arrays end at the guard page here.
 */

/* Through pointers, so that the library functions are what runs */
static size_t (* volatile p_strlen)(const char *) = strlen;
static size_t (* volatile p_strnlen)(const char *, size_t) = strnlen;
static char  *( *volatile p_strchr)(const char *, int) = strchr;
static char  *( *volatile p_strrchr)(const char *, int) = strrchr;
static int    (   *volatile p_strcmp)(const char *, const char *) = strcmp;
static int    (   *volatile p_strncmp)(const char *, const char *, size_t) = strncmp;
static char  *( *volatile p_strcpy)(char *, const char *) = strcpy;
static char  *( *volatile p_stpcpy)(char *, const char *) = stpcpy;
static char  *( *volatile p_strncpy)(char *, const char *, size_t) = strncpy;
static char  *( *volatile p_stpncpy)(char *, const char *, size_t) = stpncpy;
static char  *( *volatile p_strcat)(char *, const char *) = strcat;
static char  *( *volatile p_strncat)(char *, const char *, size_t) = strncat;
/* strlcpy and strlcat are new in POSIX.1-2024 */
#if TEST_POSIX_C_LIB_EXT >= 202405L
static size_t (* volatile p_strlcpy)(char *, const char *, size_t) = strlcpy;
static size_t (* volatile p_strlcat)(char *, const char *, size_t) = strlcat;
#endif
static size_t (* volatile p_strspn)(const char *, const char *) = strspn;
static size_t (* volatile p_strcspn)(const char *, const char *) = strcspn;
static char  *( *volatile p_strpbrk)(const char *, const char *) = strpbrk;
static char  *( *volatile p_strstr)(const char *, const char *) = strstr;
static char  *( *volatile p_strdup)(const char *) = strdup;
static char  *( *volatile p_strndup)(const char *, size_t) = strndup;
static int    (   *volatile p_strcasecmp)(const char *, const char *) = strcasecmp;
static int    (   *volatile p_strncasecmp)(const char *, const char *, size_t) = strncasecmp;
static int    (   *volatile p_strcoll)(const char *, const char *) = strcoll;
static size_t (* volatile p_strxfrm)(char *, const char *, size_t) = strxfrm;

#define MARK 0x90

/* A string of len bytes and its null, ending at the guard page */
static char *
str_end(int a, size_t len, size_t seed)
{
    unsigned char *p = at_end(a, len + 1);

    fill(p, len, seed);
    p[len] = '\0';
    return (char *)p;
}

/* The same at the start of the page */
static char *
str_start(int a, size_t len, size_t seed)
{
    unsigned char *p = at_start(a);

    fill(p, len, seed);
    p[len] = '\0';
    return (char *)p;
}

/* len bytes without a null, ending at the guard page */
static char *
arr_end(int a, size_t len, size_t seed)
{
    unsigned char *p = at_end(a, len);

    fill(p, len, seed);
    return (char *)p;
}

static int
position(size_t len, unsigned i, size_t *pos)
{
    static const unsigned picks = 5;

    if (len <= 64) {
        if (i >= len)
            return 0;
        *pos = i;
        return 1;
    }
    if (i >= picks)
        return 0;
    switch (i) {
    case 0:
        *pos = 0;
        break;
    case 1:
        *pos = 1;
        break;
    case 2:
        *pos = len / 2;
        break;
    case 3:
        *pos = len - 2;
        break;
    default:
        *pos = len - 1;
        break;
    }
    return 1;
}

typedef char *(*placer)(int a, size_t len, size_t seed);

static const struct {
    const char *name;
    placer      place;
} places[] = {
    { "at the end of a page",   str_end   },
    { "at the start of a page", str_start },
};

#define N_PLACES (sizeof(places) / sizeof(places[0]))

static void
test_length(void)
{
    unsigned li, pl, off;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        size_t k;
        char  *s;

        for (pl = 0; pl < N_PLACES; pl++) {
            s = places[pl].place(0, len, len);
            now("strlen %s len %zu", places[pl].name, len);
            if (p_strlen(s) != len)
                fail("wrong length %zu", p_strlen(s));
            now("strnlen %s len %zu", places[pl].name, len);
            if (p_strnlen(s, len + 1) != len || p_strnlen(s, len) != len
                || p_strnlen(s, SIZE_MAX) != len || p_strnlen(s, len + page_size) != len)
                fail("wrong length");
            for (k = 0; k < len && k < 70; k++)
                if (p_strnlen(s, k) != k)
                    fail("wrong length for maxlen %zu", k);
        }
        /* Arrays without a null */
        s = arr_end(0, len, len);
        now("strnlen of %zu bytes without a null", len);
        if (p_strnlen(s, len) != len)
            fail("wrong length");
        check_errno();
    }
    for (off = 0; off < 32; off++)
        for (li = 0; li < 65; li++) {
            char *s = (char *)arena[2] + 64 + off;

            fill((unsigned char *)s, li, off);
            s[li] = '\0';
            now("strlen and strnlen at offset %u len %u", off, li);
            if (p_strlen(s) != li || p_strnlen(s, li) != li || p_strnlen(s, SIZE_MAX) != li
                || (li && p_strnlen(s, li - 1) != li - 1))
                fail("wrong length");
            check_errno();
        }
}

static void
one_strchr(const char *where, char *s, size_t len)
{
    unsigned i;
    size_t   k;

    now("strchr %s len %zu", where, len);
    if (p_strchr(s, MARK) != NULL || p_strrchr(s, MARK) != NULL)
        fail("found a byte that isn't there");
    if (p_strchr(s, '\0') != s + len || p_strrchr(s, '\0') != s + len)
        fail("didn't find the null");
    for (i = 0; position(len, i, &k); i++) {
        char save = s[k];

        s[k] = (char)MARK;
        now("strchr %s len %zu, %#x at %zu", where, len, MARK, k);
        if (p_strchr(s, MARK) != s + k || p_strrchr(s, MARK) != s + k)
            fail("wrong result");
        /* c is converted to char */
        if (p_strchr(s, (char)MARK) != s + k || p_strrchr(s, (char)MARK) != s + k)
            fail("wrong result for (char)c");
        if (p_strchr(s, MARK + 256) != s + k || p_strrchr(s, MARK + 256) != s + k)
            fail("wrong result for c + 256");
        if (k > 0) {
            char save0 = s[0];

            s[0] = (char)MARK;
            if (p_strchr(s, MARK) != s || p_strrchr(s, MARK) != s + k)
                fail("wrong result with another one at 0");
            s[0] = save0;
        }
        s[k] = save;
    }
    check_errno();
}

static void
test_strchr(void)
{
    unsigned li, pl, off;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        for (pl = 0; pl < N_PLACES; pl++)
            one_strchr(places[pl].name, places[pl].place(0, len, len), len);
    }
    for (off = 0; off < 32; off++)
        for (li = 0; li < 65; li++) {
            char  where[32];
            char *s = (char *)arena[2] + 64 + off;

            snprintf(where, sizeof(where), "offset %u", off);
            fill((unsigned char *)s, li, off);
            s[li] = '\0';
            one_strchr(where, s, li);
        }
}

/* s1 and s2 hold the same len bytes */
static void
one_strcmp(const char *where, char *s1, char *s2, size_t len, int terminated, int every)
{
    static const unsigned char flips[] = { 0x80, 0x01, 0xff };
    unsigned                   i, f;
    size_t                     k;

    now("strcmp %s len %zu equal", where, len);
    if (terminated && (p_strcmp(s1, s2) != 0 || p_strncmp(s1, s2, SIZE_MAX) != 0))
        fail("equal strings compare unequal");
    if (p_strncmp(s1, s2, len) != 0)
        fail("strncmp: equal arrays compare unequal");
    if (terminated && p_strcasecmp(s1, s2) != 0)
        fail("strcasecmp: equal strings compare unequal");
    if (p_strncasecmp(s1, s2, len) != 0)
        fail("strncasecmp: equal arrays compare unequal");
    for (i = 0; position(len, i, &k); i++) {
        unsigned char *u2 = (unsigned char *)s2;
        unsigned char  save;

        /* A difference at every position, or only near the start, middle and end */
        if (!every && len > 4 && k > 1 && k != len / 2 && k < len - 2)
            continue;
        save = u2[k];

        for (f = 0; f < sizeof(flips); f++) {
            int want;

            u2[k] = save ^ flips[f];
            if (u2[k] == 0)
                continue;
            want = sign((int)(unsigned char)s1[k] - (int)u2[k]);
            now("strcmp %s len %zu, %#x and %#x at %zu", where, len, (unsigned char)s1[k], u2[k],
                k);
            if (terminated && sign(p_strcmp(s1, s2)) != want)
                fail("strcmp: wrong sign (bytes are unsigned char)");
            if (terminated && sign(p_strcmp(s2, s1)) != -want)
                fail("strcmp: wrong sign, swapped");
            if (sign(p_strncmp(s1, s2, len)) != want || sign(p_strncmp(s1, s2, k + 1)) != want)
                fail("strncmp: wrong sign");
            if (p_strncmp(s1, s2, k) != 0)
                fail("strncmp: compared past n");
        }
        /* A shorter string is less */
        if (terminated) {
            u2[k] = '\0';
            now("strcmp %s len %zu, ends at %zu", where, len, k);
            if (p_strcmp(s1, s2) <= 0 || p_strcmp(s2, s1) >= 0 || p_strncmp(s1, s2, len) <= 0)
                fail("the longer string isn't greater");
            if (p_strncmp(s1, s2, k) != 0)
                fail("strncmp: compared past n");
        }
        u2[k] = save;
    }
    check_errno();
}

static void
test_strcmp(void)
{
    unsigned li, o1, o2;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        one_strcmp("at the ends of pages", str_end(0, len, 1), str_end(1, len, 1), len, 1, 1);
        one_strcmp("at the starts of pages", str_start(0, len, 1), str_start(1, len, 1), len, 1, 1);
        one_strcmp("at an end and a start", str_end(0, len, 1), str_start(1, len, 1), len, 1, 1);
        one_strcmp("without nulls at the ends of pages", arr_end(0, len, 1), arr_end(1, len, 1),
                   len, 0, 1);
    }
    for (o1 = 0; o1 < 16; o1++)
        for (o2 = 0; o2 < 16; o2++)
            for (li = 0; li < 65; li++) {
                char  where[32];
                char *s1 = (char *)arena[2] + 64 + o1;
                char *s2 = (char *)arena[2] + 2048 + o2;

                snprintf(where, sizeof(where), "offsets %u and %u", o1, o2);
                fill((unsigned char *)s1, li, 2);
                fill((unsigned char *)s2, li, 2);
                s1[li] = s2[li] = '\0';
                one_strcmp(where, s1, s2, li, 1, 0);
            }
}

/* Copy a string of len bytes into dst, with canaries in the dst_size bytes from dst_lo */
static void
one_strcpy(const char *where, char *dst, const char *src, size_t len, unsigned char *dst_lo,
           size_t dst_size)
{
    size_t after = dst_size - (size_t)((unsigned char *)dst - dst_lo) - len - 1;

    now("strcpy %s len %zu", where, len);
    vset(dst_lo, 0xee, dst_size);
    if (p_strcpy(dst, src) != dst)
        fail("wrong return value");
    if (!same((unsigned char *)dst, (const unsigned char *)src, len + 1))
        fail("wrong contents");
    if (!all(dst_lo, 0xee, (size_t)((unsigned char *)dst - dst_lo))
        || !all((unsigned char *)dst + len + 1, 0xee, after))
        fail("wrote outside the destination");
    now("stpcpy %s len %zu", where, len);
    vset(dst_lo, 0xee, dst_size);
    if (p_stpcpy(dst, src) != dst + len)
        fail("wrong return value");
    if (!same((unsigned char *)dst, (const unsigned char *)src, len + 1))
        fail("wrong contents");
    if (!all(dst_lo, 0xee, (size_t)((unsigned char *)dst - dst_lo))
        || !all((unsigned char *)dst + len + 1, 0xee, after))
        fail("wrote outside the destination");
    check_errno();
}

static void
test_strcpy(void)
{
    unsigned li, os, od;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        one_strcpy("at the ends of pages", (char *)at_end(1, len + 1), str_end(0, len, 3), len,
                   at_start(1), page_size);
        one_strcpy("at the starts of pages", (char *)at_start(1), str_start(0, len, 3), len,
                   at_start(1), page_size);
    }
    for (os = 0; os < 16; os++)
        for (od = 0; od < 16; od++)
            for (li = 0; li < N_LENGTHS; li++) {
                size_t len = length(li);
                char   where[32];
                char  *src = (char *)arena[2] + 64 + os;

                if (len > 256)
                    continue;
                snprintf(where, sizeof(where), "offsets %u and %u", od, os);
                fill((unsigned char *)src, len, os);
                src[len] = '\0';
                one_strcpy(where, (char *)at_start(1) + 64 + od, src, len, at_start(1) + 32,
                           32 + od + len + 1 + 32);
            }
}

/* strncpy and stpncpy of n bytes from src, a string of len bytes or an array of n */
static void
one_strncpy(const char *where, char *dst, const char *src, size_t len, size_t n)
{
    size_t copied = len < n ? len : n;
    char  *want = len < n ? dst + len : dst + n;

    now("strncpy %s len %zu n %zu", where, len, n);
    vset((unsigned char *)dst, 0xee, n);
    if (p_strncpy(dst, src, n) != dst)
        fail("wrong return value");
    if (!same((unsigned char *)dst, (const unsigned char *)src, copied)
        || !all((unsigned char *)dst + copied, 0, n - copied))
        fail("wrong contents");
    now("stpncpy %s len %zu n %zu", where, len, n);
    vset((unsigned char *)dst, 0xee, n);
    if (p_stpncpy(dst, src, n) != want)
        fail("wrong return value");
    if (!same((unsigned char *)dst, (const unsigned char *)src, copied)
        || !all((unsigned char *)dst + copied, 0, n - copied))
        fail("wrong contents");
    check_errno();
}

static void
test_strncpy(void)
{
    unsigned li, ni, od;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        size_t ns[] = { 0, len / 2, len, len + 1, len + 7, len + 64 };

        for (ni = 0; ni < sizeof(ns) / sizeof(ns[0]); ni++) {
            size_t n = ns[ni];
            /* Only n bytes of the source need be there */
            char  *src = n <= len ? arr_end(0, n, 5) : str_end(0, len, 5);

            one_strncpy("at the ends of pages", (char *)at_end(1, n), src, len, n);
            if (n > len)
                src = str_start(0, len, 5);
            else {
                fill(at_start(0), n, 5);
                src = (char *)at_start(0);
            }
            one_strncpy("at the starts of pages", (char *)at_start(1), src, len, n);
            for (od = 0; od < 16; od++) {
                char where[32];

                snprintf(where, sizeof(where), "offset %u", od);
                one_strncpy(where, (char *)at_start(2) + od, src, len, n);
            }
        }
    }
}

/* Append to a dst holding a prefix of plen bytes */
static void
one_strcat(const char *where, char *dst, size_t plen, const char *src, size_t len, size_t n)
{
    size_t appended = len < n ? len : n;

    now("strncat %s prefix %zu len %zu n %zu", where, plen, len, n);
    fill((unsigned char *)dst, plen, 9);
    dst[plen] = '\0';
    vset((unsigned char *)dst + plen + 1, 0xee, appended);
    if (p_strncat(dst, src, n) != dst)
        fail("wrong return value");
    if (!same((unsigned char *)dst + plen, (const unsigned char *)src, appended)
        || dst[plen + appended] != '\0')
        fail("wrong contents");
    /* src is only a string when n > len */
    if (n > len) {
        now("strcat %s prefix %zu len %zu", where, plen, len);
        dst[plen] = '\0';
        vset((unsigned char *)dst + plen + 1, 0xee, len);
        if (p_strcat(dst, src) != dst)
            fail("wrong return value");
        if (!same((unsigned char *)dst + plen, (const unsigned char *)src, len + 1))
            fail("wrong contents");
    }
    check_errno();
}

static void
test_strcat(void)
{
    static const size_t plens[] = { 0, 1, 3, 8, 15, 16, 17, 40 };
    unsigned            li, ni, pi;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        size_t ns[] = { 0, len / 2, len, len + 5, SIZE_MAX };

        for (ni = 0; ni < sizeof(ns) / sizeof(ns[0]); ni++) {
            size_t n = ns[ni];
            size_t appended = len < n ? len : n;

            for (pi = 0; pi < sizeof(plens) / sizeof(plens[0]); pi++) {
                size_t plen = plens[pi];
                /* Only n bytes of the source need be there */
                char  *src = n <= len ? arr_end(0, n, 7) : str_end(0, len, 7);

                one_strcat("at the end of a page", (char *)at_end(1, plen + appended + 1), plen,
                           src, len, n);
                one_strcat("at the start of a page", (char *)at_start(1) + (pi & 7), plen, src, len,
                           n);
            }
        }
    }
}

#if TEST_POSIX_C_LIB_EXT >= 202405L
/* strlcpy into dst of dstsize bytes from a string of len bytes */
static void
one_strlcpy(const char *where, char *dst, size_t dstsize, const char *src, size_t len)
{
    size_t copied = dstsize ? (len < dstsize - 1 ? len : dstsize - 1) : 0;

    now("strlcpy %s len %zu dstsize %zu", where, len, dstsize);
    vset((unsigned char *)dst, 0xee, dstsize);
    if (p_strlcpy(dst, src, dstsize) != len)
        fail("wrong return value");
    if (!same((unsigned char *)dst, (const unsigned char *)src, copied)
        || (dstsize && dst[copied] != '\0')
        || !all((unsigned char *)dst + copied + 1, 0xee,
                dstsize > copied + 1 ? dstsize - copied - 1 : 0))
        fail("wrong contents");
    check_errno();
}

/* strlcat onto dst of dstsize bytes holding a string of plen bytes, or none */
static void
one_strlcat(const char *where, char *dst, size_t dstsize, size_t plen, const char *src, size_t len)
{
    size_t initial = plen < dstsize ? plen : dstsize;
    size_t room = dstsize > plen ? dstsize - plen - 1 : 0;
    size_t appended = len < room ? len : room;

    now("strlcat %s prefix %zu len %zu dstsize %zu", where, plen, len, dstsize);
    /* When plen >= dstsize, dst holds no null within dstsize */
    fill((unsigned char *)dst, initial, 11);
    if (plen < dstsize) {
        dst[plen] = '\0';
        vset((unsigned char *)dst + plen + 1, 0xee, dstsize - plen - 1);
    }
    if (p_strlcat(dst, src, dstsize) != initial + len)
        fail("wrong return value");
    if (plen < dstsize) {
        if (!same((unsigned char *)dst + plen, (const unsigned char *)src, appended)
            || dst[plen + appended] != '\0'
            || !all((unsigned char *)dst + plen + appended + 1, 0xee, room - appended))
            fail("wrong contents");
    } else {
        unsigned char want[1100];

        fill(want, initial, 11);
        if (!same((unsigned char *)dst, want, initial))
            fail("changed dst without a null in it");
    }
    check_errno();
}

static void
test_strlcpy(void)
{
    unsigned li, di, pi;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        size_t sizes[] = { 0, 1, 2, len / 2, len, len + 1, len + 2, len + 9 };
        char  *src = str_end(0, len, 13);

        for (di = 0; di < sizeof(sizes) / sizeof(sizes[0]); di++) {
            size_t dstsize = sizes[di];

            one_strlcpy("at the end of a page", (char *)at_end(1, dstsize), dstsize, src, len);
            one_strlcpy("at the start of a page", (char *)at_start(1) + (di & 7), dstsize, src,
                        len);
        }
        for (pi = 0; pi < 4; pi++) {
            static const size_t plens[] = { 0, 3, 16, 17 };
            size_t              plen = plens[pi];
            size_t dsizes[] = { 0,          plen,           plen + 1,      plen + 2, plen + len / 2,
                                plen + len, plen + len + 1, plen + len + 5 };

            for (di = 0; di < sizeof(dsizes) / sizeof(dsizes[0]); di++) {
                size_t dstsize = dsizes[di];

                if (dstsize < plen && dstsize)
                    continue;
                one_strlcat("at the end of a page", (char *)at_end(1, dstsize), dstsize, plen, src,
                            len);
                one_strlcat("at the start of a page", (char *)at_start(1) + (di & 7), dstsize, plen,
                            src, len);
            }
        }
    }
}
#endif

static size_t
ref_spn(const volatile unsigned char *s, const volatile unsigned char *set, int accept)
{
    size_t i, j;

    for (i = 0; s[i]; i++) {
        int in = 0;

        for (j = 0; set[j]; j++)
            if (s[i] == set[j])
                in = 1;
        if (in != accept)
            break;
    }
    return i;
}

static void
test_strspn(void)
{
    static const char * const sets[] = {
        "",
        "a",
        "\x80",
        "aB7\xc3",
        "\x80\xff\xc3\xa5\xfe\x81",
        "a\x80"
        "B\xff"
        "7\xc3z\x01\x7fQ\xa5\xfe_\x81m ",
        "\x90",
    };
    unsigned li, si, pl;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);

        for (si = 0; si < sizeof(sets) / sizeof(sets[0]); si++) {
            size_t setlen = strlen(sets[si]);
            char  *set = (char *)at_end(1, setlen + 1);

            memcpy(set, sets[si], setlen + 1);
            for (pl = 0; pl < N_PLACES; pl++) {
                char  *s = places[pl].place(0, len, len + si);
                size_t spn = ref_spn((unsigned char *)s, (unsigned char *)set, 1);
                size_t cspn = ref_spn((unsigned char *)s, (unsigned char *)set, 0);

                now("strspn %s len %zu set %u", places[pl].name, len, si);
                if (p_strspn(s, set) != spn)
                    fail("strspn is %zu, not %zu", p_strspn(s, set), spn);
                if (p_strcspn(s, set) != cspn)
                    fail("strcspn is %zu, not %zu", p_strcspn(s, set), cspn);
                if (p_strpbrk(s, set) != (s[cspn] ? s + cspn : NULL))
                    fail("strpbrk is wrong");
                check_errno();
            }
        }
    }
}

static const char *
ref_strstr(const volatile char *h, const volatile char *n)
{
    size_t i, j;

    for (i = 0;; i++) {
        for (j = 0; n[j] && h[i + j] == n[j]; j++)
            ;
        if (!n[j])
            return (const char *)h + i;
        if (!h[i])
            return NULL;
    }
}

static void
test_strstr(void)
{
    static const size_t nls[] = { 1, 2, 3, 4, 5, 8, 15, 16, 17, 31, 32, 33, 64, 257 };
    unsigned            li, ni, round;
    uint32_t            state = 7;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        char  *h = str_end(0, len, 0);
        char  *empty = (char *)at_end(1, 1);

        *empty = '\0';
        now("strstr len %zu empty needle", len);
        if (p_strstr(h, empty) != h)
            fail("an empty needle doesn't give s1");
        for (ni = 0; ni < sizeof(nls) / sizeof(nls[0]); ni++) {
            size_t nl = nls[ni];

            for (round = 0; round < 4; round++) {
                char  *n = (char *)at_end(1, nl + 1);
                size_t i;

                /* A small alphabet gives repeats and near misses */
                for (i = 0; i < len; i++) {
                    state = state * 1103515245 + 12345;
                    h[i] = (state >> 16) & 1 ? 'a' : (char)0x80;
                }
                if (nl <= len) {
                    size_t pos = round == 0 ? len - nl : ((state >> 8) % (len - nl + 1));

                    vcopy((unsigned char *)n, (unsigned char *)h + pos, nl);
                    if (round & 1)
                        n[(state >> 4) % nl] = 'b';
                } else {
                    vset((unsigned char *)n, 'a', nl);
                }
                n[nl] = '\0';
                now("strstr len %zu needle %zu round %u", len, nl, round);
                if (p_strstr(h, n) != ref_strstr(h, n))
                    fail("wrong result");
                check_errno();
            }
        }
    }
}

static void
test_strdup(void)
{
    unsigned li, pl;
    size_t   k;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        char  *d;
        char  *s;

        for (pl = 0; pl < N_PLACES; pl++) {
            s = places[pl].place(0, len, len);
            now("strdup %s len %zu", places[pl].name, len);
            d = p_strdup(s);
            if (d == NULL || !same((unsigned char *)d, (unsigned char *)s, len + 1))
                fail("wrong copy");
            free(d);
            d = p_strndup(s, len + 5);
            if (d == NULL || !same((unsigned char *)d, (unsigned char *)s, len + 1))
                fail("strndup: wrong copy");
            free(d);
        }
        /* strndup "shall not examine more than size bytes" */
        s = arr_end(0, len, len);
        for (k = 0; k <= len; k += len > 64 ? len / 4 + 1 : 1) {
            now("strndup of %zu bytes without a null, size %zu", len, k);
            d = p_strndup(s + len - k, k);
            if (d == NULL || !same((unsigned char *)d, (unsigned char *)s + len - k, k)
                || d[k] != '\0')
                fail("wrong copy");
            free(d);
        }
#ifdef __PICOLIBC__
        check_errno();
#else
        /* A successful call may set errno; other C libraries' malloc does */
        errno = ERRNO_MARK;
#endif
    }
}

static void
test_strcoll(void)
{
    unsigned li;

    for (li = 0; li < N_LENGTHS; li++) {
        size_t len = length(li);
        char  *s1 = str_end(0, len, 1);
        char  *s2 = str_end(1, len, 1);
        size_t x;

        now("strcoll len %zu", len);
        if (p_strcoll(s1, s2) != 0)
            fail("equal strings collate unequal");
        if (len) {
            s2[len - 1] ^= 0x01;
            if (sign(p_strcoll(s1, s2)) != sign(p_strcmp(s1, s2)))
                fail("strcoll doesn't match strcmp in the POSIX locale");
        }
        now("strxfrm len %zu", len);
        x = p_strxfrm(NULL, s1, 0);
        if (p_strxfrm((char *)at_end(2, x + 1), s1, x + 1) != x)
            fail("lengths differ");
        check_errno();
    }
}

int
main(void)
{
    setup_arenas();
    errno = ERRNO_MARK;
    test_length();
    test_strchr();
    test_strcmp();
    test_strcpy();
    test_strncpy();
    test_strcat();
#if TEST_POSIX_C_LIB_EXT >= 202405L
    test_strlcpy();
#endif
    test_strspn();
    test_strstr();
    test_strdup();
    test_strcoll();
    if (failures)
        return 1;
    printf("success\n");
    return 0;
}
