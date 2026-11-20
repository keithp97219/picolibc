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

int
_open_flags_to_linux(int flags)
{
    int kflags;

    /* O_SEARCH is part of O_ACCMODE; it is mapped below */
    kflags = flags & (O_RDONLY | O_WRONLY | O_RDWR);
#ifdef O_APPEND
    if (flags & O_APPEND)
        kflags |= LINUX_O_APPEND;
#endif
#ifdef O_ASYNC
    if (flags & O_ASYNC)
        kflags |= LINUX_O_ASYNC;
#endif
#ifdef O_CLOEXEC
    if (flags & O_CLOEXEC)
        kflags |= LINUX_O_CLOEXEC;
#endif
#ifdef O_CREAT
    if (flags & O_CREAT)
        kflags |= LINUX_O_CREAT;
#endif
#ifdef O_DIRECT
    if (flags & O_DIRECT)
        kflags |= LINUX_O_DIRECT;
#endif
#ifdef O_DIRECTORY
    if (flags & O_DIRECTORY)
        kflags |= LINUX_O_DIRECTORY;
#endif
#ifdef O_DSYNC
    if (flags & O_DSYNC)
        kflags |= LINUX_O_DSYNC;
#endif
#ifdef O_EXCL
    if (flags & O_EXCL)
        kflags |= LINUX_O_EXCL;
#endif
#ifdef O_LARGEFILE
    if (flags & O_LARGEFILE)
        kflags |= LINUX_O_LARGEFILE;
#endif
#ifdef O_NOATIME
    if (flags & O_NOATIME)
        kflags |= LINUX_O_NOATIME;
#endif
#ifdef O_NOCTTY
    if (flags & O_NOCTTY)
        kflags |= LINUX_O_NOCTTY;
#endif
#ifdef O_NOFOLLOW
    if (flags & O_NOFOLLOW)
        kflags |= LINUX_O_NOFOLLOW;
#endif
#ifdef O_NONBLOCK
    if (flags & O_NONBLOCK)
        kflags |= LINUX_O_NONBLOCK;
#endif
#ifdef O_PATH
    if (flags & O_PATH)
        kflags |= LINUX_O_PATH;
#endif
#ifdef O_SYNC
    if (flags & O_SYNC)
        kflags |= LINUX_O_SYNC;
#endif
#ifdef O_TMPFILE
    if (flags & O_TMPFILE)
        kflags |= LINUX_O_TMPFILE;
#endif
#ifdef O_TRUNC
    if (flags & O_TRUNC)
        kflags |= LINUX_O_TRUNC;
#endif
#ifdef O_SEARCH
    /* O_EXEC and O_SEARCH share a value; O_PATH serves for both */
    if (flags & O_SEARCH)
        kflags |= LINUX_O_PATH;
#endif
    return kflags | LINUX_O_LARGEFILE;
}
