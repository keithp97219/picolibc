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
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Convert the kernel's linux_dirent64 records in kbuf to posix_dent
 * records in buf, stopping before the first that doesn't fit in nbyte.
 * picolibc's posix_dent header is smaller than the kernel's and needs
 * no more alignment, so when kbuf is buf each converted record fits
 * where its source was and never reaches the next one. Returns the
 * number of bytes used, or -1 with errno set, and the number of kernel
 * bytes consumed in *used.
 */
static ssize_t
__posix_getdents_convert(char *kbuf, size_t kcount, char *buf, size_t nbyte, size_t *used)
{
    char  *in = kbuf, *out = buf;
    char  *end = in + kcount;
    size_t avail = nbyte;

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
        if (reclen > avail)
            break;
        MAP_DT(type, ktype);

        /* The name moves down within the same record, so read the header first */
        memmove(d->d_name, k->d_name, namelen + 1);
        d->d_ino = (ino_t)ino;
        d->d_reclen = reclen;
        d->d_type = type;
        out += reclen;
        avail -= reclen;
        in += kreclen;
    }
    *used = (size_t)(in - kbuf);
    return (ssize_t)(out - buf);
}

ssize_t
posix_getdents(int fd, void *buf, size_t nbyte, int flags)
{
    /*
     * The largest record the kernel writes: it rounds each one up to
     * eight bytes, which can be more than sizeof(struct
     * __kernel_dirent64) where 64-bit values are only four-byte aligned
     */
    union {
        char bytes[(offsetof(struct __kernel_dirent64, d_name) + NAME_MAX + 1 + 7) & ~(size_t)7];
        struct __kernel_dirent64 align;
    } kbuf;
    off_t   start;
    size_t  used;
    ssize_t out;
    long    ret;

    /* POSIX.1-2024 defines no flags */
    if (flags != 0) {
        errno = EINVAL;
        return -1;
    }
    if (nbyte >= sizeof(kbuf)) {
        /* Big enough for any kernel record; convert in place */
        ret = _syscall(LINUX_SYS_getdents64, fd, buf, nbyte);
        if (ret <= 0)
            return (ssize_t)ret;
        return __posix_getdents_convert(buf, (size_t)ret, buf, nbyte, &used);
    }

    /*
     * A posix_dent can be smaller than the kernel's record for the same
     * name, so a buffer that holds the converted entry might not hold the
     * kernel's. Read into a buffer large enough for any record, then move
     * the offset back to the first entry that didn't fit.
     */
    start = lseek(fd, 0, SEEK_CUR);
    if (start < 0)
        return -1;
    ret = _syscall(LINUX_SYS_getdents64, fd, kbuf.bytes, sizeof(kbuf));
    if (ret <= 0)
        return (ssize_t)ret;
    out = __posix_getdents_convert(kbuf.bytes, (size_t)ret, buf, nbyte, &used);
    if (out < 0)
        return -1;
    if (used < (size_t)ret) {
        off_t next = start;

        if (used > 0) {
            /* d_off of the last record converted is where the next one starts */
            size_t off = 0;

            for (;;) {
                struct __kernel_dirent64 *k = (struct __kernel_dirent64 *)(kbuf.bytes + off);

                off += k->d_reclen;
                if (off >= used) {
                    next = k->d_off;
                    break;
                }
            }
        }
        if (lseek(fd, next, SEEK_SET) < 0)
            return -1;
        if (out == 0) {
            errno = EINVAL;
            return -1;
        }
    }
    return out;
}
