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

#include "local-mqueue.h"
#include "local-timer.h"
#include <string.h>

int
mq_notify(mqd_t mqdes, const struct sigevent *sev)
{
    struct __kernel_sigevent kev;

    /* No sigevent removes this process's registration */
    if (!sev)
        return (int)_syscall(LINUX_SYS_mq_notify, mqdes, NULL);

    memset(&kev, 0, sizeof(kev));
    switch (sev->sigev_notify) {
    case SIGEV_NONE:
        kev.sigev_notify = LINUX_SIGEV_NONE;
        break;
    case SIGEV_SIGNAL:
        if (sev->sigev_signo <= 0 || sev->sigev_signo >= NSIG) {
            errno = EINVAL;
            return -1;
        }
        kev.sigev_notify = LINUX_SIGEV_SIGNAL;
        kev.sigev_signo = _signal_to_linux(sev->sigev_signo);
        SIMPLE_MAP_SIGVAL(&kev.sigev_value, &sev->sigev_value);
        break;
    case SIGEV_THREAD:
#ifdef __PICOLIBC_PTHREADS
        return _mq_notify_thread(mqdes, sev);
#else
        errno = EINVAL;
        return -1;
#endif
    default:
        errno = EINVAL;
        return -1;
    }
    return (int)_syscall(LINUX_SYS_mq_notify, mqdes, &kev);
}
