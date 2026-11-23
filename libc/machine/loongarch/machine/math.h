/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Jiaxun Yang <jiaxun.yang@flygoat.com>
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

#ifndef _MACHINE_MATH_H_
#define _MACHINE_MATH_H_

#ifndef __loongarch_soft_float
#define __HAVE_HW_FLOAT 1
#endif

#ifdef __loongarch_double_float
#define __HAVE_HW_DOUBLE 1
#endif

#ifndef __loongarch_soft_float

/*
 * sqrt and sqrtf set errno for negative arguments when __MATH_ERRNO is
 * defined, which <math.h> can't do without exposing the <errno.h>
 * names, so they are only inline without it.
 */

#define _FCLASS_SNAN     (1 << 0)
#define _FCLASS_QNAN     (1 << 1)
#define _FCLASS_MINF     (1 << 2)
#define _FCLASS_MNORM    (1 << 3)
#define _FCLASS_MSUBNORM (1 << 4)
#define _FCLASS_MZERO    (1 << 5)
#define _FCLASS_PINF     (1 << 6)
#define _FCLASS_PNORM    (1 << 7)
#define _FCLASS_PSUBNORM (1 << 8)
#define _FCLASS_PZERO    (1 << 9)

#define _FCLASS_ZERO     (_FCLASS_MZERO | _FCLASS_PZERO)
#define _FCLASS_SUBNORM  (_FCLASS_MSUBNORM | _FCLASS_PSUBNORM)
#define _FCLASS_NORM     (_FCLASS_MNORM | _FCLASS_PNORM)
#define _FCLASS_INF      (_FCLASS_MINF | _FCLASS_PINF)
#define _FCLASS_NAN      (_FCLASS_SNAN | _FCLASS_QNAN)

#if __loongarch_frlen >= 64

/* anything with a 64-bit FPU has FMA */
#define __HAVE_FAST_FMA 1

#define __fclass_d(_x)                                         \
    (__extension__({                                           \
        long __fclass;                                         \
        __asm__("fclass.d %0, %1" : "=f"(__fclass) : "f"(_x)); \
        __fclass;                                              \
    }))

#endif

#if __loongarch_frlen >= 32

/* anything with a 32-bit FPU has FMAF */
#define __HAVE_FAST_FMAF 1

#define __fclass_f(_x)                                         \
    (__extension__({                                           \
        long __fclass;                                         \
        __asm__("fclass.s %0, %1" : "=f"(__fclass) : "f"(_x)); \
        __fclass;                                              \
    }))

#endif

#ifdef __declare_extern_inline

#if __loongarch_frlen >= 64

/* Double-precision functions */
__declare_extern_inline(double)
copysign(double __x, double __y)
{
    double __result;
    __asm__("fcopysign.d\t%0, %1, %2" : "=f"(__result) : "f"(__x), "f"(__y));
    return __result;
}

__declare_extern_inline(double)
fabs(double __x)
{
    double __result;
    __asm__("fabs.d\t%0, %1" : "=f"(__result) : "f"(__x));
    return __result;
}

__declare_extern_inline(double)
fmax(double __x, double __y)
{
    double __result;
    if (__issignaling(__x) || __issignaling(__y))
        return __x + __y;

    __asm__ volatile("fmax.d\t%0, %1, %2" : "=f"(__result) : "f"(__x), "f"(__y));
    return __result;
}

__declare_extern_inline(double)
fmin(double __x, double __y)
{
    double __result;
    if (__issignaling(__x) || __issignaling(__y))
        return __x + __y;

    __asm__ volatile("fmin.d\t%0, %1, %2" : "=f"(__result) : "f"(__x), "f"(__y));
    return __result;
}

__declare_extern_inline(int)
__finite(double __x)
{
    long __cls = __fclass_d(__x);
    return (__cls & (_FCLASS_INF | _FCLASS_NAN)) == 0;
}

#if __MISC_VISIBLE
__declare_extern_inline(int)
finite(double __x)
{
    return __finite(__x);
}
#endif

__declare_extern_inline(int)
__isinfd(double __x)
{
    long __cls = __fclass_d(__x);
    return (__cls & _FCLASS_INF) != 0;
}

__declare_extern_inline(int)
__isnand(double __x)
{
    long __cls = __fclass_d(__x);
    return (__cls & _FCLASS_NAN) != 0;
}

__declare_extern_inline(int)
__fpclassifyd(double __x)
{
    long __cls = __fclass_d(__x);

    if (__cls & _FCLASS_ZERO)
        return FP_ZERO;
    else if (__cls & _FCLASS_NORM)
        return FP_NORMAL;
    else if (__cls & _FCLASS_SUBNORM)
        return FP_SUBNORMAL;
    else if (__cls & _FCLASS_INF)
        return FP_INFINITE;
    else
        return FP_NAN;
}

#ifndef __MATH_ERRNO
__declare_extern_inline(double)
sqrt(double __x)
{
    double __result;
    __asm__ volatile("fsqrt.d %0, %1" : "=f"(__result) : "f"(__x));
    return __result;
}
#endif

__declare_extern_inline(double)
fma(double __x, double __y, double __z)
{
    double __result;
    __asm__ volatile("fmadd.d %0, %1, %2, %3" : "=f"(__result) : "f"(__x), "f"(__y), "f"(__z));
    return __result;
}

#endif /* __loongarch_frlen >= 64 */

#if __loongarch_frlen >= 32

/* Single-precision functions */
__declare_extern_inline(float)
copysignf(float __x, float __y)
{
    float __result;
    __asm__("fcopysign.s\t%0, %1, %2" : "=f"(__result) : "f"(__x), "f"(__y));
    return __result;
}

__declare_extern_inline(float)
fabsf(float __x)
{
    float __result;
    __asm__("fabs.s\t%0, %1" : "=f"(__result) : "f"(__x));
    return __result;
}

__declare_extern_inline(float)
fmaxf(float __x, float __y)
{
    float __result;
    if (__issignalingf(__x) || __issignalingf(__y))
        return __x + __y;

    __asm__ volatile("fmax.s\t%0, %1, %2" : "=f"(__result) : "f"(__x), "f"(__y));
    return __result;
}

__declare_extern_inline(float)
fminf(float __x, float __y)
{
    float __result;
    if (__issignalingf(__x) || __issignalingf(__y))
        return __x + __y;

    __asm__ volatile("fmin.s\t%0, %1, %2" : "=f"(__result) : "f"(__x), "f"(__y));
    return __result;
}

__declare_extern_inline(int)
__finitef(float __x)
{
    long __cls = __fclass_f(__x);
    return (__cls & (_FCLASS_INF | _FCLASS_NAN)) == 0;
}

#if __MISC_VISIBLE
__declare_extern_inline(int)
finitef(float __x)
{
    return __finitef(__x);
}
#endif

__declare_extern_inline(int)
__isinff(float __x)
{
    long __cls = __fclass_f(__x);
    return (__cls & _FCLASS_INF) != 0;
}

__declare_extern_inline(int)
__isnanf(float __x)
{
    long __cls = __fclass_f(__x);
    return (__cls & _FCLASS_NAN) != 0;
}

__declare_extern_inline(int)
__fpclassifyf(float __x)
{
    long __cls = __fclass_f(__x);

    if (__cls & _FCLASS_ZERO)
        return FP_ZERO;
    else if (__cls & _FCLASS_NORM)
        return FP_NORMAL;
    else if (__cls & _FCLASS_SUBNORM)
        return FP_SUBNORMAL;
    else if (__cls & _FCLASS_INF)
        return FP_INFINITE;
    else
        return FP_NAN;
}

#ifndef __MATH_ERRNO
__declare_extern_inline(float)
sqrtf(float __x)
{
    float __result;
    __asm__ volatile("fsqrt.s %0, %1" : "=f"(__result) : "f"(__x));
    return __result;
}
#endif

__declare_extern_inline(float)
fmaf(float __x, float __y, float __z)
{
    float __result;
    __asm__ volatile("fmadd.s %0, %1, %2, %3" : "=f"(__result) : "f"(__x), "f"(__y), "f"(__z));
    return __result;
}

#endif /* __loongarch_frlen >= 32 */

#endif /* defined(__GNUC_GNU_INLINE__) || defined(__GNUC_STDC_INLINE__) */

#endif /* __loongarch_frlen */

#endif /* _MACHINE_MATH_H_ */
