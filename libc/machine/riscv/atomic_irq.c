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
 * Default interrupt masking for the libatomic functions on RISC-V
 * cores without the A extension. Clears and restores mstatus.MIE, so
 * it assumes the code runs in M-mode; programs in S- or U-mode, and
 * RTOSes, replace both with their own interrupt lock.
 */

#include <sys/lock.h>

#if __GCC_ATOMIC_INT_LOCK_FREE < 2

#define MSTATUS_MIE 0x8

unsigned long __weak __attribute__((target("arch=+zicsr")))
__retarget_atomic_irq_save(void)
{
    unsigned long mstatus;

    __asm__ volatile("csrrci %0, mstatus, %1" : "=r"(mstatus) : "i"(MSTATUS_MIE) : "memory");
    return mstatus & MSTATUS_MIE;
}

void __weak __attribute__((target("arch=+zicsr")))
__retarget_atomic_irq_restore(unsigned long state)
{
    __asm__ volatile("csrs mstatus, %0" : : "r"(state) : "memory");
}

#endif /* __GCC_ATOMIC_INT_LOCK_FREE < 2 */
