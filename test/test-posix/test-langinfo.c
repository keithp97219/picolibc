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

#define _POSIX_C_SOURCE 200809L
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * nl_langinfo gives an empty string for an invalid item and a codeset
 * name for CODESET. In strict POSIX mode, <langinfo.h> doesn't define
 * YESSTR and NOSTR, which POSIX removed and doesn't reserve.
 */

static int YESSTR;
static int NOSTR;

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static const nl_item invalid_items[] = { -1, 99999, 0x7fffffff };

#define NINVALID (sizeof(invalid_items) / sizeof(invalid_items[0]))

int
main(void)
{
    size_t   i;
    locale_t l;
    char    *s;

    (void)YESSTR;
    (void)NOSTR;

    l = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    check(l != (locale_t)0, "newlocale C");
    for (i = 0; i < NINVALID; i++) {
        s = nl_langinfo(invalid_items[i]);
        check(s != NULL && *s == '\0', "invalid item");
        s = nl_langinfo_l(invalid_items[i], l);
        check(s != NULL && *s == '\0', "invalid item with nl_langinfo_l");
    }

#ifdef __PICOLIBC__
    /* CODESET names a codeset, not a locale */
    check(strcmp(nl_langinfo(CODESET), "ASCII") == 0, "C codeset");
    check(strcmp(nl_langinfo_l(CODESET, l), "ASCII") == 0, "C codeset with nl_langinfo_l");
    if (setlocale(LC_CTYPE, "C.UTF-8") != NULL) {
        check(strcmp(nl_langinfo(CODESET), "UTF-8") == 0, "C.UTF-8 codeset");
        setlocale(LC_ALL, "C");
    }
#endif
    freelocale(l);

    printf("success\n");
    return 0;
}
