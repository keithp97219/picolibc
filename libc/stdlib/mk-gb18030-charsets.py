#!/usr/bin/env python3
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2026 Keith Packard
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#

'''
Generate the GB18030-2022 translation tables.

    python3 mk-gb18030-charsets.py > gb18030_charsets.h
    python3 mk-gb18030-charsets.py --test > ../../test/test-ctype/test-gb18030.h
    python3 mk-gb18030-charsets.py --stats

The mapping comes from Python's 'gb18030' codec, which implements
GB18030-2000, with the 2005 and 2022 changes listed in CHANGES below
applied on top.

GB18030 encodes Unicode as

  * one byte, 0x00-0x7f: ASCII

  * two bytes, lead 0x81-0xfe, trail 0x40-0x7e or 0x80-0xfe: 23940
    codes, numbered here by their "two-byte index"
    (lead - 0x81) * 190 + (trail - 0x40) - (trail > 0x7f)

  * four bytes, b1 0x81-0xfe, b2 0x30-0x39, b3 0x81-0xfe, b4 0x30-0x39,
    numbered by the "four-byte index"
    (((b1 - 0x81) * 10 + (b2 - 0x30)) * 126 + (b3 - 0x81)) * 10 + (b4 - 0x30).
    Indices 0-39419 (0x81308130-0x8431a439) hold the BMP characters
    not in the two-byte set, in 206 ranges. 0x90308130 (index 189000)
    starts the supplementary planes, U+10000 + (index - 189000), up to
    0xe3329a35 for U+10FFFF.

The tables:

  1. A bitmap over the 20902 CJK ideographs U+4E00-U+9FA5 marking the
     6763 GB2312 hanzi (lead 0xb0-0xf7, trail 0xa1-0xfe), with the
     number of set bits before each 32-bit word for constant-time rank.

  2. The other 14139 ideographs in that block are the GBK extension
     hanzi (GBK/3: lead 0x81-0xa0, any trail; GBK/4: lead 0xaa-0xfe,
     trail 0x40-0xa0) in Unicode order. They need no table: the n'th
     hanzi slot in that region is the n'th zero bit in the bitmap. The
     non-hanzi codes mixed into that region are listed by slot number
     in a sorted exception list and translated by the run table.

  3. GB2312 hanzi: a decode table indexed by the slot in the GB2312
     block, and an encode table indexed by the rank in the bitmap.

  4. All other two-byte codes: runs of consecutive codes mapping to
     consecutive Unicode values, sorted by two-byte index, with a
     second index of the same runs sorted by Unicode value.

  5. Four-byte BMP codes: ranges of consecutive codes mapping to
     consecutive Unicode values, sorted by four-byte index, with an
     index of the same ranges sorted by Unicode value.

  6. Supplementary planes: arithmetic.
'''

import sys

# Python's codec implements GB18030-2000. These are the later changes,
# as (two-byte code, GB18030-2000 value, GB18030-2022 value, four-byte
# code). In each case the two-byte code moves from its old Private Use
# value to a standard Unicode character, and the four-byte code that
# held that standard character now holds the old Private Use value, so
# both values keep a round-trip mapping.
#
# The list was derived by comparing Python 3.14's gb18030 codec with
# glibc 2.43's iconv (which implements GB18030-2022) over every one-,
# two- and four-byte code and every Unicode value in both directions:
# these 37 two-byte and four-byte codes, and the 37 matching Unicode
# values, are the only differences.
#
# It matches the published changes:
#
#  * GB18030-2005 moved one character, 0xA8BC, from U+E7C7 to U+1E3F
#    (LATIN SMALL LETTER M WITH ACUTE), which had been the four-byte
#    code 0x8135F437.
#
#  * GB18030-2022 moved 18 two-byte codes from Private Use to the
#    characters added to Unicode after 2000: ten vertical forms
#    (U+FE10-U+FE19) and eight CJK ideographs (U+9FB4-U+9FBB). Note
#    that 0xA6DA/0xA6DB map to U+FE12/U+FE11, not in code order, and
#    so their four-byte partners 0x84318237/0x84318238 hold
#    U+E78F/U+E78E.
#
#    The other six GB18030-2005 Private Use codes with suggested
#    standard characters (0xFE51, 0xFE52, 0xFE53, 0xFE6C, 0xFE76,
#    0xFE91) keep their Private Use values in 2022, as in Python and
#    glibc.
CHANGES = [
    # GB18030-2005
    (0xa8bc, 0xe7c7, 0x1e3f, 0x8135f437),
    # GB18030-2022, vertical forms
    (0xa6d9, 0xe78d, 0xfe10, 0x84318236),
    (0xa6da, 0xe78e, 0xfe12, 0x84318238),
    (0xa6db, 0xe78f, 0xfe11, 0x84318237),
    (0xa6dc, 0xe790, 0xfe13, 0x84318239),
    (0xa6dd, 0xe791, 0xfe14, 0x84318330),
    (0xa6de, 0xe792, 0xfe15, 0x84318331),
    (0xa6df, 0xe793, 0xfe16, 0x84318332),
    (0xa6ec, 0xe794, 0xfe17, 0x84318333),
    (0xa6ed, 0xe795, 0xfe18, 0x84318334),
    (0xa6f3, 0xe796, 0xfe19, 0x84318335),
    # GB18030-2022, CJK ideographs
    (0xfe59, 0xe81e, 0x9fb4, 0x82359037),
    (0xfe61, 0xe826, 0x9fb5, 0x82359038),
    (0xfe66, 0xe82b, 0x9fb6, 0x82359039),
    (0xfe67, 0xe82c, 0x9fb7, 0x82359130),
    (0xfe6d, 0xe832, 0x9fb8, 0x82359131),
    (0xfe7e, 0xe843, 0x9fb9, 0x82359132),
    (0xfe90, 0xe854, 0x9fba, 0x82359133),
    (0xfea0, 0xe864, 0x9fbb, 0x82359134),
]

NUM_TWO = 126 * 190
NUM_FOUR_BMP = 39420
SUPP_FOUR_BASE = 189000         # four-byte index of 0x90308130
HANZI_FIRST = 0x4e00
HANZI_LAST = 0x9fa5
HANZI_NUM = HANZI_LAST - HANZI_FIRST + 1
HANZI_WORDS = (HANZI_NUM + 31) // 32
GB2312_ROWS = 0xf7 - 0xb0 + 1
GB2312_SLOTS = GB2312_ROWS * 94
GBK3_SLOTS = (0xa0 - 0x81 + 1) * 190
GBK4_SLOTS = (0xfe - 0xaa + 1) * 96
GBK_SLOTS = GBK3_SLOTS + GBK4_SLOTS


def fail(msg):
    print(msg, file=sys.stderr)
    sys.exit(1)


def two_index(code):
    lead, trail = code >> 8, code & 0xff
    return (lead - 0x81) * 190 + (trail - 0x40) - (trail > 0x7f)


def two_code(index):
    lead, t = divmod(index, 190)
    return ((lead + 0x81) << 8) | (t + 0x40 + (t >= 0x3f))


def four_index(code):
    b1, b2, b3, b4 = code >> 24, (code >> 16) & 0xff, (code >> 8) & 0xff, code & 0xff
    return (((b1 - 0x81) * 10 + (b2 - 0x30)) * 126 + (b3 - 0x81)) * 10 + (b4 - 0x30)


def four_code(index):
    index, b4 = divmod(index, 10)
    index, b3 = divmod(index, 126)
    b1, b2 = divmod(index, 10)
    return ((b1 + 0x81) << 24) | ((b2 + 0x30) << 16) | ((b3 + 0x81) << 8) | (b4 + 0x30)


def code_bytes(code, n):
    return bytes((code >> (8 * (n - 1 - i))) & 0xff for i in range(n))


def py_decode(code, n):
    s = code_bytes(code, n).decode('gb18030')
    if len(s) != 1:
        fail(f'{code:#x} decodes to {len(s)} characters')
    return ord(s)


def gb2312_slot(code):
    '''Slot of a two-byte code in the GB2312 hanzi block, or None'''
    lead, trail = code >> 8, code & 0xff
    if 0xb0 <= lead <= 0xf7 and trail >= 0xa1:
        return (lead - 0xb0) * 94 + (trail - 0xa1)
    return None


def gbk_slot(code):
    '''Slot of a two-byte code in the GBK extension hanzi region, or None'''
    lead, trail = code >> 8, code & 0xff
    t = (trail - 0x40) - (trail > 0x7f)
    if 0x81 <= lead <= 0xa0:
        return (lead - 0x81) * 190 + t
    if 0xaa <= lead and trail <= 0xa0:
        return GBK3_SLOTS + (lead - 0xaa) * 96 + t
    return None


def gbk_code(slot):
    if slot < GBK3_SLOTS:
        lead, t = divmod(slot, 190)
        lead += 0x81
    else:
        lead, t = divmod(slot - GBK3_SLOTS, 96)
        lead += 0xaa
    return (lead << 8) | (t + 0x40 + (t >= 0x3f))


# Load the GB18030-2000 mapping

if py_decode(0xa8bc, 2) != 0xe7c7 or py_decode(0xa6d9, 2) != 0xe78d:
    fail('Python gb18030 codec is not GB18030-2000')

two = [py_decode(two_code(i), 2) for i in range(NUM_TWO)]
four = [py_decode(four_code(i), 4) for i in range(NUM_FOUR_BMP)]

# Apply the 2005 and 2022 changes

for code, old, new, fcode in CHANGES:
    i = two_index(code)
    f = four_index(fcode)
    if two[i] != old or four[f] != new:
        fail(f'change {code:#06x} {old:#06x} {new:#06x} {fcode:#010x} does not match codec')
    two[i] = new
    four[f] = old

# Check the result is a bijection on the BMP

decode = {}
for i, u in enumerate(two):
    decode[u] = two_code(i)
for i, u in enumerate(four):
    if u in decode:
        fail(f'U+{u:04X} mapped twice')
    decode[u] = four_code(i)
for u in range(0x80, 0x10000):
    if (0xd800 <= u <= 0xdfff) == (u in decode):
        fail(f'U+{u:04X} mapping wrong')

# 1. GB2312 hanzi bitmap

gb2312_ucs = [0] * GB2312_SLOTS
hanzi_bits = [0] * HANZI_WORDS
for i, u in enumerate(two):
    code = two_code(i)
    slot = gb2312_slot(code)
    if slot is not None and HANZI_FIRST <= u <= HANZI_LAST:
        gb2312_ucs[slot] = u
        off = u - HANZI_FIRST
        hanzi_bits[off // 32] |= 1 << (off % 32)

hanzi_rank = []
count = 0
for w in hanzi_bits:
    hanzi_rank.append(count)
    count += bin(w).count('1')
num_gb2312 = count

# 3. GB2312 hanzi encode table, indexed by rank

gb2312_code = [0] * num_gb2312
for slot, u in enumerate(gb2312_ucs):
    if u:
        off = u - HANZI_FIRST
        r = hanzi_rank[off // 32] + bin(hanzi_bits[off // 32] & ((1 << (off % 32)) - 1)).count('1')
        gb2312_code[r] = 0xb0a1 + ((slot // 94) << 8) + slot % 94

# 2. GBK extension hanzi: everything else in U+4E00-U+9FA5 must be in
# the GBK region in Unicode order. Non-hanzi in that region are
# exceptions.

zeros = [HANZI_FIRST + off for off in range(HANZI_NUM)
         if not hanzi_bits[off // 32] & (1 << (off % 32))]
gbk_except = []
gbk_hanzi = []
for slot in range(GBK_SLOTS):
    u = two[two_index(gbk_code(slot))]
    if HANZI_FIRST <= u <= HANZI_LAST:
        gbk_hanzi.append(u)
    else:
        gbk_except.append(slot)
if gbk_hanzi != zeros:
    fail('GBK extension hanzi not in Unicode order')

# 4. Runs of other two-byte codes

def is_table_hanzi(i):
    code = two_code(i)
    if gb2312_slot(code) is not None and gb2312_ucs[gb2312_slot(code)]:
        return True
    slot = gbk_slot(code)
    return slot is not None and HANZI_FIRST <= two[i] <= HANZI_LAST

runs = []
for i, u in enumerate(two):
    if is_table_hanzi(i):
        continue
    if runs and runs[-1][0] + runs[-1][2] == i and runs[-1][1] + runs[-1][2] == u:
        runs[-1][2] += 1
    else:
        runs.append([i, u, 1])
for r in runs:
    if r[2] > 0xffff or r[1] + r[2] > 0x10000:
        fail('run too long')
run_by_ucs = sorted(range(len(runs)), key=lambda r: runs[r][1])
for a, b in zip(run_by_ucs, run_by_ucs[1:]):
    if runs[a][1] + runs[a][2] > runs[b][1]:
        fail('overlapping runs')
num_other = sum(r[2] for r in runs)

# 5. Four-byte BMP ranges

ranges = []
for i, u in enumerate(four):
    if ranges and ranges[-1][0] + ranges[-1][2] == i and ranges[-1][1] + ranges[-1][2] == u:
        ranges[-1][2] += 1
    else:
        ranges.append([i, u, 1])
range_by_ucs = sorted(range(len(ranges)), key=lambda r: ranges[r][1])

sizes = [
    ('GB2312 hanzi bitmap', HANZI_WORDS * 4),
    ('GB2312 hanzi rank counts', HANZI_WORDS * 2),
    ('GB2312 hanzi decode', GB2312_SLOTS * 2),
    ('GB2312 hanzi encode', num_gb2312 * 2),
    ('GBK extension exceptions', len(gbk_except) * 2),
    ('two-byte runs', len(runs) * 6),
    ('two-byte runs by Unicode', len(runs) * 2),
    ('four-byte ranges', (len(ranges) + 1) * 4),
    ('four-byte ranges by Unicode', len(ranges) * 2),
]


def stats(out):
    print(f'two-byte codes:          {NUM_TWO}', file=out)
    print(f'  GB2312 hanzi:          {num_gb2312}', file=out)
    print(f'  GBK extension hanzi:   {len(gbk_hanzi)}', file=out)
    print(f'  GBK region exceptions: {len(gbk_except)}', file=out)
    print(f'  other:                 {num_other} in {len(runs)} runs', file=out)
    print(f'four-byte BMP codes:     {NUM_FOUR_BMP} in {len(ranges)} ranges', file=out)
    for name, size in sizes:
        print(f'{name:28s} {size:6d} bytes', file=out)
    print(f'{"total":28s} {sum(s for n, s in sizes):6d} bytes', file=out)


def dump(name, ctype, values, fmt, per_line, static=False):
    print(f'{"static " if static else ""}const {ctype} {name}[{len(values)}] = {{')
    for i in range(0, len(values), per_line):
        print('    ' + ' '.join(f'{v:{fmt}},' for v in values[i:i + per_line]))
    print('};')
    print('')


LICENSE = '''\
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
 */'''


def tables():
    print(LICENSE)
    print('''\
/*
 * GB18030-2022 tables, generated by mk-gb18030-charsets.py from
 * Python's gb18030 codec plus the 2005 and 2022 changes listed in
 * that script. Regenerate with
 *
 *     python3 mk-gb18030-charsets.py > gb18030_charsets.h
 *
 * Table sizes:
 *''')
    for name, size in sizes:
        print(f' *   {name:28s} {size:6d} bytes')
    print(f' *   {"total":28s} {sum(s for n, s in sizes):6d} bytes')
    print(''' */
/* clang-format off */

#ifndef _GB18030_CHARSETS_H_
#define _GB18030_CHARSETS_H_
''')
    print(f'#define GB18030_HANZI_FIRST     {HANZI_FIRST:#06x}')
    print(f'#define GB18030_HANZI_LAST      {HANZI_LAST:#06x}')
    print(f'#define GB18030_HANZI_WORDS     {HANZI_WORDS}')
    print(f'#define GB18030_GB2312_SLOTS    {GB2312_SLOTS}')
    print(f'#define GB18030_GB2312_HANZI    {num_gb2312}')
    print(f'#define GB18030_GBK3_SLOTS      {GBK3_SLOTS}')
    print(f'#define GB18030_GBK_SLOTS       {GBK_SLOTS}')
    print(f'#define GB18030_GBK_EXCEPT      {len(gbk_except)}')
    print(f'#define GB18030_RUNS            {len(runs)}')
    print(f'#define GB18030_FOUR_BMP        {NUM_FOUR_BMP}')
    print(f'#define GB18030_RANGES          {len(ranges)}')
    print(f'#define GB18030_SUPP_FOUR_BASE  {SUPP_FOUR_BASE}')
    print('''
struct gb18030_run {
    uint16_t index;     /* two-byte index of the first code */
    uint16_t ucs;       /* Unicode value of the first code */
    uint16_t len;       /* number of codes */
};

struct gb18030_range {
    uint16_t index;     /* four-byte index of the first code */
    uint16_t ucs;       /* Unicode value of the first code */
};

extern const uint32_t             __gb18030_hanzi_bits[GB18030_HANZI_WORDS];
extern const uint16_t             __gb18030_hanzi_rank[GB18030_HANZI_WORDS];
extern const uint16_t             __gb18030_gbk_except[GB18030_GBK_EXCEPT];
extern const struct gb18030_run   __gb18030_runs[GB18030_RUNS];
extern const struct gb18030_range __gb18030_ranges[GB18030_RANGES + 1];

#ifdef define_gb18030_common
''')
    print('/* Bit n set when U+4E00 + n is a GB2312 hanzi */')
    dump('__gb18030_hanzi_bits', 'uint32_t', hanzi_bits, '#010x', 6)
    print('/* Number of bits set in __gb18030_hanzi_bits before each word */')
    dump('__gb18030_hanzi_rank', 'uint16_t', hanzi_rank, '5d', 10)
    print('/* Slots in the GBK extension region holding something other than a hanzi */')
    dump('__gb18030_gbk_except', 'uint16_t', gbk_except, '5d', 10)
    print('/* Runs of other two-byte codes, sorted by two-byte index */')
    print(f'const struct gb18030_run __gb18030_runs[GB18030_RUNS] = {{')
    for i, u, n in runs:
        print(f'    {{ {i:5d}, {u:#06x}, {n:3d} }}, /* {two_code(i):#06x} */')
    print('};')
    print('')
    print('/* Ranges of four-byte BMP codes, sorted by four-byte index, plus an end marker */')
    print(f'const struct gb18030_range __gb18030_ranges[GB18030_RANGES + 1] = {{')
    for i, u, n in ranges:
        print(f'    {{ {i:5d}, {u:#06x} }}, /* {four_code(i):#010x} */')
    print(f'    {{ {NUM_FOUR_BMP:5d}, 0x0000 }},')
    print('};')
    print('')
    print('#endif /* define_gb18030_common */')
    print('')
    print('#ifdef define_gb18030_decode')
    print('')
    print('/* Unicode value of each slot in the GB2312 hanzi block, 0 for non-hanzi */')
    dump('__gb18030_gb2312_ucs', 'uint16_t', gb2312_ucs, '#06x', 8, static=True)
    print('#endif /* define_gb18030_decode */')
    print('')
    print('#ifdef define_gb18030_encode')
    print('')
    print('/* Two-byte code of each GB2312 hanzi, by its rank in __gb18030_hanzi_bits */')
    dump('__gb18030_gb2312_code', 'uint16_t', gb2312_code, '#06x', 8, static=True)
    print('/* __gb18030_runs sorted by Unicode value */')
    dump('__gb18030_run_by_ucs', 'uint16_t', run_by_ucs, '3d', 12, static=True)
    print('/* __gb18030_ranges sorted by Unicode value */')
    dump('__gb18030_range_by_ucs', 'uint16_t', range_by_ucs, '3d', 12, static=True)
    print('#endif /* define_gb18030_encode */')
    print('')
    print('#endif /* _GB18030_CHARSETS_H_ */')


# Reference data for test/test-ctype/test-gb18030.c: FNV-1a hashes of
# the expected results, computed directly from the mapping rather than
# from the tables above.

FNV_BASIS = 0x811c9dc5
FNV_PRIME = 0x01000193


def fnv(h, v):
    for i in range(4):
        h ^= (v >> (8 * i)) & 0xff
        h = (h * FNV_PRIME) & 0xffffffff
    return h


def test_reference():
    # Decoding: one hash per lead byte of the two-byte codes, over
    # every trail byte 0x40-0xfe (0x7f decodes to -1)
    two_hash = []
    for lead in range(0x81, 0xff):
        h = FNV_BASIS
        for trail in range(0x40, 0xff):
            if trail == 0x7f:
                v = 0xffffffff
            else:
                v = two[two_index((lead << 8) | trail)]
            h = fnv(h, v)
        two_hash.append(h)
    # Decoding: one hash per first two bytes of the four-byte BMP
    # codes, 0x8130-0x8439, over every b3 0x81-0xfe, b4 0x30-0x39
    # (codes past 0x8431a439 decode to -1)
    four_hash = []
    for b1 in range(0x81, 0x85):
        for b2 in range(0x30, 0x3a):
            h = FNV_BASIS
            for b3 in range(0x81, 0xff):
                for b4 in range(0x30, 0x3a):
                    i = four_index((b1 << 24) | (b2 << 16) | (b3 << 8) | b4)
                    h = fnv(h, four[i] if i < NUM_FOUR_BMP else 0xffffffff)
            four_hash.append(h)
    # Encoding: one hash per 256 BMP values, over the code (or -1 for
    # surrogates)
    ucs_hash = []
    for hi in range(0x100):
        h = FNV_BASIS
        for lo in range(0x100):
            u = hi * 256 + lo
            if u < 0x80:
                v = u
            elif 0xd800 <= u <= 0xdfff:
                v = 0xffffffff
            else:
                v = decode[u]
            h = fnv(h, v)
        ucs_hash.append(h)

    print(LICENSE)
    print('''\
/*
 * Reference data for test-gb18030.c, generated by
 *
 *     python3 libc/stdlib/mk-gb18030-charsets.py --test > test/test-ctype/test-gb18030.h
 */
/* clang-format off */
''')
    print('/* GB18030-2005 and GB18030-2022 changes: two-byte code, Unicode, four-byte code, Unicode */')
    print('static const struct gb18030_change {')
    print('    uint16_t two;')
    print('    uint16_t two_ucs;')
    print('    uint32_t four;')
    print('    uint16_t four_ucs;')
    print('} gb18030_changes[] = {')
    for code, old, new, fcode in CHANGES:
        print(f'    {{ {code:#06x}, {new:#06x}, {fcode:#010x}, {old:#06x} }},')
    print('};')
    print('')
    print('/* FNV-1a hashes of the decoded two-byte codes, per lead byte 0x81-0xfe */')
    dump('gb18030_two_hash', 'uint32_t', two_hash, '#010x', 6, static=True)
    print('/* FNV-1a hashes of the decoded four-byte codes, per first two bytes 0x8130-0x8439 */')
    dump('gb18030_four_hash', 'uint32_t', four_hash, '#010x', 6, static=True)
    print('/* FNV-1a hashes of the encoded BMP values, per 256 values */')
    dump('gb18030_ucs_hash', 'uint32_t', ucs_hash, '#010x', 6, static=True)


if len(sys.argv) > 1 and sys.argv[1] == '--stats':
    stats(sys.stdout)
elif len(sys.argv) > 1 and sys.argv[1] == '--test':
    test_reference()
else:
    tables()
