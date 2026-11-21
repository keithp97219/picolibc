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
/* glibc declares qsort_r only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

/*
 * qsort, qsort_r and bsearch: ascending order by the comparison
 * function, no calls for an empty array, qsort_r's argument passed on
 * as the third argument, and whole elements moved.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

struct elt {
    int  key;
    char tag[13];
};

static int calls;

static int
cmp(const void *a, const void *b)
{
    const struct elt *x = a, *y = b;

    calls++;
    return (x->key > y->key) - (x->key < y->key);
}

static int
cmp_r(const void *a, const void *b, void *arg)
{
    int *sign = arg;

    calls++;
    return *sign * cmp(a, b);
}

static int
check_tags(const struct elt *e, int n)
{
    for (int i = 0; i < n; i++) {
        char want[13];

        snprintf(want, sizeof(want), "tag%d", e[i].key);
        if (strcmp(e[i].tag, want) != 0)
            return 0;
    }
    return 1;
}

int
main(void)
{
    struct elt  e[50];
    struct elt  key;
    struct elt *found;
    int         sign;
    int         i;

    for (i = 0; i < 50; i++) {
        e[i].key = (i * 37) % 50;
        snprintf(e[i].tag, sizeof(e[i].tag), "tag%d", e[i].key);
    }

    calls = 0;
    qsort(e, 0, sizeof(e[0]), cmp);
    check(calls == 0, "qsort of no elements calls nothing");
    sign = 1;
    qsort_r(e, 0, sizeof(e[0]), cmp_r, &sign);
    check(calls == 0, "qsort_r of no elements calls nothing");

    qsort(e, 50, sizeof(e[0]), cmp);
    for (i = 0; i < 50; i++)
        check(e[i].key == i, "qsort sorts ascending");
    check(check_tags(e, 50), "qsort moves whole elements");

    key.key = 17;
    found = bsearch(&key, e, 50, sizeof(e[0]), cmp);
    check(found == &e[17], "bsearch finds");
    key.key = 99;
    check(bsearch(&key, e, 50, sizeof(e[0]), cmp) == NULL, "bsearch misses");
    calls = 0;
    check(bsearch(&key, e, 0, sizeof(e[0]), cmp) == NULL && calls == 0, "bsearch of nothing");

    /* qsort_r passes its argument to the comparison function */
    sign = -1;
    qsort_r(e, 50, sizeof(e[0]), cmp_r, &sign);
    for (i = 0; i < 50; i++)
        check(e[i].key == 49 - i, "qsort_r with the argument reversing the order");
    check(check_tags(e, 50), "qsort_r moves whole elements");

    printf("success\n");
    return 0;
}
