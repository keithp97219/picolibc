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

#include <picolibc.h>

#ifndef CONSTRUCTORS
#define CONSTRUCTORS 1
#endif

/* From the FreeBSD kernel interface (libos/freebsd's generated headers) */
#define FREEBSD_SYS_sysarch      165
#define FREEBSD_AMD64_SET_FSBASE 129
#define FREEBSD_VERSION          1501000

        .text
	.section	.text._start
	.globl	_start
	.type	_start, @function
_start:

        /* Compute argc, argv and environ.
         *
         * FreeBSD passes a pointer to this block in %rdi; %rsp is
         * aligned for a call:
         *
         *   NULL
         *   environ[nenviron-1]
         *   ..
         *   environ[0]
         *   NULL
         *   argv[argc-1]
         *   ...
         *   argv[0]
         *   argc
         */
        movslq  0(%rdi), %r12                   /* argc = *ap */
        leaq    8(%rdi), %r13                   /* argv = ap + 8 */
        movq    %r13, __argv
        leaq    16(%rdi,%r12,8), %r14           /* environ = ap + argc * 8 + 16 */
        movq    %r14, environ

        /* The kernel aligns %rsp as at a function's entry; calls need it 16-byte aligned */
        andq    $-16, %rsp

#ifdef __THREAD_LOCAL_STORAGE

#ifdef INIT_TLS
	// call to _init_tls(__tls_base)
	mov	$__tls_base, %rdi
	call	_init_tls
#endif
	// call to _set_tls(__tls_base)
	mov	$__tls_base, %rdi
	call	_set_tls
#endif

#if defined(__INIT_FINI_ARRAY) && CONSTRUCTORS
	call	__libc_init_array
#endif

        movq    %r12, %rdi
        movq    %r13, %rsi
        movq    environ, %rdx                   /* envp, the third argument Unix programs expect */
	call	main

	mov	%rax, %rdi
	call	exit

        .size   _start, . - _start

        .text
        .section        .text._set_tls
        .globl  _set_tls
        .type   _set_tls, @function
_set_tls:
        addq    $__x86_tls_tcb_offset, %rdi
        movq    %rdi, (%rdi)                    # fill-in self pointer
        pushq   %rdi                            # sysarch takes a pointer to the base
        movq    $FREEBSD_SYS_sysarch, %rax
        movq    $FREEBSD_AMD64_SET_FSBASE, %rdi
        movq    %rsp, %rsi
        syscall
        popq    %rdi
        ret

/* Reserve space for a TCB. */

        .section        .tls_tail_extra,"aw"
        .type           __tcb_space, @object
        .size           __tcb_space, 8
__tcb_space:
        .zero           8

        .section        .bss.environ
        .globl          environ
        .type           environ, @object
        .size           environ, 8
environ:
        .zero           8

        .section        .bss.__argv
        .globl          __argv
        .type           __argv, @object
        .size           __argv, 8
__argv:
        .zero           8

/* The FreeBSD ABI tag (NT_FREEBSD_ABI_TAG), naming the kernel version built for */
        .section        .note.tag,"a",@note
        .p2align        2
        .long           8                       # name size, "FreeBSD\0"
        .long           4                       # descriptor size
        .long           1                       # NT_FREEBSD_ABI_TAG
        .asciz          "FreeBSD"
        .long           FREEBSD_VERSION

#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
