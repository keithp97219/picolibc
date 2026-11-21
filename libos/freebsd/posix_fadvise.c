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
#include <freebsd/freebsd-fadvise.h>

/*
 * Like posix_fallocate, the kernel returns the error number as the
 * result; a failed call (carry set) is reported the same way
 */
static int
_posix_result(long ret)
{
    if (ret > 0)
        ret = -ret;
    return ret < 0 ? _syscall_errno(ret) : 0;
}

int
posix_fadvise(int fd, off_t offset, off_t len, int advice)
{
    int kadvice;

    switch (advice) {
    case POSIX_FADV_NORMAL:
        kadvice = FREEBSD_POSIX_FADV_NORMAL;
        break;
    case POSIX_FADV_RANDOM:
        kadvice = FREEBSD_POSIX_FADV_RANDOM;
        break;
    case POSIX_FADV_SEQUENTIAL:
        kadvice = FREEBSD_POSIX_FADV_SEQUENTIAL;
        break;
    case POSIX_FADV_WILLNEED:
        kadvice = FREEBSD_POSIX_FADV_WILLNEED;
        break;
    case POSIX_FADV_DONTNEED:
        kadvice = FREEBSD_POSIX_FADV_DONTNEED;
        break;
    case POSIX_FADV_NOREUSE:
        kadvice = FREEBSD_POSIX_FADV_NOREUSE;
        break;
    default:
        return EINVAL;
    }
    return _posix_result(_syscall_raw(FREEBSD_SYS_posix_fadvise, fd, offset, len, kadvice));
}
