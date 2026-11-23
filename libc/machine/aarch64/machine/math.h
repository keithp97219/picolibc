/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2022 Keith Packard
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

#if __ARM_FP != 0
#define __HAVE_HW_FLOAT  1
#define __HAVE_HW_DOUBLE 1
#endif

#if __ARM_FP & 0x8
#define __HAVE_FAST_FMA 1
#endif

#if __ARM_FP & 0x4
#define __HAVE_FAST_FMAF 1
#endif

#ifdef __declare_extern_inline

/*
 * sqrt and sqrtf set errno for negative arguments when __MATH_ERRNO is
 * defined, which <math.h> can't do without exposing the <errno.h>
 * names, so they are only inline without it.
 */

#if __ARM_FP & 0x8

#ifndef __MATH_ERRNO
__declare_extern_inline(double)
sqrt(double __x)
{
    double __result;
    __asm__ __volatile__("fsqrt\t%d0, %d1" : "=w"(__result) : "w"(__x));
    return __result;
}
#endif

__declare_extern_inline(double)
fma(double __x, double __y, double __z)
{
    double __result;
    __asm__ __volatile__("fmadd\t%d0, %d1, %d2, %d3"
                         : "=w"(__result)
                         : "w"(__x), "w"(__y), "w"(__z));
    return __result;
}

#endif /* __ARM_FP & 0x8 */

#if __ARM_FP & 0x4
#ifndef __MATH_ERRNO
__declare_extern_inline(float)
sqrtf(float __x)
{
    float __result;
    __asm__ __volatile__("fsqrt\t%s0, %s1" : "=w"(__result) : "w"(__x));
    return __result;
}
#endif

__declare_extern_inline(float)
fmaf(float __x, float __y, float __z)
{
    float __result;
    __asm__ __volatile__("fmadd\t%s0, %s1, %s2, %s3"
                         : "=w"(__result)
                         : "w"(__x), "w"(__y), "w"(__z));
    return __result;
}

#endif /* __ARM_FP & 0x4 */
#endif

#endif /* _MACHINE_MATH_H_ */
