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

#ifndef _LOCAL_POLL_H_
#define _LOCAL_POLL_H_

#include "local-signal.h"
#include <poll.h>
#include <stddef.h>
#include <freebsd/freebsd-poll.h>
#include <freebsd/freebsd-pollfd-struct.h>
#include <freebsd/freebsd-timespec-struct.h>

_Static_assert(sizeof(struct pollfd) == sizeof(struct __freebsd_pollfd), "struct pollfd size");
_Static_assert(offsetof(struct pollfd, fd) == offsetof(struct __freebsd_pollfd, fd),
               "struct pollfd fd");
_Static_assert(offsetof(struct pollfd, events) == offsetof(struct __freebsd_pollfd, events),
               "struct pollfd events");
_Static_assert(offsetof(struct pollfd, revents) == offsetof(struct __freebsd_pollfd, revents),
               "struct pollfd revents");

/* poll and ppoll: FreeBSD's ppoll with a converted timeout and mask */
int _ppoll(struct pollfd *fds, nfds_t nfds, const struct __freebsd_timespec *kts,
           const struct __freebsd_sigset *kmask);

#endif /* _LOCAL_POLL_H_ */
