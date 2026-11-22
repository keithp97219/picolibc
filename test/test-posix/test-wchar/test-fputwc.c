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
#include "../test-flags.h"
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#if TEST_POSIX_FILE_SYSTEM
#include <fcntl.h>
#endif

/*
 * Wide-character stream I/O converts as if by wcrtomb and mbrtowc (XSH
 * 2.5): fputwc, fputws and fwprintf write multibyte characters,
 * fgetwc and fgetws read them, encoding errors give EILSEQ and set the
 * error indicator, and ungetwc pushes back any wide character. Also
 * swprintf's EOVERFLOW. The UTF-8 checks run where a C.UTF-8 locale
 * exists.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define SENTINEL EDOM

/* POSIX (CX) sets the error indicator for encoding errors too; glibc 2.43 doesn't */
#ifdef __GLIBC__
#define encoding_error(f) 1
#else
#define encoding_error(f) ferror(f)
#endif

#if TEST_POSIX_FILE_SYSTEM
static char name[64];
#else
static char mem[256];
#endif

/* A stream for output whose bytes can be read back (glibc's fmemopen streams start byte-oriented)
 */
static FILE *
output(void)
{
#if TEST_POSIX_FILE_SYSTEM
    FILE *f = tmpfile();
#else
    FILE *f;

    memset(mem, 0, sizeof(mem));
    f = fmemopen(mem, sizeof(mem), "w");
#endif
    check(f != NULL, "output stream");
    return f;
}

/* The bytes written so far */
static size_t
written(FILE *f, char *bytes, size_t max)
{
    long len;

    check(fflush(f) == 0, "fflush");
    len = ftell(f);
    check(len >= 0 && (size_t)len <= max, "ftell");
#if TEST_POSIX_FILE_SYSTEM
    check(pread(fileno(f), bytes, (size_t)len, 0) == len, "pread");
#else
    memcpy(bytes, mem, (size_t)len);
#endif
    return (size_t)len;
}

/* A new unoriented stream holding the bytes */
static FILE *
input(const char *bytes, size_t len)
{
    FILE *f;

#if TEST_POSIX_FILE_SYSTEM
    int fd;

    snprintf(name, sizeof(name), "fputwc-%ld", (long)getpid());
    fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0 && write(fd, bytes, len) == (ssize_t)len && close(fd) == 0, "input file");
    f = fopen(name, "r");
    check(f != NULL, "fopen");
    check(remove(name) == 0, "remove");
#else
    memcpy(mem, bytes, len);
    f = fmemopen(mem, len, "r");
    check(f != NULL, "fmemopen");
#endif
    return f;
}

static void
c_locale(void)
{
    FILE   *f;
    char    bytes[64];
    wchar_t line[16];
    long    pos;

    /* One byte for each portable character */
    f = output();
    errno = SENTINEL;
    check(fputwc(L'A', f) == L'A', "fputwc");
    check(fputws(L"bc", f) >= 0, "fputws");
    check(fwprintf(f, L"%d%ls", 4, L"de") == 3, "fwprintf");
    check(written(f, bytes, sizeof(bytes)) == 6 && memcmp(bytes, "Abc4de", 6) == 0,
          "single bytes written");
    check(errno == SENTINEL && !ferror(f), "errno after the output");

    /*
     * A wide character the locale lacks: EILSEQ and the error indicator.
     * glibc 2.43's fputwc writes something for it in the C locale.
     */
#if WCHAR_MAX > 0x10000 && !defined(__GLIBC__)
    errno = 0;
    check(fputwc((wchar_t)0x10000, f) == WEOF && errno == EILSEQ, "fputwc EILSEQ");
    check(ferror(f), "fputwc EILSEQ sets the error indicator");
    clearerr(f);
    {
        static const wchar_t bad[] = { L'x', (wchar_t)0x10000, L'\0' };

        errno = 0;
        check(fputws(bad, f) == -1 && errno == EILSEQ && ferror(f), "fputws EILSEQ");
        clearerr(f);
        errno = 0;
        check(fwprintf(f, L"%lc", (wint_t)0x10000) < 0 && errno == EILSEQ, "fwprintf EILSEQ");
    }
#endif
    fclose(f);

    /* Reading converts back; end-of-file sets the indicator */
    f = input("ab\ncd", 5);
    errno = SENTINEL;
    check(fgetwc(f) == L'a', "fgetwc");
    pos = ftell(f);
    check(ungetwc(WEOF, f) == WEOF, "ungetwc(WEOF) fails");
    check(ftell(f) == pos, "ungetwc(WEOF) leaves the stream alone");
    check(ungetwc(L'z', f) == L'z', "ungetwc");
    check(fgetwc(f) == L'z', "fgetwc returns the pushed-back character");
    check(ftell(f) == pos, "the position is back where it was");
    check(fgetws(line, 16, f) == line && wcscmp(line, L"b\n") == 0, "fgetws to the newline");
    check(fgetws(line, 16, f) == line && wcscmp(line, L"cd") == 0, "fgetws of the last line");
    check(fgetwc(f) == WEOF && feof(f) && !ferror(f), "fgetwc at the end");
    check(fgetws(line, 16, f) == NULL, "fgetws at the end");
    check(ungetwc(L'q', f) == L'q' && !feof(f), "ungetwc clears end-of-file");
    check(fgetwc(f) == L'q', "and the character comes back");
    check(errno == SENTINEL, "errno after the input");
    fclose(f);

    {
        /* swprintf: EOVERFLOW when n or more characters were asked for */
        wchar_t buf[4];

        check(swprintf(buf, 4, L"abc") == 3 && wcscmp(buf, L"abc") == 0, "swprintf fits");
        errno = 0;
        check(swprintf(buf, 4, L"abcd") < 0, "swprintf too long");
#ifndef __GLIBC__
        /* glibc 2.43 doesn't set errno */
        check(errno == EOVERFLOW, "swprintf EOVERFLOW");
#endif
        errno = 0;
        check(swprintf(buf, 0, L"") < 0, "swprintf with n 0");
#ifndef __GLIBC__
        check(errno == EOVERFLOW, "swprintf with n 0 EOVERFLOW");
#endif
    }
}

static void
utf8_locale(void)
{
    FILE   *f;
    char    bytes[64];
    wchar_t line[16];

    /* Multibyte characters are written and read */
    f = output();
    errno = SENTINEL;
    check(fputwc(0xe9, f) == 0xe9, "fputwc of U+00E9");
    check(fputwc(0x20ac, f) == 0x20ac, "fputwc of U+20AC");
    check(written(f, bytes, sizeof(bytes)) == 5 && memcmp(bytes, "\xc3\xa9\xe2\x82\xac", 5) == 0,
          "UTF-8 written");
#ifndef __GLIBC__
    /* glibc 2.43's fputwc writes nothing for a surrogate and reports success */
    errno = 0;
    check(fputwc((wchar_t)0xd800, f) == WEOF && errno == EILSEQ && ferror(f), "fputwc EILSEQ");
#endif
    fclose(f);

    f = input("\xc3\xa9\xe2\x82\xac\n", 6);
    errno = SENTINEL;
    check(fgetwc(f) == 0xe9 && fgetwc(f) == 0x20ac && fgetwc(f) == L'\n', "fgetwc of UTF-8");
    check(errno == SENTINEL, "errno after fgetwc");
#if WCHAR_MAX > 0x10000
    check(ungetwc(0x1f600, f) == 0x1f600 && fgetwc(f) == 0x1f600, "ungetwc of U+1F600");
#endif
    fclose(f);

    /* Encoding errors: an invalid byte, and a character cut off at the end */
    f = input("a\xff", 2);
    check(fgetwc(f) == L'a', "before the invalid byte");
    errno = 0;
    check(fgetwc(f) == WEOF && errno == EILSEQ && encoding_error(f), "fgetwc EILSEQ");
    fclose(f);
    f = input("a\xc3", 2);
    check(fgetwc(f) == L'a', "before the partial character");
    errno = 0;
    check(fgetwc(f) == WEOF, "fgetwc of a partial character");
#ifndef __GLIBC__
    /* glibc 2.43 reports the cut-off character as end-of-file, without EILSEQ */
    check(errno == EILSEQ && ferror(f), "fgetwc of a partial character is EILSEQ");
#endif
    fclose(f);

    /* fgetws returns a null pointer after an encoding error in the line */
    f = input("ab\xff", 3);
    errno = 0;
    check(fgetws(line, 16, f) == NULL && errno == EILSEQ && encoding_error(f), "fgetws EILSEQ");
    fclose(f);
}

int
main(void)
{
    c_locale();

    if (setlocale(LC_CTYPE, "C.UTF-8") != NULL) {
        utf8_locale();
        check(setlocale(LC_CTYPE, "C") != NULL, "back to the C locale");
    } else {
        printf("no C.UTF-8 locale, skipping the UTF-8 checks\n");
    }

#if TEST_POSIX_FILE_SYSTEM
    {
        /* A write error sets the error indicator */
        FILE *f = fopen("/dev/full", "w");

        if (f != NULL) {
            check(setvbuf(f, NULL, _IONBF, 0) == 0, "unbuffered");
            errno = 0;
            check(fputwc(L'a', f) == WEOF && ferror(f) && errno == ENOSPC, "fputwc ENOSPC");
            fclose(f);
        } else {
            printf("no /dev/full, skipping the write error\n");
        }
    }
#endif

    printf("success\n");
    return 0;
}
