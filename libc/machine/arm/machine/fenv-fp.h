/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2023 Keith Packard
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
 * Use the compiler builtins when available so that the compiler knows
 * these access FPSCR. Inline asm can't say that it writes FPSCR, so
 * clang will split a floating point comparison around it, testing the
 * condition flags loaded by the write instead of the comparison result.
 */
#ifdef __has_builtin
#if __has_builtin(__builtin_arm_get_fpscr) && __has_builtin(__builtin_arm_set_fpscr)
#define _HAVE_FPSCR_BUILTINS
#endif
#endif

#ifdef _HAVE_FPSCR_BUILTINS
#define _vmrs_fpscr(__r) ((__r) = __builtin_arm_get_fpscr())
#define _vmsr_fpscr(__r) __builtin_arm_set_fpscr(__r)
#else
#define _vmrs_fpscr(__r) __asm__ __volatile__("vmrs %0, fpscr" : "=&r"(__r))
#define _vmsr_fpscr(__r) __asm__ __volatile__("vmsr fpscr, %0" : : "r"(__r))
#endif

#define _FPU_MASK_SHIFT 8
#define _ROUND_MASK     (FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO)

__declare_fenv_inline(int) feclearexcept(int __excepts)
{
    fexcept_t __fpsr;

    _vmrs_fpscr(__fpsr);
    __fpsr &= ~__excepts;
    _vmsr_fpscr(__fpsr);
    return (0);
}

/* GNU extensions; libm/fenv/fenv.c defines them for every version */
#if __GNU_VISIBLE || defined(__PICOLIBC_FENV_ALL)
__declare_fenv_inline(int) fedisableexcept(int __mask)
{
    fenv_t __old_fpsr, __new_fpsr;

    _vmrs_fpscr(__old_fpsr);
    __new_fpsr = __old_fpsr & ~((__mask & FE_ALL_EXCEPT) << _FPU_MASK_SHIFT);
    _vmsr_fpscr(__new_fpsr);
    return ((__old_fpsr >> _FPU_MASK_SHIFT) & FE_ALL_EXCEPT);
}
__declare_fenv_inline(int) feenableexcept(int __mask)
{
    fenv_t __old_fpsr, __new_fpsr, __check_fpsr;

    _vmrs_fpscr(__old_fpsr);
    __new_fpsr = __old_fpsr | ((__mask & FE_ALL_EXCEPT) << _FPU_MASK_SHIFT);
    _vmsr_fpscr(__new_fpsr);
    _vmrs_fpscr(__check_fpsr);
    if (__check_fpsr != __new_fpsr)
        return -1;
    return ((__old_fpsr >> _FPU_MASK_SHIFT) & FE_ALL_EXCEPT);
}
#endif

__declare_fenv_inline(int) fegetenv(fenv_t *__envp)
{
    _vmrs_fpscr(*__envp);
    return (0);
}

#if __GNU_VISIBLE || defined(__PICOLIBC_FENV_ALL)
__declare_fenv_inline(int) fegetexcept(void)
{
    fenv_t __fpsr;

    _vmrs_fpscr(__fpsr);
    return (__fpsr & FE_ALL_EXCEPT);
}
#endif

__declare_fenv_inline(int) fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
    fexcept_t __fpsr;

    _vmrs_fpscr(__fpsr);
    *__flagp = __fpsr & __excepts;
    return (0);
}

__declare_fenv_inline(int) fegetround(void)
{
    fenv_t __fpsr;

    _vmrs_fpscr(__fpsr);
    return (__fpsr & _ROUND_MASK);
}
__declare_fenv_inline(int) feholdexcept(fenv_t *__envp)
{
    fenv_t __env;

    _vmrs_fpscr(__env);
    *__envp = __env;
    __env &= ~(FE_ALL_EXCEPT);
    _vmsr_fpscr(__env);
    return (0);
}

__declare_fenv_inline(int) fesetenv(const fenv_t *__envp)
{
    _vmsr_fpscr(*__envp);
    return (0);
}

__declare_fenv_inline(int) fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
    fexcept_t __fpsr;

    _vmrs_fpscr(__fpsr);
    __fpsr &= ~__excepts;
    __fpsr |= *__flagp & __excepts;
    _vmsr_fpscr(__fpsr);
    return (0);
}

__declare_fenv_inline(int) feraiseexcept(int __excepts)
{
    fexcept_t __ex = __excepts;

    return fesetexceptflag(&__ex, __excepts);
}

/* C23; libm/fenv/fenv.c defines it for every version */
#if __ISO_C_VISIBLE >= 2023 || defined(__PICOLIBC_FENV_ALL)
__declare_fenv_inline(int) fesetexcept(int __excepts)
{
    return feraiseexcept(__excepts);
}
#endif

__declare_fenv_inline(int) fesetround(int __round)
{
    fenv_t __fpsr;

    /* Check for invalid rounding modes */
    if (__round & ~(_ROUND_MASK))
        return 1;
    _vmrs_fpscr(__fpsr);
    __fpsr &= ~(_ROUND_MASK);
    __fpsr |= __round;
    _vmsr_fpscr(__fpsr);
    return (0);
}

__declare_fenv_inline(int) fetestexcept(int __excepts)
{
    fexcept_t __fpsr;

    _vmrs_fpscr(__fpsr);
    return (__fpsr & __excepts);
}

__declare_fenv_inline(int) feupdateenv(const fenv_t *__envp)
{
    fexcept_t __fpsr;

    _vmrs_fpscr(__fpsr);
    _vmsr_fpscr(*__envp);
    feraiseexcept(__fpsr & FE_ALL_EXCEPT);
    return (0);
}
