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

#ifndef _FREEBSD_THR_PARAM_STRUCT_H_
#define _FREEBSD_THR_PARAM_STRUCT_H_

struct __freebsd_thr_param {
    void             *start_func;
    void             *arg;
    char             *stack_base;
    __picolibc_size_t stack_size;
    char             *tls_base;
    __picolibc_size_t tls_size;
    long             *child_tid;
    long             *parent_tid;
    int               flags;
    __uint8_t         __adjust_68[4];
    void             *rtp;
    __uint8_t         __adjust_80[24];
};

_Static_assert(sizeof(struct __freebsd_thr_param) == 104, "struct thr_param size");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, start_func) == 0,
               "struct thr_param.start_func");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, arg) == 8, "struct thr_param.arg");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, stack_base) == 16,
               "struct thr_param.stack_base");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, stack_size) == 24,
               "struct thr_param.stack_size");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, tls_base) == 32,
               "struct thr_param.tls_base");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, tls_size) == 40,
               "struct thr_param.tls_size");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, child_tid) == 48,
               "struct thr_param.child_tid");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, parent_tid) == 56,
               "struct thr_param.parent_tid");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, flags) == 64,
               "struct thr_param.flags");
_Static_assert(__builtin_offsetof(struct __freebsd_thr_param, rtp) == 72, "struct thr_param.rtp");

#endif /* _FREEBSD_THR_PARAM_STRUCT_H_ */
