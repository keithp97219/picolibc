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
    long                     ru_ixrss;
    long                     ru_idrss;
    long                     ru_isrss;
    long                     ru_minflt;
    long                     ru_majflt;
    long                     ru_nswap;
    long                     ru_inblock;
    long                     ru_oublock;
    long                     ru_msgsnd;
    long                     ru_msgrcv;
    long                     ru_nsignals;
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
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_ixrss) == 40,
               "struct rusage.ru_ixrss");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_idrss) == 48,
               "struct rusage.ru_idrss");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_isrss) == 56,
               "struct rusage.ru_isrss");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_minflt) == 64,
               "struct rusage.ru_minflt");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_majflt) == 72,
               "struct rusage.ru_majflt");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_nswap) == 80,
               "struct rusage.ru_nswap");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_inblock) == 88,
               "struct rusage.ru_inblock");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_oublock) == 96,
               "struct rusage.ru_oublock");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_msgsnd) == 104,
               "struct rusage.ru_msgsnd");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_msgrcv) == 112,
               "struct rusage.ru_msgrcv");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_nsignals) == 120,
               "struct rusage.ru_nsignals");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_nvcsw) == 128,
               "struct rusage.ru_nvcsw");
_Static_assert(__builtin_offsetof(struct __freebsd_rusage, ru_nivcsw) == 136,
               "struct rusage.ru_nivcsw");

#endif /* _FREEBSD_RUSAGE_STRUCT_H_ */
