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

        .section .text._syscall_raw
        .globl _syscall_raw
        .type  _syscall_raw, @function
_syscall_raw:
#ifdef __x86_64

        # x86_64 calling convention
        #
        #       rdi     syscall
        #       rsi     arg0
        #       rdx     arg1
        #       rcx     arg2
        #       r8      arg3
        #       r9      arg4
        #       8(rsp)  arg5
        #
        # Registers passed to the FreeBSD kernel
        #
        #       rax     syscall
        #       rcx     return address (handled by syscall instruction)
        #       r11     saved rflags (handled by syscall instruction)
        #       rdi     arg0
        #       rsi     arg1
        #       rdx     arg2
        #       r10     arg3 (can't use rcx because of syscall instruction)
        #       r8      arg4
        #       r9      arg5
        #
        # The kernel sets the carry flag on failure, with the positive
        # errno in rax; return that negated, as Linux does.

	movq %rdi, %rax         # syscall
	movq %rsi, %rdi         # arg0
	movq %rdx, %rsi         # arg1
	movq %rcx, %rdx         # arg2
	movq %r8, %r10          # arg3
	movq %r9, %r8           # arg4
	movq 8(%rsp),%r9        # arg5
        syscall
        jnc     1f
        negq    %rax
1:
        ret
#else
#error "libos/freebsd supports only amd64 on x86"
#endif
        .size   _syscall_raw, . - _syscall_raw

#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
