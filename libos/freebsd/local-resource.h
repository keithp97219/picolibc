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

#ifndef _LOCAL_RESOURCE_H_
#define _LOCAL_RESOURCE_H_

#include "local-freebsd.h"
#include <sys/resource.h>
#include <freebsd/freebsd-resource.h>
#include <freebsd/freebsd-rlimit-struct.h>

/* getpriority and setpriority take these as they are */
_Static_assert(PRIO_PROCESS == FREEBSD_PRIO_PROCESS && PRIO_PGRP == FREEBSD_PRIO_PGRP
                   && PRIO_USER == FREEBSD_PRIO_USER,
               "PRIO_ values match the kernel's");

/* Translate a resource, returning -1 for one FreeBSD lacks */
static inline int
_resource_to_freebsd(int resource)
{
    switch (resource) {
    case RLIMIT_CORE:
        return FREEBSD_RLIMIT_CORE;
    case RLIMIT_CPU:
        return FREEBSD_RLIMIT_CPU;
    case RLIMIT_DATA:
        return FREEBSD_RLIMIT_DATA;
    case RLIMIT_FSIZE:
        return FREEBSD_RLIMIT_FSIZE;
    case RLIMIT_NOFILE:
        return FREEBSD_RLIMIT_NOFILE;
    case RLIMIT_STACK:
        return FREEBSD_RLIMIT_STACK;
    case RLIMIT_AS:
        return FREEBSD_RLIMIT_AS;
    default:
        return -1;
    }
}

static inline rlim_t
_rlim_from_freebsd(__int64_t v)
{
    return v == FREEBSD_RLIM_INFINITY ? RLIM_INFINITY : (rlim_t)v;
}

/* RLIM_INFINITY is larger than any other value, even where rlim_t has larger ones */
static inline __int64_t
_rlim_to_freebsd(rlim_t v)
{
    return v >= (rlim_t)RLIM_INFINITY ? FREEBSD_RLIM_INFINITY : (__int64_t)v;
}

#endif /* _LOCAL_RESOURCE_H_ */
