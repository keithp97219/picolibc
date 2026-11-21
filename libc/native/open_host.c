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

#define _GNU_SOURCE
#include <fcntl.h>
#include "if_native.h"

int
__open_host(const char *path, int if_flags, int mode)
{
    int flags = if_flags & IF_O_ACCMODE;

    if (if_flags & IF_O_CREAT)
        flags |= O_CREAT;
    if (if_flags & IF_O_EXCL)
        flags |= O_EXCL;
    if (if_flags & IF_O_NOCTTY)
        flags |= O_NOCTTY;
    if (if_flags & IF_O_TRUNC)
        flags |= O_TRUNC;
    if (if_flags & IF_O_APPEND)
        flags |= O_APPEND;
    if (if_flags & IF_O_NONBLOCK)
        flags |= O_NONBLOCK;
    if (if_flags & IF_O_SYNC)
        flags |= O_SYNC;
    if (if_flags & IF_O_DIRECTORY)
        flags |= O_DIRECTORY;
    if (if_flags & IF_O_NOFOLLOW)
        flags |= O_NOFOLLOW;
    if (if_flags & IF_O_CLOEXEC)
        flags |= O_CLOEXEC;
    if (if_flags & IF_O_DIRECT)
        flags |= O_DIRECT;
    return open(path, flags, mode);
}
