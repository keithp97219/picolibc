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

#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <stdarg.h>
#include "if_native.h"

/* Translate picolibc's open flags for the host */
int
open(const char *path, int flags, ...)
{
    int     if_flags = flags & O_ACCMODE;
    int     mode = 0;
    va_list ap;

    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
        if_flags |= IF_O_CREAT;
    }
    if (flags & O_EXCL)
        if_flags |= IF_O_EXCL;
    if (flags & O_NOCTTY)
        if_flags |= IF_O_NOCTTY;
    if (flags & O_TRUNC)
        if_flags |= IF_O_TRUNC;
    if (flags & O_APPEND)
        if_flags |= IF_O_APPEND;
    if (flags & O_NONBLOCK)
        if_flags |= IF_O_NONBLOCK;
    if (flags & O_SYNC)
        if_flags |= IF_O_SYNC;
    if (flags & O_DIRECTORY)
        if_flags |= IF_O_DIRECTORY;
    if (flags & O_NOFOLLOW)
        if_flags |= IF_O_NOFOLLOW;
    if (flags & O_CLOEXEC)
        if_flags |= IF_O_CLOEXEC;
#ifdef O_DIRECT
    if (flags & O_DIRECT)
        if_flags |= IF_O_DIRECT;
#endif
    return __open_host(path, if_flags, mode);
}
