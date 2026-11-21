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

#ifndef _FREEBSD_SIGEVENT_STRUCT_H_
#define _FREEBSD_SIGEVENT_STRUCT_H_

struct __freebsd_sigevent {
    int        sigev_notify;
    int        sigev_signo;
    __uint64_t sigev_value;
    int        sigev_notify_thread_id;
    __uint8_t  __adjust_20[60];
};

_Static_assert(sizeof(struct __freebsd_sigevent) == 80, "struct sigevent size");
_Static_assert(__builtin_offsetof(struct __freebsd_sigevent, sigev_notify) == 0,
               "struct sigevent.sigev_notify");
_Static_assert(__builtin_offsetof(struct __freebsd_sigevent, sigev_signo) == 4,
               "struct sigevent.sigev_signo");
_Static_assert(__builtin_offsetof(struct __freebsd_sigevent, sigev_value) == 8,
               "struct sigevent.sigev_value");
_Static_assert(__builtin_offsetof(struct __freebsd_sigevent, sigev_notify_thread_id) == 16,
               "struct sigevent.sigev_notify_thread_id");

#endif /* _FREEBSD_SIGEVENT_STRUCT_H_ */
