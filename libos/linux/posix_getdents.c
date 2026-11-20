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
#include <dirent.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Let the kernel fill buf with its linux_dirent64 records, then convert
 * them in place. picolibc's posix_dent header is smaller than the
 * kernel's and needs no more alignment, so each converted record fits
 * where its source was and never reaches the next one.
 */
ssize_t
posix_getdents(int fd, void *buf, size_t nbyte, int flags)
{
    char *in, *out, *end;
    long  ret;

    /* POSIX.1-2024 defines no flags */
    if (flags != 0) {
        errno = EINVAL;
        return -1;
    }
    ret = _syscall(LINUX_SYS_getdents64, fd, buf, nbyte);
    if (ret <= 0)
        return (ssize_t)ret;

    in = out = buf;
    end = in + ret;
    while (in < end) {
        struct __kernel_dirent64 *k = (struct __kernel_dirent64 *)in;
        struct posix_dent        *d = (struct posix_dent *)out;
        __uint64_t                ino = k->d_ino;
        unsigned                  kreclen = k->d_reclen;
        unsigned char             ktype = k->d_type;
        size_t                    namelen = strlen((char *)k->d_name);
        size_t                    reclen;
        unsigned char             type = DT_UNKNOWN;

        if ((ino_t)ino != ino) {
            errno = EOVERFLOW;
            return -1;
        }
        reclen = offsetof(struct posix_dent, d_name) + namelen + 1;
        reclen = (reclen + _Alignof(struct posix_dent) - 1) & ~(_Alignof(struct posix_dent) - 1);
        MAP_DT(type, ktype);

        /* The name moves down within the same record, so read the header first */
        memmove(d->d_name, k->d_name, namelen + 1);
        d->d_ino = (ino_t)ino;
        d->d_reclen = reclen;
        d->d_type = type;
        out += reclen;
        in += kreclen;
    }
    return (ssize_t)(out - (char *)buf);
}
