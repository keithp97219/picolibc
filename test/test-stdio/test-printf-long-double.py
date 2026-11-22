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
"""Generate test-printf-long-double.h.

For a set of 80-bit and binary128 values, compute with exact integer
arithmetic the leading decimal digits, enough to round correctly to
any precision the test checks.

Usage: ./test-printf-long-double.py \
           | clang-format --assume-filename=test-printf-long-double.h \
           > test-printf-long-double.h
"""

import random
import sys
from fractions import Fraction

sys.set_int_max_str_digits(0)

NDIGITS = 45

FORMATS = {
    # mant_dig: (explicit leading bit, min exponent of the leading bit)
    64: -16382,
    113: -16382,
}
MAX_EXP = 16383


def value_of(m, e):
    return Fraction(m) * (Fraction(2) ** e)


def decimal_exp(v):
    """floor(log10(v)) for v > 0"""
    e = len(str(v.numerator)) - len(str(v.denominator))
    while Fraction(10) ** e > v:
        e -= 1
    while Fraction(10) ** (e + 1) <= v:
        e += 1
    return e


def exact_digits(v):
    """NDIGITS leading digits, decimal exponent, sticky (more nonzero digits)"""
    e = decimal_exp(v)
    scaled = v / (Fraction(10) ** (e - NDIGITS + 1))
    d = scaled.numerator // scaled.denominator
    sticky = scaled.numerator % scaled.denominator != 0
    return str(d), e, sticky


def normalize(m, e, mant_dig):
    """Return (m, e) with m < 2**mant_dig and e >= min lsb exponent"""
    min_e = FORMATS[mant_dig] - (mant_dig - 1)
    while m >= 2**mant_dig:
        assert m & 1 == 0
        m >>= 1
        e += 1
    while m < 2 ** (mant_dig - 1) and e > min_e:
        m <<= 1
        e -= 1
    assert e >= min_e
    return m, e


def nearest(x, mant_dig):
    """Round the Fraction x > 0 to the nearest (m, e), ties to even"""
    min_e = FORMATS[mant_dig] - (mant_dig - 1)
    b = x.numerator.bit_length() - x.denominator.bit_length()
    e = b - (mant_dig - 1) - 1
    while True:
        e2 = max(e, min_e)
        q = x / (Fraction(2) ** e2)
        m = q.numerator // q.denominator
        r = q - m
        if r > Fraction(1, 2) or (r == Fraction(1, 2) and m % 2 == 1):
            m += 1
        if m == 0:
            return None
        if m >= 2**mant_dig:
            e += 1
            continue
        if m < 2 ** (mant_dig - 1) and e2 > min_e:
            e -= 1
            continue
        return normalize(m, e2, mant_dig)


def literal(m, e, mant_dig):
    """C hexadecimal floating constant for m * 2**e"""
    s = "0x%xp%+dL" % (m, e)
    return s


def make_values(mant_dig):
    vals = []
    min_e = FORMATS[mant_dig] - (mant_dig - 1)
    max_m = 2**mant_dig - 1
    max_e = MAX_EXP - (mant_dig - 1)

    def add(m, e, note=""):
        vals.append((normalize(m, e, mant_dig), note))

    def add_dec(s, note=""):
        n = nearest(Fraction(s.lstrip("-")), mant_dig)
        if n:
            vals.append((n, note or s.lstrip("-")))

    add(max_m, max_e, "LDBL_MAX")
    add(1, FORMATS[mant_dig], "LDBL_MIN")
    add(1, min_e, "LDBL_TRUE_MIN")
    add(3, min_e, "denorm")
    add(2 ** (mant_dig - 1) - 1, min_e, "largest denorm")
    add(2 ** (mant_dig - 1) + 1, min_e, "LDBL_MIN + ulp")
    add(1, 0, "1")
    add(1, -1, "0.5")
    add(5, -1, "2.5")
    add(3, -1, "1.5")
    add(1, -3, "0.125")
    add(19, -1, "9.5")
    add(21, -1, "10.5")
    add(2**mant_dig - 1, 0, "2^mant_dig - 1")
    add(1, 63, "2^63")
    add(1, 64, "2^64")
    add(2**64 - 1, 0, "2^64-1")
    add(1, 112, "2^112")
    add(1, 113, "2^113")
    for k in (0, 1, 2, 3, 15, 19, 20, 21, 22, 27, 30, 34, 35, 36, 37, 38, 39, 55, 56):
        add_dec("1e%d" % k)
        add_dec("1e-%d" % k)
    for k in (100, 308, 1000, 4000, 4931, 4932, 4933, 4950, 4951, 4965, 4966):
        if k <= 4932:
            add_dec("1e%d" % k)
        add_dec("1e-%d" % k)
    for e in (-16494, -16445, -16400, -16383, -16382, -16381, -1000, -64, -63, -1, 1, 63, 64, 1000, 16383):
        if e >= min_e:
            add(1, e, "2^%d" % e)
        if e - mant_dig >= min_e:
            add(2**mant_dig - 1, e - mant_dig, "2^%d - ulp" % e)
    for s in (
        "999.995",
        "1234321234321234",
        "0.1",
        "0.3",
        "0.05",
        "123456789012345678901234567890",
        "1e23",
        "9007199254740993",
        "18446744073709551615",
        "3.14159265358979323846264338327950288",
        "2.71828182845904523536028747135266250",
        # From Ryu's generic_128_test.cc
        "2.2250738585072014E-308",
        "2.98023223876953125E-8",
        "-2.109808898695963E16",
        "4.940656E-318",
        "1.18575755E-316",
        "2.989102097996E-312",
        "9.0608011534336E15",
        "4.708356024711512E18",
        "9.409340012568248E18",
        "1.2345678",
        "1.10169395793497080013e-4927",
        "1.1016939579349708003E-4927",
        "1.1016939579349708001E-4927",
        "1.1754944E-38",
        "3.4028235E38",
        "1.7976931348623157E308",
        "5E-324",
    ):
        add_dec(s)
    rng = random.Random(mant_dig)
    for i in range(150):
        e = rng.randint(min_e, max_e)
        m = rng.randint(2 ** (mant_dig - 1), max_m)
        add(m, e, "random")
    for i in range(100):
        e = rng.randint(-mant_dig - 80, 80)
        m = rng.randint(2 ** (mant_dig - 1), max_m)
        add(m, e, "random")
    for i in range(20):
        m = rng.randint(1, 2 ** (mant_dig - 1) - 1)
        add(m, min_e, "random denorm")
    for i in range(30):
        n = rng.randint(1, 10**rng.randint(1, 30))
        if n < 2**mant_dig:
            add(n, 0, "integer")
    return vals


def emit(mant_dig):
    print("#if __LDBL_MANT_DIG__ == %d" % mant_dig)
    print("static const struct ld_vector ld_vectors[] = {")
    seen = set()
    for (m, e), note in make_values(mant_dig):
        if (m, e) in seen:
            continue
        seen.add((m, e))
        v = value_of(m, e)
        digits, dexp, sticky = exact_digits(v)
        print("    /* %s */" % note)
        print('    LDV(%s, "%s", %d, %d),' % (literal(m, e, mant_dig), digits, dexp, int(sticky)))
    print("};")
    print("#endif")


print("/* Generated by test-printf-long-double.py. Do not edit */")
print()
for mant_dig in sorted(FORMATS):
    emit(mant_dig)
