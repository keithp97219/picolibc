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
#include <inttypes.h>

#ifdef __GLIBC__
/* glibc's <inttypes.h> doesn't define wchar_t */
#include <stddef.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * <inttypes.h>: it includes <stdint.h>, defines imaxdiv_t and wchar_t
 * (the test uses wchar_t with no other header, except for glibc),
 * every PRI and SCN macro for the types <stdint.h> has, and the six
 * functions. strtoimax and the others follow the strtol rules.
 */

#ifndef INTMAX_MAX
#error "<inttypes.h> doesn't include <stdint.h>"
#endif

#ifndef PRId8
#error "PRId8"
#endif
#ifndef PRId16
#error "PRId16"
#endif
#ifndef PRId32
#error "PRId32"
#endif
#ifndef PRId64
#error "PRId64"
#endif
#ifndef PRIdLEAST8
#error "PRIdLEAST8"
#endif
#ifndef PRIdLEAST16
#error "PRIdLEAST16"
#endif
#ifndef PRIdLEAST32
#error "PRIdLEAST32"
#endif
#ifndef PRIdLEAST64
#error "PRIdLEAST64"
#endif
#ifndef PRIdFAST8
#error "PRIdFAST8"
#endif
#ifndef PRIdFAST16
#error "PRIdFAST16"
#endif
#ifndef PRIdFAST32
#error "PRIdFAST32"
#endif
#ifndef PRIdFAST64
#error "PRIdFAST64"
#endif
#ifndef PRIdMAX
#error "PRIdMAX"
#endif
#ifndef PRIdPTR
#error "PRIdPTR"
#endif
#ifndef PRIi8
#error "PRIi8"
#endif
#ifndef PRIi16
#error "PRIi16"
#endif
#ifndef PRIi32
#error "PRIi32"
#endif
#ifndef PRIi64
#error "PRIi64"
#endif
#ifndef PRIiLEAST8
#error "PRIiLEAST8"
#endif
#ifndef PRIiLEAST16
#error "PRIiLEAST16"
#endif
#ifndef PRIiLEAST32
#error "PRIiLEAST32"
#endif
#ifndef PRIiLEAST64
#error "PRIiLEAST64"
#endif
#ifndef PRIiFAST8
#error "PRIiFAST8"
#endif
#ifndef PRIiFAST16
#error "PRIiFAST16"
#endif
#ifndef PRIiFAST32
#error "PRIiFAST32"
#endif
#ifndef PRIiFAST64
#error "PRIiFAST64"
#endif
#ifndef PRIiMAX
#error "PRIiMAX"
#endif
#ifndef PRIiPTR
#error "PRIiPTR"
#endif
#ifndef PRIo8
#error "PRIo8"
#endif
#ifndef PRIo16
#error "PRIo16"
#endif
#ifndef PRIo32
#error "PRIo32"
#endif
#ifndef PRIo64
#error "PRIo64"
#endif
#ifndef PRIoLEAST8
#error "PRIoLEAST8"
#endif
#ifndef PRIoLEAST16
#error "PRIoLEAST16"
#endif
#ifndef PRIoLEAST32
#error "PRIoLEAST32"
#endif
#ifndef PRIoLEAST64
#error "PRIoLEAST64"
#endif
#ifndef PRIoFAST8
#error "PRIoFAST8"
#endif
#ifndef PRIoFAST16
#error "PRIoFAST16"
#endif
#ifndef PRIoFAST32
#error "PRIoFAST32"
#endif
#ifndef PRIoFAST64
#error "PRIoFAST64"
#endif
#ifndef PRIoMAX
#error "PRIoMAX"
#endif
#ifndef PRIoPTR
#error "PRIoPTR"
#endif
#ifndef PRIu8
#error "PRIu8"
#endif
#ifndef PRIu16
#error "PRIu16"
#endif
#ifndef PRIu32
#error "PRIu32"
#endif
#ifndef PRIu64
#error "PRIu64"
#endif
#ifndef PRIuLEAST8
#error "PRIuLEAST8"
#endif
#ifndef PRIuLEAST16
#error "PRIuLEAST16"
#endif
#ifndef PRIuLEAST32
#error "PRIuLEAST32"
#endif
#ifndef PRIuLEAST64
#error "PRIuLEAST64"
#endif
#ifndef PRIuFAST8
#error "PRIuFAST8"
#endif
#ifndef PRIuFAST16
#error "PRIuFAST16"
#endif
#ifndef PRIuFAST32
#error "PRIuFAST32"
#endif
#ifndef PRIuFAST64
#error "PRIuFAST64"
#endif
#ifndef PRIuMAX
#error "PRIuMAX"
#endif
#ifndef PRIuPTR
#error "PRIuPTR"
#endif
#ifndef PRIx8
#error "PRIx8"
#endif
#ifndef PRIx16
#error "PRIx16"
#endif
#ifndef PRIx32
#error "PRIx32"
#endif
#ifndef PRIx64
#error "PRIx64"
#endif
#ifndef PRIxLEAST8
#error "PRIxLEAST8"
#endif
#ifndef PRIxLEAST16
#error "PRIxLEAST16"
#endif
#ifndef PRIxLEAST32
#error "PRIxLEAST32"
#endif
#ifndef PRIxLEAST64
#error "PRIxLEAST64"
#endif
#ifndef PRIxFAST8
#error "PRIxFAST8"
#endif
#ifndef PRIxFAST16
#error "PRIxFAST16"
#endif
#ifndef PRIxFAST32
#error "PRIxFAST32"
#endif
#ifndef PRIxFAST64
#error "PRIxFAST64"
#endif
#ifndef PRIxMAX
#error "PRIxMAX"
#endif
#ifndef PRIxPTR
#error "PRIxPTR"
#endif
#ifndef PRIX8
#error "PRIX8"
#endif
#ifndef PRIX16
#error "PRIX16"
#endif
#ifndef PRIX32
#error "PRIX32"
#endif
#ifndef PRIX64
#error "PRIX64"
#endif
#ifndef PRIXLEAST8
#error "PRIXLEAST8"
#endif
#ifndef PRIXLEAST16
#error "PRIXLEAST16"
#endif
#ifndef PRIXLEAST32
#error "PRIXLEAST32"
#endif
#ifndef PRIXLEAST64
#error "PRIXLEAST64"
#endif
#ifndef PRIXFAST8
#error "PRIXFAST8"
#endif
#ifndef PRIXFAST16
#error "PRIXFAST16"
#endif
#ifndef PRIXFAST32
#error "PRIXFAST32"
#endif
#ifndef PRIXFAST64
#error "PRIXFAST64"
#endif
#ifndef PRIXMAX
#error "PRIXMAX"
#endif
#ifndef PRIXPTR
#error "PRIXPTR"
#endif
#ifndef SCNd8
#error "SCNd8"
#endif
#ifndef SCNd16
#error "SCNd16"
#endif
#ifndef SCNd32
#error "SCNd32"
#endif
#ifndef SCNd64
#error "SCNd64"
#endif
#ifndef SCNdLEAST8
#error "SCNdLEAST8"
#endif
#ifndef SCNdLEAST16
#error "SCNdLEAST16"
#endif
#ifndef SCNdLEAST32
#error "SCNdLEAST32"
#endif
#ifndef SCNdLEAST64
#error "SCNdLEAST64"
#endif
#ifndef SCNdFAST8
#error "SCNdFAST8"
#endif
#ifndef SCNdFAST16
#error "SCNdFAST16"
#endif
#ifndef SCNdFAST32
#error "SCNdFAST32"
#endif
#ifndef SCNdFAST64
#error "SCNdFAST64"
#endif
#ifndef SCNdMAX
#error "SCNdMAX"
#endif
#ifndef SCNdPTR
#error "SCNdPTR"
#endif
#ifndef SCNi8
#error "SCNi8"
#endif
#ifndef SCNi16
#error "SCNi16"
#endif
#ifndef SCNi32
#error "SCNi32"
#endif
#ifndef SCNi64
#error "SCNi64"
#endif
#ifndef SCNiLEAST8
#error "SCNiLEAST8"
#endif
#ifndef SCNiLEAST16
#error "SCNiLEAST16"
#endif
#ifndef SCNiLEAST32
#error "SCNiLEAST32"
#endif
#ifndef SCNiLEAST64
#error "SCNiLEAST64"
#endif
#ifndef SCNiFAST8
#error "SCNiFAST8"
#endif
#ifndef SCNiFAST16
#error "SCNiFAST16"
#endif
#ifndef SCNiFAST32
#error "SCNiFAST32"
#endif
#ifndef SCNiFAST64
#error "SCNiFAST64"
#endif
#ifndef SCNiMAX
#error "SCNiMAX"
#endif
#ifndef SCNiPTR
#error "SCNiPTR"
#endif
#ifndef SCNo8
#error "SCNo8"
#endif
#ifndef SCNo16
#error "SCNo16"
#endif
#ifndef SCNo32
#error "SCNo32"
#endif
#ifndef SCNo64
#error "SCNo64"
#endif
#ifndef SCNoLEAST8
#error "SCNoLEAST8"
#endif
#ifndef SCNoLEAST16
#error "SCNoLEAST16"
#endif
#ifndef SCNoLEAST32
#error "SCNoLEAST32"
#endif
#ifndef SCNoLEAST64
#error "SCNoLEAST64"
#endif
#ifndef SCNoFAST8
#error "SCNoFAST8"
#endif
#ifndef SCNoFAST16
#error "SCNoFAST16"
#endif
#ifndef SCNoFAST32
#error "SCNoFAST32"
#endif
#ifndef SCNoFAST64
#error "SCNoFAST64"
#endif
#ifndef SCNoMAX
#error "SCNoMAX"
#endif
#ifndef SCNoPTR
#error "SCNoPTR"
#endif
#ifndef SCNu8
#error "SCNu8"
#endif
#ifndef SCNu16
#error "SCNu16"
#endif
#ifndef SCNu32
#error "SCNu32"
#endif
#ifndef SCNu64
#error "SCNu64"
#endif
#ifndef SCNuLEAST8
#error "SCNuLEAST8"
#endif
#ifndef SCNuLEAST16
#error "SCNuLEAST16"
#endif
#ifndef SCNuLEAST32
#error "SCNuLEAST32"
#endif
#ifndef SCNuLEAST64
#error "SCNuLEAST64"
#endif
#ifndef SCNuFAST8
#error "SCNuFAST8"
#endif
#ifndef SCNuFAST16
#error "SCNuFAST16"
#endif
#ifndef SCNuFAST32
#error "SCNuFAST32"
#endif
#ifndef SCNuFAST64
#error "SCNuFAST64"
#endif
#ifndef SCNuMAX
#error "SCNuMAX"
#endif
#ifndef SCNuPTR
#error "SCNuPTR"
#endif
#ifndef SCNx8
#error "SCNx8"
#endif
#ifndef SCNx16
#error "SCNx16"
#endif
#ifndef SCNx32
#error "SCNx32"
#endif
#ifndef SCNx64
#error "SCNx64"
#endif
#ifndef SCNxLEAST8
#error "SCNxLEAST8"
#endif
#ifndef SCNxLEAST16
#error "SCNxLEAST16"
#endif
#ifndef SCNxLEAST32
#error "SCNxLEAST32"
#endif
#ifndef SCNxLEAST64
#error "SCNxLEAST64"
#endif
#ifndef SCNxFAST8
#error "SCNxFAST8"
#endif
#ifndef SCNxFAST16
#error "SCNxFAST16"
#endif
#ifndef SCNxFAST32
#error "SCNxFAST32"
#endif
#ifndef SCNxFAST64
#error "SCNxFAST64"
#endif
#ifndef SCNxMAX
#error "SCNxMAX"
#endif
#ifndef SCNxPTR
#error "SCNxPTR"
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

#define check_type(f, t) _Generic(&(f), t: 1, default: 0)
#define check_expr(e, t) _Generic((e), t: 1, default: 0)

/* Print a negative value and its unsigned counterpart with each macro */
#define PRI_CHECK(w, t, ut, v)                                                                     \
    do {                                                                                           \
        char t##_buf[64];                                                                          \
        t    sv = (t)(v);                                                                          \
        ut   uv = (ut)0x2a;                                                                        \
        snprintf(t##_buf, sizeof(t##_buf),                                                         \
                 "%" PRId##w " %" PRIi##w " %" PRIo##w " %" PRIu##w " %" PRIx##w " %" PRIX##w, sv, \
                 sv, uv, uv, uv, uv);                                                              \
        check(strcmp(t##_buf, "-42 -42 52 42 2a 2A") == 0, "PRI" #w);                              \
    } while (0)

/* Scan with each macro */
#define SCN_CHECK(w, t, ut)                                                                     \
    do {                                                                                        \
        t  sd = 0, si = 0;                                                                      \
        ut uo = 0, uu = 0, ux = 0;                                                              \
        check(sscanf("-42 -0x2a 52 42 2a",                                                      \
                     "%" SCNd##w " %" SCNi##w " %" SCNo##w " %" SCNu##w " %" SCNx##w, &sd, &si, \
                     &uo, &uu, &ux)                                                             \
                  == 5,                                                                         \
              "SCN" #w);                                                                        \
        check(sd == -42 && si == -42 && uo == 42 && uu == 42 && ux == 42, "SCN" #w " values");  \
    } while (0)

int
main(void)
{
    int       ok = 1;
    imaxdiv_t q;
    char     *end;
    wchar_t  *wend;
    char      big[64];
    wchar_t   wbig[64];
    size_t    i;

    ok &= check_type(imaxabs, intmax_t (*)(intmax_t));
    ok &= check_type(imaxdiv, imaxdiv_t (*)(intmax_t, intmax_t));
    ok &= check_type(strtoimax, intmax_t (*)(const char *, char **, int));
    ok &= check_type(strtoumax, uintmax_t (*)(const char *, char **, int));
    ok &= check_type(wcstoimax, intmax_t (*)(const wchar_t *, wchar_t **, int));
    ok &= check_type(wcstoumax, uintmax_t (*)(const wchar_t *, wchar_t **, int));
    q = imaxdiv(7, 2);
    ok &= check_expr(q.quot, intmax_t) && check_expr(q.rem, intmax_t);
    check(ok, "types and prototypes");

    PRI_CHECK(8, int8_t, uint8_t, -42);
    PRI_CHECK(16, int16_t, uint16_t, -42);
    PRI_CHECK(32, int32_t, uint32_t, -42);
    PRI_CHECK(64, int64_t, uint64_t, -42);
    PRI_CHECK(LEAST8, int_least8_t, uint_least8_t, -42);
    PRI_CHECK(LEAST16, int_least16_t, uint_least16_t, -42);
    PRI_CHECK(LEAST32, int_least32_t, uint_least32_t, -42);
    PRI_CHECK(LEAST64, int_least64_t, uint_least64_t, -42);
    PRI_CHECK(FAST8, int_fast8_t, uint_fast8_t, -42);
    PRI_CHECK(FAST16, int_fast16_t, uint_fast16_t, -42);
    PRI_CHECK(FAST32, int_fast32_t, uint_fast32_t, -42);
    PRI_CHECK(FAST64, int_fast64_t, uint_fast64_t, -42);
    PRI_CHECK(MAX, intmax_t, uintmax_t, -42);
    PRI_CHECK(PTR, intptr_t, uintptr_t, -42);
    SCN_CHECK(8, int8_t, uint8_t);
    SCN_CHECK(16, int16_t, uint16_t);
    SCN_CHECK(32, int32_t, uint32_t);
    SCN_CHECK(64, int64_t, uint64_t);
    SCN_CHECK(LEAST8, int_least8_t, uint_least8_t);
    SCN_CHECK(LEAST16, int_least16_t, uint_least16_t);
    SCN_CHECK(LEAST32, int_least32_t, uint_least32_t);
    SCN_CHECK(LEAST64, int_least64_t, uint_least64_t);
    SCN_CHECK(FAST8, int_fast8_t, uint_fast8_t);
    SCN_CHECK(FAST16, int_fast16_t, uint_fast16_t);
    SCN_CHECK(FAST32, int_fast32_t, uint_fast32_t);
    SCN_CHECK(FAST64, int_fast64_t, uint_fast64_t);
    SCN_CHECK(MAX, intmax_t, uintmax_t);
    SCN_CHECK(PTR, intptr_t, uintptr_t);

    check(imaxabs(-INTMAX_MAX) == INTMAX_MAX && imaxabs(5) == 5, "imaxabs");
    q = imaxdiv(-7, 2);
    check(q.quot == -3 && q.rem == -1, "imaxdiv truncates toward zero");
    q = imaxdiv(INTMAX_MAX, 10);
    check(q.quot == INTMAX_MAX / 10 && q.rem == INTMAX_MAX % 10, "imaxdiv INTMAX_MAX");

    /* Success leaves errno alone */
    errno = 1234;
    check(strtoimax(" -123x", &end, 10) == -123 && *end == 'x' && errno == 1234, "strtoimax");
    check(strtoumax("0x1F", &end, 0) == 31 && *end == '\0' && errno == 1234, "strtoumax base 0");
    check(strtoumax("-1", &end, 10) == UINTMAX_MAX && errno == 1234, "strtoumax negates");

    /* Out of range */
    snprintf(big, sizeof(big), "%" PRIuMAX "0", UINTMAX_MAX);
    errno = 0;
    check(strtoimax(big, &end, 10) == INTMAX_MAX && errno == ERANGE && *end == '\0',
          "strtoimax ERANGE high");
    big[0] = '-';
    errno = 0;
    check(strtoimax(big, &end, 10) == INTMAX_MIN && errno == ERANGE, "strtoimax ERANGE low");
    snprintf(big, sizeof(big), "%" PRIuMAX "0", UINTMAX_MAX);
    errno = 0;
    check(strtoumax(big, &end, 10) == UINTMAX_MAX && errno == ERANGE, "strtoumax ERANGE");

    /* An unsupported base */
    errno = 0;
    check(strtoimax("10", NULL, 1) == 0 && errno == EINVAL, "strtoimax base 1");
    errno = 0;
    check(strtoumax("10", NULL, 37) == 0 && errno == EINVAL, "strtoumax base 37");

    /* No conversion: endptr is nptr */
    check(strtoimax("  +", &end, 10) == 0 && strcmp(end, "  +") == 0, "strtoimax no digits");

    /* The wide versions */
    errno = 1234;
    check(wcstoimax(L" -77z", &wend, 10) == -77 && *wend == L'z' && errno == 1234, "wcstoimax");
    check(wcstoumax(L"0777", &wend, 0) == 0777 && *wend == L'\0', "wcstoumax octal");
    snprintf(big, sizeof(big), "%" PRIuMAX "0", UINTMAX_MAX);
    for (i = 0; big[i]; i++)
        wbig[i] = (wchar_t)big[i];
    wbig[i] = 0;
    errno = 0;
    check(wcstoimax(wbig, &wend, 10) == INTMAX_MAX && errno == ERANGE, "wcstoimax ERANGE");
    errno = 0;
    check(wcstoumax(wbig, &wend, 10) == UINTMAX_MAX && errno == ERANGE, "wcstoumax ERANGE");
    errno = 0;
    check(wcstoimax(L"1", NULL, 99) == 0 && errno == EINVAL, "wcstoimax base 99");

    printf("success\n");
    return 0;
}
