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

/*
 * Checks for one floating type, included by test-remquo.c with T the
 * type, F(name) the function for it, and LIT(x) a constant of it
 */

static int
F(test_type)(int mode)
{
    volatile T zero = LIT(0.0), one = LIT(1.0), inf = (T)HUGE_VAL, nanv = (T)NAN;
    T          frac, ipart;
    int        e, q;

    /* frexp: +-0 give +-0 and *exp 0, +-Inf and NaN come back */
    e = 12;
    CALLT(F(frexp)(-zero, &e));
    check(gotT == 0 && signbit(gotT) && e == 0 && no_error("frexp(-0)"), "frexp(-0)");
    e = 12;
    CALLT(F(frexp)(zero, &e));
    check(gotT == 0 && !signbit(gotT) && e == 0 && no_error("frexp(0)"), "frexp(0)");
    CALLT(F(frexp)(-inf, &e));
    check(gotT == -inf && no_error("frexp(-Inf)"), "frexp(-Inf)");
    CALLT(F(frexp)(nanv, &e));
    check(isnan(gotT) && no_error("frexp(NaN)"), "frexp(NaN)");
    CALLT(F(frexp)(LIT(-3.0) + zero, &e));
    check(gotT == LIT(-0.75) && e == 2 && no_error("frexp(-3)"), "frexp(-3)");
    CALLT(F(frexp)(MIN_SUB + zero, &e));
    check(gotT == LIT(0.5) && e == MIN_EXP - MANT_DIG + 1 && no_error("frexp(subnormal)"),
          "frexp(subnormal)");

    /* modf: +-Inf give +-0 with *iptr +-Inf; NaN gives NaN in both */
    CALLT(F(modf)(-inf, &ipart));
    check(gotT == 0 && signbit(gotT) && ipart == -inf && no_error("modf(-Inf)"), "modf(-Inf)");
    CALLT(F(modf)(inf, &ipart));
    check(gotT == 0 && !signbit(gotT) && ipart == inf && no_error("modf(Inf)"), "modf(Inf)");
    CALLT(F(modf)(nanv, &ipart));
    check(isnan(gotT) && isnan(ipart) && no_error("modf(NaN)"), "modf(NaN)");
    CALLT(F(modf)(LIT(-3.5) + zero, &ipart));
    check(gotT == LIT(-0.5) && ipart == LIT(-3.0) && no_error("modf(-3.5)"), "modf(-3.5)");
    frac = F(modf)(LIT(-2.0) + zero, &ipart);
    check(frac == 0 && signbit(frac) && ipart == LIT(-2.0), "modf(-2) is -0");

    /* remquo: the remainder, and quo has x/y's sign and its low bits */
    CALLT(F(remquo)(LIT(7.0) + zero, LIT(2.0), &q));
    check(gotT == LIT(-1.0) && q > 0 && (q & 7) == 4 && no_error("remquo(7, 2)"), "remquo(7, 2)");
    CALLT(F(remquo)(LIT(-7.0) + zero, LIT(2.0), &q));
    check(gotT == LIT(1.0) && q < 0 && (-q & 7) == 4 && no_error("remquo(-7, 2)"), "remquo(-7, 2)");
    CALLT(F(remquo)(LIT(10.0) + zero, LIT(-3.0), &q));
    check(gotT == LIT(1.0) && q < 0 && (-q & 7) == 3 && no_error("remquo(10, -3)"),
          "remquo(10, -3)");
    CALLT(F(remquo)(LIT(1000.0) + zero, LIT(3.0), &q));
    check(gotT == LIT(1.0) && q > 0 && (q & 7) == (333 & 7) && no_error("remquo(1000, 3)"),
          "remquo(1000, 3)");
    /* y zero or x infinite is a domain error giving NaN */
    CALLT(F(remquo)(one, zero, &q));
    check(isnan(gotT) && REMQUO_ERROR("remquo(1, 0)", EDOM, FE_INVALID), "remquo(1, 0)");
    CALLT(F(remquo)(one, -zero, &q));
    check(isnan(gotT) && REMQUO_ERROR("remquo(1, -0)", EDOM, FE_INVALID), "remquo(1, -0)");
    CALLT(F(remquo)(inf, one, &q));
    check(isnan(gotT) && REMQUO_ERROR("remquo(Inf, 1)", EDOM, FE_INVALID), "remquo(Inf, 1)");
    CALLT(F(remquo)(-inf, zero, &q));
    check(isnan(gotT) && REMQUO_ERROR("remquo(-Inf, 0)", EDOM, FE_INVALID), "remquo(-Inf, 0)");
    /* A NaN argument gives NaN without an error */
    CALLT(F(remquo)(nanv, zero, &q));
    check(isnan(gotT) && no_error("remquo(NaN, 0)"), "remquo(NaN, 0)");
    CALLT(F(remquo)(inf, nanv, &q));
    check(isnan(gotT) && no_error("remquo(Inf, NaN)"), "remquo(Inf, NaN)");

    /*
     * scalbln and scalbn. Overflow and underflow give +-HUGE_VAL and 0 in
     * round to nearest; the directed modes round them as IEEE 754 says
     */
    if (mode == FE_TONEAREST) {
        CALLT(F(scalbln)(one, LONG_MAX));
        check(gotT == inf && must_error("scalbln(1, LONG_MAX)", ERANGE, FE_OVERFLOW),
              "scalbln(1, LONG_MAX)");
        CALLT(F(scalbln)(-one, LONG_MAX));
        check(gotT == -inf && must_error("scalbln(-1, LONG_MAX)", ERANGE, FE_OVERFLOW),
              "scalbln(-1, LONG_MAX)");
        CALLT(F(scalbln)(one, LONG_MIN));
        check(gotT == 0 && !signbit(gotT)
                  && may_error("scalbln(1, LONG_MIN)", ERANGE, FE_UNDERFLOW),
              "scalbln(1, LONG_MIN)");
        CALLT(F(scalbln)(-one, LONG_MIN));
        check(gotT == 0 && signbit(gotT)
                  && may_error("scalbln(-1, LONG_MIN)", ERANGE, FE_UNDERFLOW),
              "scalbln(-1, LONG_MIN)");
        CALLT(F(scalbn)(-one, INT_MAX));
        check(gotT == -inf && must_error("scalbn(-1, INT_MAX)", ERANGE, FE_OVERFLOW),
              "scalbn(-1, INT_MAX)");
    } else {
        CALLT(F(scalbln)(-one, LONG_MAX));
        check(DIRECTED_OVERFLOW("scalbln(-1, LONG_MAX)", ERANGE, FE_OVERFLOW),
              "scalbln(-1, LONG_MAX)");
    }
    CALLT(F(scalbln)(one, MIN_EXP - MANT_DIG));
    check(gotT == MIN_SUB && may_error("scalbln(1, subnormal)", ERANGE, FE_UNDERFLOW),
          "scalbln(1, subnormal)");
    CALLT(F(scalbln)(LIT(3.0) + zero, 4));
    check(gotT == LIT(48.0) && no_error("scalbln(3, 4)"), "scalbln(3, 4)");
    CALLT(F(scalbln)(nanv, 3));
    check(isnan(gotT) && no_error("scalbln(NaN, 3)"), "scalbln(NaN, 3)");
    CALLT(F(scalbln)(-zero, LONG_MAX));
    check(gotT == 0 && signbit(gotT) && no_error("scalbln(-0, LONG_MAX)"), "scalbln(-0, LONG_MAX)");
    CALLT(F(scalbln)(-inf, LONG_MIN));
    check(gotT == -inf && no_error("scalbln(-Inf, LONG_MIN)"), "scalbln(-Inf, LONG_MIN)");
    CALLT(F(scalbln)(MIN_SUB + zero, 0));
    check(gotT == MIN_SUB && no_error("scalbln(x, 0)"), "scalbln(x, 0)");
    CALLT(F(scalbn)(-zero, INT_MAX));
    check(gotT == 0 && signbit(gotT) && no_error("scalbn(-0, INT_MAX)"), "scalbn(-0, INT_MAX)");
    CALLT(F(scalbn)(inf, INT_MIN));
    check(gotT == inf && no_error("scalbn(Inf, INT_MIN)"), "scalbn(Inf, INT_MIN)");

    /* logb(+-0) is a pole error; ilogb(0), ilogb(Inf) and ilogb(NaN) domain errors */
    CALLT(F(logb)(-zero));
    check(gotT == -inf && LOGB_ERROR("logb(-0)", ERANGE, FE_DIVBYZERO), "logb(-0)");
    CALLT(F(logb)(zero));
    check(gotT == -inf && LOGB_ERROR("logb(0)", ERANGE, FE_DIVBYZERO), "logb(0)");
    CALLT(F(logb)(-inf));
    check(gotT == inf && no_error("logb(-Inf)"), "logb(-Inf)");
    CALLT(F(logb)(MIN_SUB + zero));
    check(gotT == MIN_EXP - MANT_DIG && no_error("logb(subnormal)"), "logb(subnormal)");
    {
        int il;

        CALLI(il, F(ilogb)(zero));
        check(il == FP_ILOGB0 && must_error("ilogb(0)", EDOM, FE_INVALID), "ilogb(0)");
        CALLI(il, F(ilogb)(-inf));
        check(il == INT_MAX && must_error("ilogb(-Inf)", EDOM, FE_INVALID), "ilogb(-Inf)");
        CALLI(il, F(ilogb)(nanv));
        check(il == FP_ILOGBNAN && must_error("ilogb(NaN)", EDOM, FE_INVALID), "ilogb(NaN)");
    }

    /* nan returns a quiet NaN */
    CALLT(F(nan)(""));
    check(isnan(gotT) && no_error("nan(\"\")"), "nan(\"\")");
    CALLT(F(nan)("0x123"));
    check(isnan(gotT) && no_error("nan(\"0x123\")"), "nan(\"0x123\")");
    CALLT(F(nan)("junk"));
    check(isnan(gotT) && no_error("nan(\"junk\")"), "nan(\"junk\")");
    {
        volatile T qnan = F(nan)("");

        CALLT(qnan + one);
        check(isnan(gotT) && no_error("nan(\"\") is quiet"), "nan(\"\") is quiet");
    }

    /* copysign */
    check(F(copysign)(one, -zero) == LIT(-1.0), "copysign(1, -0)");
    frac = F(copysign)(-zero, one);
    check(frac == 0 && !signbit(frac), "copysign(-0, 1)");
    check(F(copysign)(inf, LIT(-2.0)) == -inf, "copysign(Inf, -2)");
    frac = F(copysign)(nanv, -one);
    check(isnan(frac) && signbit(frac), "copysign(NaN, -1)");

    /* log1p: -1 is a pole, less than -1 a domain error */
    CALLT(F(log1p)(-one));
    check(gotT == -inf && must_error("log1p(-1)", ERANGE, FE_DIVBYZERO), "log1p(-1)");
    CALLT(F(log1p)(LIT(-2.0) + zero));
    check(isnan(gotT) && must_error("log1p(-2)", EDOM, FE_INVALID), "log1p(-2)");
    CALLT(F(log1p)(-inf));
    check(isnan(gotT) && must_error("log1p(-Inf)", EDOM, FE_INVALID), "log1p(-Inf)");
    CALLT(F(log1p)(-zero));
    check(gotT == 0 && signbit(gotT) && no_error("log1p(-0)"), "log1p(-0)");
    CALLT(F(log1p)(inf));
    check(gotT == inf && no_error("log1p(Inf)"), "log1p(Inf)");
    CALLT(F(log1p)(nanv));
    check(isnan(gotT) && no_error("log1p(NaN)"), "log1p(NaN)");
    CALLT(F(log1p)(MIN_SUB + zero));
    check(F(fabs)(gotT) <= MIN_NORM && may_error("log1p(subnormal)", ERANGE, FE_UNDERFLOW),
          "log1p(subnormal)");

    /* atan: +-0 and +-Inf */
    CALLT(F(atan)(-zero));
    check(gotT == 0 && signbit(gotT) && no_error("atan(-0)"), "atan(-0)");
    /* pi/2, rounded to nearest, or one of its neighbours in the directed modes */
    {
        T pio2 = LIT(1.5707963267948966192313216916397514420986);

        CALLT(F(atan)(inf));
        check((gotT == pio2
               || (mode != FE_TONEAREST
                   && (gotT == F(nextafter)(pio2, 0) || gotT == F(nextafter)(pio2, 2))))
                  && no_error("atan(Inf)"),
              "atan(Inf)");
        CALLT(F(atan)(-inf));
        check((gotT == -pio2
               || (mode != FE_TONEAREST
                   && (gotT == -F(nextafter)(pio2, 0) || gotT == -F(nextafter)(pio2, 2))))
                  && no_error("atan(-Inf)"),
              "atan(-Inf)");
    }
    CALLT(F(atan)(MIN_SUB + zero));
    check(F(fabs)(gotT) <= MIN_NORM && may_error("atan(subnormal)", ERANGE, FE_UNDERFLOW),
          "atan(subnormal)");

    /* erfc: special values and an underflow to 0 */
    CALLT(F(erfc)(-zero));
    check(gotT == 1 && no_error("erfc(-0)"), "erfc(-0)");
    CALLT(F(erfc)(-inf));
    check(gotT == 2 && no_error("erfc(-Inf)"), "erfc(-Inf)");
    CALLT(F(erfc)(inf));
    check(gotT == 0 && !signbit(gotT) && no_error("erfc(Inf)"), "erfc(Inf)");
    CALLT(F(erfc)(nanv));
    check(isnan(gotT) && no_error("erfc(NaN)"), "erfc(NaN)");
    CALLT(F(erfc)(LIT(120.0) + zero));
    check((gotT == 0 || (mode == FE_UPWARD && gotT == MIN_SUB))
              && may_error("erfc(120)", ERANGE, FE_UNDERFLOW),
          "erfc(120)");

    /* The rounding functions keep the sign and pass +-0, +-Inf and NaN through */
    frac = F(ceil)(LIT(-0.5) + zero);
    check(frac == 0 && signbit(frac), "ceil(-0.5) is -0");
    frac = F(trunc)(LIT(-0.7) + zero);
    check(frac == 0 && signbit(frac), "trunc(-0.7) is -0");
    frac = F(round)(LIT(-0.4) + zero);
    check(frac == 0 && signbit(frac), "round(-0.4) is -0");
    check(F(round)(LIT(-2.5) + zero) == LIT(-3.0), "round(-2.5) away from zero");
    check(F(round)(LIT(0.5) + zero) == LIT(1.0), "round(0.5) away from zero");
    check(F(ceil)(LIT(1.25) + zero) == LIT(2.0), "ceil(1.25)");
    check(F(floor)(LIT(-1.25) + zero) == LIT(-2.0), "floor(-1.25)");
    check(F(trunc)(LIT(-1.75) + zero) == LIT(-1.0), "trunc(-1.75)");
    frac = F(floor)(-zero);
    check(frac == 0 && signbit(frac), "floor(-0)");
    check(F(ceil)(-inf) == -inf && F(floor)(inf) == inf && F(trunc)(-inf) == -inf
              && F(round)(inf) == inf && F(nearbyint)(-inf) == -inf,
          "rounding +-Inf");
    check(isnan(F(ceil)(nanv)) && isnan(F(floor)(nanv)) && isnan(F(trunc)(nanv))
              && isnan(F(round)(nanv)) && isnan(F(nearbyint)(nanv)),
          "rounding NaN");
    frac = F(nearbyint)(-zero);
    check(frac == 0 && signbit(frac), "nearbyint(-0)");
    if (mode == FE_TONEAREST) {
        /* nearbyint doesn't raise inexact */
        CALLT(F(nearbyint)(LIT(0.5) + zero));
        check(gotT == 0 && !signbit(gotT) && !(got_except & FE_INEXACT)
                  && no_error("nearbyint(0.5)"),
              "nearbyint(0.5)");
        CALLT(F(nearbyint)(LIT(-0.25) + zero));
        check(gotT == 0 && signbit(gotT) && !(got_except & FE_INEXACT)
                  && no_error("nearbyint(-0.25)"),
              "nearbyint(-0.25)");
    }

    /* fmax and fmin return the other argument when just one is NaN */
    check(F(fmax)(nanv, LIT(-2.0)) == LIT(-2.0) && F(fmax)(LIT(-2.0), nanv) == LIT(-2.0),
          "fmax with one NaN");
    check(F(fmin)(nanv, LIT(-2.0)) == LIT(-2.0) && F(fmin)(LIT(-2.0), nanv) == LIT(-2.0),
          "fmin with one NaN");
    check(isnan(F(fmax)(nanv, nanv)) && isnan(F(fmin)(nanv, nanv)), "fmax and fmin of NaNs");

    return 0;
}
