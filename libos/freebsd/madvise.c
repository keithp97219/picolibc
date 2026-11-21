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

int
madvise(void *addr, size_t len, int advice)
{
    int kadvice;

    switch (advice) {
    case MADV_NORMAL:
        kadvice = FREEBSD_MADV_NORMAL;
        break;
    case MADV_RANDOM:
        kadvice = FREEBSD_MADV_RANDOM;
        break;
    case MADV_SEQUENTIAL:
        kadvice = FREEBSD_MADV_SEQUENTIAL;
        break;
    case MADV_WILLNEED:
        kadvice = FREEBSD_MADV_WILLNEED;
        break;
    case MADV_DONTNEED:
        kadvice = FREEBSD_MADV_DONTNEED;
        break;
    case MADV_FREE:
        kadvice = FREEBSD_MADV_FREE;
        break;
    case MADV_DONTDUMP:
        kadvice = FREEBSD_MADV_NOCORE;
        break;
    case MADV_DODUMP:
        kadvice = FREEBSD_MADV_CORE;
        break;
    default:
        /* The other Linux advice has no FreeBSD equivalent */
        errno = EINVAL;
        return -1;
    }
    return (int)_syscall(FREEBSD_SYS_madvise, addr, len, kadvice);
}
