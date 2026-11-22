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

#ifndef _LOCAL_GETCWD_H_
#define _LOCAL_GETCWD_H_

/*
 * The parts of getcwd the OS layers share: walking up through ".."
 * when the kernel can't give the name, and allocating the buffer for
 * getcwd(NULL, size). Each OS layer's getcwd.c includes this and
 * decides when to walk; it supplies, before the include,
 *
 *   _GETCWD_DIR_FLAGS    openat flags for each parent (optional,
 *                        OS_O_DIRECTORY | OS_O_CLOEXEC by default)
 *   _GETCWD_UNREACHABLE  the error when ".." is the directory itself
 *                        before reaching the process root, that is
 *                        when the working directory is outside it
 *                        (optional; without it, getcwd returns the
 *                        name from that other root, as the kernel
 *                        does on FreeBSD)
 *
 * and after it the definition of _getcwd_stat. local-os.h and
 * local-dirent.h supply the system calls and the kernel's dirent
 * layout, so the walk reads directories itself, raw, without libc's
 * dirent functions.
 */

#include "local-os.h"
#include "local-dirent.h"
#include _OS_ERRNO_H
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef _GETCWD_DIR_FLAGS
#define _GETCWD_DIR_FLAGS (OS_O_DIRECTORY | OS_O_CLOEXEC)
#endif

#define _GETCWD_FDCWD ((int)OS_AT_FDCWD)

struct _getcwd_id {
    __uint64_t dev;
    __uint64_t ino;
};

/*
 * The device and inode of name in fd, not following a symbolic link,
 * or of fd itself for "". Returns 0 or the _syscall_raw error.
 */
static long _getcwd_stat(int fd, const char *name, struct _getcwd_id *id);

static inline int
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
 * entry with _getcwd_stat when no inode number matched. Returns the
 * length of the name, or a _syscall_raw error.
 */
static long
_getcwd_lookup(int dfd, __uint64_t pdev, const struct _getcwd_id *id, char *name)
{
    union {
        char               bytes[4096];
        struct __os_dirent align;
    } kbuf;
    struct _getcwd_id st;
    long              saved = 0;
    long              ret;
    int               pass;

    for (pass = pdev == id->dev ? 0 : 1; pass < 2; pass++) {
        ret = _syscall_raw(OS_SYS_lseek, (long)dfd, 0L, (long)SEEK_SET);
        if (ret < 0)
            return ret;
        for (;;) {
            char *in, *end;

            ret = _getdents_raw(dfd, kbuf.bytes, sizeof(kbuf.bytes));
            if (ret < 0)
                return ret;
            if (ret == 0)
                break;
            for (in = kbuf.bytes, end = in + ret; in < end;
                 in += ((struct __os_dirent *)in)->d_reclen) {
                struct __os_dirent *k = (struct __os_dirent *)in;
                size_t              len = _dirent_namlen(k);

                if (_dirent_is_hole(k))
                    continue;
                if (k->d_name[0] == '.' && (len == 1 || (len == 2 && k->d_name[1] == '.')))
                    continue;
                if (pass == 0) {
                    if (_dirent_ino(k) != id->ino)
                        continue;
                } else if (k->d_type != OS_DT_DIR && k->d_type != OS_DT_UNKNOWN) {
                    continue;
                }
                memcpy(name, k->d_name, len);
                name[len] = '\0';
                if (pass != 0) {
                    ret = _getcwd_stat(dfd, name, &st);
                    if (ret < 0) {
                        /* Keep going, but report this if nothing matches */
                        saved = ret;
                        continue;
                    }
                    if (!_getcwd_same(&st, id))
                        continue;
                }
                return (long)len;
            }
        }
    }
    return saved ? saved : _OS_ERR(OS_ENOENT);
}

/*
 * Walk up from the working directory, building the name backwards.
 * Each parent is opened relative to the child's descriptor, so this
 * works however long the name is. Reports ERANGE when the name
 * doesn't fit in size bytes.
 */
static char *
_getcwd_walk(char *buf, size_t size)
{
    struct _getcwd_id root, st;
    char              name[sizeof(((struct __os_dirent *)0)->d_name) + 1];
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

        pfd = _syscall_raw(OS_SYS_openat, (long)fd, "..", (long)(_GETCWD_DIR_FLAGS), 0L);
        if (pfd < 0) {
            ret = pfd;
            goto fail;
        }
        if (fd != _GETCWD_FDCWD)
            (void)_syscall_raw(OS_SYS_close, (long)fd);
        fd = (int)pfd;
        ret = _getcwd_stat(fd, "", &st);
        if (ret < 0)
            goto fail;
        /* ".." is the directory itself: the top of some other root */
        if (_getcwd_same(&st, &child)) {
#ifdef _GETCWD_UNREACHABLE
            ret = _GETCWD_UNREACHABLE;
            goto fail;
#else
            break;
#endif
        }
        ret = _getcwd_lookup(fd, st.dev, &child, name);
        if (ret < 0)
            goto fail;
        if ((size_t)(pos - buf) < (size_t)ret + 1) {
            ret = _OS_ERR(OS_ERANGE);
            goto fail;
        }
        pos -= ret;
        memcpy(pos, name, (size_t)ret);
        *--pos = '/';
    }
    if (fd != _GETCWD_FDCWD)
        (void)_syscall_raw(OS_SYS_close, (long)fd);
    if (!*pos)
        *--pos = '/';
    memmove(buf, pos, (size_t)(buf + size - pos));
    return buf;

fail:
    if (fd != _GETCWD_FDCWD)
        (void)_syscall_raw(OS_SYS_close, (long)fd);
    errno = _syscall_errno(ret);
    return NULL;
}

/*
 * getcwd(NULL, size): allocate the buffer, size bytes or, for a
 * zero size, as big as the name needs, which can be more than
 * PATH_MAX. Not POSIX, but glibc, musl and the BSDs all do it and
 * programs (gnulib, among others) count on it.
 */
static char *
_getcwd_alloc(size_t size)
{
    size_t len = size ? size : PATH_MAX;
    char  *buf, *shrunk;
    int    err;

    for (;;) {
        buf = malloc(len);
        if (!buf)
            return NULL;
        if (getcwd(buf, len))
            break;
        err = errno;
        free(buf);
        if (size || err != ERANGE || len > SIZE_MAX / 2) {
            errno = err;
            return NULL;
        }
        len *= 2;
    }
    if (!size) {
        shrunk = realloc(buf, strlen(buf) + 1);
        if (shrunk)
            buf = shrunk;
    }
    return buf;
}

#endif /* _LOCAL_GETCWD_H_ */
