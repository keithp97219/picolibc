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

#ifndef _SYS_SYSINFO_H_
#define _SYS_SYSINFO_H_

#include <sys/cdefs.h>

_BEGIN_STD_C

/*
 * Linux's sysinfo: overall system statistics. The layout is the
 * kernel's; its __kernel_long_t is long on every Linux target picolibc
 * supports (not x32).
 */
struct sysinfo {
    long           uptime;    /* seconds since boot */
    unsigned long  loads[3];  /* 1, 5 and 15 minute load averages, << SI_LOAD_SHIFT */
    unsigned long  totalram;  /* usable memory, in mem_unit */
    unsigned long  freeram;   /* free memory */
    unsigned long  sharedram; /* shared memory */
    unsigned long  bufferram; /* memory in buffers */
    unsigned long  totalswap; /* swap space */
    unsigned long  freeswap;  /* free swap space */
    unsigned short procs;     /* number of processes */
    unsigned short pad;
    unsigned long  totalhigh; /* high memory */
    unsigned long  freehigh;  /* free high memory */
    unsigned int   mem_unit;  /* memory unit size in bytes */
    char           _f[20 - 2 * sizeof(unsigned long) - sizeof(unsigned int)];
};

#define SI_LOAD_SHIFT 16

int sysinfo(struct sysinfo *__info) __picolibc_export;

_END_STD_C

#endif /* _SYS_SYSINFO_H_ */
