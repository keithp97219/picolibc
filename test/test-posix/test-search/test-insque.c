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
/* insque is an XSI interface */
#define _XOPEN_SOURCE 800
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * insque links elements into doubly-linked lists, linear (ended by null
 * pointers, started with insque(&e, NULL)) or circular (started by
 * pointing the first element at itself), after the given predecessor,
 * whatever the structure's names and further members are
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

struct elem {
    struct elem *next, *prev;
    int          value;
};

/* Any names, and more members after the two pointers */
struct other {
    struct other *fwd;
    struct other *back;
    char          payload[13];
};

int
main(void)
{
    struct elem  a = { .value = 1 }, b = { .value = 2 }, c = { .value = 3 }, d = { .value = 4 };
    struct other x, y;

    /* A linear list: insque(&a, NULL) clears both pointers */
    a.next = a.prev = &d;
    insque(&a, NULL);
    check(a.next == NULL && a.prev == NULL, "new list");
    insque(&c, &a);
    insque(&b, &a);
    check(a.next == &b && b.next == &c && c.next == NULL, "forward links");
    check(c.prev == &b && b.prev == &a && a.prev == NULL, "backward links");

    /* Inserting after the last element */
    insque(&d, &c);
    check(c.next == &d && d.prev == &c && d.next == NULL, "insert at the end");
    check(a.value == 1 && b.value == 2 && c.value == 3 && d.value == 4, "values untouched");

    /* A circular list of one element pointing at itself */
    a.next = a.prev = &a;
    insque(&b, &a);
    check(a.next == &b && b.next == &a && a.prev == &b && b.prev == &a, "circular two");
    insque(&c, &b);
    check(a.next == &b && b.next == &c && c.next == &a, "circular forward");
    check(a.prev == &c && c.prev == &b && b.prev == &a, "circular backward");
    insque(&d, &a);
    check(a.next == &d && d.next == &b && b.prev == &d && d.prev == &a, "circular middle");

    /* Other member names, and data after the pointers */
    memset(x.payload, 'x', sizeof(x.payload));
    memset(y.payload, 'y', sizeof(y.payload));
    insque(&x, NULL);
    insque(&y, &x);
    check(x.fwd == &y && y.back == &x && y.fwd == NULL && x.back == NULL, "other names");
    check(x.payload[0] == 'x' && x.payload[12] == 'x' && y.payload[0] == 'y'
              && y.payload[12] == 'y',
          "payload untouched");

    printf("success\n");
    return 0;
}
