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
/* remque is an XSI interface */
#define _XOPEN_SOURCE 800
#include <search.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * remque unlinks elements from doubly-linked lists, linear or circular,
 * at the head, the tail or in the middle, leaving the rest linked
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
};

int
main(void)
{
    struct elem a, b, c, d;

    /* Linear: middle, head, tail, and the last one */
    insque(&a, NULL);
    insque(&b, &a);
    insque(&c, &b);
    insque(&d, &c);
    remque(&b);
    check(a.next == &c && c.prev == &a, "middle removed");
    remque(&a);
    check(c.prev == NULL && c.next == &d && d.prev == &c, "head removed");
    remque(&d);
    check(c.next == NULL && c.prev == NULL, "tail removed");
    remque(&c);

    /* Circular */
    a.next = a.prev = &a;
    insque(&b, &a);
    insque(&c, &b);
    insque(&d, &c);
    remque(&c);
    check(b.next == &d && d.prev == &b, "circular middle removed");
    remque(&a);
    check(d.next == &b && b.prev == &d && b.next == &d && d.prev == &b, "circular head removed");
    remque(&d);
    check(b.next == &b && b.prev == &b, "circular down to one");

    printf("success\n");
    return 0;
}
