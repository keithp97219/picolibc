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

#include "local-select.h"
#include "local-cancel.h"
#include <string.h>

/*
 * The kernel's fd_set is an array of longs with a bit per descriptor.
 * Translate picolibc's fd_set to and from that rather than depending
 * on the two layouts matching.
 */
#define FREEBSD_NFDBITS ((int)sizeof(unsigned long) * 8)

_Static_assert(sizeof(struct __freebsd_fd_set) * 8 >= FD_SETSIZE, "fd_set size");

static struct __freebsd_fd_set *
_fdset_to_freebsd(struct __freebsd_fd_set *kset, const fd_set *set, int nfds)
{
    if (!set)
        return NULL;
    memset(kset, 0, sizeof(*kset));
    for (int fd = 0; fd < nfds; fd++)
        if (FD_ISSET(fd, set))
            kset->fds_bits[fd / FREEBSD_NFDBITS] |= 1UL << (fd % FREEBSD_NFDBITS);
    return kset;
}

static void
_fdset_from_freebsd(fd_set *set, const struct __freebsd_fd_set *kset, int nfds)
{
    if (!set)
        return;
    for (int fd = 0; fd < nfds; fd++) {
        if (kset->fds_bits[fd / FREEBSD_NFDBITS] & (1UL << (fd % FREEBSD_NFDBITS)))
            FD_SET(fd, set);
        else
            FD_CLR(fd, set);
    }
}

int
_pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
         const struct __freebsd_timespec *kts, const struct __freebsd_sigset *kmask)
{
    struct __freebsd_fd_set  kread, kwrite, kexcept;
    struct __freebsd_fd_set *pkread, *pkwrite, *pkexcept;
    void                    *cancel;
    int                      ret;

    if (nfds < 0 || nfds > FD_SETSIZE) {
        errno = EINVAL;
        return -1;
    }
    pkread = _fdset_to_freebsd(&kread, readfds, nfds);
    pkwrite = _fdset_to_freebsd(&kwrite, writefds, nfds);
    pkexcept = _fdset_to_freebsd(&kexcept, exceptfds, nfds);
    cancel = _cancel_enter();
    ret = _syscall(FREEBSD_SYS_pselect, nfds, pkread, pkwrite, pkexcept, kts, kmask);
    _cancel_leave(cancel);
    /* The sets are left alone when the call fails */
    if (ret >= 0) {
        _fdset_from_freebsd(readfds, pkread, nfds);
        _fdset_from_freebsd(writefds, pkwrite, nfds);
        _fdset_from_freebsd(exceptfds, pkexcept, nfds);
    }
    return ret;
}
