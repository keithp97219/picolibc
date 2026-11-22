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
#include <string.h>

/*
 * Descriptors opened O_SEARCH or O_PATH (both Linux O_PATH) refuse fchmod and fsync
 * with EBADF. Name the open file through /proc/self/fd instead, as
 * musl does. Returns buf holding the name, or NULL when fd isn't an
 * O_PATH descriptor. buf holds at least _OPATH_NAME_MAX bytes.
 */
char *
_opath_name(int fd, char *buf)
{
    static const char prefix[] = "/proc/self/fd/";
    long              flags;
    char              digits[12];
    char             *p = buf;
    int               n = 0;
    unsigned          u;

    if (fd < 0)
        return NULL;
    flags = _syscall_raw(LINUX_SYS_fcntl, fd, LINUX_F_GETFL);
    if (flags < 0 || !(flags & LINUX_O_PATH))
        return NULL;
    u = (unsigned)fd;
    do {
        digits[n++] = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    memcpy(p, prefix, sizeof(prefix) - 1);
    p += sizeof(prefix) - 1;
    while (n)
        *p++ = digits[--n];
    *p = '\0';
    return buf;
}
