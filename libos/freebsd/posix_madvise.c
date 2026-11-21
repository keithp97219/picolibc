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

/*
 * posix_madvise returns the error rather than setting errno. FreeBSD's
 * MADV_DONTNEED keeps what the pages hold, as POSIX requires.
 */
int
posix_madvise(void *addr, size_t len, int advice)
{
    int  kadvice;
    long ret;

    /* FreeBSD rounds the address down to a page, and skips unmapped pages */
    if ((uintptr_t)addr & FREEBSD_PAGE_MASK)
        return EINVAL;
    if (!_range_mapped(addr, len))
        return ENOMEM;

    switch (advice) {
    case POSIX_MADV_NORMAL:
        kadvice = FREEBSD_POSIX_MADV_NORMAL;
        break;
    case POSIX_MADV_RANDOM:
        kadvice = FREEBSD_POSIX_MADV_RANDOM;
        break;
    case POSIX_MADV_SEQUENTIAL:
        kadvice = FREEBSD_POSIX_MADV_SEQUENTIAL;
        break;
    case POSIX_MADV_WILLNEED:
        kadvice = FREEBSD_POSIX_MADV_WILLNEED;
        break;
    case POSIX_MADV_DONTNEED:
        kadvice = FREEBSD_POSIX_MADV_DONTNEED;
        break;
    default:
        return EINVAL;
    }
    ret = _syscall_raw(FREEBSD_SYS_madvise, addr, len, kadvice);
    if (ret < 0)
        return _syscall_errno(ret);
    return 0;
}
