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

The same values, extended to cover the decimal exponents of long
double input, serve decimal input of long double in atod_ryu.c.
Decimal input n * 10^e10, with n of up to H digits, becomes the binary
significand m2 = floor(n * 10^e10 / 2^e2) and whether that was exact.
m2 has mant_dig + 2 or mant_dig + 3 bits; e2 depends only on e10 and
the bit length bl of n, so for each e10 and bl the value is n times a
fixed ratio N/D. m2 is the top of n times pow5(e10) (or
invpow5(-e10)), shifted right by bl + bits - mant_dig - 3. Those
values are low (high) by less than K units, so the product is off by
less than K * n units and the bits below m2 show the value exact when
they are that close to a multiple of the shift. That is right as long
as no inexact n * N/D is within K * n / 2^shift of an integer;
check_parse() shows that with minmax_euclid over all n of each length,
for every e10 the range checks in atod_ryu.c let through.

H for each type is the most digits its table supports, which is at
least the type's *_DECIMAL_DIG, so printf output reads back exactly:

 * float and double use the 125-bit values of Ryu's ryu_table.c
   (K = 1): 27 and 17 digits.

 * 80-bit and binary128 long double use the corrected values here
   (K = 1): 50 and 36 digits.

 * double-double uses the start of the split tables, copied into
   ryu_table_atod.c and computed without corrections (K = 3): 39
   digits.

This script simulates the C code and checks every value. Its output
replaces the tables in ryu_table_128.c, or with --atod those in
ryu_table_atod.c (then run clang-format). The parse check takes a
few minutes using all CPUs.
"""

import sys
import math
import multiprocessing
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

# Decimal input: (H, (mant_dig, min_exp, max_exp)), as in local-stdio.h
FLT = (24, -125, 128)
DBL = (53, -1021, 1024)
# ryu_table.c, 125 bits, 5^i up to 325 and 5^-i up to 341
PARSE_RYU = [(27, FLT), (17, DBL)]
RYU_BITCOUNT = 125
# ryu_table_atod.c
PARSE_SMALL = [(39, (106, -968, 1024))]
# ryu_table_128.c
PARSE_LARGE = [(50, (64, -16381, 16384)), (50, (64, -16382, 16384)), (36, (113, -16381, 16384))]


def parse_e2(e10, bl, md):
    """e2 in atod_ryu.c"""
    if e10 >= 0:
        return bl - 1 + e10 + pow5bits(e10) - 1 - (md + 1)
    return bl - 1 + e10 - pow5bits(-e10) - (md + 1)


def parse_in_range(e2, md, min_exp, max_exp):
    """the range checks in atod_ryu.c"""
    return min_exp - 2 * md - 4 < e2 < max_exp - md - 1


def parse_limits(configs):
    """largest e10 and -e10 that atod_ryu.c looks up"""
    max_pos = max_neg = 0
    for h, (md, min_exp, max_exp) in configs:
        bl = (10**h - 1).bit_length()
        e = 0
        while parse_in_range(parse_e2(e + 1, 1, md), md, min_exp, max_exp):
            e += 1
        max_pos = max(max_pos, e)
        e = 0
        while parse_in_range(parse_e2(-(e + 1), bl, md), md, min_exp, max_exp):
            e += 1
        max_neg = max(max_neg, e)
    return max_pos, max_neg


SMALL_POW5, SMALL_INV_POW5 = parse_limits(PARSE_SMALL)
LARGE_POW5, LARGE_INV_POW5 = parse_limits(PARSE_LARGE)
assert parse_limits(PARSE_RYU) <= (325, 341)
MAX_POW5 = max(MAX_POW5, LARGE_POW5)
MAX_INV_POW5 = max(MAX_INV_POW5, LARGE_INV_POW5)


def parse_class(args):
    """Check one e10 for all lengths of n; return the bits needed"""
    e10, h, md, min_exp, max_exp, bits, k = args
    nmax = 10**h - 1
    need = 0
    for bl in range(1, nmax.bit_length() + 1):
        e2 = parse_e2(e10, bl, md)
        if not parse_in_range(e2, md, min_exp, max_exp):
            continue
        # m2 = n * num / den
        if e10 >= 0:
            num, den = (10**e10 << -e2, 1) if e2 <= 0 else (10**e10, 1 << e2)
        else:
            num, den = (1 << -e2, 10**-e10) if e2 <= 0 else (1, 10**-e10 << e2)
        g = math.gcd(num, den)
        num //= g
        den //= g
        if den == 1:
            continue
        n_hi = min((1 << bl) - 1, nmax)
        # smallest distance of an inexact n * num / den from an integer, times den
        if n_hi >= den:
            dist = 1
        else:
            mn, mx = minmax_euclid(num % den, den, n_hi)
            dist = min(mn, den - mx)
        # the error is below k * n / 2^shift
        shift = bl + bits - md - 3
        assert dist << shift > k * n_hi * den, (e10, bl, md)
        b = md + 3 - bl
        while dist << (bl + b - md - 3) <= k * n_hi * den:
            b += 1
        need = max(need, b)
    return need


def check_parse(configs, bits, k):
    """Check that the computed values give exact results for decimal input"""
    out = []
    with multiprocessing.get_context("fork").Pool() as pool:
        for h, (md, min_exp, max_exp) in configs:
            lo = -(-min_exp + 2 * md + 4 + (10**h).bit_length()) * 10 // 33 - 1
            hi = max_exp * 10 // 33 + 1
            args = [(e, h, md, min_exp, max_exp, bits(md), k) for e in range(lo, hi + 1)]
            out.append((h, md, max(pool.map(parse_class, args, chunksize=16)), bits(md)))
    return out


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


def parse_error(split, inv_split, max_pos, max_neg):
    """Largest error, in units, of the uncorrected values in ryu_table_atod.c"""
    err = 0
    exact = (pow5, lambda q: invpow5(q) - 1)
    for i in range(max_pos + 1):
        base = i // POW5_TABLE_SIZE
        base2 = base * POW5_TABLE_SIZE
        r = (5 ** (i - base2) * split[base]) >> (pow5bits(i) - pow5bits(base2))
        assert r < 1 << BITCOUNT
        err = max(err, exact[0](i) - r)
    for q in range(1, max_neg + 1):
        base = (q + POW5_TABLE_SIZE - 1) // POW5_TABLE_SIZE
        base2 = base * POW5_TABLE_SIZE
        r = (5 ** (base2 - q) * inv_split[base]) >> (pow5bits(base2) - pow5bits(q))
        assert r < 1 << BITCOUNT
        err = max(err, exact[1](q) - r)
    # floor(x) - r <= err, so x - r < err + 1; atod_ryu.c adds that to 5^-i
    assert err >= 0
    return err + 1


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
nsmall = SMALL_POW5 // POW5_TABLE_SIZE + 1
ninv_small = (SMALL_INV_POW5 + POW5_TABLE_SIZE - 1) // POW5_TABLE_SIZE + 1
PARSE_ERR = parse_error(split, inv_split, SMALL_POW5, SMALL_INV_POW5)
assert PARSE_ERR <= 3
PARSE_ERR = 3

if sys.argv[1:] == ["--atod"]:
    parse = check_parse(PARSE_SMALL, lambda md: BITCOUNT, PARSE_ERR)
    print("/* Generated by mk-ryu-table-128.py --atod */")
    print()
    for h, md, need, bits in parse:
        print("/* %d-bit significand, %d digit input: needs %d of %d bits */" % (md, h, need, bits))
    print()
    print("#define POW5_TABLE_SIZE %d" % POW5_TABLE_SIZE)
    print('_Static_assert(ATOD_RYU_ERR == %d, "decimal input error");' % PARSE_ERR)
    print("#define MAX_POW5        %d" % SMALL_POW5)
    print("#define MAX_INV_POW5    %d" % SMALL_INV_POW5)
    print()
    emit_split("ATOD_POW5_SPLIT", split[:nsmall])
    emit_split("ATOD_POW5_INV_SPLIT", inv_split[:ninv_small])
    sys.exit(0)

parse = check_parse(PARSE_RYU, lambda md: RYU_BITCOUNT, 1)
parse += check_parse(PARSE_LARGE, lambda md: BITCOUNT, 1)
print("/* Generated by mk-ryu-table-128.py */")
print()
for md, e2min, e2max in FORMATS:
    print("/* %d-bit significand: vr has at least %d digits */" % (md, check_precision(md, e2min, e2max)))
for h, md, need, bits in parse:
    print("/* %d-bit significand, %d digit input: needs %d of %d bits */" % (md, h, need, bits))
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
