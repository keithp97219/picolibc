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
#include <arpa/inet.h>
#include <ctype.h>
#include <stdint.h>

/*
 * The traditional forms: a.b.c.d, a.b.c (c is 16 bits), a.b (b is 24
 * bits) or a (32 bits), each part decimal, octal (leading 0) or hex
 * (leading 0x).
 */
in_addr_t
inet_addr(const char *cp)
{
    uint32_t parts[4];
    int      n = 0;

    for (;;) {
        uint32_t value = 0;
        int      base = 10, digits = 0;

        if (*cp == '0') {
            base = 8;
            cp++;
            digits = 1;
            if (*cp == 'x' || *cp == 'X') {
                base = 16;
                cp++;
                digits = 0;
            }
        }
        for (;;) {
            int d;

            if (isdigit((unsigned char)*cp))
                d = *cp - '0';
            else if (base == 16 && isxdigit((unsigned char)*cp))
                d = (tolower((unsigned char)*cp) - 'a') + 10;
            else
                break;
            if (d >= base)
                return (in_addr_t)-1;
            if (value > (UINT32_MAX - (uint32_t)d) / (uint32_t)base)
                return (in_addr_t)-1;
            value = value * (uint32_t)base + (uint32_t)d;
            digits++;
            cp++;
        }
        if (!digits)
            return (in_addr_t)-1;
        parts[n++] = value;
        if (*cp == '.' && n < 4) {
            cp++;
            continue;
        }
        break;
    }
    if (*cp && !isspace((unsigned char)*cp))
        return (in_addr_t)-1;

    switch (n) {
    case 1:
        break;
    case 2:
        if (parts[0] > 0xff || parts[1] > 0xffffff)
            return (in_addr_t)-1;
        parts[0] = parts[0] << 24 | parts[1];
        break;
    case 3:
        if (parts[0] > 0xff || parts[1] > 0xff || parts[2] > 0xffff)
            return (in_addr_t)-1;
        parts[0] = parts[0] << 24 | parts[1] << 16 | parts[2];
        break;
    default:
        if (parts[0] > 0xff || parts[1] > 0xff || parts[2] > 0xff || parts[3] > 0xff)
            return (in_addr_t)-1;
        parts[0] = parts[0] << 24 | parts[1] << 16 | parts[2] << 8 | parts[3];
        break;
    }
    return htonl(parts[0]);
}
