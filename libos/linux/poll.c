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

#include "local-time.h"
#include "local-cancel.h"
#include "local-poll.h"
#include <poll.h>
#include <stdint.h>

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    void *cancel;
    int   ret;

    _poll_fds_to_linux(fds, nfds);
    cancel = _cancel_enter();
#ifdef LINUX_SYS_poll
    ret = _syscall(LINUX_SYS_poll, fds, nfds, timeout);
#else
    /* A negative timeout waits forever */
    struct __kernel_timespec tmo
        = { .tv_sec = timeout / 1000, .tv_nsec = ((uint64_t)timeout % 1000) * 1000000UL };
    ret = _syscall(LINUX_SYS_ppoll, fds, nfds, timeout < 0 ? NULL : &tmo, NULL, 0);
#endif
    _cancel_leave(cancel);
    _poll_fds_from_linux(fds, nfds);
    return ret;
}
