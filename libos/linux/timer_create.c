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

#include "local-timer.h"
#include <string.h>

int
timer_create(clockid_t clock_id, struct sigevent * __restrict evp, timer_t * __restrict timerid)
{
    struct __kernel_sigevent kev, *pkev = NULL;
    int                      kclock;
    int                      kid;
    int                      ret;

    MAP_CLOCK_ID(kclock, clock_id);

    /* With no sigevent, the kernel sends SIGALRM with the timer id as value */
    if (evp) {
        memset(&kev, 0, sizeof(kev));
        switch (evp->sigev_notify) {
        case SIGEV_NONE:
            kev.sigev_notify = LINUX_SIGEV_NONE;
            break;
        case SIGEV_SIGNAL:
            if (evp->sigev_signo <= 0 || evp->sigev_signo >= NSIG) {
                errno = EINVAL;
                return -1;
            }
            kev.sigev_notify = LINUX_SIGEV_SIGNAL;
            kev.sigev_signo = _signal_to_linux(evp->sigev_signo);
            SIMPLE_MAP_SIGVAL(&kev.sigev_value, &evp->sigev_value);
            break;
        case SIGEV_THREAD:
#ifdef __PICOLIBC_PTHREADS
            ret = _timer_create_thread(kclock, evp, timerid);
            if (ret) {
                errno = ret;
                return -1;
            }
            return 0;
#else
            errno = EINVAL;
            return -1;
#endif
        default:
            errno = EINVAL;
            return -1;
        }
        pkev = &kev;
    }
    ret = (int)_syscall(LINUX_SYS_timer_create, kclock, pkev, &kid);
    if (ret < 0)
        return -1;
    *timerid = (timer_t)kid;
    return 0;
}
