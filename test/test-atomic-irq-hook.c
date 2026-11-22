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
 * A program (or RTOS) replaces the interrupt masking the libatomic
 * functions use on targets without atomic int. Check that the
 * library calls the replacement around each operation, and that
 * interrupts are masked while the operation runs.
 */

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/lock.h>

#ifdef __GNUCLIKE_PRAGMA_DIAGNOSTIC
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Watomic-alignment"
#endif

/* Targets where the libatomic functions mask interrupts, on bare metal */
#if __GCC_ATOMIC_INT_LOCK_FREE < 2 && (defined(__arm__) || defined(__riscv)) && !defined(__linux__)
#define TEST_IRQ

#if defined(__arm__) && __ARM_ARCH_PROFILE == 'M'

static int
irq_enabled(void)
{
    unsigned long primask;
    __asm__ volatile("mrs %0, primask" : "=r"(primask));
    return !(primask & 1);
}

static void
irq_set(int enable)
{
    if (enable)
        __asm__ volatile("cpsie i" ::: "memory");
    else
        __asm__ volatile("cpsid i" ::: "memory");
}

#elif defined(__arm__)

/* Thumb-1 can't reach the CPSR */
static __attribute__((noinline, target("arm"))) int
irq_enabled(void)
{
    unsigned long cpsr;
    __asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
    return !(cpsr & 0x80);
}

static __attribute__((noinline, target("arm"))) void
irq_set(int enable)
{
    unsigned long cpsr;
    __asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
    if (enable)
        cpsr &= ~0x80UL;
    else
        cpsr |= 0x80;
    __asm__ volatile("msr cpsr_c, %0" : : "r"(cpsr) : "memory");
}

#else

static __attribute__((target("arch=+zicsr"))) int
irq_enabled(void)
{
    unsigned long mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    return (mstatus & 8) != 0;
}

static __attribute__((target("arch=+zicsr"))) void
irq_set(int enable)
{
    if (enable)
        __asm__ volatile("csrsi mstatus, 8" ::: "memory");
    else
        __asm__ volatile("csrci mstatus, 8" ::: "memory");
}

#endif

#define check(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("%s:%d: failed: %s\n", __FILE__, __LINE__, #cond); \
            ret = 1;                                                  \
        }                                                             \
    } while (0)

struct big {
    uint32_t v[3];
};

static _Atomic int        a_int;
static _Atomic uint64_t   a_u64;
static _Atomic struct big a_big;

#endif /* TEST_IRQ */

#ifdef TEST_IRQ

static int saves, restores, depth, enabled_inside;

unsigned long
__retarget_atomic_irq_save(void)
{
    unsigned long state = irq_enabled();

    irq_set(0);
    saves++;
    depth++;
    return state;
}

void
__retarget_atomic_irq_restore(unsigned long state)
{
    /* Still masked when the operation finishes */
    if (irq_enabled())
        enabled_inside++;
    restores++;
    depth--;
    irq_set((int)state);
}

#endif

int
main(void)
{
#ifdef TEST_IRQ
    int        ret = 0;
    int        was = irq_enabled();
    struct big b = {
        { 4, 5, 6 }
    };

    irq_set(1);
    check(atomic_fetch_add(&a_int, 3) == 0);
    check(saves == 1 && restores == 1);
    check(atomic_fetch_add(&a_u64, 0x100000000ULL) == 0);
    check(saves == 2 && restores == 2);
    atomic_store(&a_big, b);
    b = atomic_load(&a_big);
    check(b.v[0] == 4 && b.v[2] == 6);
    check(saves == 4 && restores == 4);
    check(depth == 0);
    check(enabled_inside == 0);
    check(irq_enabled());

    irq_set(was);
    printf("%s\n", ret ? "failed" : "passed");
    return ret;
#else
    printf("libatomic functions don't mask interrupts here, skipping\n");
    return 77;
#endif
}
