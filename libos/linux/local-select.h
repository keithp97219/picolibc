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

#ifndef _LOCAL_SELECT_H_
#define _LOCAL_SELECT_H_

#include "local-linux.h"
#include <string.h>
#include <sys/select.h>

/*
 * The kernel's fd_set is an array of kernel longs with a bit per
 * descriptor. Translate picolibc's fd_set to and from that rather than
 * depending on the two layouts matching.
 */
typedef unsigned long __kernel_fd_mask;

#define __KERNEL_NFDBITS ((int)sizeof(__kernel_fd_mask) * 8)

struct __kernel_fd_set {
    __kernel_fd_mask fds_bits[(FD_SETSIZE + __KERNEL_NFDBITS - 1) / __KERNEL_NFDBITS];
};

static inline struct __kernel_fd_set *
_fdset_to_linux(struct __kernel_fd_set *kset, const fd_set *set, int nfds)
{
    if (!set)
        return NULL;
    memset(kset, 0, sizeof(*kset));
    for (int fd = 0; fd < nfds; fd++)
        if (FD_ISSET(fd, set))
            kset->fds_bits[fd / __KERNEL_NFDBITS] |= (__kernel_fd_mask)1 << (fd % __KERNEL_NFDBITS);
    return kset;
}

static inline void
_fdset_from_linux(fd_set *set, const struct __kernel_fd_set *kset, int nfds)
{
    if (!set)
        return;
    for (int fd = 0; fd < nfds; fd++) {
        if (kset->fds_bits[fd / __KERNEL_NFDBITS]
            & ((__kernel_fd_mask)1 << (fd % __KERNEL_NFDBITS)))
            FD_SET(fd, set);
        else
            FD_CLR(fd, set);
    }
}

#endif /* _LOCAL_SELECT_H_ */
