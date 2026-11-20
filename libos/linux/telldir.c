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

#include "local-dirent.h"
#include <limits.h>
#include <stdlib.h>

/*
 * The position of the next entry readdir will return. Where long is
 * narrower than the kernel's offsets, positions that don't fit are kept
 * in a table and reported as negative values, which no kernel position
 * (always non-negative) can be.
 */
long
telldir(DIR *dir)
{
    __int64_t pos;

    if (dir->__dd_offset == dir->__dd_count) {
        pos = lseek(dir->__dd_fd, 0, SEEK_CUR);
    } else {
        /* The kernel's offset of the entry readdir returned last */
        struct __kernel_dirent64 *k;
        size_t                    off = 0;

        do {
            k = (struct __kernel_dirent64 *)(dir->__dd_buf + off);
            off += k->d_reclen;
        } while (off < dir->__dd_offset);
        pos = k->d_off;
    }
    if (pos >= 0 && pos == (long)pos)
        return (long)pos;
    if (pos < 0)
        return -1;
#if __SIZEOF_LONG__ < 8
    {
        size_t     i;
        __int64_t *tell;

        for (i = 0; i < dir->__dd_ntell; i++)
            if (dir->__dd_tell[i] == pos)
                return -2 - (long)i;
        if (dir->__dd_ntell >= (size_t)LONG_MAX - 1) {
            errno = EOVERFLOW;
            return -1;
        }
        tell = realloc(dir->__dd_tell, (dir->__dd_ntell + 1) * sizeof(*tell));
        if (!tell)
            return -1;
        tell[dir->__dd_ntell] = pos;
        dir->__dd_tell = tell;
        return -2 - (long)dir->__dd_ntell++;
    }
#else
    errno = EOVERFLOW;
    return -1;
#endif
}
