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

#ifndef _FREEBSD_RESOURCE_H_
#define _FREEBSD_RESOURCE_H_

#define FREEBSD_PRIO_MAX        20
#define FREEBSD_PRIO_MIN        -20
#define FREEBSD_PRIO_PGRP       1
#define FREEBSD_PRIO_PROCESS    0
#define FREEBSD_PRIO_USER       2
#define FREEBSD_RLIMIT_AS       10
#define FREEBSD_RLIMIT_CORE     4
#define FREEBSD_RLIMIT_CPU      0
#define FREEBSD_RLIMIT_DATA     2
#define FREEBSD_RLIMIT_FSIZE    1
#define FREEBSD_RLIMIT_KQUEUES  13
#define FREEBSD_RLIMIT_MEMLOCK  6
#define FREEBSD_RLIMIT_NOFILE   8
#define FREEBSD_RLIMIT_NPROC    7
#define FREEBSD_RLIMIT_NPTS     11
#define FREEBSD_RLIMIT_PIPEBUF  15
#define FREEBSD_RLIMIT_RSS      5
#define FREEBSD_RLIMIT_SBSIZE   9
#define FREEBSD_RLIMIT_STACK    3
#define FREEBSD_RLIMIT_SWAP     12
#define FREEBSD_RLIMIT_UMTXP    14
#define FREEBSD_RLIMIT_VMEM     10
#define FREEBSD_RLIM_INFINITY   9223372036854775807L
#define FREEBSD_RLIM_NLIMITS    16
#define FREEBSD_RUSAGE_CHILDREN -1
#define FREEBSD_RUSAGE_SELF     0
#define FREEBSD_RUSAGE_THREAD   1

#endif /* _FREEBSD_RESOURCE_H_ */
