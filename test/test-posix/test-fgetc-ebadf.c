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

#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

/*
 * fgetc and fputc: EBADF and the error indicator for the wrong direction
 * (also through the other byte and wide functions), and the end-of-file
 * indicator staying set
 */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-fgetc-ebadf.txt"
#endif

#define NAME      TEST_FILE_NAME
#define WIDE_NAME TEST_FILE_NAME ".wide"

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(NAME);                        \
            (void)unlink(WIDE_NAME);                   \
            exit(1);                                   \
        }                                              \
    } while (0)

#define bad(call, fail, message)                                         \
    do {                                                                 \
        clearerr(f);                                                     \
        errno = 0;                                                       \
        check((call) == (fail) && errno == EBADF && ferror(f), message); \
    } while (0)

int
main(void)
{
    FILE *f;
    char  buf[8];
    int   fd;

    /* Reading a stream open only for writing */
    f = fopen(NAME, "w");
    check(f != NULL, "open w");
    bad(fgetc(f), EOF, "fgetc on a write-only stream");
    bad(getc(f), EOF, "getc on a write-only stream");
    bad(fgets(buf, sizeof(buf), f), NULL, "fgets on a write-only stream");
    bad(fread(buf, 1, 1, f), 0, "fread on a write-only stream");
    check(fputs("ab", f) >= 0 && fclose(f) == 0, "write");

    /* Writing a stream open only for reading */
    f = fopen(NAME, "r");
    check(f != NULL, "open r");
    bad(fputc('x', f), EOF, "fputc on a read-only stream");
    bad(putc('x', f), EOF, "putc on a read-only stream");
    bad(fputs("x", f), EOF, "fputs on a read-only stream");
    bad(fwrite("x", 1, 1, f), 0, "fwrite on a read-only stream");
    bad(fprintf(f, "x") < 0, 1, "fprintf on a read-only stream");
    clearerr(f);

    /* The wide functions, on streams of their own as they set the orientation */
    {
        FILE *r = f;

        f = fopen(WIDE_NAME, "w");
        check(f != NULL, "open wide w");
        bad(fgetwc(f), WEOF, "fgetwc on a write-only stream");
        check(fclose(f) == 0, "close wide");
        f = fopen(WIDE_NAME, "r");
        check(f != NULL, "open wide r");
        bad(fputwc(L'x', f), WEOF, "fputwc on a read-only stream");
        check(fclose(f) == 0, "close wide");
        f = r;
    }

    /* Once end-of-file is seen, fgetc keeps returning EOF */
    check(fgetc(f) == 'a' && fgetc(f) == 'b' && fgetc(f) == EOF && feof(f), "end of file");
    fd = open(NAME, O_WRONLY | O_APPEND);
    check(fd >= 0 && write(fd, "cd", 2) == 2 && close(fd) == 0, "append more data");
    check(fgetc(f) == EOF && feof(f), "fgetc with the end-of-file indicator set");
    check(fread(buf, 1, 1, f) == 0, "fread with the end-of-file indicator set");
    check(fgets(buf, sizeof(buf), f) == NULL, "fgets with the end-of-file indicator set");
    clearerr(f);
    check(fgetc(f) == 'c', "clearerr lets reading continue");
    check(fgetc(f) == 'd' && fgetc(f) == EOF && feof(f), "end of file again");
    check(ungetc('z', f) == 'z' && !feof(f) && fgetc(f) == 'z', "ungetc clears end-of-file");
    check(fclose(f) == 0, "close");

    check(unlink(NAME) == 0 && unlink(WIDE_NAME) == 0, "cleanup");
    printf("success\n");
    return 0;
}
