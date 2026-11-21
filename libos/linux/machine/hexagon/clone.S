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

#include <linux/linux-syscall.h>

/*
 * long _linux_clone(unsigned long flags, void *stack, int *ptid, int *ctid, void *tls)
 *
 * The new stack holds the function for the child to run followed by
 * its argument. The child calls it and exits the thread with the
 * value it returns.
 */

        .section .text._linux_clone
        .p2align 4
        .globl _linux_clone
        .type _linux_clone,@function
_linux_clone:
        /*
         * Linux hexagon clone takes (flags, stack, ptid, ctid, tls) in
         * r0 to r4, just as they arrive here
         */
        r6 = #LINUX_SYS_clone
        trap0(#1)
        p0 = cmp.eq(r0, #0)
        if (!p0) jumpr r31

        /* child: no frame to return to */
        r30 = #0
        r31 = #0
        r2 = memw(r29+#0)               /* function */
        r0 = memw(r29+#4)               /* argument */
        r29 = add(r29, #8)
        callr r2
        r6 = #LINUX_SYS_exit
1:
        trap0(#1)
        jump 1b

        .size _linux_clone, . - _linux_clone

#if defined(__ELF__)
        .section .note.GNU-stack,"",%progbits
#endif
