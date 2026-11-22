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

#define _DEFAULT_SOURCE
#include <unistd.h>
#include "../test-flags.h"

#include <stdio.h>

/*
 * Linux's sysinfo (not POSIX; gnulib's readutmp takes the boot time
 * from it): plausible uptime, memory and process count
 */

#if defined(__linux__) && TEST_POSIX_MULTI_PROCESS && __has_include(<sys/sysinfo.h>)
#include <sys/sysinfo.h>

int
main(void)
{
    struct sysinfo si;

    if (sysinfo(&si) != 0) {
        printf("sysinfo failed\n");
        return 1;
    }
    if (si.uptime <= 0 || si.totalram == 0 || si.mem_unit == 0 || si.freeram > si.totalram
        || si.procs == 0) {
        printf("uptime %ld totalram %lu freeram %lu mem_unit %u procs %u\n", si.uptime, si.totalram,
               si.freeram, si.mem_unit, si.procs);
        return 1;
    }
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no sysinfo, skipping\n");
    return 77;
}

#endif
