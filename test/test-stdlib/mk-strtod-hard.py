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
"""Generate the tables in test-strtod-hard.c (then run clang-format).

atod_ryu.c computes m2 = floor(n * 10^e10 / 2^e2) from a fixed-precision
power of five and decides whether that was exact from the bits below
m2, which only works when no inexact value comes too close to a
multiple of 2^e2; mk-ryu-table-128.py shows that for the precision it
uses. For each decimal exponent and length of n, this finds the n
closest to such a multiple (with minmax_euclid) and keeps those that
would be misjudged with a few bits less precision than picolibc has,
along with the representable values around each and the nearest one.
"""

import math
import random
import sys
from fractions import Fraction

sys.set_int_max_str_digits(0)


def minmax_euclid(a, b, M):
    """the n in [1, M] giving the min and max of (a * n) % b, gcd(a, b) == 1"""
    a %= b
    xl, vl = 1, a
    xu, wu = 0, b
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
    return xl, xu


def pow5bits(e):
    """ceil(log2(5^e)), 1 for e == 0, as generic_pow5bits"""
    return ((e * 163391164108059) >> 46) + 1


def class_ratio(e10, bl, p):
    """m2 = n * num / den for decimal exponent e10 and n of bl bits"""
    if e10 >= 0:
        e2 = bl - 1 + e10 + pow5bits(e10) - 1 - (p + 1)
        return (10**e10 << -e2, 1) if e2 <= 0 else (10**e10, 1 << e2)
    e2 = bl - 1 + e10 - pow5bits(-e10) - (p + 1)
    return (1 << -e2, 10**-e10) if e2 <= 0 else (1, 10**-e10 << e2)


def candidates(p, mn, mx, H, weak, estep=1, hmin=1):
    """inputs with digits in [hmin, H] misjudged with 'weak' table bits"""
    MH = 10**H - 1
    BL = MH.bit_length()
    lo = -(-mn + 2 * p + 4 + BL) * 10 // 33 - 1
    hi = mx * 10 // 33 + 1
    out = []
    for e in range(lo, hi + 1, estep):
        for bl in range(1, BL + 1):
            if e >= 0:
                g = bl - 1 + e + pow5bits(e) - 1 - (p + 1)
            else:
                g = bl - 1 + e - pow5bits(-e) - (p + 1)
            if not (mn - 2 * p - 4 < g < mx - p - 1):
                continue
            N, D = class_ratio(e, bl, p)
            gg = math.gcd(N, D)
            N //= gg
            D //= gg
            nlo, nhi = 1 << (bl - 1), min((1 << bl) - 1, MH)
            if D == 1 or nhi < nlo or nhi >= D:
                continue
            for x in minmax_euclid(N % D, D, nhi):
                for n in (x, x * max(1, nlo // x)):
                    if not (nlo <= n <= nhi) or len(str(n)) < hmin:
                        continue
                    r = (n * N) % D
                    dist = min(r, D - r)
                    if dist == 0:
                        continue
                    t = bl + weak - p - 3
                    # misjudged with weak bits: within 3 n / 2^t of an integer
                    if dist << t <= 3 * n * D:
                        out.append((n, e))
    return sorted(set(out))


def neighbors(x, p, mn, mx):
    """representable values below and above positive x, and the nearest (even)"""
    big = Fraction(2) ** mx
    top = big - Fraction(2) ** (mx - p)
    if x > top:
        # beyond the largest value; inf (None) once past its half ulp
        return top, None, top if x < big - Fraction(2) ** (mx - p - 1) else None
    # binary exponent of the ulp
    e = x.numerator.bit_length() - x.denominator.bit_length()
    if Fraction(2) ** e > x:
        e -= 1
    q = max(e, mn - 1) - (p - 1)
    s = x / Fraction(2) ** q
    lo = s.numerator // s.denominator
    hi = lo if lo == s else lo + 1
    if s - lo < Fraction(1, 2) or (s - lo == Fraction(1, 2) and lo % 2 == 0):
        nr = lo
    else:
        nr = hi

    def val(k):
        v = Fraction(k) * Fraction(2) ** q
        return None if v >= big else v

    return val(lo), val(hi), val(nr)


def hexlit(v, p, suffix):
    if v is None:
        return "INF" + suffix
    if v == 0:
        return "0x0p+0" + suffix
    e = v.numerator.bit_length() - v.denominator.bit_length()
    if Fraction(2) ** e > v:
        e -= 1
    m = v / Fraction(2) ** e  # in [1, 2)
    frac = m - 1
    nh = (p - 1 + 3) // 4
    f = frac * 16**nh
    assert f.denominator == 1
    return "0x1.%0*xp%+d%s" % (nh, f.numerator, e, suffix)


FMT = {
    "f": (24, -125, 128, "f"),
    "d": (53, -1021, 1024, ""),
    "x": (64, -16381, 16384, "L"),
    "q": (113, -16381, 16384, "L"),
}


def emit(name, fmt, items, typ):
    p, mn, mx, suf = FMT[fmt]
    print("static const struct hard_%s %s[] = {" % (typ, name))
    for n, e in items:
        x = Fraction(n) * Fraction(10) ** e
        lo, hi, nr = neighbors(x, p, mn, mx)
        print(
            '    { "%de%d", %d, %s, %s, %s },'
            % (
                n,
                e,
                len(str(n)),
                hexlit(lo, p, suf),
                hexlit(hi, p, suf),
                hexlit(nr, p, suf),
            )
        )
    print("};")
    print()


random.seed(1)


def pick(c, k):
    return sorted(random.sample(c, min(k, len(c))), key=lambda t: (t[1], t[0]))


if __name__ == "__main__":
    # Each type with as many digits as picolibc supports for it (see
    # local-stdio.h), misjudged with a few bits less than it uses
    f27 = candidates(24, -125, 128, 27, 116)
    d17 = candidates(53, -1021, 1024, 17, 118)
    x50 = candidates(64, -16381, 16384, 50, 244, estep=3)
    q36 = candidates(113, -16381, 16384, 36, 244, estep=3)
    emit("hard_float", "f", pick(f27, 40), "float")
    print("#if __DBL_MANT_DIG__ == 53")
    emit("hard_double", "d", pick(d17, 56), "double")
    print("#endif")
    print("#if defined(CHECK_LONG_DOUBLE) && __LDBL_MANT_DIG__ == 64")
    emit("hard_long_double", "x", pick(x50, 24), "long_double")
    print("#elif defined(CHECK_LONG_DOUBLE) && __LDBL_MANT_DIG__ == 113")
    emit("hard_long_double", "q", pick(q36, 24), "long_double")
    print("#endif")
