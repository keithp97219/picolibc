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

#ifndef _STDBIT_H_
#define _STDBIT_H_

#define __STDC_VERSION_STDBIT_H__ 202311L

#include <sys/cdefs.h>
#define __need_size_t
#include <stddef.h>
#include <sys/_exactint.h>
#include <sys/_leastint.h>

#define __STDC_ENDIAN_LITTLE__ __ORDER_LITTLE_ENDIAN__
#define __STDC_ENDIAN_BIG__    __ORDER_BIG_ENDIAN__
#define __STDC_ENDIAN_NATIVE__ __BYTE_ORDER__

_BEGIN_STD_C

#define ___stdbit_func(a, b)  a##b
#define __stdbit_func(a, b)   ___stdbit_func(a, b)
#define __stdbit_func_uc(a)   __stdbit_func(a, _uc)
#define __stdbit_func_us(a)   __stdbit_func(a, _us)
#define __stdbit_func_ui(a)   __stdbit_func(a, _ui)
#define __stdbit_func_ul(a)   __stdbit_func(a, _ul)
#define __stdbit_func_ull(a)  __stdbit_func(a, _ull)
#define __stdbit_func_u128(a) __stdbit_func(a, _u128)

#if __STDC_VERSION__ >= 201112L

#ifdef __SIZEOF_INT128__
#define __stdbit_int128_case(func) , unsigned __int128 : func
#else
#define __stdbit_int128_case(func)
#endif
#define __stdbit_generic_ui(func, __value)                             \
    (_Generic(__value,                                                 \
         unsigned char: __stdbit_func_uc(func),                        \
         unsigned short: __stdbit_func_us(func),                       \
         unsigned int: __stdbit_func_ui(func),                         \
         unsigned long: __stdbit_func_ul(func),                        \
         unsigned long long: __stdbit_func_ull(func)                   \
             __stdbit_int128_case(__stdbit_func_u128(func)))(__value))

#define __stdbit_generic_ub(func, __value) __stdbit_generic_ui(func, __value)

#else /* __STDC_VERSION__ >= 201112L */

#ifdef __SIZEOF_INT128__
#define __stdbit_int128_case(func, __value)                                     \
    sizeof(__value) == sizeof(__int128) ? __stdbit_func_u128(func)(__value) : 0
#else
#define __stdbit_int128_case(func, __value) 0
#endif

#define __stdbit_generic_ui(func, __value)                                             \
    (sizeof(__value) == sizeof(char)            ? __stdbit_func_uc(func)(__value)      \
         : sizeof(__value) == sizeof(short)     ? __stdbit_func_us(func)(__value)      \
         : sizeof(__value) == sizeof(int)       ? __stdbit_func_ui(func)(__value)      \
         : sizeof(__value) == sizeof(long)      ? __stdbit_func_ul(func)(__value)      \
         : sizeof(__value) == sizeof(long long) ? __stdbit_func_ull(func)(__value)     \
                                                : __stdbit_int128_case(func, __value))

#define __stdbit_generic_ub(func, __value) ((__typeof(__value))__stdbit_generic_ui(func, __value))

#endif /* else __STDC_VERSION__ >= 201112L */

/* Count leading ones */

unsigned int stdc_leading_ones_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_ones_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_ones_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_ones_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_ones_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_leading_ones_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_leading_ones)
#define stdc_leading_ones(__value) __builtin_stdc_leading_ones(__value)
#elif __has_builtin(__builtin_clzg)
#define stdc_leading_ones(__value)                                     \
    ((unsigned int)__builtin_clzg((__typeof(__value))~(__value),       \
                                  (int)sizeof(__typeof(__value)) * 8))
#else
#define stdc_leading_ones(__value) __stdbit_generic_ui(stdc_leading_ones, __value)
#endif

/* Count leading zeros */

unsigned int stdc_leading_zeros_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_zeros_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_zeros_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_zeros_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned int stdc_leading_zeros_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_leading_zeros_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_leading_zeros)
#define stdc_leading_zeros(__value) __builtin_stdc_leading_zeros(__value)
#elif __has_builtin(__builtin_clzg)
#define stdc_leading_zeros(__value)                                             \
    ((unsigned int)__builtin_clzg(__value, (int)sizeof(__typeof(__value)) * 8))
#else
#define stdc_leading_zeros(__value) __stdbit_generic_ui(stdc_leading_zeros, __value)
#endif

/* Count trailing ones */

unsigned int stdc_trailing_ones_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_ones_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_ones_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_ones_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_ones_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_trailing_ones_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_trailing_ones)
#define stdc_trailing_ones(__value) __builtin_stdc_trailing_ones(__value)
#elif __has_builtin(__builtin_ctzg)
#define stdc_trailing_ones(__value)                                    \
    ((unsigned int)__builtin_ctzg((__typeof(__value))~(__value),       \
                                  (int)sizeof(__typeof(__value)) * 8))
#else
#define stdc_trailing_ones(__value) __stdbit_generic_ui(stdc_trailing_ones, __value)
#endif

/* Count trailing zeros */

unsigned int stdc_trailing_zeros_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_zeros_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_zeros_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_zeros_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned int stdc_trailing_zeros_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_trailing_zeros_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_trailing_zeros)
#define stdc_trailing_zeros(__value) __builtin_stdc_trailing_zeros(__value)
#elif __has_builtin(__builtin_ctzg)
#define stdc_trailing_zeros(__value)                                            \
    ((unsigned int)__builtin_ctzg(__value, (int)sizeof(__typeof(__value)) * 8))
#else
#define stdc_trailing_zeros(__value) __stdbit_generic_ui(stdc_trailing_zeros, __value)
#endif

/* First leading ones */

unsigned int stdc_first_leading_one_uc(unsigned char __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_one_us(unsigned short __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_one_ui(unsigned int __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_one_ul(unsigned long __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_one_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_first_leading_one_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_first_leading_one)
#define stdc_first_leading_one(__value) __builtin_stdc_first_leading_one(__value)
#elif __has_builtin(__builtin_clzg)
#define stdc_first_leading_one(__value) ((unsigned int)__builtin_clzg(__value, -1) + 1U)
#else
#define stdc_first_leading_one(__value) __stdbit_generic_ui(stdc_first_leading_one, __value)
#endif

/* First leading zeros */

unsigned int stdc_first_leading_zero_uc(unsigned char __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_zero_us(unsigned short __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_zero_ui(unsigned int __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_zero_ul(unsigned long __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_leading_zero_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_first_leading_zero_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_first_leading_zero)
#define stdc_first_leading_zero(__value) __builtin_stdc_first_leading_zero(__value)
#elif __has_builtin(__builtin_clzg)
#define stdc_first_leading_zero(__value)                                   \
    ((unsigned int)__builtin_clzg((__typeof(__value))~(__value), -1) + 1U)
#else
#define stdc_first_leading_zero(__value) __stdbit_generic_ui(stdc_first_leading_zero, __value)
#endif

/* First trailing ones */

unsigned int stdc_first_trailing_one_uc(unsigned char __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_one_us(unsigned short __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_one_ui(unsigned int __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_one_ul(unsigned long __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_one_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_first_trailing_one_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_first_trailing_one)
#define stdc_first_trailing_one(__value) __builtin_stdc_first_trailing_one(__value)
#elif __has_builtin(__builtin_ctzg)
#define stdc_first_trailing_one(__value) ((unsigned int)__builtin_ctzg(__value, -1) + 1U)
#else
#define stdc_first_trailing_one(__value) __stdbit_generic_ui(stdc_first_trailing_one, __value)
#endif

/* First trailing zeros */

unsigned int stdc_first_trailing_zero_uc(unsigned char __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_zero_us(unsigned short __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_zero_ui(unsigned int __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_zero_ul(unsigned long __value) __unsequenced __const
__picolibc_export;
unsigned int stdc_first_trailing_zero_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_first_trailing_zero_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_first_trailing_zero)
#define stdc_first_trailing_zero(__value) __builtin_stdc_first_trailing_zero(__value)
#elif __has_builtin(__builtin_ctzg)
#define stdc_first_trailing_zero(__value)                                  \
    ((unsigned int)__builtin_ctzg((__typeof(__value))~(__value), -1) + 1U)
#else
#define stdc_first_trailing_zero(__value) __stdbit_generic_ui(stdc_first_trailing_zero, __value)
#endif

/* Count ones */

unsigned int stdc_count_ones_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_ones_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_ones_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_ones_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_ones_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_count_ones_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_count_ones)
#define stdc_count_ones(__value) __builtin_stdc_count_ones(__value)
#elif __has_builtin(__builtin_popcountg)
#define stdc_count_ones(__value) __builtin_popcountg(__value)
#else
#define stdc_count_ones(__value) __stdbit_generic_ui(stdc_count_ones, __value)
#endif

/* Count zeros */

unsigned int stdc_count_zeros_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_zeros_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_zeros_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_zeros_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned int stdc_count_zeros_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_count_zeros_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_count_zeros)
#define stdc_count_zeros(__value) __builtin_stdc_count_zeros(__value)
#elif __has_builtin(__builtin_popcountg)
#define stdc_count_zeros(__value) __builtin_popcountg((__typeof(__value))~(__value))
#else
#define stdc_count_zeros(__value) __stdbit_generic_ui(stdc_count_zeros, __value)
#endif

/* Check for values with only one bit set */

_Bool stdc_has_single_bit_uc(unsigned char __value) __unsequenced __const __picolibc_export;
_Bool stdc_has_single_bit_us(unsigned short __value) __unsequenced __const __picolibc_export;
_Bool stdc_has_single_bit_ui(unsigned int __value) __unsequenced __const __picolibc_export;
_Bool stdc_has_single_bit_ul(unsigned long __value) __unsequenced __const __picolibc_export;
_Bool stdc_has_single_bit_ull(unsigned long long __value) __unsequenced __const __picolibc_export;
#ifdef __SIZEOF_INT128__
_Bool stdc_has_single_bit_u128(unsigned __int128 __value) __unsequenced __const __picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_has_single_bit)
#define stdc_has_single_bit(__value) __builtin_stdc_has_single_bit(__value)
#elif __has_builtin(__builtin_popcountg)
#define stdc_has_single_bit(__value) ((_Bool)(__builtin_popcountg(__value) == 1))
#else
#define stdc_has_single_bit(__value) __stdbit_generic_ui(stdc_has_single_bit, __value)
#endif

/* Compute number bits needed to hold a value */

unsigned int stdc_bit_width_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned int stdc_bit_width_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int stdc_bit_width_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned int stdc_bit_width_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned int stdc_bit_width_ull(unsigned long long __value) __unsequenced __const __picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned int stdc_bit_width_u128(unsigned __int128 __value) __unsequenced __const __picolibc_export;
#endif

#if __has_builtin(__builtin_stdc_bit_width)
#define stdc_bit_width(__value) __builtin_stdc_bit_width(__value)
#elif __has_builtin(__builtin_clzg)
#define stdc_bit_width(__value)                                     \
    ((unsigned int)(sizeof(__typeof(__value)) * 8)                  \
     - __builtin_clzg(__value, (int)sizeof(__typeof(__value)) * 8))
#else
#warning using generic bit width
#define stdc_bit_width(__value) __stdbit_generic_ui(stdc_bit_width, __value)
#endif

/* Largest power of two <= value */

unsigned char  stdc_bit_floor_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned short stdc_bit_floor_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int   stdc_bit_floor_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned long  stdc_bit_floor_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned long long stdc_bit_floor_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned __int128 stdc_bit_floor_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if defined(__declare_extern_inline) && __has_builtin(__builtin_clzg)
#define __stdc_bit_floor(type, ext)                                                \
    __declare_extern_inline(type) __stdbit_func(stdc_bit_floor, ext)(type __value) \
    {                                                                              \
        if (__value == 0)                                                          \
            return 0;                                                              \
        return (type)1 << (sizeof(type) * 8 - 1 - __builtin_clzg(__value));        \
    }
__stdc_bit_floor(unsigned char, _uc);
__stdc_bit_floor(unsigned short, _us);
__stdc_bit_floor(unsigned int, _ui);
__stdc_bit_floor(unsigned long, _ul);
__stdc_bit_floor(unsigned long long, _ull);
#ifdef __SIZEOF_INT128__
__stdc_bit_floor(unsigned __int128, _u128);
#endif
#endif

#if __has_builtin(__builtin_stdc_bit_floor)
#define stdc_bit_floor(__value) __builtin_stdc_bit_floor(__value)
#else
#define stdc_bit_floor(__value) __stdbit_generic_ub(stdc_bit_floor, __value)
#endif

/* Smallest power of two >= value */

unsigned char      stdc_bit_ceil_uc(unsigned char __value) __unsequenced __const __picolibc_export;
unsigned short     stdc_bit_ceil_us(unsigned short __value) __unsequenced __const __picolibc_export;
unsigned int       stdc_bit_ceil_ui(unsigned int __value) __unsequenced __const __picolibc_export;
unsigned long      stdc_bit_ceil_ul(unsigned long __value) __unsequenced __const __picolibc_export;
unsigned long long stdc_bit_ceil_ull(unsigned long long __value) __unsequenced __const
__picolibc_export;
#ifdef __SIZEOF_INT128__
unsigned __int128 stdc_bit_ceil_u128(unsigned __int128 __value) __unsequenced __const
__picolibc_export;
#endif

#if defined(__declare_extern_inline) && __has_builtin(__builtin_clzg)
#define __stdc_bit_ceil(type, ext)                                                      \
    __declare_extern_inline(type) __stdbit_func(stdc_bit_ceil, ext)(type __value)       \
    {                                                                                   \
        if (__value <= 1)                                                               \
            return 1;                                                                   \
        return (type)2 << (sizeof(type) * 8 - 1 - __builtin_clzg((type)(__value - 1))); \
    }
__stdc_bit_ceil(unsigned char, _uc);
__stdc_bit_ceil(unsigned short, _us);
__stdc_bit_ceil(unsigned int, _ui);
__stdc_bit_ceil(unsigned long, _ul);
__stdc_bit_ceil(unsigned long long, _ull);
#ifdef __SIZEOF_INT128__
__stdc_bit_ceil(unsigned __int128, _u128);
#endif
#endif

#if __has_builtin(__builtin_stdc_bit_ceil)
#define stdc_bit_ceil(__value) __builtin_stdc_bit_ceil(__value)
#else
#define stdc_bit_ceil(__value) __stdbit_generic_ub(stdc_bit_ceil, __value)
#endif

_END_STD_C

#endif /* _STDBIT_H_ */
