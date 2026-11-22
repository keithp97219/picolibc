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
 * Which floating types follow the floating point environment. On
 * targets that do double (and long double) arithmetic in software,
 * such as Arm cores with a single-precision FPU, those types neither
 * raise exceptions nor follow the rounding mode; picolibc's <fenv.h>
 * says so with __DOUBLE_NOEXCEPT, __DOUBLE_NOROUND and the long double
 * versions. Tests check those types' exceptions only where they're
 * raised, and their results as rounded to nearest.
 *
 * Include after <fenv.h>.
 */

#ifndef _TEST_FP_ENV_H_
#define _TEST_FP_ENV_H_

#ifdef __FLOAT_NOEXCEPT
#define TEST_FLOAT_RAISES 0
#else
#define TEST_FLOAT_RAISES 1
#endif
#ifdef __DOUBLE_NOEXCEPT
#define TEST_DOUBLE_RAISES 0
#else
#define TEST_DOUBLE_RAISES 1
#endif
#ifdef __LONG_DOUBLE_NOEXCEPT
#define TEST_LONG_DOUBLE_RAISES 0
#else
#define TEST_LONG_DOUBLE_RAISES 1
#endif

#ifdef __FLOAT_NOROUND
#define TEST_FLOAT_ROUNDS 0
#else
#define TEST_FLOAT_ROUNDS 1
#endif
#ifdef __DOUBLE_NOROUND
#define TEST_DOUBLE_ROUNDS 0
#else
#define TEST_DOUBLE_ROUNDS 1
#endif
#ifdef __LONG_DOUBLE_NOROUND
#define TEST_LONG_DOUBLE_ROUNDS 0
#else
#define TEST_LONG_DOUBLE_ROUNDS 1
#endif

/* Whether arithmetic on the type of x raises exceptions */
#define TEST_RAISES(x)                        \
    _Generic((x),                             \
        float: TEST_FLOAT_RAISES,             \
        double: TEST_DOUBLE_RAISES,           \
        long double: TEST_LONG_DOUBLE_RAISES)

/* Whether arithmetic on the type of x follows the rounding mode */
#define TEST_ROUNDS(x)                        \
    _Generic((x),                             \
        float: TEST_FLOAT_ROUNDS,             \
        double: TEST_DOUBLE_ROUNDS,           \
        long double: TEST_LONG_DOUBLE_ROUNDS)

#endif /* _TEST_FP_ENV_H_ */
