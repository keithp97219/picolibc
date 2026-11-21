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

#ifndef _FREEBSD_SIGINFO_STRUCT_H_
#define _FREEBSD_SIGINFO_STRUCT_H_

struct __freebsd_siginfo {
    int        si_signo;
    int        si_errno;
    int        si_code;
    int        si_pid;
    __uint32_t si_uid;
    int        si_status;
    void      *si_addr;
    __uint64_t si_value;
    long       si_band;
    __uint8_t  __adjust_48[32];
};

_Static_assert(sizeof(struct __freebsd_siginfo) == 80, "siginfo_t size");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_signo) == 0, "siginfo_t.si_signo");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_errno) == 4, "siginfo_t.si_errno");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_code) == 8, "siginfo_t.si_code");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_pid) == 12, "siginfo_t.si_pid");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_uid) == 16, "siginfo_t.si_uid");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_status) == 20,
               "siginfo_t.si_status");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_addr) == 24, "siginfo_t.si_addr");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_value) == 32, "siginfo_t.si_value");
_Static_assert(__builtin_offsetof(struct __freebsd_siginfo, si_band) == 40, "siginfo_t.si_band");

#endif /* _FREEBSD_SIGINFO_STRUCT_H_ */
