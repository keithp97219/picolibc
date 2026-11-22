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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* vsprintf and vsscanf */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
call_vsprintf(char *buf, const char *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vsprintf(buf, format, ap);
    va_end(ap);
    return ret;
}

static int
call_vsscanf(const char *s, const char *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vsscanf(s, format, ap);
    va_end(ap);
    return ret;
}

int
main(void)
{
    char buf[64];
    char word[8];
    int  a = 0, b = 0;

    memset(buf, 'x', sizeof(buf));
    check(call_vsprintf(buf, "%d-%s-%c", 42, "ab", 'z') == 7, "vsprintf");
    check(strcmp(buf, "42-ab-z") == 0 && buf[8] == 'x', "vsprintf output");
    check(call_vsprintf(buf, "") == 0 && buf[0] == '\0', "vsprintf of nothing");
    check(call_vsprintf(buf, "%2$s%1$s", "b", "a") == 2 && strcmp(buf, "ab") == 0,
          "vsprintf numbered arguments");

    check(call_vsscanf("12 34 word", "%d %d %7s", &a, &b, word) == 3 && a == 12 && b == 34
              && strcmp(word, "word") == 0,
          "vsscanf");
    check(call_vsscanf("12 x", "%d %d", &a, &b) == 1 && a == 12, "vsscanf matching failure");
    check(call_vsscanf("", "%d", &a) == EOF, "vsscanf of an empty string");

    printf("success\n");
    return 0;
}
