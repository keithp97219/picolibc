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

        .syntax unified
        .section .text._linux_clone
        .align 4
        .globl _linux_clone
        .type _linux_clone,%function
#ifdef __thumb__
	.thumb
#endif
_linux_clone:
	@ Linux arm clone takes (flags, stack, ptid, tls, ctid)
	@ in r0, r1, r2, r3, r4

	push	{r4, r7}
	mov	r4, r3			@ ctid
	ldr	r3, [sp, #8]		@ tls
	mov	r7, #LINUX_SYS_clone
	swi	0x0
	cmp	r0, #0
	bne	1f

	@ child
	mov	r11, #0
	mov	lr, #0
	pop	{r0, r1}		@ function, argument
	mov	r2, r0
	mov	r0, r1
	blx	r2
	mov	r7, #LINUX_SYS_exit
2:
	swi	0x0
	b	2b
1:
	pop	{r4, r7}
	bx	lr

        .size _linux_clone, . - _linux_clone

#if defined(__ELF__)
        .section .note.GNU-stack,"",%progbits
#endif
