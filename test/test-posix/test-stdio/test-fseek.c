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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FD_MGMT) \
    && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * fseek, fseeko, ftell, ftello, fgetpos, fsetpos and rewind: positions,
 * the file offset after fflush (XSH 2.5.1), the error indicator and errno
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define DATA "0123456789"

static FILE *
open_data(const char *mode)
{
    FILE *f = fopen("data", mode);

    check(f != NULL, "open data");
    return f;
}

static off_t
offset(FILE *f)
{
    return lseek(fileno(f), 0, SEEK_CUR);
}

int
main(void)
{
    char   dir[] = "fseek-XXXXXX";
    FILE  *f;
    fpos_t pos;
    int    p[2];

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    f = open_data("w");
    check(fputs(DATA, f) >= 0 && fclose(f) == 0, "make data");

    /* SEEK_SET, SEEK_CUR and SEEK_END */
    f = open_data("r+");
    check(fseek(f, 3, SEEK_SET) == 0 && ftell(f) == 3 && fgetc(f) == '3', "SEEK_SET");
    check(fseek(f, 2, SEEK_CUR) == 0 && ftell(f) == 6 && fgetc(f) == '6', "SEEK_CUR");
    check(fseek(f, -2, SEEK_END) == 0 && ftell(f) == 8 && fgetc(f) == '8', "SEEK_END");
    check(fseeko(f, (off_t)1, SEEK_SET) == 0 && ftello(f) == 1, "fseeko and ftello");

    /* ftell doesn't change errno when it succeeds */
    errno = EDOM;
    check(ftell(f) == 1 && errno == EDOM, "ftell keeps errno");

    /* fseek clears end-of-file and drops pushed-back bytes */
    check(fseek(f, 0, SEEK_END) == 0 && fgetc(f) == EOF && feof(f), "end of file");
    check(fseek(f, 0, SEEK_SET) == 0 && !feof(f), "fseek clears end-of-file");
    check(fgetc(f) == '0' && fgetc(f) == '1', "read");
    check(ungetc('x', f) == 'x' && ftell(f) == 1, "ungetc moves the position back");
    check(fseek(f, 0, SEEK_CUR) == 0 && ftell(f) == 1 && fgetc(f) == '1', "fseek drops ungetc");

    /* fgetpos and fsetpos */
    check(fgetpos(f, &pos) == 0, "fgetpos");
    check(fgetc(f) == '2' && fgetc(f) == '3', "read on");
    check(fsetpos(f, &pos) == 0 && fgetc(f) == '2', "fsetpos");

    /* Past the end: the gap reads as zeros */
    check(fseek(f, 12, SEEK_SET) == 0 && fputc('Z', f) == 'Z', "write past the end");
    check(fseek(f, 10, SEEK_SET) == 0 && fgetc(f) == 0 && fgetc(f) == 0 && fgetc(f) == 'Z',
          "zero gap");

    /* Errors: EINVAL */
    errno = 0;
    check(fseek(f, -1, SEEK_SET) == -1 && errno == EINVAL, "negative position");
    errno = 0;
    check(fseek(f, -100, SEEK_CUR) == -1 && errno == EINVAL, "negative SEEK_CUR");
    errno = 0;
    check(fseek(f, 0, 12345) == -1 && errno == EINVAL, "bad whence");
    check(fclose(f) == 0, "close");

    /*
     * XSH 2.5.1 and fseek: after fflush, fseek sets the descriptor's
     * offset, even when another handle moved it.
     */
    f = open_data("r");
    check(fgetc(f) == '0' && fgetc(f) == '1' && fgetc(f) == '2', "read three");
    check(fflush(f) == 0 && offset(f) == 3, "fflush sets the offset");
    check(lseek(fileno(f), 7, SEEK_SET) == 7, "another handle moves the offset");
    /* ftell doesn't count as an operation here; its value is unspecified */
    (void)ftell(f);
    check(fseek(f, 3, SEEK_SET) == 0, "fseek after fflush");
#ifndef __GLIBC__
    /* glibc doesn't call lseek when the position is unchanged */
    check(offset(f) == 3, "fseek after fflush sets the offset");
#endif
    check(fgetc(f) == '3', "read after fseek");
    check(fclose(f) == 0, "close");

    f = open_data("r+");
    check(fputs("ab", f) >= 0 && fflush(f) == 0 && offset(f) == 2, "write and flush");
    check(lseek(fileno(f), 0, SEEK_END) == 13, "another handle moves the offset");
    check(fseek(f, 0, SEEK_CUR) == 0, "fseek SEEK_CUR after fflush");
#ifndef __GLIBC__
    /* glibc doesn't call lseek when the position is unchanged */
    check(offset(f) == 2, "fseek SEEK_CUR after fflush sets the offset");
#endif
    check(fclose(f) == 0, "close");

    /* ESPIPE on a pipe */
    check(pipe(p) == 0, "pipe");
    f = fdopen(p[0], "r");
    check(f != NULL, "fdopen pipe");
    errno = 0;
    check(fseek(f, 0, SEEK_SET) == -1 && errno == ESPIPE, "fseek ESPIPE");
    errno = 0;
    check(ftell(f) == -1 && errno == ESPIPE, "ftell ESPIPE");
    errno = 0;
    check(ftello(f) == -1 && errno == ESPIPE, "ftello ESPIPE");
    check(fclose(f) == 0 && close(p[1]) == 0, "close pipe");

    /* A write error while flushing sets the error indicator */
    f = fopen("/dev/full", "w");
    if (f) {
        check(setvbuf(f, NULL, _IOFBF, 64) == 0 && fputs("x", f) >= 0, "buffer a byte");
        errno = 0;
        check(fseek(f, 0, SEEK_SET) == -1 && errno == ENOSPC, "fseek ENOSPC");
        check(ferror(f), "fseek sets the error indicator");
        rewind(f);
        check(!ferror(f), "rewind clears the error indicator");
        check(fputs("y", f) >= 0, "buffer another");
        errno = 0;
        check(fflush(f) == EOF && errno == ENOSPC && ferror(f), "fflush ENOSPC");
        clearerr(f);
        check(fputs("z", f) >= 0, "buffer another");
        errno = 0;
        check(fclose(f) == EOF && errno == ENOSPC, "fclose ENOSPC");
    } else {
        printf("no /dev/full, skipping the write error checks\n");
    }

    /* ftell, fseek: EOVERFLOW when the offset doesn't fit in a long */
    if (sizeof(long) < sizeof(off_t)) {
        off_t big = (off_t)3 << 30;
        int   fd = open("big", O_RDWR | O_CREAT | O_TRUNC, 0600);

        check(fd >= 0 && ftruncate(fd, big) == 0, "sparse file");
        f = fdopen(fd, "r");
        check(f != NULL, "fdopen big");
        check(fseeko(f, big - 1, SEEK_SET) == 0 && ftello(f) == big - 1, "fseeko past LONG_MAX");
        errno = 0;
        check(ftell(f) == -1 && errno == EOVERFLOW, "ftell EOVERFLOW");
        check(fgetpos(f, &pos) == 0, "fgetpos past LONG_MAX");
        check(fseeko(f, 0, SEEK_SET) == 0 && fsetpos(f, &pos) == 0 && ftello(f) == big - 1,
              "fsetpos past LONG_MAX");
#ifndef __GLIBC__
        /* glibc's fseek succeeds when the new offset doesn't fit in a long */
        errno = 0;
        check(fseek(f, 0, SEEK_END) == -1 && errno == EOVERFLOW, "fseek EOVERFLOW");
#endif
        check(fclose(f) == 0 && unlink("big") == 0, "close big");
    }

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
