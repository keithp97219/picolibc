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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * fmemopen: the modes, positions and end positions, the null byte after
 * writes, and EINVAL from fseek outside the buffer
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    char  buf[8];
    FILE *f;

    /* 'r': reads stop at max_size, nulls aren't special, the buffer isn't changed */
    memcpy(buf, "ab\0cdefg", 8);
    f = fmemopen(buf, 5, "r");
    check(f != NULL, "fmemopen r");
    check(fgetc(f) == 'a' && fgetc(f) == 'b' && fgetc(f) == 0 && fgetc(f) == 'c', "read");
    check(fgetc(f) == 'd' && fgetc(f) == EOF && feof(f), "end at max_size");
    check(fseek(f, -1, SEEK_END) == 0 && fgetc(f) == 'd', "SEEK_END relative to max_size");
    check(fputc('x', f) == EOF, "can't write");
    check(fclose(f) == 0 && memcmp(buf, "ab\0cdefg", 8) == 0, "buffer unchanged");

    /* 'w' truncates with a null byte; writes add a null when it fits */
    memcpy(buf, "xxxxxxxx", 8);
    f = fmemopen(buf, 4, "w");
    check(f != NULL, "fmemopen w");
#ifndef __GLIBC__
    /* glibc doesn't write the null byte until something is written */
    check(buf[0] == '\0', "w truncates");
#endif
    check(fputc('1', f) == '1' && fflush(f) == 0 && memcmp(buf, "1\0", 2) == 0, "null after write");
    check(fputs("234", f) >= 0 && fflush(f) == 0, "fill");
#ifndef __GLIBC__
    /* glibc writes a null byte past max_size */
    check(memcmp(buf, "1234x", 5) == 0, "no null past max_size");
#endif
    check(fputc('5', f) == EOF || fflush(f) == EOF, "no write past max_size");
    check(fseek(f, 0, SEEK_END) == 0 && ftell(f) == 4, "end position is max_size");
    (void)fclose(f);

    /* 'a': starts at the first null; writes go to the end */
    memcpy(buf, "ab\0xxxxx", 8);
    f = fmemopen(buf, 8, "a+");
    check(f != NULL && ftell(f) == 2, "a starts at the first null");
    check(fseek(f, 0, SEEK_SET) == 0 && fgetc(f) == 'a', "read from the start");
    check(fseek(f, 0, SEEK_SET) == 0 && fputc('c', f) == 'c' && fflush(f) == 0, "append");
    check(memcmp(buf, "abc\0", 4) == 0, "written at the end");
    check(fclose(f) == 0, "fclose");

    /* A null buffer with '+' is allocated and starts empty */
    f = fmemopen(NULL, 8, "w+");
    check(f != NULL && fputs("hi", f) >= 0 && fseek(f, 0, SEEK_SET) == 0, "fmemopen NULL w+");
    check(fgetc(f) == 'h' && fgetc(f) == 'i' && fgetc(f) == EOF, "read back");
    check(fclose(f) == 0, "fclose");

    /* Seeking before the start or past max_size fails with EINVAL */
    f = fmemopen(buf, 8, "r+");
    check(f != NULL, "fmemopen r+");
    errno = 0;
    check(fseek(f, -1, SEEK_SET) == -1 && errno == EINVAL, "negative position");
    errno = 0;
    check(fseek(f, 9, SEEK_SET) == -1 && errno == EINVAL, "past max_size");
    check(fseek(f, 8, SEEK_SET) == 0 && ftell(f) == 8, "max_size itself");
    check(fclose(f) == 0, "fclose");

    printf("success\n");
    return 0;
}
