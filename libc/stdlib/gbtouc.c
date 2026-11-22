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

#include <inttypes.h>
#include <wchar.h>

#ifdef __MB_EXTENDED_CHARSETS_GB18030

#define define_gb18030_decode
#include "gb18030.h"

/*
 * Offset from U+4E00 of the n'th CJK ideograph which is not a GB2312
 * hanzi: the n'th zero bit in the bitmap
 */
static unsigned
gbk_hanzi(unsigned n)
{
    unsigned lo = 0, hi = GB18030_HANZI_WORDS;
    uint32_t zeros;
    unsigned bit;

    /* Find the last word with no more than n zero bits before it */
    while (hi - lo > 1) {
        unsigned mid = (lo + hi) / 2;
        if (mid * 32 - __gb18030_hanzi_rank[mid] <= n)
            lo = mid;
        else
            hi = mid;
    }
    n -= lo * 32 - __gb18030_hanzi_rank[lo];
    zeros = ~__gb18030_hanzi_bits[lo];
    for (bit = 0;; bit++) {
        if (zeros & ((uint32_t)1 << bit)) {
            if (n == 0)
                break;
            n--;
        }
    }
    return lo * 32 + bit;
}

/*
 * Number of exceptions before a slot in the GBK extension region;
 * sets *except when the slot is one of them
 */
static unsigned
gbk_except_before(unsigned slot, bool *except)
{
    unsigned lo = 0, hi = GB18030_GBK_EXCEPT;

    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (__gb18030_gbk_except[mid] < slot)
            lo = mid + 1;
        else
            hi = mid;
    }
    *except = lo < GB18030_GBK_EXCEPT && __gb18030_gbk_except[lo] == slot;
    return lo;
}

/* Find the two-byte run holding a two-byte index */
static wint_t
two_run(unsigned index)
{
    unsigned                  lo = 0, hi = GB18030_RUNS;
    const struct gb18030_run *run;

    while (hi - lo > 1) {
        unsigned mid = (lo + hi) / 2;
        if (__gb18030_runs[mid].index <= index)
            lo = mid;
        else
            hi = mid;
    }
    run = &__gb18030_runs[lo];
    if (index - run->index >= run->len)
        return WEOF;
    return run->ucs + (index - run->index);
}

/* Find the four-byte range holding a four-byte BMP index */
static wint_t
four_range(unsigned index)
{
    unsigned lo = 0, hi = GB18030_RANGES;

    while (hi - lo > 1) {
        unsigned mid = (lo + hi) / 2;
        if (__gb18030_ranges[mid].index <= index)
            lo = mid;
        else
            hi = mid;
    }
    return __gb18030_ranges[lo].ucs + (index - __gb18030_ranges[lo].index);
}

wint_t
__gb18030_to_unicode(uint32_t code)
{
    if (code < 0x10000) {
        uint8_t  lead = (uint8_t)(code >> 8);
        uint8_t  trail = (uint8_t)code;
        unsigned t, index;

        if (lead < 0x81 || lead > 0xfe || trail < 0x40 || trail > 0xfe || trail == 0x7f)
            return WEOF;
        t = (unsigned)(trail - 0x40) - (trail > 0x7f);
        index = (unsigned)(lead - 0x81) * 190 + t;
        if (0xb0 <= lead && lead <= 0xf7 && trail >= 0xa1) {
            /* GB2312 hanzi block */
            uint16_t ucs = __gb18030_gb2312_ucs[(lead - 0xb0) * 94 + (trail - 0xa1)];
            if (ucs)
                return ucs;
        } else {
            /* GBK extension hanzi region */
            unsigned slot = GB18030_GBK_SLOTS;

            if (lead <= 0xa0)
                slot = index;
            else if (lead >= 0xaa && trail <= 0xa0)
                slot = GB18030_GBK3_SLOTS + (unsigned)(lead - 0xaa) * 96 + t;
            if (slot < GB18030_GBK_SLOTS) {
                bool     except;
                unsigned before = gbk_except_before(slot, &except);
                if (!except)
                    return GB18030_HANZI_FIRST + gbk_hanzi(slot - before);
            }
        }
        return two_run(index);
    } else {
        uint8_t  b1 = (uint8_t)(code >> 24);
        uint8_t  b2 = (uint8_t)(code >> 16);
        uint8_t  b3 = (uint8_t)(code >> 8);
        uint8_t  b4 = (uint8_t)code;
        uint32_t index;

        if (b1 < 0x81 || b1 > 0xfe || b2 < 0x30 || b2 > 0x39 || b3 < 0x81 || b3 > 0xfe || b4 < 0x30
            || b4 > 0x39)
            return WEOF;
        index = (((uint32_t)(b1 - 0x81) * 10 + (uint32_t)(b2 - 0x30)) * 126 + (uint32_t)(b3 - 0x81))
                * 10
            + (uint32_t)(b4 - 0x30);
        if (index < GB18030_FOUR_BMP)
            return four_range(index);
        if (index >= GB18030_SUPP_FOUR_BASE && index - GB18030_SUPP_FOUR_BASE <= 0xfffff)
            return (wint_t)(0x10000 + (index - GB18030_SUPP_FOUR_BASE));
        return WEOF;
    }
}

#endif /* __MB_EXTENDED_CHARSETS_GB18030 */
