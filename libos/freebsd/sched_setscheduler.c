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

#include "local-sched.h"

/* Returns the former policy, as POSIX requires */
int
sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
    struct __freebsd_sched_param kparam = { .sched_priority = param->sched_priority };
    int                          kpolicy = _sched_policy_to_freebsd(policy);
    int                          former;

    /*
     * Time sharing has the one priority 0; the kernel checks the
     * real-time ranges itself (after privilege, so an unprivileged
     * caller may see EPERM for a bad priority, which POSIX allows)
     */
    if (kpolicy < 0 || (policy == SCHED_OTHER && param->sched_priority != 0)) {
        errno = EINVAL;
        return -1;
    }
    former = sched_getscheduler(pid);
    if (former < 0)
        return -1;
    /*
     * FreeBSD requires privilege for sched_setscheduler even when the
     * policy stays the same; sched_setparam does that part without
     */
    if (former == policy) {
        if (sched_setparam(pid, param) < 0)
            return -1;
        return former;
    }
    /* Time sharing has the one priority 0; run at the kernel's highest */
    if (policy == SCHED_OTHER) {
        long max = _sched_other_kernel_max();

        if (max < 0) {
            errno = EINVAL;
            return -1;
        }
        kparam.sched_priority = (int)max;
    }
    if (_syscall(FREEBSD_SYS_sched_setscheduler, pid, kpolicy, &kparam) < 0)
        return -1;
    return former;
}
