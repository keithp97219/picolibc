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
#include "local-dirent.h"
#include "../common/local-getcwd.h"
#include <freebsd/freebsd-errno.h>
#include <freebsd/freebsd-timespec-struct.h>
#include <freebsd/freebsd-stat-struct.h>
#include <stdint.h>

/*
 * FreeBSD's __getcwd builds the name from the kernel's name cache,
 * into a buffer of at most MAXPATHLEN (1024) bytes, and gives up on
 * longer names and on directories whose names it can't find. Then, as
 * FreeBSD's libc does, walk up through "..", finding each directory's
 * name by reading its parent for the entry with the same device and
 * inode. The walk opens each parent relative to the child's descriptor,
 * so it works however long the name is.
 */

#define _GETCWD_DIR_FLAGS (FREEBSD_O_RDONLY | FREEBSD_O_DIRECTORY | FREEBSD_O_CLOEXEC)

static long
_getcwd_stat(int fd, const char *name, struct __freebsd_stat *st)
{
    return _syscall_raw(FREEBSD_SYS_fstatat, (long)fd, name, st, (long)FREEBSD_AT_SYMLINK_NOFOLLOW);
}

/*
 * Find the entry in directory dfd, on device pdev, naming the
 * directory dev/ino and copy its name to name. Entries on the same
 * device match by their inode number; at a mount point the entry's
 * inode is the covered directory's, not the mounted root's, so (and
 * for file systems whose entries don't report the inode stat does)
 * look at each entry with fstatat when no inode number matched.
 * Returns the length of the name, or a negative kernel errno.
 */
static long
_getcwd_lookup(int dfd, __uint64_t pdev, __uint64_t dev, __uint64_t ino, char *name)
{
    union {
        char                    bytes[4096];
        struct __freebsd_dirent align;
    } kbuf;
    struct __freebsd_stat st;
    long                  saved = 0;
    long                  ret;
    int                   pass;

    for (pass = pdev == dev ? 0 : 1; pass < 2; pass++) {
        ret = _syscall_raw(FREEBSD_SYS_lseek, (long)dfd, 0L, (long)SEEK_SET);
        if (ret < 0)
            return ret;
        for (;;) {
            char *in, *end;

            ret = _syscall_raw(FREEBSD_SYS_getdirentries, (long)dfd, kbuf.bytes, sizeof(kbuf.bytes),
                               NULL);
            if (ret < 0)
                return ret;
            if (ret == 0)
                break;
            for (in = kbuf.bytes, end = in + ret; in < end;
                 in += ((struct __freebsd_dirent *)in)->d_reclen) {
                struct __freebsd_dirent *k = (struct __freebsd_dirent *)in;
                size_t                   len = k->d_namlen;

                if (_dirent_is_hole(k))
                    continue;
                if (k->d_name[0] == '.' && (len == 1 || (len == 2 && k->d_name[1] == '.')))
                    continue;
                if (pass == 0) {
                    if (k->d_fileno != ino)
                        continue;
                } else {
                    if (k->d_type != FREEBSD_DT_DIR && k->d_type != FREEBSD_DT_UNKNOWN)
                        continue;
                    memcpy(name, k->d_name, len);
                    name[len] = '\0';
                    ret = _getcwd_stat(dfd, name, &st);
                    if (ret < 0) {
                        /* Keep going, but report this if nothing matches */
                        saved = ret;
                        continue;
                    }
                    if (st.st_dev != dev || st.st_ino != ino)
                        continue;
                }
                memcpy(name, k->d_name, len);
                name[len] = '\0';
                return (long)len;
            }
        }
    }
    return saved ? saved : _FREEBSD_ERR(FREEBSD_ENOENT);
}

/* Walk up from the working directory, building the name backwards */
static char *
_getcwd_walk(char *buf, size_t size)
{
    struct __freebsd_stat root, st;
    char                  name[sizeof(((struct __freebsd_dirent *)0)->d_name) + 1];
    char                 *pos = buf + size;
    int                   fd = FREEBSD_AT_FDCWD;
    long                  ret;

    ret = _getcwd_stat(FREEBSD_AT_FDCWD, "/", &root);
    if (ret >= 0)
        ret = _getcwd_stat(FREEBSD_AT_FDCWD, ".", &st);
    if (ret < 0)
        goto fail;
    *--pos = '\0';
    while (st.st_dev != root.st_dev || st.st_ino != root.st_ino) {
        __uint64_t dev = st.st_dev, ino = st.st_ino;
        long       pfd;

        pfd = _syscall_raw(FREEBSD_SYS_openat, (long)fd, "..", (long)_GETCWD_DIR_FLAGS, 0L);
        if (pfd < 0) {
            ret = pfd;
            goto fail;
        }
        if (fd != FREEBSD_AT_FDCWD)
            (void)_syscall_raw(FREEBSD_SYS_close, (long)fd);
        fd = (int)pfd;
        ret = _syscall_raw(FREEBSD_SYS_fstat, (long)fd, &st);
        if (ret < 0)
            goto fail;
        /* ".." is the directory itself: the top of some other root */
        if (st.st_dev == dev && st.st_ino == ino)
            break;
        ret = _getcwd_lookup(fd, st.st_dev, dev, ino, name);
        if (ret < 0)
            goto fail;
        if ((size_t)(pos - buf) < (size_t)ret + 1) {
            ret = _FREEBSD_ERR(FREEBSD_ERANGE);
            goto fail;
        }
        pos -= ret;
        memcpy(pos, name, (size_t)ret);
        *--pos = '/';
    }
    if (fd != FREEBSD_AT_FDCWD)
        (void)_syscall_raw(FREEBSD_SYS_close, (long)fd);
    if (!*pos)
        *--pos = '/';
    memmove(buf, pos, (size_t)(buf + size - pos));
    return buf;

fail:
    if (fd != FREEBSD_AT_FDCWD)
        (void)_syscall_raw(FREEBSD_SYS_close, (long)fd);
    errno = _syscall_errno(ret);
    return NULL;
}

char *
getcwd(char *buf, size_t size)
{
    long ret;

    if (!buf) {
        char *shrunk;

        if (size)
            return _getcwd_alloc(size);

        /* As big as the name needs, which can be more than PATH_MAX */
        for (size = PATH_MAX;; size *= 2) {
            buf = malloc(size);
            if (!buf)
                return NULL;
            if (getcwd(buf, size))
                break;
            ret = errno;
            free(buf);
            if (ret != ERANGE || size > SIZE_MAX / 2) {
                errno = (int)ret;
                return NULL;
            }
        }
        shrunk = realloc(buf, strlen(buf) + 1);
        return shrunk ? shrunk : buf;
    }

    /*
     * FreeBSD gives EINVAL for any buffer shorter than two bytes, but
     * the shortest name, "/", fits in two, so one byte is just too small
     */
    if (size < 2) {
        errno = size == 0 ? EINVAL : ERANGE;
        return NULL;
    }
    ret = _syscall_raw(FREEBSD_SYS___getcwd, buf, size);
    if (ret >= 0)
        return buf;

    /*
     * The kernel's ENOMEM means the name didn't fit, in either the
     * buffer or the kernel's own; ENOENT that the name cache didn't
     * have a name. In both, the walk finds the name if there is one
     * and reports ERANGE when it doesn't fit.
     */
    return _getcwd_walk(buf, size);
}
