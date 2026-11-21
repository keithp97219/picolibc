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
/* glibc declares asprintf and vasprintf only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>

/* <stdio.h>: the types, macros and prototypes of POSIX.1-2024 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Types */
static FILE    *file_ptr __attribute__((unused));
static fpos_t   pos __attribute__((unused));
static off_t    off __attribute__((unused));
static size_t   size __attribute__((unused));
static ssize_t  ssize __attribute__((unused));
static va_list *ap __attribute__((unused));

_Static_assert((off_t)-1 < 0, "off_t is signed");
_Static_assert((ssize_t)-1 < 0, "ssize_t is signed");
_Static_assert((size_t)-1 > 0, "size_t is unsigned");

/* Integer constant expressions */
_Static_assert(BUFSIZ > 0, "BUFSIZ");
_Static_assert(L_ctermid > 0, "L_ctermid");
_Static_assert(L_tmpnam > 0, "L_tmpnam");
_Static_assert(_IOFBF != _IOLBF && _IOFBF != _IONBF && _IOLBF != _IONBF, "_IO*BF distinct");
_Static_assert(SEEK_CUR != SEEK_END && SEEK_CUR != SEEK_SET && SEEK_END != SEEK_SET,
               "SEEK_* distinct");
_Static_assert(FILENAME_MAX > 0, "FILENAME_MAX");
_Static_assert(FOPEN_MAX >= 8, "FOPEN_MAX");
_Static_assert(TMP_MAX >= 25, "TMP_MAX");
_Static_assert(EOF < 0, "EOF");
_Static_assert(_Generic(EOF, int: 1, default: 0), "EOF is an int");

#if !defined(BUFSIZ) || !defined(L_ctermid) || !defined(L_tmpnam) || !defined(FILENAME_MAX) \
    || !defined(FOPEN_MAX) || !defined(TMP_MAX) || !defined(EOF) || !defined(NULL)          \
    || !defined(SEEK_SET) || !defined(_IONBF) || !defined(stdin) || !defined(stdout)        \
    || !defined(stderr)
#error missing <stdio.h> macro
#endif

/* Every function, with its type; _Generic doesn't evaluate, so nothing is linked */
#define proto(name, type) _Static_assert(_Generic((name), type: 1, default: 0), #name)

proto(clearerr, void (*)(FILE *));
proto(ctermid, char *(*)(char *));
proto(dprintf, int (*)(int, const char *, ...));
proto(fclose, int (*)(FILE *));
proto(fdopen, FILE *(*)(int, const char *));
proto(feof, int (*)(FILE *));
proto(ferror, int (*)(FILE *));
proto(fflush, int (*)(FILE *));
proto(fgetc, int (*)(FILE *));
proto(fgetpos, int (*)(FILE *, fpos_t *));
proto(fgets, char *(*)(char *, int, FILE *));
proto(fileno, int (*)(FILE *));
proto(flockfile, void (*)(FILE *));
proto(fmemopen, FILE *(*)(void *, size_t, const char *));
proto(fopen, FILE *(*)(const char *, const char *));
proto(fprintf, int (*)(FILE *, const char *, ...));
proto(fputc, int (*)(int, FILE *));
proto(fputs, int (*)(const char *, FILE *));
proto(fread, size_t (*)(void *, size_t, size_t, FILE *));
proto(freopen, FILE *(*)(const char *, const char *, FILE *));
proto(fscanf, int (*)(FILE *, const char *, ...));
proto(fseek, int (*)(FILE *, long, int));
proto(fseeko, int (*)(FILE *, off_t, int));
proto(fsetpos, int (*)(FILE *, const fpos_t *));
proto(ftell, long (*)(FILE *));
proto(ftello, off_t (*)(FILE *));
proto(ftrylockfile, int (*)(FILE *));
proto(funlockfile, void (*)(FILE *));
proto(fwrite, size_t (*)(const void *, size_t, size_t, FILE *));
proto(getc, int (*)(FILE *));
proto(getchar, int (*)(void));
proto(getc_unlocked, int (*)(FILE *));
proto(getchar_unlocked, int (*)(void));
proto(getdelim, ssize_t (*)(char **, size_t *, int, FILE *));
proto(getline, ssize_t (*)(char **, size_t *, FILE *));
proto(open_memstream, FILE *(*)(char **, size_t *));
proto(pclose, int (*)(FILE *));
proto(perror, void (*)(const char *));
proto(popen, FILE *(*)(const char *, const char *));
proto(printf, int (*)(const char *, ...));
proto(putc, int (*)(int, FILE *));
proto(putchar, int (*)(int));
proto(putc_unlocked, int (*)(int, FILE *));
proto(putchar_unlocked, int (*)(int));
proto(puts, int (*)(const char *));
proto(remove, int (*)(const char *));
proto(rename, int (*)(const char *, const char *));
proto(renameat, int (*)(int, const char *, int, const char *));
proto(rewind, void (*)(FILE *));
proto(scanf, int (*)(const char *, ...));
proto(setbuf, void (*)(FILE *, char *));
proto(setvbuf, int (*)(FILE *, char *, int, size_t));
proto(snprintf, int (*)(char *, size_t, const char *, ...));
proto(sprintf, int (*)(char *, const char *, ...));
proto(sscanf, int (*)(const char *, const char *, ...));
proto(tmpfile, FILE *(*)(void));
proto(tmpnam, char *(*)(char *));
proto(ungetc, int (*)(int, FILE *));
proto(vdprintf, int (*)(int, const char *, va_list));
proto(vfprintf, int (*)(FILE *, const char *, va_list));
proto(vfscanf, int (*)(FILE *, const char *, va_list));
proto(vprintf, int (*)(const char *, va_list));
proto(vscanf, int (*)(const char *, va_list));
proto(vsnprintf, int (*)(char *, size_t, const char *, va_list));
proto(vsprintf, int (*)(char *, const char *, va_list));
proto(vsscanf, int (*)(const char *, const char *, va_list));
/* POSIX.1-2024 additions */
proto(asprintf, int (*)(char **, const char *, ...));
proto(vasprintf, int (*)(char **, const char *, va_list));

int
main(void)
{
    char buf[L_ctermid];

    /* The standard streams are pointers to FILE */
    check(_Generic(stdin, FILE *: 1, default: 0) && _Generic(stdout, FILE *: 1, default: 0)
              && _Generic(stderr, FILE *: 1, default: 0),
          "stdin, stdout and stderr are FILE *");
    check(stdin != NULL && stdout != NULL && stderr != NULL, "standard streams");

    /* The function-like macros take the function's arguments */
    check(sizeof(getchar_unlocked()) == sizeof(int), "getchar_unlocked()");
    check(sizeof(putchar_unlocked('x')) == sizeof(int), "putchar_unlocked(c)");
    check(sizeof(getc_unlocked(stdin)) == sizeof(int), "getc_unlocked(f)");
    check(sizeof(putc_unlocked('x', stdout)) == sizeof(int), "putc_unlocked(c, f)");
    check(sizeof(getc(stdin)) == sizeof(int) && sizeof(putc('x', stdout)) == sizeof(int),
          "getc and putc");

    check(ctermid(buf) == buf && ctermid(NULL) != NULL, "ctermid");

    printf("success\n");
    return 0;
}
