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

#include "local-itimer.h"
#include <unistd.h>

#define FREEBSD_ITIMER_MAX (0x7fffffff / 2)

unsigned int
alarm(unsigned int seconds)
{
    struct __freebsd_itimerval knew = { 0 }, kold;
    unsigned int               retval;

    /*
     * alarm can't fail, so clamp to the longest interval the kernel
     * takes (INT32_MAX / 2 seconds, from kern_setitimer)
     */
    knew.it_value.tv_sec = seconds < FREEBSD_ITIMER_MAX ? seconds : FREEBSD_ITIMER_MAX;
    if (_syscall_raw(FREEBSD_SYS_setitimer, FREEBSD_ITIMER_REAL, &knew, &kold) < 0)
        return 0;
    /* Round to the nearest second, but report any time left at all */
    retval = (unsigned int)kold.it_value.tv_sec;
    if (kold.it_value.tv_usec >= 500000 || (retval == 0 && kold.it_value.tv_usec != 0))
        ++retval;
    return retval;
}
