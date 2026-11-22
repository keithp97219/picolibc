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
"""Generate the tables in ryu_table_128.c and check their precision.

ldtoa_ryu.c computes vr = floor(mv * 2^e2 / 10^q) for mv = 4 * m2,
with m2 the significand normalized so that its top bit is set (also
for subnormal values), and e2 the exponent of its lsb, less 2. That
needs, for 0 <= i <= MAX_POW5 and 0 <= q <= MAX_INV_POW5, the
BITCOUNT-bit values

    pow5(i)    = floor(5^i / 2^(pow5bits(i) - BITCOUNT))
    invpow5(q) = floor(2^(pow5bits(q) - 1 + BITCOUNT) / 5^q) + 1

These are computed from a table of every POW5_TABLE_SIZE'th value
and a power of five that fits in 256 bits, as in Ryu's generic_128.h
(which uses a step of 56 and a 128-bit power of five). The product
can be low by up to 3; the error tables hold that correction for
each value in two bits.

q is chosen two below floor(log10(2^e2)) (or floor(log10(5^-e2)) for
negative e2), one lower than Ryu, so vr has at least 22 digits for
80-bit values and 37 for binary128: one more than LDTOA_MAX_DIG, so
that many digits can be rounded correctly.

The precision check follows Ryu's ComputeRequiredBitSizes: for every
e2, it uses the extremes of (mv * 2^(e2-q)) mod 5^q (or (mv * 5^i) mod
2^q) over all mv to show that the truncated multiplier still gives
the exact floor. With q two below the estimate, binary128 needs 250
bits; BITCOUNT has one more.

The limits cover the 15-bit binary exponent of the Intel/Motorola
80-bit and IEEE binary128 formats.

This script simulates the C code and checks every value. Its output
replaces the tables in ryu_table_128.c (then run clang-format).
"""

import sys
from fractions import Fraction

POW5_TABLE_SIZE = 111
BITCOUNT = 251


def pow5bits(e):
    return ((e * 163391164108059) >> 46) + 1


def log10pow2(e):
    return (e * 169464822037455) >> 49


def log10pow5(e):
    return (e * 196742565691928) >> 48


def q_pos(e2):
    """q for e2 >= 0, as in ldtoa_ryu.c"""
    return max(0, log10pow2(e2) - 2)


def q_neg(e2):
    """q for e2 < 0, as in ldtoa_ryu.c"""
    return max(0, log10pow5(-e2) - 2)


# (significand bits, smallest e2, largest e2) for Motorola 80-bit
# (subnormal exponent 0), Intel 80-bit and binary128, with subnormal
# values normalized
FORMATS = [
    (64, 0 - 16383 - 63 - 63 - 2, 32766 - 16383 - 63 - 2),
    (64, 1 - 16383 - 63 - 63 - 2, 32766 - 16383 - 63 - 2),
    (113, 1 - 16383 - 112 - 112 - 2, 32766 - 16383 - 112 - 2),
]
E2_MIN = min(f[1] for f in FORMATS)
E2_MAX = max(f[2] for f in FORMATS)

MAX_POW5 = max(-e2 - q_neg(e2) for e2 in range(E2_MIN, 0))
MAX_INV_POW5 = max(q_pos(e2) for e2 in range(0, E2_MAX + 1))


def minmax_euclid(a, b, M):
    """min and max of (a * x) % b for 1 <= x <= M, gcd(a, b) == 1"""
    a %= b
    xl, vl = 1, a  # (a * xl) % b == vl
    xu, wu = 0, b  # (a * xu) % b == b - wu
    while True:
        if vl < wu:
            k = (wu - 1) // vl
            kmax = (M - xu) // xl
            stop = k > kmax
            k = min(k, kmax)
            xu += k * xl
            wu -= k * vl
        else:
            k = (vl - 1) // wu
            kmax = (M - xl) // xu if xu else 0
            stop = k > kmax
            k = min(k, kmax)
            xl += k * xu
            vl -= k * wu
        if stop or k == 0:
            break
    return vl, (b - wu if xu >= 1 else 0)


def check_precision(md, e2min, e2max):
    """Check that vr is exact and has enough digits for every e2"""
    n = 4 * (2**md - 1)
    mv_min = 4 * 2 ** (md - 1)
    min_vr = None
    for e2 in range(e2min, e2max + 1):
        if e2 >= 0:
            q = q_pos(e2)
            k = BITCOUNT + pow5bits(q) - 1
            shift = -e2 + q + k
            if q > 0:
                b = 5**q
                d = invpow5(q) * b - (1 << k)
                if n >= b:
                    mx = b - 1
                else:
                    mx = minmax_euclid((1 << (e2 - q)) % b, b, n)[1]
                # floor(mv * invpow5(q) / 2^shift) is low by less than one
                assert mx * (1 << k) + n * (1 << (e2 - q)) * d < b * (1 << k), e2
            factor = Fraction(1 << (e2 - q), 5**q)
        else:
            q = q_neg(e2)
            i = -e2 - q
            k = pow5bits(i) - BITCOUNT
            shift = q - k
            if k > 0:
                r = 5**i % (1 << k)
                if r:
                    # no exact results, and the truncation stays below the
                    # smallest fraction
                    assert (1 << q) > n, e2
                    mn = minmax_euclid(5**i % (1 << q), 1 << q, n)[0]
                    assert mn >= n * r, e2
            factor = Fraction(5**i, 1 << q)
        assert 0 < shift < 64 * 6, e2
        # vr must fit in 128 bits
        assert n * factor < 2**128, e2
        if q > 0:
            v = mv_min * factor
            min_vr = v if min_vr is None else min(min_vr, v)
    return len(str(int(min_vr)))


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
for md, e2min, e2max in FORMATS:
    print("/* %d-bit significand: vr has at least %d digits */" % (md, check_precision(md, e2min, e2max)))
print()
print("#define POW5_TABLE_SIZE %d" % POW5_TABLE_SIZE)
print(
    "_Static_assert(FLOAT_128_POW5_BITCOUNT == %d && FLOAT_128_POW5_INV_BITCOUNT == %d, "
    '"table precision");' % (BITCOUNT, BITCOUNT)
)
print("#define MAX_POW5        %d" % MAX_POW5)
print("#define MAX_INV_POW5    %d" % MAX_INV_POW5)
print()
emit_split("GENERIC_POW5_SPLIT", split)
emit_errors("POW5_ERRORS", errors)
emit_split("GENERIC_POW5_INV_SPLIT", inv_split)
emit_errors("POW5_INV_ERRORS", inv_errors)
sys.exit(0)
