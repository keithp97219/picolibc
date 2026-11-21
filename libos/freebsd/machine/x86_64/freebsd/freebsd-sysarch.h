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

#ifndef _FREEBSD_SYSARCH_H_
#define _FREEBSD_SYSARCH_H_

#define FREEBSD_AMD64_CLEAR_PKRU      134
#define FREEBSD_AMD64_DISABLE_TLSBASE 137
#define FREEBSD_AMD64_GET_FSBASE      128
#define FREEBSD_AMD64_GET_GSBASE      130
#define FREEBSD_AMD64_GET_TLSBASE     135
#define FREEBSD_AMD64_GET_XFPUSTATE   132
#define FREEBSD_AMD64_PKRU_EXCL       1
#define FREEBSD_AMD64_PKRU_PERSIST    2
#define FREEBSD_AMD64_SET_FSBASE      129
#define FREEBSD_AMD64_SET_GSBASE      131
#define FREEBSD_AMD64_SET_PKRU        133
#define FREEBSD_AMD64_SET_TLSBASE     136
#define FREEBSD_I386_CLEAR_PKRU       13
#define FREEBSD_I386_GET_FSBASE       7
#define FREEBSD_I386_GET_GSBASE       9
#define FREEBSD_I386_GET_IOPERM       3
#define FREEBSD_I386_GET_LDT          0
#define FREEBSD_I386_GET_XFPUSTATE    11
#define FREEBSD_I386_SET_FSBASE       8
#define FREEBSD_I386_SET_GSBASE       10
#define FREEBSD_I386_SET_IOPERM       4
#define FREEBSD_I386_SET_LDT          1
#define FREEBSD_I386_SET_PKRU         12

#endif /* _FREEBSD_SYSARCH_H_ */
