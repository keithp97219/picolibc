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
 * Default interrupt masking for the libatomic functions on Arm cores
 * without atomic read-modify-write instructions: armv6-m and
 * armv4t/armv5. Masks IRQ only, not FIQ or NMI, and needs privileged
 * execution: in unprivileged modes the writes are ignored. An RTOS
 * replaces both with its own interrupt lock.
 */

#include <sys/lock.h>

#if __GCC_ATOMIC_INT_LOCK_FREE < 2

#if __ARM_ARCH_PROFILE == 'M'

/* PRIMASK, which cpsid and msr reach from Thumb-1 */

unsigned long __weak
__retarget_atomic_irq_save(void)
{
    unsigned long primask;

    __asm__ volatile("mrs %0, primask\n"
                     "cpsid i"
                     : "=r"(primask)
                     :
                     : "memory");
    return primask;
}

void __weak
__retarget_atomic_irq_restore(unsigned long state)
{
    __asm__ volatile("msr primask, %0" : : "r"(state) : "memory");
}

#else

/*
 * The CPSR I bit. Thumb-1 can't reach the CPSR, so these are always
 * built in ARM state; callers in Thumb state reach them by
 * interworking.
 */

#define ARM_CPSR_I 0x80

unsigned long __weak __attribute__((target("arm")))
__retarget_atomic_irq_save(void)
{
    unsigned long cpsr, masked;

    __asm__ volatile("mrs %0, cpsr\n"
                     "orr %1, %0, %2\n"
                     "msr cpsr_c, %1"
                     : "=&r"(cpsr), "=&r"(masked)
                     : "i"(ARM_CPSR_I)
                     : "memory");
    return cpsr & ARM_CPSR_I;
}

void __weak __attribute__((target("arm")))
__retarget_atomic_irq_restore(unsigned long state)
{
    unsigned long cpsr;

    __asm__ volatile("mrs %0, cpsr\n"
                     "bic %0, %0, %2\n"
                     "orr %0, %0, %1\n"
                     "msr cpsr_c, %0"
                     : "=&r"(cpsr)
                     : "r"(state), "i"(ARM_CPSR_I)
                     : "memory");
}

#endif /* __ARM_ARCH_PROFILE == 'M' */

#endif /* __GCC_ATOMIC_INT_LOCK_FREE < 2 */
