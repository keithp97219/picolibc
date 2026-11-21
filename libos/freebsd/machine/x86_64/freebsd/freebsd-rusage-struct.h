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

#ifndef _FREEBSD_RUSAGE_STRUCT_H_
#define _FREEBSD_RUSAGE_STRUCT_H_

struct __freebsd_rusage {
    struct __freebsd_timeval ru_utime;
    struct __freebsd_timeval ru_stime;
    long                     ru_maxrss;
    __uint8_t                __adjust_40[24];
    long                     ru_minflt;
    long                     ru_majflt;
    __uint8_t                __adjust_80[48];
    long                     ru_nvcsw;
    long                     ru_nivcsw;
};

_Static_assert(sizeof(struct __freebsd_rusage) == 144, "struct rusage size");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_utime) == 0,
               "struct rusage.ru_utime");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_stime) == 16,
               "struct rusage.ru_stime");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_maxrss) == 32,
               "struct rusage.ru_maxrss");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_minflt) == 64,
               "struct rusage.ru_minflt");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_majflt) == 72,
               "struct rusage.ru_majflt");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_nvcsw) == 128,
               "struct rusage.ru_nvcsw");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_nivcsw) == 136,
               "struct rusage.ru_nivcsw");

#endif /* _FREEBSD_RUSAGE_STRUCT_H_ */
