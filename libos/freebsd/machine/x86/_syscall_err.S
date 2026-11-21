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

        .section .text._syscall_err
        .globl _syscall_err
        .type  _syscall_err, @function
_syscall_err:
#ifdef __x86_64

        # long _syscall_err(long sys_call, long a, long b, long c,
        #                   long d, int *kerrno)
        #
        #       rdi     syscall
        #       rsi     arg0
        #       rdx     arg1
        #       rcx     arg2
        #       r8      arg3
        #       r9      kerrno
        #
        # For calls whose results include small negative values, which
        # _syscall_raw's -errno return can't tell from failure. The
        # kernel sets the carry flag on failure, with the positive
        # errno in rax: store that through kerrno, or 0 on success.

        pushq   %r9             # kerrno, which the kernel needn't preserve
        movq    %rdi, %rax      # syscall
        movq    %rsi, %rdi      # arg0
        movq    %rdx, %rsi      # arg1
        movq    %rcx, %rdx      # arg2
        movq    %r8, %r10       # arg3
        syscall
        popq    %r9             # leaves the carry flag alone
        jc      1f
        movl    $0, (%r9)
        ret
1:
        movl    %eax, (%r9)
        ret
#else
#error "libos/freebsd supports only amd64 on x86"
#endif
        .size   _syscall_err, . - _syscall_err

#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
