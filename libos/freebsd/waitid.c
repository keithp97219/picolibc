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
#include <string.h>

int
waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
    struct __freebsd_siginfo kinfo;
    int                      koptions = 0;
    int                      kstatus;
    long                     ret;
    int                      kidtype;

    switch (idtype) {
    case P_ALL:
        kidtype = FREEBSD_P_ALL;
        break;
    case P_PID:
        kidtype = FREEBSD_P_PID;
        break;
    case P_PGID:
        /* No process group has ID 0 */
        kidtype = id == 0 ? -1 : FREEBSD_P_PGID;
        break;
    default:
        kidtype = -1;
        break;
    }
    if (options & ~(WEXITED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT))
        kidtype = -1;
    /* There must be something to wait for */
    if (!(options & (WEXITED | WSTOPPED | WCONTINUED)))
        kidtype = -1;
    if (kidtype < 0) {
        errno = EINVAL;
        return -1;
    }

    if (options & WEXITED)
        koptions |= FREEBSD_WEXITED;
    if (options & WSTOPPED)
        koptions |= FREEBSD_WSTOPPED;
    if (options & WCONTINUED)
        koptions |= FREEBSD_WCONTINUED;
    if (options & WNOHANG)
        koptions |= FREEBSD_WNOHANG;
    if (options & WNOWAIT)
        koptions |= FREEBSD_WNOWAIT;

    /* With WNOHANG and no child ready, wait6 returns 0 */
    memset(&kinfo, 0, sizeof(kinfo));
    ret = _syscall(FREEBSD_SYS_wait6, kidtype, (long)id, &kstatus, koptions, NULL, &kinfo);
    if (ret < 0)
        return -1;
    memset(infop, 0, sizeof(*infop));
    if (ret > 0 && kinfo.si_signo)
        _siginfo_from_freebsd(infop, &kinfo);
    return 0;
}
