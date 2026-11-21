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

#define _POSIX_C_SOURCE 202405L
/* sigaltstack and SA_ONSTACK are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_SIGNALS)

#include <fenv.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(_POSIX_THREADS) && _POSIX_THREADS > 0
#include <pthread.h>
#endif

/*
 * setjmp, longjmp, sigsetjmp and siglongjmp: the values returned, the
 * state kept and restored (registers, objects, the floating-point
 * environment) and the signal mask, also when jumping out of a signal
 * handler.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NOINLINE __attribute__((noinline))

static jmp_buf       env;
static sigjmp_buf    senv;

/* Values the compiler can't see through */
static volatile long opaque_base = 1000;

static NOINLINE long
opaque(long v)
{
    return opaque_base + v;
}

/*
 * Puts junk in every callee-saved register before jumping, so the
 * jump has to restore them all from the buffer
 */
static NOINLINE _Noreturn void
clobber_and_jump(int sig, int val)
{
#if defined(__aarch64__)
    __asm__ volatile("mov x19, #19\n\tmov x20, #20\n\tmov x21, #21\n\tmov x22, #22\n\t"
                     "mov x23, #23\n\tmov x24, #24\n\tmov x25, #25\n\tmov x26, #26\n\t"
                     "mov x27, #27\n\tmov x28, #28\n\t"
                     "fmov d8, #1.0\n\tfmov d9, #2.0\n\tfmov d10, #3.0\n\tfmov d11, #4.0\n\t"
                     "fmov d12, #5.0\n\tfmov d13, #6.0\n\tfmov d14, #7.0\n\tfmov d15, #8.0"
                     :
                     :
                     : "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "d8",
                       "d9", "d10", "d11", "d12", "d13", "d14", "d15");
#elif defined(__arm__)
    __asm__ volatile("mov r4, #4\n\tmov r5, #5\n\tmov r6, #6\n\tmov r8, #8\n\t"
                     "mov r9, #9\n\tmov r10, #10"
                     :
                     :
                     : "r4", "r5", "r6", "r8", "r9", "r10");
#if defined(__ARM_FP) && (__ARM_FP & 8)
    __asm__ volatile("vmov.f64 d8, #1.0\n\tvmov.f64 d9, #2.0\n\tvmov.f64 d10, #3.0\n\t"
                     "vmov.f64 d11, #4.0\n\tvmov.f64 d12, #5.0\n\tvmov.f64 d13, #6.0\n\t"
                     "vmov.f64 d14, #7.0\n\tvmov.f64 d15, #8.0"
                     :
                     :
                     : "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15");
#endif
#endif
    if (sig)
        siglongjmp(senv, val);
    longjmp(env, val);
}

/* Several frames deep, then the jump */
static void deep(int depth, int sig, int val);
static void (* volatile deeper)(int, int, int) = deep;

static NOINLINE _Noreturn void
deep(int depth, int sig, int val)
{
    volatile char pad[64];

    pad[0] = (char)depth;
    if (depth > 0)
        deeper(depth - 1, sig, val + pad[0] - depth);
    clobber_and_jump(sig, val);
}

/* What setjmp, or sigsetjmp, returns after a jump with val */
static NOINLINE int
returned(int sig, int savemask, int val)
{
    if (sig) {
        switch (sigsetjmp(senv, savemask)) {
        case 0:
            deep(3, sig, val);
        case 1:
            return 1;
        case 42:
            return 42;
        case -1:
            return -1;
        case INT_MAX:
            return INT_MAX;
        case INT_MIN:
            return INT_MIN;
        default:
            return 0;
        }
    }
    switch (setjmp(env)) {
    case 0:
        deep(3, sig, val);
    case 1:
        return 1;
    case 42:
        return 42;
    case -1:
        return -1;
    case INT_MAX:
        return INT_MAX;
    case INT_MIN:
        return INT_MIN;
    default:
        return 0;
    }
}

static void
check_values(void)
{
    static const int values[] = { 1, 42, -1, INT_MAX, INT_MIN };
    unsigned         i;
    int              kind;

    /* setjmp, then sigsetjmp without and with the mask */
    for (kind = 0; kind < 3; kind++) {
        int sig = kind > 0, savemask = kind > 1;

        for (i = 0; i < sizeof(values) / sizeof(values[0]); i++)
            check(returned(sig, savemask, values[i]) == values[i], "value passed through");
        /* A zero val makes setjmp return 1, never 0 */
        check(returned(sig, savemask, 0) == 1, "val 0 returns 1");
    }
}

/* Objects: volatile and static ones keep the values they had at the jump */
static int changed_static;

static NOINLINE int
check_objects(int sig)
{
    volatile int changed = 1;
    int          unchanged = (int)opaque(5);

    changed_static = 1;
    if (sig) {
        if (sigsetjmp(senv, 1) == 0) {
            changed = 2;
            changed_static = 2;
            deep(2, sig, 1);
        }
    } else if (setjmp(env) == 0) {
        changed = 2;
        changed_static = 2;
        deep(2, sig, 1);
    }
    check(changed == 2, "volatile automatic object");
    check(changed_static == 2, "static object");
    check(unchanged == 1005, "unchanged automatic object");
    return 0;
}

/*
 * The caller's values, held in callee-saved registers across the
 * call, survive a jump inside it
 */
static NOINLINE void
check_registers(int sig)
{
    long   a = opaque(1), b = opaque(2), c = opaque(3), d = opaque(4), e = opaque(5);
    long   f = opaque(6), g = opaque(7), h = opaque(8), i = opaque(9), j = opaque(10);
    double x = (double)opaque(11) / 4, y = (double)opaque(12) / 4, z = (double)opaque(13) / 4;
    double u = (double)opaque(14) / 4, v = (double)opaque(15) / 4, w = (double)opaque(16) / 4;
    double s = (double)opaque(17) / 4, t = (double)opaque(18) / 4;

    check_objects(sig);
    check(a == 1001 && b == 1002 && c == 1003 && d == 1004 && e == 1005, "integer registers");
    check(f == 1006 && g == 1007 && h == 1008 && i == 1009 && j == 1010, "integer registers");
    check(x == 1011 / 4.0 && y == 1012 / 4.0 && z == 1013 / 4.0 && u == 1014 / 4.0,
          "floating-point registers");
    check(v == 1015 / 4.0 && w == 1016 / 4.0 && s == 1017 / 4.0 && t == 1018 / 4.0,
          "floating-point registers");
}

/* The floating-point environment is as it was at the jump */
static void
check_fenv(void)
{
#ifdef FE_DIVBYZERO
    feclearexcept(FE_ALL_EXCEPT);
    if (feraiseexcept(FE_DIVBYZERO) == 0 && fetestexcept(FE_DIVBYZERO)) {
        feclearexcept(FE_ALL_EXCEPT);
        if (setjmp(env) == 0) {
            feraiseexcept(FE_DIVBYZERO);
            longjmp(env, 1);
        }
        check(fetestexcept(FE_DIVBYZERO) != 0, "status flags kept");
        feclearexcept(FE_ALL_EXCEPT);
    } else {
        printf("floating-point status flags not supported, skipped\n");
    }
#endif
#ifdef FE_UPWARD
    if (fesetround(FE_UPWARD) == 0 && fegetround() == FE_UPWARD) {
        fesetround(FE_TONEAREST);
        if (setjmp(env) == 0) {
            fesetround(FE_UPWARD);
            longjmp(env, 1);
        }
        check(fegetround() == FE_UPWARD, "rounding mode kept");
        fesetround(FE_TONEAREST);
    } else {
        printf("rounding modes not supported, skipped\n");
    }
#endif
}

static int
blocked(int sig)
{
    sigset_t set;

    check(sigprocmask(SIG_SETMASK, NULL, &set) == 0, "sigprocmask");
    return sigismember(&set, sig);
}

static void
set_mask(int how, int sig)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, sig);
    check(sigprocmask(how, &set, NULL) == 0, "sigprocmask");
}

/* siglongjmp restores the mask if and only if sigsetjmp saved it */
static void
check_mask(void)
{
    set_mask(SIG_UNBLOCK, SIGUSR1);
    if (sigsetjmp(senv, 1) == 0) {
        set_mask(SIG_BLOCK, SIGUSR1);
        deep(2, 1, 1);
    }
    check(!blocked(SIGUSR1), "mask restored");

    if (sigsetjmp(senv, 0) == 0) {
        set_mask(SIG_BLOCK, SIGUSR1);
        deep(2, 1, 1);
    }
    check(blocked(SIGUSR1), "mask not restored without savemask");

    /* Saved while blocked, restored after unblocking */
    if (sigsetjmp(senv, 1) == 0) {
        set_mask(SIG_UNBLOCK, SIGUSR1);
        deep(2, 1, 1);
    }
    check(blocked(SIGUSR1), "blocked mask restored");
    set_mask(SIG_UNBLOCK, SIGUSR1);
}

/* Jumps out of signal handlers */
static volatile sig_atomic_t handled;
static volatile int          use_longjmp;
static char                 *alt_base;
static size_t                alt_size;
static volatile int          on_alt;

static void
handler(int sig)
{
    char here;

    (void)sig;
    handled++;
    on_alt = alt_base && &here >= alt_base && &here < alt_base + alt_size;
    if (use_longjmp)
        longjmp(env, 7);
    siglongjmp(senv, 7);
}

static void
install(int flags)
{
    struct sigaction sa = { 0 };

    sa.sa_handler = handler;
    sa.sa_flags = flags;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR2);
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
}

static void
check_handler(void)
{
    install(0);
    set_mask(SIG_UNBLOCK, SIGUSR1);
    set_mask(SIG_UNBLOCK, SIGUSR2);

    /* With the mask saved, the handler's mask is undone */
    handled = 0;
    if (sigsetjmp(senv, 1) == 0) {
        raise(SIGUSR1);
        check(0, "handler returned");
    }
    check(handled == 1, "handler ran");
    check(!blocked(SIGUSR1) && !blocked(SIGUSR2), "mask restored after the handler");

    /* Without it, the handler's mask stays */
    handled = 0;
    if (sigsetjmp(senv, 0) == 0) {
        raise(SIGUSR1);
        check(0, "handler returned");
    }
    check(handled == 1, "handler ran");
    check(blocked(SIGUSR1) && blocked(SIGUSR2), "handler's mask kept");
    set_mask(SIG_UNBLOCK, SIGUSR1);
    set_mask(SIG_UNBLOCK, SIGUSR2);

    /* longjmp works too; what it does to the mask is unspecified */
    handled = 0;
    use_longjmp = 1;
    if (setjmp(env) == 0) {
        raise(SIGUSR1);
        check(0, "handler returned");
    }
    use_longjmp = 0;
    check(handled == 1, "handler ran");
    printf("after longjmp from a handler, SIGUSR1 is %s\n",
           blocked(SIGUSR1) ? "still blocked" : "unblocked");
    set_mask(SIG_UNBLOCK, SIGUSR1);
    set_mask(SIG_UNBLOCK, SIGUSR2);

    /* The signal can be delivered again, and jumped out of again */
    handled = 0;
    if (sigsetjmp(senv, 1) == 0)
        raise(SIGUSR1);
    check(handled == 1, "delivered once");
    if (sigsetjmp(senv, 1) == 0)
        raise(SIGUSR1);
    check(handled == 2, "delivered twice");
}

/* Jumping out of a handler on the alternate stack leaves it */
static void
check_altstack(void)
{
#if defined(__PICOLIBC_XSI_SIGNALS) && defined(SA_ONSTACK)
    stack_t ss, cur;
    int     n;

    alt_size = SIGSTKSZ;
    alt_base = malloc(alt_size);
    check(alt_base != NULL, "malloc");
    ss.ss_sp = alt_base;
    ss.ss_size = alt_size;
    ss.ss_flags = 0;
    check(sigaltstack(&ss, NULL) == 0, "sigaltstack");
    install(SA_ONSTACK);
    for (n = 0; n < 2; n++) {
        handled = 0;
        on_alt = 0;
        if (sigsetjmp(senv, 1) == 0)
            raise(SIGUSR1);
        check(handled == 1 && on_alt, "handler ran on the alternate stack");
        check(sigaltstack(NULL, &cur) == 0, "sigaltstack");
        check(!(cur.ss_flags & SS_ONSTACK), "off the alternate stack after the jump");
    }
    ss.ss_flags = SS_DISABLE;
    check(sigaltstack(&ss, NULL) == 0, "sigaltstack");
    free(alt_base);
    alt_base = NULL;
#endif
}

#if defined(_POSIX_THREADS) && _POSIX_THREADS > 0
/* sigsetjmp saves the calling thread's mask */
static void *
thread(void *arg)
{
    (void)arg;
    set_mask(SIG_BLOCK, SIGUSR2);
    if (sigsetjmp(senv, 1) == 0) {
        set_mask(SIG_UNBLOCK, SIGUSR2);
        set_mask(SIG_BLOCK, SIGUSR1);
        deep(2, 1, 1);
    }
    check(blocked(SIGUSR2) && !blocked(SIGUSR1), "thread's mask restored");
    return NULL;
}

static void
check_thread(void)
{
    pthread_t t;

    set_mask(SIG_UNBLOCK, SIGUSR1);
    set_mask(SIG_UNBLOCK, SIGUSR2);
    check(pthread_create(&t, NULL, thread, NULL) == 0, "pthread_create");
    check(pthread_join(t, NULL) == 0, "pthread_join");
    check(!blocked(SIGUSR1) && !blocked(SIGUSR2), "main thread's mask unchanged");
}
#endif

int
main(void)
{
    check_values();
    check_registers(0);
    check_registers(1);
    check_fenv();
    check_mask();
    check_handler();
    check_altstack();
#if defined(_POSIX_THREADS) && _POSIX_THREADS > 0
    check_thread();
#endif
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
