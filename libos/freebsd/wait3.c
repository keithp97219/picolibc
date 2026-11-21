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

#include "local-wait.h"
#include <sys/resource.h>
#include <freebsd/freebsd-timeval-struct.h>
#include <freebsd/freebsd-rusage-struct.h>

pid_t
wait3(int *wstatus, int options, struct rusage *rusage)
{
    struct __freebsd_rusage kusage;
    pid_t                   ret;
    int                     kstatus = 0;

    ret = (pid_t)_syscall(FREEBSD_SYS_wait4, -1, &kstatus, _wait_options_to_freebsd(options),
                          rusage ? &kusage : NULL);
    if (ret > 0) {
        if (wstatus)
            *wstatus = _wait_status_from_freebsd(kstatus);
        if (rusage) {
            rusage->ru_utime.tv_sec = kusage.ru_utime.tv_sec;
            rusage->ru_utime.tv_usec = kusage.ru_utime.tv_usec;
            rusage->ru_stime.tv_sec = kusage.ru_stime.tv_sec;
            rusage->ru_stime.tv_usec = kusage.ru_stime.tv_usec;
        }
    }
    return ret;
}
