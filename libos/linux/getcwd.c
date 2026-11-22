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

#include "local-statx.h"
#include "../common/local-getcwd.h"
#include <linux/linux-dirent64-struct.h>
#include <linux/linux-dirent.h>
#include <linux/linux-errno.h>
#include <stdint.h>

/*
 * Linux's getcwd system call builds the name in a page of its own and
 * gives up with ENAMETOOLONG on names longer than that. Then, as glibc
 * does, walk up through "..", finding each directory's name by reading
 * its parent for the entry with the same device and inode. The walk
 * opens each parent relative to the child's descriptor, so it works
 * however long the name is.
 */

#define _GETCWD_DIR_FLAGS (LINUX_O_DIRECTORY | LINUX_O_CLOEXEC | LINUX_O_LARGEFILE)
#define _GETCWD_FDCWD     ((int)LINUX_AT_FDCWD)

struct _getcwd_id {
    __uint64_t dev;
    __uint64_t ino;
};

/* The device and inode of name in fd, or of fd itself for "" */
static long
_getcwd_stat(int fd, const char *name, struct _getcwd_id *id)
{
    struct __kernel_statx stx;
    long                  flags = LINUX_AT_SYMLINK_NOFOLLOW | LINUX_AT_STATX_SYNC_AS_STAT;
    long                  ret;

    if (!*name)
        flags |= LINUX_AT_EMPTY_PATH;
    ret = _syscall_raw(LINUX_SYS_statx, (long)fd, name, flags, (long)LINUX_STATX_INO, &stx);
    if (ret < 0)
        return ret;
    id->dev = ((__uint64_t)stx.stx_dev_major << 32) | stx.stx_dev_minor;
    id->ino = stx.stx_ino;
    return 0;
}

static int
_getcwd_same(const struct _getcwd_id *a, const struct _getcwd_id *b)
{
    return a->dev == b->dev && a->ino == b->ino;
}

/*
 * Find the entry in directory dfd, on device pdev, naming the
 * directory id and copy its name to name. Entries on the same device
 * match by their inode number; at a mount point the entry's inode is
 * the covered directory's, not the mounted root's, so (and for file
 * systems whose entries don't report the inode stat does) look at each
 * entry with statx when no inode number matched. Returns the length of
 * the name, or a negative kernel errno.
 */
static long
_getcwd_lookup(int dfd, __uint64_t pdev, const struct _getcwd_id *id, char *name)
{
    union {
        char                     bytes[4096];
        struct __kernel_dirent64 align;
    } kbuf;
    struct _getcwd_id st;
    long              saved = 0;
    long              ret;
    int               pass;

    for (pass = pdev == id->dev ? 0 : 1; pass < 2; pass++) {
        ret = _syscall_raw(LINUX_SYS_lseek, (long)dfd, 0L, (long)SEEK_SET);
        if (ret < 0)
            return ret;
        for (;;) {
            char *in, *end;

            ret = _syscall_raw(LINUX_SYS_getdents64, (long)dfd, kbuf.bytes, sizeof(kbuf.bytes));
            if (ret < 0)
                return ret;
            if (ret == 0)
                break;
            for (in = kbuf.bytes, end = in + ret; in < end;
                 in += ((struct __kernel_dirent64 *)in)->d_reclen) {
                struct __kernel_dirent64 *k = (struct __kernel_dirent64 *)in;
                const char               *kname = (const char *)k->d_name;
                size_t                    len = strlen(kname);

                if (kname[0] == '.' && (len == 1 || (len == 2 && kname[1] == '.')))
                    continue;
                if (pass == 0) {
                    if (k->d_ino != id->ino)
                        continue;
                } else {
                    if (k->d_type != LINUX_DT_DIR && k->d_type != LINUX_DT_UNKNOWN)
                        continue;
                    ret = _getcwd_stat(dfd, kname, &st);
                    if (ret < 0) {
                        /* Keep going, but report this if nothing matches */
                        saved = ret;
                        continue;
                    }
                    if (!_getcwd_same(&st, id))
                        continue;
                }
                memcpy(name, kname, len + 1);
                return (long)len;
            }
        }
    }
    return saved ? saved : -LINUX_ENOENT;
}

/* Walk up from the working directory, building the name backwards */
static char *
_getcwd_walk(char *buf, size_t size)
{
    struct _getcwd_id root, st;
    char              name[NAME_MAX + 1];
    char             *pos = buf + size;
    int               fd = _GETCWD_FDCWD;
    long              ret;

    ret = _getcwd_stat(_GETCWD_FDCWD, "/", &root);
    if (ret >= 0)
        ret = _getcwd_stat(_GETCWD_FDCWD, ".", &st);
    if (ret < 0)
        goto fail;
    *--pos = '\0';
    while (!_getcwd_same(&st, &root)) {
        struct _getcwd_id child = st;
        long              pfd;

        pfd = _syscall_raw(LINUX_SYS_openat, (long)fd, "..", (long)_GETCWD_DIR_FLAGS, 0L);
        if (pfd < 0) {
            ret = pfd;
            goto fail;
        }
        if (fd != _GETCWD_FDCWD)
            (void)_syscall_raw(LINUX_SYS_close, (long)fd);
        fd = (int)pfd;
        ret = _getcwd_stat(fd, "", &st);
        if (ret < 0)
            goto fail;
        /*
         * ".." is the directory itself: the top of some other root, so
         * the directory can't be reached from this one; the system call
         * gives "(unreachable)" names for these, which getcwd reports
         * as ENOENT
         */
        if (_getcwd_same(&st, &child)) {
            ret = -LINUX_ENOENT;
            goto fail;
        }
        ret = _getcwd_lookup(fd, st.dev, &child, name);
        if (ret < 0)
            goto fail;
        if ((size_t)(pos - buf) < (size_t)ret + 1) {
            ret = -LINUX_ERANGE;
            goto fail;
        }
        pos -= ret;
        memcpy(pos, name, (size_t)ret);
        *--pos = '/';
    }
    if (fd != _GETCWD_FDCWD)
        (void)_syscall_raw(LINUX_SYS_close, (long)fd);
    if (!*pos)
        *--pos = '/';
    memmove(buf, pos, (size_t)(buf + size - pos));
    return buf;

fail:
    if (fd != _GETCWD_FDCWD)
        (void)_syscall_raw(LINUX_SYS_close, (long)fd);
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

    /* Linux gives ERANGE for a zero size */
    if (size == 0) {
        errno = EINVAL;
        return NULL;
    }
    ret = _syscall_raw(LINUX_SYS_getcwd, buf, size);
    if (ret >= 0) {
        /*
         * A directory outside the root (after chroot or in another
         * mount namespace) comes back as "(unreachable)/...", which
         * isn't an absolute pathname. glibc reports ENOENT for these
         * rather than walking up, which would find some other root
         */
        if (buf[0] != '/') {
            errno = ENOENT;
            return NULL;
        }
        return buf;
    }

    /*
     * ENAMETOOLONG means the name didn't fit in the kernel's page; as
     * glibc does, walk up to find it, reporting ERANGE when it doesn't
     * fit in the buffer. Other errors (ERANGE for a short buffer,
     * ENOENT for a removed directory) are the answer.
     */
    if (ret != -LINUX_ENAMETOOLONG) {
        errno = _syscall_errno(ret);
        return NULL;
    }
    return _getcwd_walk(buf, size);
}
