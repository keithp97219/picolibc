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

#include "local-linux.h"

/* Translate the file status flags F_GETFL reports */
int
_open_flags_from_linux(int kflags)
{
    int flags = kflags & O_ACCMODE;

    if (kflags & LINUX_O_APPEND)
        flags |= O_APPEND;
    if (kflags & LINUX_O_NONBLOCK)
        flags |= O_NONBLOCK;
#ifdef O_ASYNC
    if (kflags & LINUX_O_ASYNC)
        flags |= O_ASYNC;
#endif
#ifdef O_DIRECT
    if (kflags & LINUX_O_DIRECT)
        flags |= O_DIRECT;
#endif
#ifdef O_NOATIME
    if (kflags & LINUX_O_NOATIME)
        flags |= O_NOATIME;
#endif
    /* LINUX_O_SYNC includes the LINUX_O_DSYNC bit */
    if ((kflags & LINUX_O_SYNC) == LINUX_O_SYNC)
        flags |= O_SYNC;
    else if (kflags & LINUX_O_DSYNC)
        flags |= O_DSYNC;
#ifdef O_SEARCH
    if (kflags & LINUX_O_PATH)
        flags |= O_SEARCH;
#endif
    return flags;
}
