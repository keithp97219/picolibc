/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2020 Keith Packard
 * Copyright (c) 2017 embedded brains GmbH. All rights reserved
   Copyright (c) 2011, 2012 ARM Ltd.  All rights reserved.
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

#if defined(__ARM_FP) && (__ARM_FP & 4)
#define __HAVE_HW_FLOAT 1
#endif

#if defined(__ARM_FP) && (__ARM_FP & 8)
#define __HAVE_HW_DOUBLE 1
#endif

#if defined(__ARM_FEATURE_FMA) && __ARM_FEATURE_FMA && defined(__ARM_FP) && (__ARM_FP & 8)
#define __HAVE_FAST_FMA 1
#endif

#if defined(__ARM_FEATURE_FMA) && __ARM_FEATURE_FMA && defined(__ARM_FP) && (__ARM_FP & 4)
#define __HAVE_FAST_FMAF 1
#endif

#ifdef __declare_extern_inline

/*
 * sqrt and sqrtf set errno for negative arguments when __MATH_ERRNO is
 * defined, which <math.h> can't do without exposing the <errno.h>
 * names, so they are only inline without it.
 */

#if defined(__ARM_FP) && (__ARM_FP & 0x8)

/*
 * Double precision routines
 */

#ifndef __MATH_ERRNO
__declare_extern_inline(double)
sqrt(double __x)
{
    double __result;
#if __ARM_ARCH >= 6
    __asm__ volatile("vsqrt.f64 %P0, %P1" : "=w"(__result) : "w"(__x));
#else
    /* VFP9 Erratum 760019, see GCC sources "gcc/config/arm/vfp.md" */
    __asm__ volatile("vsqrt.f64 %P0, %P1" : "=&w"(__result) : "w"(__x));
#endif
    return __result;
}
#endif

__declare_extern_inline(double)
fabs(double __x)
{
    double __result;
    __asm__("vabs.f64\t%P0, %P1" : "=w"(__result) : "w"(__x));
    return __result;
}

#if __ARM_ARCH >= 8
__declare_extern_inline(double)
ceil(double __x)
{
    double __result;
    __asm__ volatile("vrintp.f64\t%P0, %P1" : "=w"(__result) : "w"(__x));
    return __result;
}

__declare_extern_inline(double)
floor(double __x)
{
    double __result;
    __asm__ volatile("vrintm.f64\t%P0, %P1" : "=w"(__result) : "w"(__x));
    return __result;
}

__declare_extern_inline(double)
nearbyint(double __x)
{
    if (isnan(__x))
        return __x + __x;
#if defined(FE_INEXACT)
    fenv_t __env;
    fegetenv(&__env);
#endif
    __asm__ volatile("vrintr.f64\t%P0, %P1" : "=w"(__x) : "w"(__x));
#if defined(FE_INEXACT)
    fesetenv(&__env);
#endif
    return __x;
}

__declare_extern_inline(double)
rint(double __x)
{
    double __result;
    __asm__ volatile("vrintx.f64\t%P0, %P1" : "=w"(__result) : "w"(__x));
    return __result;
}

__declare_extern_inline(double)
round(double __x)
{
    double __result;
    __asm__ volatile("vrinta.f64\t%P0, %P1" : "=w"(__result) : "w"(__x));
    return __result;
}

__declare_extern_inline(double)
trunc(double __x)
{
    double __result;
    __asm__ volatile("vrintz.f64\t%P0, %P1" : "=w"(__result) : "w"(__x));
    return __result;
}
#endif /* __ARM_ARCH >= 8 */

#ifdef __HAVE_FAST_FMA

__declare_extern_inline(double)
fma(double __x, double __y, double __z)
{
    __asm__ volatile("vfma.f64 %P0, %P1, %P2" : "+w"(__z) : "w"(__x), "w"(__y));
    return __z;
}

#endif

#endif /* (__ARM_FP & 0x8) */

#if defined(__ARM_FP) && (__ARM_FP & 0x4)

/*
 * Single precision functions
 */

#ifndef __MATH_ERRNO
__declare_extern_inline(float)
sqrtf(float __x)
{
    float __result;
#if __ARM_ARCH >= 6
    __asm__ volatile("vsqrt.f32 %0, %1" : "=w"(__result) : "w"(__x));
#else
    /* VFP9 Erratum 760019, see GCC sources "gcc/config/arm/vfp.md" */
    __asm__ volatile("vsqrt.f32 %0, %1" : "=&w"(__result) : "w"(__x) : "cc", "memory");
#endif
    return __result;
}
#endif

__declare_extern_inline(float)
fabsf(float __x)
{
    float __result;
    __asm__("vabs.f32\t%0, %1" : "=t"(__result) : "t"(__x));
    return __result;
}

#if __ARM_ARCH >= 8
__declare_extern_inline(float)
ceilf(float __x)
{
    float __result;
    __asm__ volatile("vrintp.f32\t%0, %1" : "=t"(__result) : "t"(__x));
    return __result;
}

__declare_extern_inline(float)
floorf(float __x)
{
    float __result;
    __asm__ volatile("vrintm.f32\t%0, %1" : "=t"(__result) : "t"(__x));
    return __result;
}

__declare_extern_inline(float)
nearbyintf(float __x)
{
    if (isnan(__x))
        return __x + __x;
#if defined(FE_INEXACT)
    fenv_t __env;
    fegetenv(&__env);
#endif
    __asm__ volatile("vrintr.f32\t%0, %1" : "=t"(__x) : "t"(__x));
#if defined(FE_INEXACT)
    fesetenv(&__env);
#endif
    return __x;
}

__declare_extern_inline(float)
rintf(float __x)
{
    float __result;
    __asm__ volatile("vrintx.f32\t%0, %1" : "=t"(__result) : "t"(__x));
    return __result;
}

__declare_extern_inline(float)
roundf(float __x)
{
    float __result;
    __asm__ volatile("vrinta.f32\t%0, %1" : "=t"(__result) : "t"(__x));
    return __result;
}

__declare_extern_inline(float)
truncf(float __x)
{
    float __result;
    __asm__ volatile("vrintz.f32\t%0, %1" : "=t"(__result) : "t"(__x));
    return __result;
}
#endif /* __ARM_ARCH >= 8 */

#ifdef __HAVE_FAST_FMAF

__declare_extern_inline(float)
fmaf(float __x, float __y, float __z)
{
    __asm__ volatile("vfma.f32 %0, %1, %2" : "+t"(__z) : "t"(__x), "t"(__y));
    return __z;
}

#endif

#endif /* (__ARM_FP & 0x4) */

#endif /* have attributes */

#endif /* _MACHINE_MATH_H_ */
