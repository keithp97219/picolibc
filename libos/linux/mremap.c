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

/*
 * mremap: Linux extension (glibc and musl have it), declared with
 * _GNU_SOURCE. Grow, shrink or move a mapping; picolibc's MREMAP_*
 * flags are translated to the kernel's.
 */

#define _GNU_SOURCE
#include "local-linux.h"
#include <stdarg.h>
#include <sys/mman.h>

void *
mremap(void *old_address, size_t old_size, size_t new_size, int flags, ...)
{
    void *new_address = NULL;
    int   linux_flags = 0;

    if (flags & ~(MREMAP_MAYMOVE | MREMAP_FIXED | MREMAP_DONTUNMAP)) {
        errno = EINVAL;
        return MAP_FAILED;
    }
    if (flags & MREMAP_MAYMOVE)
        linux_flags |= LINUX_MREMAP_MAYMOVE;
    if (flags & MREMAP_DONTUNMAP)
        linux_flags |= LINUX_MREMAP_DONTUNMAP;
    if (flags & MREMAP_FIXED) {
        va_list ap;

        va_start(ap, flags);
        new_address = va_arg(ap, void *);
        va_end(ap);
        linux_flags |= LINUX_MREMAP_FIXED;
    }
    return (void *)_syscall(LINUX_SYS_mremap, old_address, old_size, new_size, linux_flags,
                            new_address);
}
