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
        .globl _linux_clone
        .type  _linux_clone, @function
_linux_clone:
#ifdef __x86_64
        # rdi flags, rsi stack, rdx ptid, rcx ctid, r8 tls
        #
        # Linux x86_64 clone takes (flags, stack, ptid, ctid, tls)
        # in rdi, rsi, rdx, r10, r8

        movq    %rcx, %r10
        movl    $LINUX_SYS_clone, %eax
        syscall
        testq   %rax, %rax
        jnz     1f

        # child
        xorl    %ebp, %ebp
        popq    %rax                    # function
        popq    %rdi                    # argument
        andq    $-16, %rsp
        call    *%rax
        movq    %rax, %rdi
        movl    $LINUX_SYS_exit, %eax
        syscall
        hlt
1:
        ret
#else
        # Linux i386 clone takes (flags, stack, ptid, tls, ctid)
        # in ebx, ecx, edx, esi, edi

        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    16(%esp), %ebx          # flags
        movl    20(%esp), %ecx          # stack
        movl    24(%esp), %edx          # ptid
        movl    28(%esp), %edi          # ctid
        movl    32(%esp), %esi          # tls
        movl    $LINUX_SYS_clone, %eax
        int     $0x80
        testl   %eax, %eax
        jnz     1f

        # child
        xorl    %ebp, %ebp
        popl    %eax                    # function
        popl    %ecx                    # argument
        andl    $-16, %esp
        subl    $12, %esp
        pushl   %ecx
        call    *%eax
        movl    %eax, %ebx
        movl    $LINUX_SYS_exit, %eax
        int     $0x80
        hlt
1:
        popl    %edi
        popl    %esi
        popl    %ebx
        ret
#endif
        .size _linux_clone, . - _linux_clone

#if defined(__ELF__)
        .section .note.GNU-stack,"",%progbits
#endif
