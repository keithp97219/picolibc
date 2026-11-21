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
#include <iconv.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * iconv_open's errors, and iconv's results: pointers and counts after
 * E2BIG, EILSEQ and EINVAL, null bytes, the reset call, and the
 * indicator suffixes for characters the output codeset lacks
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char   out[64];
static char  *inp, *outp;
static size_t inleft, outleft;

/* Convert len bytes of in into at most room bytes of out */
static size_t
convert(iconv_t cd, const char *in, size_t len, size_t room)
{
    memset(out, 'x', sizeof(out));
    inp = (char *)in;
    inleft = len;
    outp = out;
    outleft = room;
    errno = 0;
    return iconv(cd, &inp, &inleft, &outp, &outleft);
}

static iconv_t
open_ok(const char *to, const char *from)
{
    iconv_t cd = iconv_open(to, from);

    if (cd == (iconv_t)-1) {
        printf("iconv_open(\"%s\", \"%s\") failed\n", to, from);
        exit(1);
    }
    return cd;
}

static void
check_ascii(void)
{
    static const char hello[] = "hello";
    static const char nul[] = "a\0b";
    static const char bad[] = "ab\x80"
                              "cd";
    iconv_t           cd = open_ok("ASCII", "ASCII");
    char             *nullp = NULL;

    /* Everything converted */
    check(convert(cd, hello, 5, sizeof(out)) == 0, "convert");
    check(inleft == 0 && inp == hello + 5 && outleft == sizeof(out) - 5 && outp == out + 5
              && memcmp(out, hello, 5) == 0,
          "pointers after a full conversion");

    /* Null bytes are characters like any other */
    check(convert(cd, nul, 3, sizeof(out)) == 0 && inleft == 0 && outp == out + 3
              && memcmp(out, nul, 3) == 0,
          "null byte");

    /* E2BIG stops at the last character that fits */
    check(convert(cd, hello, 5, 3) == (size_t)-1 && errno == E2BIG, "E2BIG");
    check(inp == hello + 3 && inleft == 2 && outp == out + 3 && outleft == 0, "E2BIG pointers");
    check(convert(cd, hello, 5, 0) == (size_t)-1 && errno == E2BIG && inleft == 5 && inp == hello,
          "no room at all");

    /* EILSEQ stops before the invalid byte */
    check(convert(cd, bad, 5, sizeof(out)) == (size_t)-1 && errno == EILSEQ, "EILSEQ");
    check(inp == bad + 2 && inleft == 3 && outp == out + 2 && memcmp(out, "ab", 2) == 0,
          "EILSEQ pointers");

    /* The reset calls succeed and, for a stateless codeset, write nothing */
    check(iconv(cd, NULL, NULL, NULL, NULL) == 0, "reset");
    outp = out;
    outleft = sizeof(out);
    check(iconv(cd, NULL, NULL, &outp, &outleft) == 0 && outp == out && outleft == sizeof(out),
          "reset with an output buffer");
    inleft = 3;
    check(iconv(cd, &nullp, &inleft, &outp, &outleft) == 0 && outp == out, "*inbuf null");
    check(iconv_close(cd) == 0, "iconv_close");

#ifndef __GLIBC__
    /* glibc 2.43 reports EILSEQ at the end even though //IGNORE dropped the byte */
    cd = open_ok("ASCII//IGNORE", "ASCII");
    check(convert(cd, bad, 5, sizeof(out)) == 0 && inleft == 0 && outp == out + 4
              && memcmp(out, "abcd", 4) == 0,
          "//IGNORE drops invalid bytes");
    check(iconv_close(cd) == 0, "iconv_close");
#endif
}

static void
check_utf8(void)
{
    static const char e_acute[] = "a\xc3\xa9"
                                  "b";
    static const char two[] = "\xc3\xa9\xc3\xa9";
    static const char partial[] = "ab\xc3";
    static const char invalid[] = "a\xff";
    iconv_t           cd;
    size_t            r;

    cd = iconv_open("ASCII", "UTF-8");
    if (cd == (iconv_t)-1) {
        printf("no UTF-8 conversion, skipping its checks\n");
        return;
    }

#ifndef __GLIBC__
    /*
     * Without a suffix, a character missing from the output codeset
     * gets an implementation-defined conversion and is counted.
     * glibc 2.43 stops with EILSEQ.
     */
    r = convert(cd, e_acute, 4, sizeof(out));
    check(r == 1 && inleft == 0 && outp - out >= 3 && out[0] == 'a' && outp[-1] == 'b',
          "non-identical conversion counted");
#endif
    check(iconv_close(cd) == 0, "iconv_close");

    /* //TRANSLIT converts it to something and counts it */
    cd = open_ok("ASCII//TRANSLIT", "UTF-8");
    r = convert(cd, e_acute, 4, sizeof(out));
    check(r == 1 && inleft == 0 && out[0] == 'a' && outp[-1] == 'b', "//TRANSLIT");
    check(iconv_close(cd) == 0, "iconv_close");

#ifndef __GLIBC__
    /* glibc 2.43 reports EILSEQ after //IGNORE drops a character */
    cd = open_ok("ASCII//IGNORE", "UTF-8");
    r = convert(cd, e_acute, 4, sizeof(out));
    check(r == 1 && inleft == 0 && outp == out + 2 && memcmp(out, "ab", 2) == 0,
          "//IGNORE discards and counts");
    check(iconv_close(cd) == 0, "iconv_close");
#endif

#ifndef __GLIBC__
    /* glibc 2.43 accepts //NON_IDENTICAL_DISCARD but doesn't discard */
    cd = open_ok("ASCII//NON_IDENTICAL_DISCARD", "UTF-8");
    {
        r = convert(cd, e_acute, 4, sizeof(out));
        check(r == 1 && inleft == 0 && outp == out + 2 && memcmp(out, "ab", 2) == 0,
              "//NON_IDENTICAL_DISCARD discards and counts");
        check(iconv_close(cd) == 0, "iconv_close");
    }
#endif

    cd = open_ok("UTF-8", "UTF-8");

    /* An incomplete character at the end is EINVAL and stays unconverted */
    check(convert(cd, partial, 3, sizeof(out)) == (size_t)-1 && errno == EINVAL, "EINVAL");
    check(inp == partial + 2 && inleft == 1 && outp == out + 2, "EINVAL pointers");

    /* E2BIG doesn't split or consume a character that doesn't fit */
    check(convert(cd, two, 4, 3) == (size_t)-1 && errno == E2BIG, "E2BIG");
    check(inp == two + 2 && inleft == 2 && outp == out + 2 && outleft == 1, "E2BIG pointers");

    /* Invalid input is EILSEQ */
    check(convert(cd, invalid, 2, sizeof(out)) == (size_t)-1 && errno == EILSEQ, "EILSEQ");
    check(inp == invalid + 1 && inleft == 1 && outp == out + 1, "EILSEQ pointers");
    check(iconv_close(cd) == 0, "iconv_close");
}

int
main(void)
{
    size_t (*p_iconv)(iconv_t, char ** restrict, size_t * restrict, char ** restrict,
                      size_t * restrict)
        = iconv;
    int     (*p_iconv_close)(iconv_t) = iconv_close;
    iconv_t (*p_iconv_open)(const char *, const char *) = iconv_open;
    size_t  size __attribute__((unused)) = 0;

    check(p_iconv && p_iconv_close && p_iconv_open, "prototypes");

    /* Unknown codesets */
    errno = 0;
    check(iconv_open("NO-SUCH-CODESET", "ASCII") == (iconv_t)-1 && errno == EINVAL, "to");
    errno = 0;
    check(iconv_open("ASCII", "NO-SUCH-CODESET") == (iconv_t)-1 && errno == EINVAL, "from");
#ifdef __PICOLIBC__
    /* picolibc rejects suffixes it doesn't know, which POSIX allows */
    errno = 0;
    check(iconv_open("ASCII//NO-SUCH-SUFFIX", "ASCII") == (iconv_t)-1 && errno == EINVAL,
          "unknown suffix");

    /* An invalid descriptor may give EBADF */
    errno = 0;
    check(iconv((iconv_t)-1, NULL, NULL, NULL, NULL) == (size_t)-1 && errno == EBADF,
          "iconv EBADF");
    errno = 0;
    check(iconv_close((iconv_t)-1) == -1 && errno == EBADF, "iconv_close EBADF");
#endif

    check_ascii();
    check_utf8();

    printf("success\n");
    return 0;
}
