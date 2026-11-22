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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * Stream orientation (XSH 2.5.2) and fwide: streams start without
 * orientation, the first byte or wide I/O function sets it, fwide
 * changes only an unoriented stream, freopen removes it, and perror
 * leaves an unoriented stream unoriented.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#if !TEST_POSIX_FILE_SYSTEM
static char buf[256];
#endif

/* glibc's fmemopen streams start byte-oriented, so use files where there are files */
static FILE *
new_stream(void)
{
#if TEST_POSIX_FILE_SYSTEM
    FILE *f = tmpfile();
#else
    FILE *f = fmemopen(buf, sizeof(buf), "w+");
#endif

    check(f != NULL, "new stream");
    check(fwide(f, 0) == 0, "a new stream has no orientation");
    return f;
}

int
main(void)
{
    FILE *f;
    char  line[8];
    int   in, out, err;

    /* The standard streams are unoriented at start-up */
    in = fwide(stdin, 0);
    out = fwide(stdout, 0);
    err = fwide(stderr, 0);
    check(in == 0 && out == 0 && err == 0, "standard streams unoriented");

    /* fwide(f, 0) leaves it alone; fwide with a mode sets it, once */
    f = new_stream();
    check(fwide(f, 0) == 0, "fwide(f, 0) doesn't orient");
    errno = EDOM;
    check(fwide(f, 1) > 0, "fwide makes it wide");
    check(errno == EDOM, "fwide leaves errno alone");
    check(fwide(f, -1) > 0, "fwide doesn't change a wide stream");
    check(fwide(f, 0) > 0, "still wide");
    fclose(f);

    f = new_stream();
    check(fwide(f, -1) < 0, "fwide makes it byte-oriented");
    check(fwide(f, 1) < 0, "fwide doesn't change a byte stream");
    fclose(f);

    /* Byte functions make the stream byte-oriented */
    f = new_stream();
    check(fputc('a', f) == 'a', "fputc");
    check(fwide(f, 1) < 0, "fputc made it byte-oriented");
    fclose(f);
    f = new_stream();
    check(fputs("ab", f) >= 0 && fwide(f, 0) < 0, "fputs");
    fclose(f);
    f = new_stream();
    check(fprintf(f, "x") == 1 && fwide(f, 0) < 0, "fprintf");
    fclose(f);
    f = new_stream();
    check(fwrite("ab", 1, 2, f) == 2 && fwide(f, 0) < 0, "fwrite");
    fclose(f);
    f = new_stream();
    rewind(f);
    (void)fgetc(f);
    check(fwide(f, 0) < 0, "fgetc");
    fclose(f);
    f = new_stream();
    (void)fgets(line, sizeof(line), f);
    check(fwide(f, 0) < 0, "fgets");
    fclose(f);
    f = new_stream();
    (void)fread(line, 1, 1, f);
    check(fwide(f, 0) < 0, "fread");
    fclose(f);
    f = new_stream();
    (void)ungetc('a', f);
#ifndef __GLIBC__
    /* glibc 2.43's ungetc doesn't orient the stream, though C lists it as a byte function */
    check(fwide(f, 0) < 0, "ungetc");
#endif
    fclose(f);

    /* Wide functions make it wide-oriented */
    f = new_stream();
    check(fputwc(L'a', f) == L'a', "fputwc");
    check(fwide(f, -1) > 0, "fputwc made it wide");
    fclose(f);
    f = new_stream();
    check(fputws(L"ab", f) >= 0 && fwide(f, 0) > 0, "fputws");
    fclose(f);
    f = new_stream();
    check(fwprintf(f, L"x") == 1 && fwide(f, 0) > 0, "fwprintf");
    fclose(f);
    f = new_stream();
    (void)fgetwc(f);
    check(fwide(f, 0) > 0, "fgetwc");
    fclose(f);
    f = new_stream();
    (void)ungetwc(L'a', f);
    check(fwide(f, 0) > 0, "ungetwc");
    fclose(f);

    /* Other operations don't orient */
    f = new_stream();
    check(fflush(f) == 0 && fseek(f, 0, SEEK_SET) == 0 && ftell(f) == 0, "fflush, fseek, ftell");
    clearerr(f);
    (void)ferror(f);
    (void)feof(f);
    check(fwide(f, 0) == 0, "positioning and indicators don't orient");
    fclose(f);

#if TEST_POSIX_FILE_SYSTEM
    {
        /* freopen removes the orientation */
        char name[64];

        snprintf(name, sizeof(name), "fwide-%ld", (long)getpid());
        f = fopen(name, "w");
        check(f != NULL, "fopen");
        check(fwide(f, 1) > 0, "wide");
        f = freopen(name, "r", f);
        check(f != NULL, "freopen");
        check(fwide(f, 0) == 0, "freopen removed the orientation");
        check(fwide(f, -1) < 0, "and it can be set again");
        fclose(f);
        remove(name);
    }
#endif

    /* perror leaves an unoriented stderr unoriented */
    errno = ENOENT;
    perror("test-fwide");
#ifndef __GLIBC__
    /* glibc 2.43's perror makes an unoriented stderr byte-oriented */
    check(fwide(stderr, 0) == 0, "perror leaves stderr unoriented");
#endif

    printf("success\n");
    return 0;
}
