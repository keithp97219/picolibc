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
#include <sys/mman.h>

void *
mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    int  kprot = 0;
    int  kflags = 0;
    long ret;

    if (prot & PROT_READ)
        kprot |= FREEBSD_PROT_READ;
    if (prot & PROT_WRITE)
        kprot |= FREEBSD_PROT_WRITE;
    if (prot & PROT_EXEC)
        kprot |= FREEBSD_PROT_EXEC;
    if (flags & MAP_SHARED)
        kflags |= FREEBSD_MAP_SHARED;
    if (flags & MAP_PRIVATE)
        kflags |= FREEBSD_MAP_PRIVATE;
    if (flags & MAP_FIXED)
        kflags |= FREEBSD_MAP_FIXED;
    if (flags & MAP_ANONYMOUS)
        kflags |= FREEBSD_MAP_ANON;
    ret = _syscall(FREEBSD_SYS_mmap, addr, length, kprot, kflags, fd, offset);
    if (ret == -1)
        return MAP_FAILED;
    return (void *)ret;
}
