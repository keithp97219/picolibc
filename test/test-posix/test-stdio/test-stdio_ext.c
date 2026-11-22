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
#include "../posix-groups.h"

/*
 * The stdio_ext.h functions are a Solaris/glibc extension; check
 * picolibc and glibc (the -native test) against each other
 */
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && (defined(__PICOLIBC__) || defined(TEST_GLIBC))

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * stdio_ext.h: pending output, read/write direction, line buffering,
 * buffer size and purging, as gnulib's close-stdout, freading,
 * fwriting and fpurge use them
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char
first_byte(const char *name)
{
    FILE *f = fopen(name, "r");
    int   c;

    check(f != NULL, "reopen");
    c = getc(f);
    fclose(f);
    return (char)c;
}

int
main(void)
{
    char  dir[] = "stdio_ext-XXXXXX";
    char  buf[64];
    FILE *f;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "temporary directory");

    /* A write-only stream is always writing, never reading */
    f = fopen("data", "w");
    check(f != NULL, "fopen w");
    check(__fwritable(f) && !__freadable(f), "w is writable only");
    check(__fwriting(f) && !__freading(f), "w is writing");
    check(__fpending(f) == 0, "nothing pending yet");
    check(fputs("hello", f) >= 0, "fputs");
    check(__fpending(f) == 5, "five bytes pending");
    check(fflush(f) == 0 && __fpending(f) == 0, "fflush empties the buffer");

    /* __fpurge drops pending output */
    check(fputs("XYZ", f) >= 0 && __fpending(f) == 3, "more pending");
    __fpurge(f);
    check(__fpending(f) == 0, "purged");
    check(fclose(f) == 0, "fclose w");
    check(first_byte("data") == 'h', "purged output not written");

    /* A read-only stream is always reading */
    f = fopen("data", "r");
    check(f != NULL, "fopen r");
    check(__freadable(f) && !__fwritable(f), "r is readable only");
    check(__freading(f) && !__fwriting(f), "r is reading");
    check(getc(f) == 'h', "getc");
#ifdef __PICOLIBC__
    check(__freadahead(f) == 4, "four bytes buffered");
    {
        size_t      size = 0;
        const char *p = __freadptr(f, &size);

        check(p != NULL && size == 4 && memcmp(p, "ello", 4) == 0, "freadptr");
        __freadptrinc(f, 2);
        check(getc(f) == 'l', "freadptrinc skipped two bytes");
    }
    check(ungetc('l', f) == 'l' && __freadahead(f) == 2, "ungetc counts");
    {
        size_t size = 0;

        check(__freadptr(f, &size) == NULL, "no pointer with a pushed-back byte");
    }
#endif
    /* Purging the input drops what's buffered */
    __fpurge(f);
    check(getc(f) == EOF, "purged input is gone");
    check(fclose(f) == 0, "fclose r");

    /* An update stream follows the last operation */
    f = fopen("data", "r+");
    check(f != NULL, "fopen r+");
    check(__freadable(f) && __fwritable(f), "r+ is read/write");
    check(getc(f) == 'h', "getc r+");
    check(__freading(f) && !__fwriting(f), "reading after getc");
    check(fseek(f, 0, SEEK_CUR) == 0, "fseek before write");
    check(putc('E', f) == 'E', "putc");
    check(__fwriting(f) && !__freading(f), "writing after putc");
    check(__fpending(f) == 1, "one byte pending");
    check(fseek(f, 0, SEEK_SET) == 0, "fseek before read");
    check(ungetc('x', f) == 'x', "ungetc");
    check(__freading(f), "reading after ungetc");
    check(fclose(f) == 0, "fclose r+");

    /* Buffer mode and size */
    f = fopen("data", "w");
    check(f != NULL, "fopen w");
    check(setvbuf(f, buf, _IOLBF, sizeof(buf)) == 0, "setvbuf");
    check(__flbf(f), "line buffered");
    check(__fbufsize(f) == sizeof(buf), "buffer size");
    check(putc('a', f) == 'a' && __fpending(f) == 1, "line pending");
    check(putc('\n', f) == '\n' && __fpending(f) == 0, "newline flushes");
    check(__fsetlocking(f, FSETLOCKING_QUERY) != 0, "locking query");
    check(fclose(f) == 0, "fclose lbf");

    check(unlink("data") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
