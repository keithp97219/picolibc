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

#define define_gb18030_encode
#include "gb18030.h"

static uint32_t
two_code(unsigned index)
{
    unsigned lead = index / 190;
    unsigned t = index % 190;

    return ((lead + 0x81) << 8) | (t + 0x40 + (t >= 0x3f));
}

static uint32_t
four_code(uint32_t index)
{
    uint32_t b4 = index % 10;
    uint32_t b3;
    uint32_t b2;

    index /= 10;
    b3 = index % 126;
    index /= 126;
    b2 = index % 10;
    index /= 10;
    return ((index + 0x81) << 24) | ((b2 + 0x30) << 16) | ((b3 + 0x81) << 8) | (b4 + 0x30);
}

uint32_t
__unicode_to_gb18030(uint32_t ucs)
{
    unsigned lo, hi;

    if (ucs < 0x80)
        return ucs;

    if (GB18030_HANZI_FIRST <= ucs && ucs <= GB18030_HANZI_LAST) {
        unsigned off = ucs - GB18030_HANZI_FIRST;
        unsigned ones = __gb18030_hanzi_ones(off);
        unsigned slot, e;

        /* GB2312 hanzi */
        if (__gb18030_hanzi_bits[off >> 5] & ((uint32_t)1 << (off & 31)))
            return __gb18030_gb2312_code[ones];

        /* GBK extension hanzi, skipping the non-hanzi slots */
        slot = off - ones;
        for (e = 0; e < GB18030_GBK_EXCEPT && __gb18030_gbk_except[e] <= slot; e++)
            slot++;
        if (slot < GB18030_GBK3_SLOTS)
            return two_code(slot);
        slot -= GB18030_GBK3_SLOTS;
        lo = slot % 96;
        return ((0xaa + slot / 96) << 8) | (lo + 0x40 + (lo >= 0x3f));
    }

    if (ucs > 0xffff) {
        if (ucs > 0x10ffff)
            return GB18030_INVALID;
        return four_code(GB18030_SUPP_FOUR_BASE + (ucs - 0x10000));
    }

    if (0xd800 <= ucs && ucs <= 0xdfff)
        return GB18030_INVALID;

    /* Other two-byte codes */
    lo = 0;
    hi = GB18030_RUNS;
    while (hi - lo > 1) {
        unsigned mid = (lo + hi) / 2;
        if (__gb18030_runs[__gb18030_run_by_ucs[mid]].ucs <= ucs)
            lo = mid;
        else
            hi = mid;
    }
    {
        const struct gb18030_run *run = &__gb18030_runs[__gb18030_run_by_ucs[lo]];
        if (run->ucs <= ucs && ucs - run->ucs < run->len)
            return two_code(run->index + (ucs - run->ucs));
    }

    /* Four-byte BMP codes */
    lo = 0;
    hi = GB18030_RANGES;
    while (hi - lo > 1) {
        unsigned mid = (lo + hi) / 2;
        if (__gb18030_ranges[__gb18030_range_by_ucs[mid]].ucs <= ucs)
            lo = mid;
        else
            hi = mid;
    }
    {
        const struct gb18030_range *range = &__gb18030_ranges[__gb18030_range_by_ucs[lo]];
        if (range->ucs <= ucs && ucs - range->ucs < (unsigned)(range[1].index - range->index))
            return four_code(range->index + (ucs - range->ucs));
    }

    return GB18030_INVALID;
}

#endif /* __MB_EXTENDED_CHARSETS_GB18030 */
