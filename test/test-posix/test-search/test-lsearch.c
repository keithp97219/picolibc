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
/* lsearch is an XSI interface */
#define _XOPEN_SOURCE 800
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* lsearch finds an element, appending it when missing */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
cmp(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

/* Elements of an odd width; equality only, so any non-zero means different */
struct rec {
    char name[3];
    char extra[4];
};

static const void *rec_key;
static int         rec_bad;

static int
rec_cmp(const void *a, const void *b)
{
    if (a != rec_key && b != rec_key)
        rec_bad++;
    return memcmp(a, b, 3) != 0;
}

static void
set_rec(struct rec *r, const char *name, const char *extra)
{
    memcpy(r->name, name, sizeof(r->name));
    memcpy(r->extra, extra, sizeof(r->extra));
}

int
main(void)
{
    int    a[4] = { 5, 3 }, key;
    size_t n = 2;
    struct rec  recs[4] = { { "ab", "123" }, { "cd", "456" } }, rk;
    struct rec *r;

    key = 3;
    check(lsearch(&key, a, &n, sizeof(int), cmp) == &a[1] && n == 2, "found");
    key = 8;
    check(lsearch(&key, a, &n, sizeof(int), cmp) == &a[2] && n == 3 && a[2] == 8, "appended");
    key = 8;
    check(lsearch(&key, a, &n, sizeof(int), cmp) == &a[2] && n == 3, "found after appending");

    /* The whole element is copied to the end, and found from then on */
    _Static_assert(sizeof(struct rec) == 7, "odd width");
    n = 2;
    set_rec(&rk, "ef", "789");
    rec_key = &rk;
    r = lsearch(&rk, recs, &n, sizeof(struct rec), rec_cmp);
    check(r == &recs[2] && n == 3, "odd width appended");
    check(memcmp(&recs[2], &rk, sizeof(rk)) == 0, "whole element copied");
    check(memcmp(recs[0].extra, "123", 4) == 0 && memcmp(recs[1].extra, "456", 4) == 0,
          "others untouched");
    set_rec(&rk, "cd", "xyz");
    r = lsearch(&rk, recs, &n, sizeof(struct rec), rec_cmp);
    check(r == &recs[1] && n == 3 && memcmp(recs[1].extra, "456", 4) == 0, "found, not replaced");
    check(rec_bad == 0, "compar sees the key");

    /* An empty table gets the key as its first element */
    n = 0;
    set_rec(&rk, "gh", "000");
    r = lsearch(&rk, recs, &n, sizeof(struct rec), rec_cmp);
    check(r == &recs[0] && n == 1 && memcmp(&recs[0], &rk, sizeof(rk)) == 0, "empty table");

    printf("success\n");
    return 0;
}
