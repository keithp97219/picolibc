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

#include "local-mman.h"
#include <freebsd/freebsd-errno.h>

void *
mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    int  kflags = 0;
    long ret;

    /*
     * FreeBSD maps at the page holding an unaligned offset, and takes
     * an anonymous mapping that is neither shared nor private as
     * private; POSIX wants EINVAL for those and for no length
     */
    if (length == 0 || (offset & FREEBSD_PAGE_MASK)
        || ((flags & MAP_FIXED) && ((uintptr_t)addr & FREEBSD_PAGE_MASK))
        || ((flags & (MAP_SHARED | MAP_PRIVATE)) != MAP_SHARED
            && (flags & (MAP_SHARED | MAP_PRIVATE)) != MAP_PRIVATE)) {
        errno = EINVAL;
        return MAP_FAILED;
    }
    /*
     * POSIX wants a descriptor open for reading whatever the
     * protection; FreeBSD maps a write-only one for writing
     */
    if (!(flags & MAP_ANONYMOUS)) {
        long fl = _syscall_raw(FREEBSD_SYS_fcntl, fd, FREEBSD_F_GETFL);

        if (fl >= 0 && ((fl & FREEBSD_O_ACCMODE) == FREEBSD_O_WRONLY || (fl & FREEBSD_O_EXEC))) {
            errno = EACCES;
            return MAP_FAILED;
        }
    }
    if (flags & MAP_SHARED)
        kflags |= FREEBSD_MAP_SHARED;
    if (flags & MAP_PRIVATE)
        kflags |= FREEBSD_MAP_PRIVATE;
    if (flags & MAP_FIXED)
        kflags |= FREEBSD_MAP_FIXED;
    if (flags & MAP_ANONYMOUS)
        kflags |= FREEBSD_MAP_ANON;
    ret = _syscall_raw(FREEBSD_SYS_mmap, addr, length, _prot_to_freebsd(prot), kflags, fd, offset);
    if (ret < 0 && ret > -4096) {
        /*
         * A fixed range past the end of the address space is EINVAL to
         * FreeBSD and ENOMEM to POSIX. User space is the lower half
         */
        if (ret == _FREEBSD_ERR(FREEBSD_EINVAL) && (flags & MAP_FIXED)
            && ((uintptr_t)addr + length < (uintptr_t)addr
                || (intptr_t)((uintptr_t)addr + length - 1) < 0))
            errno = ENOMEM;
        else
            errno = _syscall_errno(ret);
        return MAP_FAILED;
    }
    return (void *)ret;
}
