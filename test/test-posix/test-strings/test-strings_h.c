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

/*
 * <strings.h> in strict POSIX mode, without XSI: its types and
 * prototypes, and nothing it doesn't reserve. Parameter names an
 * application may use as macros must not appear in it.
 */
#define c      )
#define i      )
#define n      )
#define s      )
#define s1     )
#define s2     )
#define locale )

#include <strings.h>

#undef c
#undef i
#undef n
#undef s
#undef s1
#undef s2
#undef locale

/*
 * ffs, ffsl and ffsll are XSI; bcmp, bcopy, bzero, index and rindex
 * were removed in Issue 7; the rest are BSD extensions. Each is
 * defined here with another type, which doesn't compile if the header
 * declared it.
 */
static int bcmp __attribute__((unused));
static int bcopy __attribute__((unused));
static int bzero __attribute__((unused));
static int explicit_bzero __attribute__((unused));
static int ffs __attribute__((unused));
static int ffsl __attribute__((unused));
static int ffsll __attribute__((unused));
static int fls __attribute__((unused));
static int flsl __attribute__((unused));
static int flsll __attribute__((unused));
static int index __attribute__((unused));
static int rindex __attribute__((unused));
/* <strings.h> isn't <string.h> */
static int strlen __attribute__((unused));
static int strcmp __attribute__((unused));

#include <stdio.h>

/* size_t is the type of sizeof, and locale_t is defined */
_Static_assert(_Generic(sizeof(int), size_t: 1, default: 0), "size_t");
static locale_t a_locale __attribute__((unused));

#define proto(f, type) _Static_assert(_Generic(&(f), type: 1, default: 0), #f " prototype")

proto(strcasecmp, int (*)(const char *, const char *));
proto(strcasecmp_l, int (*)(const char *, const char *, locale_t));
proto(strncasecmp, int (*)(const char *, const char *, size_t));
proto(strncasecmp_l, int (*)(const char *, const char *, size_t, locale_t));

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

int
main(void)
{
    /* Functions, even if there are macros too */
    check((strcasecmp)("Hello", "hELLO") == 0, "strcasecmp");
    check((strncasecmp)("Hello", "hELP", 3) == 0, "strncasecmp");
    printf("success\n");
    return 0;
}
