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
 * <string.h> in strict POSIX mode: its types, macros and prototypes,
 * and nothing it doesn't reserve.
 *
 * An application may define any name that isn't reserved as a macro
 * before including the header, so these parameter names (and free,
 * which is only reserved with <stdlib.h>) must not appear in it.
 */
#define a         )
#define c         )
#define h         )
#define n         )
#define s         )
#define s1        )
#define s2        )
#define dst       )
#define src       )
#define str       )
#define size      )
#define state     )
#define sep       )
#define delim     )
#define errnum    )
#define signum    )
#define locale    )
#define maxlen    )
#define buflen    )
#define dstsize   )
#define strerrbuf )
#define haystack  )
#define needle    )
#define free      )

#include <string.h>

#undef a
#undef c
#undef h
#undef n
#undef s
#undef s1
#undef s2
#undef dst
#undef src
#undef str
#undef size
#undef state
#undef sep
#undef delim
#undef errnum
#undef signum
#undef locale
#undef maxlen
#undef buflen
#undef dstsize
#undef strerrbuf
#undef haystack
#undef needle
#undef free

/*
 * Names <string.h> must not declare in strict POSIX mode: BSD and GNU
 * extensions, the XSI memccpy, the <strings.h> functions and the
 * interfaces removed from <strings.h>. Each is defined here with
 * another type, which doesn't compile if the header declared it.
 */
static int bcmp __attribute__((unused));
static int bcopy __attribute__((unused));
static int bzero __attribute__((unused));
static int explicit_bzero __attribute__((unused));
static int ffs __attribute__((unused));
static int free __attribute__((unused));
static int index __attribute__((unused));
static int memccpy __attribute__((unused));
static int mempcpy __attribute__((unused));
static int memrchr __attribute__((unused));
static int rawmemchr __attribute__((unused));
static int rindex __attribute__((unused));
static int strcasecmp __attribute__((unused));
static int strcasestr __attribute__((unused));
static int strchrnul __attribute__((unused));
static int strerrordesc_np __attribute__((unused));
static int strerrorname_np __attribute__((unused));
static int strlwr __attribute__((unused));
static int strncasecmp __attribute__((unused));
static int strnstr __attribute__((unused));
static int strsep __attribute__((unused));
static int strupr __attribute__((unused));
static int strverscmp __attribute__((unused));
static int timingsafe_bcmp __attribute__((unused));
static int timingsafe_memcmp __attribute__((unused));

#ifdef strdupa
#error "strdupa is defined in strict mode"
#endif
#ifdef strndupa
#error "strndupa is defined in strict mode"
#endif

#include <stdio.h>

#ifndef NULL
#error "NULL isn't defined"
#endif

/* size_t is the type of sizeof, and locale_t is defined */
_Static_assert(_Generic(sizeof(int), size_t: 1, default: 0), "size_t");
static locale_t a_locale __attribute__((unused));

/*
 * Each function is declared with the prototype POSIX gives. restrict
 * on a parameter doesn't change the function's type, so it isn't
 * checked here
 */
#define proto(f, type) _Static_assert(_Generic(&(f), type: 1, default: 0), #f " prototype")

proto(memchr, void *(*)(const void *, int, size_t));
proto(memcmp, int (*)(const void *, const void *, size_t));
proto(memcpy, void *(*)(void *, const void *, size_t));
/* glibc 2.43 declares memmem, strlcat and strlcpy only for _DEFAULT_SOURCE */
#ifndef __GLIBC__
proto(memmem, void *(*)(const void *, size_t, const void *, size_t));
#endif
proto(memmove, void *(*)(void *, const void *, size_t));
proto(memset, void *(*)(void *, int, size_t));
proto(stpcpy, char *(*)(char *, const char *));
proto(stpncpy, char *(*)(char *, const char *, size_t));
proto(strcat, char *(*)(char *, const char *));
proto(strchr, char *(*)(const char *, int));
proto(strcmp, int (*)(const char *, const char *));
proto(strcoll, int (*)(const char *, const char *));
proto(strcoll_l, int (*)(const char *, const char *, locale_t));
proto(strcpy, char *(*)(char *, const char *));
proto(strcspn, size_t (*)(const char *, const char *));
proto(strdup, char *(*)(const char *));
proto(strerror, char *(*)(int));
proto(strerror_l, char *(*)(int, locale_t));
proto(strerror_r, int (*)(int, char *, size_t));
/* glibc 2.43 declares memmem, strlcat and strlcpy only for _DEFAULT_SOURCE */
#ifndef __GLIBC__
proto(strlcat, size_t (*)(char *, const char *, size_t));
proto(strlcpy, size_t (*)(char *, const char *, size_t));
#endif
proto(strlen, size_t (*)(const char *));
proto(strncat, char *(*)(char *, const char *, size_t));
proto(strncmp, int (*)(const char *, const char *, size_t));
proto(strncpy, char *(*)(char *, const char *, size_t));
proto(strndup, char *(*)(const char *, size_t));
proto(strnlen, size_t (*)(const char *, size_t));
proto(strpbrk, char *(*)(const char *, const char *));
proto(strrchr, char *(*)(const char *, int));
proto(strsignal, char *(*)(int));
proto(strspn, size_t (*)(const char *, const char *));
proto(strstr, char *(*)(const char *, const char *));
proto(strtok, char *(*)(char *, const char *));
proto(strtok_r, char *(*)(char *, const char *, char **));
proto(strxfrm, size_t (*)(char *, const char *, size_t));
proto(strxfrm_l, size_t (*)(char *, const char *, size_t, locale_t));

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
    static const char hello[] = "hello";
    char              buf[16];
    char             *last;
    void             *p;
    volatile size_t   one = 1, three = 3;

    /*
     * Each one is a function: a macro version may be suppressed with
     * parentheses
     */
    check((memchr)(hello, 'l', 5) == hello + 2, "memchr");
    check((memcmp)(hello, "help", 4) < 0, "memcmp");
    check((memcpy)(buf, hello, 6) == buf && (strcmp)(buf, hello) == 0, "memcpy");
#ifndef __GLIBC__
    check((memmem)(hello, 5, "ll", 2) == hello + 2, "memmem");
#endif
    check((memmove)(buf + 1, buf, 6) == buf + 1 && (strcmp)(buf + 1, hello) == 0, "memmove");
    check((memset)(buf, 'x', 3) == buf && buf[2] == 'x', "memset");
    check((stpcpy)(buf, hello) == buf + 5, "stpcpy");
    check((stpncpy)(buf, hello, 8) == buf + 5, "stpncpy");
    check((strcat)(buf, "!") == buf && (strcmp)(buf, "hello!") == 0, "strcat");
    check((strchr)(hello, 'o') == hello + 4, "strchr");
    check((strcoll)("a", "b") < 0, "strcoll");
    check((strcpy)(buf, hello) == buf, "strcpy");
    check((strcspn)(hello, "l") == 2, "strcspn");
    p = (strdup)(hello);
    check(p != NULL && (strcmp)(p, hello) == 0, "strdup");
    check((strerror)(0) != NULL, "strerror");
    check((strerror_r)(0, buf, sizeof(buf)) == 0, "strerror_r");
#ifndef __GLIBC__
    check((strlcat)(buf, "x", sizeof(buf)) == (strlen)(buf), "strlcat");
    check((strlcpy)(buf, hello, sizeof(buf)) == 5, "strlcpy");
#endif
    check((strlen)(hello) == 5, "strlen");
    (strcpy)(buf, hello);
    check((strncat)(buf, "abc", one) == buf && (strcmp)(buf, "helloa") == 0, "strncat");
    check((strncmp)(hello, "help", 3) == 0, "strncmp");
    check((strncpy)(buf, hello, three) == buf && buf[2] == 'l', "strncpy");
    check((strnlen)(hello, 3) == 3, "strnlen");
    check((strpbrk)(hello, "ol") == hello + 2, "strpbrk");
    check((strrchr)(hello, 'l') == hello + 3, "strrchr");
    check((strsignal)(1) != NULL, "strsignal");
    check((strspn)(hello, "eh") == 2, "strspn");
    check((strstr)(hello, "lo") == hello + 3, "strstr");
    (strcpy)(buf, "a b");
    check((strtok)(buf, " ") == buf, "strtok");
    (strcpy)(buf, "a b");
    check((strtok_r)(buf, " ", &last) == buf, "strtok_r");
    check((strxfrm)(NULL, hello, 0) == 5, "strxfrm");
    printf("success\n");
    return 0;
}
