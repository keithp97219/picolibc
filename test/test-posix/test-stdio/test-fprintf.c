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
/* glibc declares asprintf only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../posix-groups.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../is-qemu.h"

/*
 * fprintf and friends: numbered arguments (%n$, *m$), asprintf,
 * snprintf with n zero, and EOVERFLOW past INT_MAX
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define same(got, want, message)                                         \
    do {                                                                 \
        if (strcmp((got), (want)) != 0)                                  \
            printf("%s: got \"%s\", want \"%s\"\n", message, got, want); \
        check(strcmp((got), (want)) == 0, message);                      \
    } while (0)

static int
call_vsnprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, fmt);
    ret = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return ret;
}

int
main(void)
{
    char            buf[128];
    char           *s = NULL;
    int             ret;
    /* Not constants, so the compiler doesn't warn about the sizes */
    volatile size_t small = 3;
    volatile int    half = INT_MAX / 2 + 1;

    /* Numbered arguments, used in any order and more than once */
    check(snprintf(buf, sizeof(buf), "%2$s %1$s %2$s", "a", "b") == 5, "reorder count");
    same(buf, "b a b", "reorder");
    /* The example from the fprintf page */
    snprintf(buf, sizeof(buf), "%1$d:%2$.*3$d:%4$.*3$d", 9, 5, 2, 7);
    same(buf, "9:05:07", "*m$ precision");
    snprintf(buf, sizeof(buf), "%1$*2$d|%1$-*2$d|", 42, 5);
    same(buf, "   42|42   |", "*m$ width");
    snprintf(buf, sizeof(buf), "%3$s, %2$d. %1$s 100%%", "Juli", 3, "Sonntag");
    same(buf, "Sonntag, 3. Juli 100%", "%% mixed with %n$");
    /* At least NL_ARGMAX (9) arguments */
    snprintf(buf, sizeof(buf), "%9$d%8$d%7$d%6$d%5$d%4$d%3$d%2$d%1$d", 1, 2, 3, 4, 5, 6, 7, 8, 9);
    same(buf, "987654321", "nine numbered arguments");
    check(NL_ARGMAX >= 9, "NL_ARGMAX");
    /* Mixed types */
    snprintf(buf, sizeof(buf), "%3$c%2$ld%1$.1f", 2.5, 7L, 'x');
    same(buf, "x72.5", "mixed types");
    call_vsnprintf(buf, sizeof(buf), "%2$s%1$s", "y", "x");
    same(buf, "xy", "vsnprintf numbered");

    /* asprintf allocates the result and returns its length */
    ret = asprintf(&s, "%2$s-%1$d", 12, "ab");
    check(ret == 5 && s != NULL, "asprintf");
    same(s, "ab-12", "asprintf output");
    free(s);

    /* snprintf with n zero writes nothing, s may be null, and returns the length */
    check(snprintf(NULL, 0, "%d", 12345) == 5, "snprintf NULL 0");
    memset(buf, 'z', 4);
    check(snprintf(buf, 0, "abc") == 3 && buf[0] == 'z', "snprintf 0 writes nothing");
    check(snprintf(buf, small, "%s", "abcdef") == 6 && strcmp(buf, "ab") == 0,
          "snprintf truncates");

    /*
     * EOVERFLOW when the count would pass INT_MAX. It counts INT_MAX
     * bytes, which is too slow under qemu or on bare-metal simulators.
     */
#ifdef __PICOLIBC_POSIX_DEVICE_IO
    if (is_qemu()) {
        printf("qemu: skipping EOVERFLOW (it writes INT_MAX bytes)\n");
    } else {
        errno = 0;
        ret = snprintf(NULL, 0, "%*d%*d", half, 1, half, 2);
        check(ret < 0 && errno == EOVERFLOW, "EOVERFLOW");
    }
#else
    (void)half;
#endif

    /* scanf numbered arguments */
    {
        int  a = 0, b = 0;
        char word[8];

        check(sscanf("10 20 hi", "%2$d %1$d %3$7s", &a, &b, word) == 3, "sscanf numbered");
        check(a == 20 && b == 10 && strcmp(word, "hi") == 0, "sscanf numbered values");
    }

    printf("success\n");
    return 0;
}
