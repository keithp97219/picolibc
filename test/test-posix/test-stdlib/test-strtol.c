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
#include <stdlib.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
 * strtol, strtoll, strtoul and strtoull in the POSIX locale: the
 * subject sequence, endptr, the bases, EINVAL for an unsupported base,
 * ERANGE with the limits, and errno left alone on success.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

int
main(void)
{
    char       *end;
    const char *s;
    char        buf[80];

    /* Success leaves errno alone */
    errno = 1234;
    check(strtol(" \t\n-42z", &end, 10) == -42 && *end == 'z', "leading white space and a sign");
    check(strtol("+17", &end, 10) == 17 && *end == '\0', "plus sign");
    check(strtol("0x1f", &end, 0) == 31 && *end == '\0', "base 0 hexadecimal");
    check(strtol("0X1F", &end, 16) == 31 && *end == '\0', "0X prefix with base 16");
    check(strtol("0755", &end, 0) == 0755 && *end == '\0', "base 0 octal");
    check(strtol("0789", &end, 0) == 07 && *end == '8', "octal stops at 8");
    check(strtol("123", &end, 0) == 123, "base 0 decimal");
    check(strtol("zz", &end, 36) == 35 * 36 + 35 && *end == '\0', "base 36");
    check(strtol("Zz", &end, 36) == 35 * 36 + 35, "letters in either case");
    check(strtol("1012", &end, 2) == 5 && *end == '2', "only digits below the base");
    check(strtol("-0", &end, 10) == 0 && *end == '\0', "minus zero");
    check(strtoll("-9223372036854775808", &end, 10) == LLONG_MIN && *end == '\0', "LLONG_MIN");
    check(strtoul("-1", &end, 10) == ULONG_MAX, "strtoul negates in its return type");
    check(strtoull("-2", &end, 10) == ULLONG_MAX - 1, "strtoull negates in its return type");
    check(errno == 1234, "errno unchanged on success");

    /* "0x" without a hexadecimal digit converts the 0 */
    check(strtol("0xg", &end, 16) == 0 && *end == 'x', "0x without digits, base 16");
    check(strtol("0x", &end, 0) == 0 && *end == 'x', "0x without digits, base 0");

    /* No conversion: 0, and endptr is nptr */
    s = "  -  1";
    check(strtol(s, &end, 10) == 0 && end == s, "a sign alone");
    s = "   ";
    check(strtoul(s, &end, 10) == 0 && end == s, "only white space");
    s = "";
    check(strtoll(s, &end, 10) == 0 && end == s, "empty string");
    s = "g";
    check(strtoull(s, &end, 16) == 0 && end == s, "no digit of the base");
    check(strtol("12", NULL, 10) == 12, "a null endptr");

    /* An unsupported base: 0 and EINVAL */
    errno = 0;
    check(strtol("10", NULL, 1) == 0 && errno == EINVAL, "strtol base 1");
    errno = 0;
    check(strtoll("10", NULL, 37) == 0 && errno == EINVAL, "strtoll base 37");
    errno = 0;
    check(strtoul("10", NULL, -1) == 0 && errno == EINVAL, "strtoul base -1");
    errno = 0;
    check(strtoull("10", NULL, 99) == 0 && errno == EINVAL, "strtoull base 99");

    /* Out of range: the limit for the sign, ERANGE, and the whole subject consumed */
    snprintf(buf, sizeof(buf), "%ld9x", LONG_MAX);
    errno = 0;
    check(strtol(buf, &end, 10) == LONG_MAX && errno == ERANGE && *end == 'x', "LONG_MAX");
    snprintf(buf, sizeof(buf), "%ld9", LONG_MIN);
    errno = 0;
    check(strtol(buf, &end, 10) == LONG_MIN && errno == ERANGE && *end == '\0', "LONG_MIN");
    snprintf(buf, sizeof(buf), "%ld", LONG_MIN);
    errno = 0;
    check(strtol(buf, &end, 10) == LONG_MIN && errno == 0, "LONG_MIN itself");
    snprintf(buf, sizeof(buf), "%lld0", LLONG_MAX);
    errno = 0;
    check(strtoll(buf, &end, 10) == LLONG_MAX && errno == ERANGE, "LLONG_MAX");
    errno = 0;
    check(strtoll("-99999999999999999999999", &end, 10) == LLONG_MIN && errno == ERANGE,
          "LLONG_MIN");
    snprintf(buf, sizeof(buf), "%lu0", ULONG_MAX);
    errno = 0;
    check(strtoul(buf, &end, 10) == ULONG_MAX && errno == ERANGE, "ULONG_MAX");
    snprintf(buf, sizeof(buf), "-%lu0", ULONG_MAX);
    errno = 0;
    check(strtoul(buf, &end, 10) == ULONG_MAX && errno == ERANGE, "negative ULONG_MAX");
    snprintf(buf, sizeof(buf), "%llx0", ULLONG_MAX);
    errno = 0;
    check(strtoull(buf, &end, 16) == ULLONG_MAX && errno == ERANGE && *end == '\0', "ULLONG_MAX");

    /* atoi and friends convert as strtol does with base 10 */
    check(atoi(" -12a") == -12 && atol("077") == 77 && atoll("+5") == 5, "atoi, atol, atoll");

    printf("success\n");
    return 0;
}
