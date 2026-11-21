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
#include <stddef.h>
#include <string.h>

/*
 * Convert the kernel's struct dirent records in kbuf to posix_dent
 * records in buf, stopping before the first that doesn't fit in nbyte
 * and dropping the holes left by removed entries. picolibc's
 * posix_dent header is smaller than the kernel's and needs no more
 * alignment, so when kbuf is buf each converted record fits where its
 * source was. Returns the number of bytes used, and the number of
 * kernel bytes consumed in *used.
 */
static ssize_t
__posix_getdents_convert(char *kbuf, size_t kcount, char *buf, size_t nbyte, size_t *used)
{
    char  *in = kbuf, *out = buf;
    char  *end = in + kcount;
    size_t avail = nbyte;

    _Static_assert(offsetof(struct posix_dent, d_name) <= offsetof(struct __freebsd_dirent, d_name),
                   "posix_dent header");
    while (in < end) {
        struct __freebsd_dirent *k = (struct __freebsd_dirent *)in;
        struct posix_dent       *d = (struct posix_dent *)out;
        __uint64_t               ino = k->d_fileno;
        unsigned                 kreclen = k->d_reclen;
        unsigned char            type = _dt_from_freebsd(k->d_type);
        size_t                   namelen = k->d_namlen;
        size_t                   reclen;

        if (_dirent_is_hole(k)) {
            in += kreclen;
            continue;
        }
        reclen = offsetof(struct posix_dent, d_name) + namelen + 1;
        reclen = (reclen + _Alignof(struct posix_dent) - 1) & ~(_Alignof(struct posix_dent) - 1);
        if (reclen > avail)
            break;
        /* The name moves down within the same record, so read the header first */
        memmove(d->d_name, k->d_name, namelen);
        d->d_name[namelen] = '\0';
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
    /* Room for a few of the largest records */
    union {
        char                    bytes[4 * sizeof(struct __freebsd_dirent)];
        struct __freebsd_dirent align;
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

    for (;;) {
        if (nbyte >= sizeof(kbuf)
            && ((__uintptr_t)buf & (_Alignof(struct __freebsd_dirent) - 1)) == 0) {
            /* Big enough for any kernel record; convert in place */
            ret = _syscall(FREEBSD_SYS_getdirentries, fd, buf, nbyte, NULL);
            if (ret <= 0)
                return (ssize_t)ret;
            out = __posix_getdents_convert(buf, (size_t)ret, buf, nbyte, &used);
            /* Everything fits in place; a block of holes means read on */
            if (out > 0)
                return out;
            continue;
        }

        /*
         * A posix_dent can be smaller than the kernel's record for the
         * same name, so a buffer that holds the converted entry might not
         * hold the kernel's. Read into a buffer large enough for any
         * record, then move the offset back to the first entry that
         * didn't fit.
         */
        start = lseek(fd, 0, SEEK_CUR);
        if (start < 0)
            return -1;
        ret = _syscall(FREEBSD_SYS_getdirentries, fd, kbuf.bytes, sizeof(kbuf), NULL);
        if (ret <= 0)
            return (ssize_t)ret;
        out = __posix_getdents_convert(kbuf.bytes, (size_t)ret, buf, nbyte, &used);
        if (used < (size_t)ret) {
            off_t next = start;

            if (used > 0) {
                /* d_off of the last record consumed is where the next one starts */
                size_t off = 0;
                for (;;) {
                    struct __freebsd_dirent *k = (struct __freebsd_dirent *)(kbuf.bytes + off);
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
        if (out > 0)
            return out;
    }
}
