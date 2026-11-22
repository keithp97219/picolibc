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
/*
 * glibc 2.43 shows <endian.h> only with _DEFAULT_SOURCE and doesn't
 * define the uint types there
 */
#ifdef TEST_GLIBC
#define _DEFAULT_SOURCE
#include <stdint.h>
#endif
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <endian.h> (POSIX.1-2024) defines BYTE_ORDER, LITTLE_ENDIAN and
 * BIG_ENDIAN for #if, the uint16_t, uint32_t and uint64_t types, and
 * the twelve conversions as functions, macros or both.
 */

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN) || !defined(BIG_ENDIAN)
#error "<endian.h> must define BYTE_ORDER, LITTLE_ENDIAN and BIG_ENDIAN"
#endif

#if LITTLE_ENDIAN == BIG_ENDIAN
#error "LITTLE_ENDIAN and BIG_ENDIAN must differ"
#endif

#if BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN
#error "BYTE_ORDER must be LITTLE_ENDIAN or BIG_ENDIAN"
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Without <stdint.h>, these types come from <endian.h> */
static uint16_t            v16 = 0x0102;
static uint32_t            v32 = 0x01020304;
static uint64_t            v64 = 0x0102030405060708;

/* Wider than the parameters, to see them converted */
static uint32_t            wide = 0x10203;
static uint64_t            wide64 = 0x100010203;

static const unsigned char big[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
static const unsigned char little16[2] = { 2, 1 };
static const unsigned char little32[4] = { 4, 3, 2, 1 };
static const unsigned char little64[8] = { 8, 7, 6, 5, 4, 3, 2, 1 };

/* The bytes of v in memory, in order */
#define bytes_are(v, expect) (memcmp(&(v), (expect), sizeof(v)) == 0)

int
main(void)
{
    uint16_t r16;
    uint32_t r32;
    uint64_t r64;

    /* The host's order is the one BYTE_ORDER names */
#if BYTE_ORDER == LITTLE_ENDIAN
    check(bytes_are(v32, little32), "BYTE_ORDER is LITTLE_ENDIAN");
#else
    check(bytes_are(v32, big), "BYTE_ORDER is BIG_ENDIAN");
#endif

    /* Each conversion returns its own size */
    check(sizeof(htobe16(v16)) == 2 && sizeof(htole16(v16)) == 2, "16-bit results");
    check(sizeof(be16toh(v16)) == 2 && sizeof(le16toh(v16)) == 2, "16-bit results");
    check(sizeof(htobe32(v32)) == 4 && sizeof(htole32(v32)) == 4, "32-bit results");
    check(sizeof(be32toh(v32)) == 4 && sizeof(le32toh(v32)) == 4, "32-bit results");
    check(sizeof(htobe64(v64)) == 8 && sizeof(htole64(v64)) == 8, "64-bit results");
    check(sizeof(be64toh(v64)) == 8 && sizeof(le64toh(v64)) == 8, "64-bit results");

    /* To a given order, whatever the host's */
    r16 = htobe16(v16);
    check(bytes_are(r16, big), "htobe16");
    r16 = htole16(v16);
    check(bytes_are(r16, little16), "htole16");
    r32 = htobe32(v32);
    check(bytes_are(r32, big), "htobe32");
    r32 = htole32(v32);
    check(bytes_are(r32, little32), "htole32");
    r64 = htobe64(v64);
    check(bytes_are(r64, big), "htobe64");
    r64 = htole64(v64);
    check(bytes_are(r64, little64), "htole64");

    /* And back from it */
    memcpy(&r16, big, sizeof(r16));
    check(be16toh(r16) == v16, "be16toh");
    memcpy(&r16, little16, sizeof(r16));
    check(le16toh(r16) == v16, "le16toh");
    memcpy(&r32, big, sizeof(r32));
    check(be32toh(r32) == v32, "be32toh");
    memcpy(&r32, little32, sizeof(r32));
    check(le32toh(r32) == v32, "le32toh");
    memcpy(&r64, big, sizeof(r64));
    check(be64toh(r64) == v64, "be64toh");
    memcpy(&r64, little64, sizeof(r64));
    check(le64toh(r64) == v64, "le64toh");

    /* Arguments convert to the parameter type, as a prototype would */
    check(htobe16(wide) == htobe16(0x0203), "htobe16 argument conversion");
    check(le32toh(wide64) == le32toh(0x10203), "le32toh argument conversion");

    printf("success\n");
    return 0;
}
