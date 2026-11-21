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

#include "local-freebsd.h"

int
_open_flags_from_freebsd(int kflags)
{
    int flags;

    flags = kflags & (FREEBSD_O_RDONLY | FREEBSD_O_WRONLY | FREEBSD_O_RDWR);
#ifdef O_APPEND
    if (kflags & FREEBSD_O_APPEND)
        flags |= O_APPEND;
#endif
#ifdef O_ASYNC
    if (kflags & FREEBSD_O_ASYNC)
        flags |= O_ASYNC;
#endif
#ifdef O_DIRECT
    if (kflags & FREEBSD_O_DIRECT)
        flags |= O_DIRECT;
#endif
#ifdef O_DSYNC
    if (kflags & FREEBSD_O_DSYNC)
        flags |= O_DSYNC;
#endif
#ifdef O_NONBLOCK
    if (kflags & FREEBSD_O_NONBLOCK)
        flags |= O_NONBLOCK;
#endif
#ifdef O_SYNC
    if (kflags & FREEBSD_O_SYNC)
        flags |= O_SYNC;
#endif
#ifdef O_SEARCH
    if (kflags & FREEBSD_O_EXEC)
        flags = (flags & ~O_ACCMODE) | O_SEARCH;
#endif
    return flags;
}
