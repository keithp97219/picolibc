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

#ifndef _GB18030_H_
#define _GB18030_H_

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>
#include "gb18030_charsets.h"

/*
 * GB18030 codes are passed around packed into a uint32_t: values
 * below 0x80 are single bytes, values below 0x10000 are two-byte
 * codes and larger values are four-byte codes, first byte in the most
 * significant position.
 */

#define GB18030_INVALID 0xffffffffUL

/* Unicode value of a two-byte or four-byte GB18030 code, or WEOF */
wint_t   __gb18030_to_unicode(uint32_t code);

/* GB18030 code of a Unicode value, or GB18030_INVALID */
uint32_t __unicode_to_gb18030(uint32_t ucs);

static inline unsigned
__gb18030_popcount(uint32_t v)
{
    v = v - ((v >> 1) & 0x55555555UL);
    v = (v & 0x33333333UL) + ((v >> 2) & 0x33333333UL);
    v = (v + (v >> 4)) & 0x0f0f0f0fUL;
    return (unsigned)((uint32_t)(v * 0x01010101UL) >> 24);
}

/* Number of GB2312 hanzi before U+4E00 + off */
static inline unsigned
__gb18030_hanzi_ones(unsigned off)
{
    unsigned w = off >> 5;
    uint32_t mask = ((uint32_t)1 << (off & 31)) - 1;

    return __gb18030_hanzi_rank[w] + __gb18030_popcount(__gb18030_hanzi_bits[w] & mask);
}

#endif /* _GB18030_H_ */
