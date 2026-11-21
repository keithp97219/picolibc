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

#include "local-ipc.h"
#include <freebsd/freebsd-errno.h>
#include <limits.h>

/*
 * POSIX's lowest type up to that magnitude, for a negative type; past
 * this, the kernel's choice
 */
#define _MSGRCV_PROBE_MAX 1024

ssize_t
msgrcv(int id, void *msg, size_t size, long type, int flags)
{
    /*
     * For a negative type, FreeBSD takes the first message of any type
     * up to the magnitude, not the first of the lowest such type. Ask
     * for each type in turn without waiting; the first one present is
     * the lowest
     */
    if (type < 0) {
        long max = type == LONG_MIN ? LONG_MAX : -type;
        long t;

        for (t = 1; t <= max && t <= _MSGRCV_PROBE_MAX; t++) {
            long ret = _syscall_raw(FREEBSD_SYS_msgrcv, id, msg, size, t, flags | IPC_NOWAIT);

            if (ret >= 0)
                return (ssize_t)ret;
            if (ret != _FREEBSD_ERR(FREEBSD_ENOMSG)) {
                errno = _syscall_errno(ret);
                return -1;
            }
        }
        /* None that low: whatever arrives next is the lowest present */
        if (t > max && (flags & IPC_NOWAIT)) {
            errno = ENOMSG;
            return -1;
        }
    }
    return (ssize_t)_syscall(FREEBSD_SYS_msgrcv, id, msg, size, type, flags);
}
