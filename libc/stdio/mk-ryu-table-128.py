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
"""Generate the tables in ryu_table_128.c.

The 128-bit Ryu code needs, for 0 <= i <= MAX_POW5 and 0 <= q <=
MAX_INV_POW5, the 249-bit values

    pow5(i)    = floor(5^i / 2^(pow5bits(i) - 249))
    invpow5(q) = floor(2^(pow5bits(q) - 1 + 249) / 5^q) + 1

These are computed from a table of every POW5_TABLE_SIZE'th value
and a power of five that fits in 256 bits, as in Ryu's generic_128.h
(which uses a step of 56 and a 128-bit power of five). The product
can be low by up to 3; the error tables hold that correction for
each value in two bits.

The limits cover the 15-bit binary exponent of the Intel/Motorola
80-bit and IEEE binary128 formats.

This script simulates the C code and checks every value. Its output
replaces the tables in ryu_table_128.c (then run clang-format).
"""

import sys

POW5_TABLE_SIZE = 111
BITCOUNT = 249


def pow5bits(e):
    return ((e * 163391164108059) >> 46) + 1


def log10pow2(e):
    return (e * 169464822037455) >> 49


def log10pow5(e):
    return (e * 196742565691928) >> 48


# Range of e2 in ldtoa_ryu.c: the exponent of the lsb, less 2, for
# Motorola (denorm exponent 0) and Intel (1) 80-bit and binary128
E2_MIN = min(0 - 16383 - 63 - 2, 1 - 16383 - 63 - 2, 1 - 16383 - 112 - 2)
E2_MAX = max(32766 - 16383 - 63 - 2, 32766 - 16383 - 112 - 2)

# largest i = -e2 - q for e2 < 0, largest q for e2 >= 0
MAX_POW5 = max(-e2 - (log10pow5(-e2) - (-e2 > 1)) for e2 in range(E2_MIN, 0))
MAX_INV_POW5 = max(log10pow2(e2) - (e2 > 3) for e2 in range(0, E2_MAX + 1))


def pow5(i):
    k = pow5bits(i) - BITCOUNT
    return (5**i) >> k if k >= 0 else (5**i) << -k


def invpow5(q):
    return (1 << (pow5bits(q) - 1 + BITCOUNT)) // 5**q + 1


M256 = (1 << 256) - 1


def words(v, n=4):
    return [(v >> (64 * j)) & ((1 << 64) - 1) for j in range(n)]


def gen():
    assert 5 ** (POW5_TABLE_SIZE - 1) < 1 << 256
    nsplit = MAX_POW5 // POW5_TABLE_SIZE + 1
    ninv = (MAX_INV_POW5 + POW5_TABLE_SIZE - 1) // POW5_TABLE_SIZE + 1
    split = [pow5(b * POW5_TABLE_SIZE) for b in range(nsplit)]
    inv_split = [invpow5(b * POW5_TABLE_SIZE) - 1 for b in range(ninv)]

    errors = [0] * ((MAX_POW5 + 32) // 32)
    for i in range(MAX_POW5 + 1):
        base = i // POW5_TABLE_SIZE
        base2 = base * POW5_TABLE_SIZE
        if i == base2:
            continue
        r = ((5 ** (i - base2) * split[base]) >> (pow5bits(i) - pow5bits(base2))) & M256
        corr = pow5(i) - r
        assert 0 <= corr <= 3
        errors[i // 32] |= corr << (2 * (i % 32))

    inv_errors = [0] * ((MAX_INV_POW5 + 32) // 32)
    for q in range(MAX_INV_POW5 + 1):
        base = (q + POW5_TABLE_SIZE - 1) // POW5_TABLE_SIZE
        base2 = base * POW5_TABLE_SIZE
        if q == base2:
            assert inv_split[base] + 1 == invpow5(q)
            continue
        r = ((5 ** (base2 - q) * inv_split[base]) >> (pow5bits(base2) - pow5bits(q))) & M256
        corr = invpow5(q) - r - 1
        assert 0 <= corr <= 3
        inv_errors[q // 32] |= corr << (2 * (q % 32))
    return split, inv_split, errors, inv_errors


def emit_split(name, table):
    print("static const uint64_t %s[%d][4] = {" % (name, len(table)))
    for v in table:
        print("    { %s }," % ", ".join("%du" % w for w in words(v)))
    print("};")
    print()


def emit_errors(name, table):
    print("static const uint64_t %s[%d] = {" % (name, len(table)))
    for i in range(0, len(table), 4):
        print("    %s," % ", ".join("0x%016xu" % w for w in table[i : i + 4]))
    print("};")
    print()


split, inv_split, errors, inv_errors = gen()
print("/* Generated by mk-ryu-table-128.py */")
print()
print("#define POW5_TABLE_SIZE %d" % POW5_TABLE_SIZE)
print("#define MAX_POW5        %d" % MAX_POW5)
print("#define MAX_INV_POW5    %d" % MAX_INV_POW5)
print()
emit_split("GENERIC_POW5_SPLIT", split)
emit_errors("POW5_ERRORS", errors)
emit_split("GENERIC_POW5_INV_SPLIT", inv_split)
emit_errors("POW5_INV_ERRORS", inv_errors)
sys.exit(0)
