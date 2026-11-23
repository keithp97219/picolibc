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
"""Generate the tables in ryu_table_float.c and check their precision.

ftoa_ryu.c computes vr2 = floor(2 * m2 * 2^e2 / 10^e10) using only
32x32->64 bit multiplies. m2 is the float significand normalized so
that 2^23 <= m2 < 2^24 (also for subnormal values) and e2 the exponent
of its lsb. e10 = floor(log10(2^(e2 + 23))) - 8, which leaves
10^8 <= m2 * 2^e2 / 10^e10 < 2 * 10^9, so vr2 fits in 32 bits and
vr2 / 2 has 9 or 10 digits: enough, along with the lsb of vr2 and
whether the division was exact, to round correctly to FTOA_MAX_DIG (9)
digits. That needs, for 0 <= q <= MAX_INV_POW5 and
1 <= p <= MAX_POW5, the values

    invpow5(q) = floor(2^(pow5bits(q) - 1 + INV_BITCOUNT) / 5^q) + 1
    pow5(p)    = floor(5^p / 2^(pow5bits(p) - BITCOUNT))

pow5(p) for p <= MAX_EXACT_POW5 is 5^p shifted left; ftoa_ryu.c
computes those, so the table starts after them.

The precision check follows Ryu's ComputeRequiredBitSizes: for every
e2, it uses the extremes of (m2 * 2^s) mod 5^q (or (m2 * 5^p) mod
2^t) over all m2 to show that the truncated multiplier still gives
the exact floor. INV_BITCOUNT needs at least 58 bits and BITCOUNT 63.

This script prints the tables for ryu_table_float.c (then run
clang-format).
"""

import sys
from fractions import Fraction

INV_BITCOUNT = 63
BITCOUNT = 64
MANT_BITS = 23
E2_MIN = 1 - 127 - MANT_BITS - MANT_BITS
E2_MAX = 254 - 127 - MANT_BITS
DIGITS = 9


def pow5bits(e):
    return ((e * 1217359) >> 19) + 1


def log10pow2(e):
    """floor(log10(2^e)), also for negative e, as in ftoa_ryu.c"""
    return (e * 78913) >> 18


def e10_of(e2):
    return log10pow2(e2 + MANT_BITS) - (DIGITS - 1)


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


def invpow5(q):
    return (1 << (pow5bits(q) - 1 + INV_BITCOUNT)) // 5**q + 1


def pow5(p):
    k = pow5bits(p) - BITCOUNT
    return (5**p) >> k if k >= 0 else (5**p) << -k


def check():
    """Check that vr2 is exact and in range for every e2"""
    n = (1 << (MANT_BITS + 1)) - 1
    m_min = 1 << MANT_BITS
    max_q = 0
    max_p = 0
    for b in range(-200, 200):
        f = log10pow2(b)
        assert Fraction(10) ** f <= Fraction(2) ** b < Fraction(10) ** (f + 1), b
    for e2 in range(E2_MIN, E2_MAX + 1):
        e10 = e10_of(e2)
        if e10 >= 0:
            q = e10
            s = e2 + 1 - q
            assert s >= 0, e2
            k = pow5bits(q) - 1 + INV_BITCOUNT
            shift = k - s
            if q > 0:
                b = 5**q
                d = invpow5(q) * b - (1 << k)
                if n >= b:
                    mx = b - 1
                else:
                    mx = minmax_euclid((1 << s) % b, b, n)[1]
                # floor(m2 * invpow5(q) / 2^shift) is low by less than one
                assert mx * (1 << k) + n * (1 << s) * d < b * (1 << k), e2
            factor = Fraction(1 << s, 5**q)
            max_q = max(max_q, q)
        else:
            p = -e10
            t = -(e2 + 1 + p)
            k = pow5bits(p) - BITCOUNT
            shift = t - k
            if k > 0:
                r = 5**p % (1 << k)
                if r:
                    # no exact results, and the truncation stays below the
                    # smallest fraction
                    assert (1 << t) > n, e2
                    mn = minmax_euclid(5**p % (1 << t), 1 << t, n)[0]
                    assert mn >= n * r, e2
            factor = Fraction(5**p, 1 << t) if t >= 0 else Fraction(5**p << -t)
            max_p = max(max_p, p)
        # mulShift32 handles shifts between 32 and 64
        assert 32 < shift < 64, e2
        # vr2 / 2 has 9 or 10 digits and vr2 fits in 32 bits
        assert 10 ** (DIGITS - 1) <= m_min * factor / 2, e2
        assert n * factor < 4 * 10**DIGITS < 2**32, e2
    return max_q, max_p


MAX_INV_POW5, MAX_POW5 = check()

# Smaller powers of five fit in 64 bits and are computed instead
MAX_EXACT_POW5 = 0
while pow5bits(MAX_EXACT_POW5 + 1) <= BITCOUNT:
    MAX_EXACT_POW5 += 1


def emit(name, values):
    print("static const uint64_t %s[%d] = {" % (name, len(values)))
    for v in values:
        assert 0 < v < 1 << 64
        print("    %du," % v)
    print("};")
    print()


print("/* Generated by mk-ryu-table-float.py */")
print()
print("#define FLOAT_POW5_INV_BITCOUNT %d" % INV_BITCOUNT)
print("#define FLOAT_POW5_BITCOUNT     %d" % BITCOUNT)
print("#define FLOAT_MAX_INV_POW5      %d" % MAX_INV_POW5)
print("#define FLOAT_MAX_POW5          %d" % MAX_POW5)
print("#define FLOAT_MAX_EXACT_POW5    %d" % MAX_EXACT_POW5)
print()
emit("FLOAT_POW5_INV", [invpow5(q) for q in range(MAX_INV_POW5 + 1)])
emit("FLOAT_POW5", [pow5(p) for p in range(MAX_EXACT_POW5 + 1, MAX_POW5 + 1)])
sys.exit(0)
