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

#define _DEFAULT_SOURCE
#include <ftw.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int (*__nftw_fn)(const char *, const struct stat *, int, struct FTW *);

struct __nftw {
    __nftw_fn fn;
    int       flags;
    dev_t     dev;
    char      path[PATH_MAX];
    char      cwd[PATH_MAX]; /* the starting directory, for FTW_CHDIR */
};

/* The directories being walked, innermost first, to find links back to them */
struct __nftw_dir {
    dev_t              dev;
    ino_t              ino;
    struct __nftw_dir *up;
};

/*
 * Calls the function. For FTW_CHDIR, that happens in the file's
 * directory, returning to the starting directory afterwards so the
 * walk's own paths still work.
 */
static int
__nftw_call(struct __nftw *w, const struct stat *st, int type, struct FTW *ftw)
{
    int ret;

    if ((w->flags & FTW_CHDIR) && ftw->base > 0) {
        char c = w->path[ftw->base];

        w->path[ftw->base] = '\0';
        ret = chdir(w->path);
        w->path[ftw->base] = c;
        if (ret < 0)
            return -1;
        ret = w->fn(w->path, st, type, ftw);
        if (chdir(w->cwd) < 0)
            return -1;
        return ret;
    }
    return w->fn(w->path, st, type, ftw);
}

static int
__nftw_walk(struct __nftw *w, size_t len, int base, int level, struct __nftw_dir *up)
{
    struct stat       st;
    struct FTW        ftw = { .base = base, .level = level };
    struct __nftw_dir here, *a;
    int               type, ret, sep;
    DIR              *dir;
    struct dirent    *ent;

    if ((w->flags & FTW_PHYS) ? lstat(w->path, &st) : stat(w->path, &st)) {
        int error = errno;

        if (!(w->flags & FTW_PHYS) && error == ENOENT && lstat(w->path, &st) == 0
            && S_ISLNK(st.st_mode)) {
            /* A symbolic link to nothing */
            type = FTW_SLN;
        } else if (level > 0 && error == EACCES) {
            /* No permission to examine it */
            type = FTW_NS;
        } else {
            /* The starting path must exist and be searchable; other failures are errors */
            errno = error;
            return -1;
        }
        return __nftw_call(w, &st, type, &ftw);
    }
    if ((w->flags & FTW_MOUNT) && level > 0 && st.st_dev != w->dev)
        return 0;
    if (S_ISLNK(st.st_mode))
        return __nftw_call(w, &st, FTW_SL, &ftw);
    if (!S_ISDIR(st.st_mode))
        return __nftw_call(w, &st, FTW_F, &ftw);

    /* A directory reached again through a symbolic link isn't walked twice */
    for (a = up; a; a = a->up)
        if (a->dev == st.st_dev && a->ino == st.st_ino)
            return (w->flags & FTW_DEPTH) ? 0 : __nftw_call(w, &st, FTW_D, &ftw);

    dir = opendir(w->path);
    if (!dir) {
        if (errno == EACCES)
            return __nftw_call(w, &st, FTW_DNR, &ftw);
        return -1;
    }
    if (!(w->flags & FTW_DEPTH)) {
        ret = __nftw_call(w, &st, FTW_D, &ftw);
        if (ret) {
            closedir(dir);
            return ret;
        }
    }
    here.dev = st.st_dev;
    here.ino = st.st_ino;
    here.up = up;
    ret = 0;
    sep = len && w->path[len - 1] != '/';
    while (!ret) {
        size_t start = len + (size_t)sep, nlen;

        errno = 0;
        ent = readdir(dir);
        if (!ent) {
            if (errno)
                ret = -1;
            break;
        }
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        nlen = strlen(ent->d_name);
        if (start + nlen >= sizeof(w->path)) {
            errno = ENAMETOOLONG;
            ret = -1;
            break;
        }
        if (sep)
            w->path[len] = '/';
        memcpy(w->path + start, ent->d_name, nlen + 1);
        ret = __nftw_walk(w, start + nlen, (int)start, level + 1, &here);
        w->path[len] = '\0';
    }
    closedir(dir);
    if (!ret && (w->flags & FTW_DEPTH))
        ret = __nftw_call(w, &st, FTW_DP, &ftw);
    return ret;
}

int
nftw(const char *path, __nftw_fn fn, int fd_limit, int flags)
{
    struct __nftw *w;
    struct stat    st;
    size_t         len = strlen(path);
    const char    *slash;
    int            ret;

    /* Directories are read one at a time, so fd_limit is never reached */
    (void)fd_limit;
    if (len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }
    w = malloc(sizeof(*w));
    if (!w)
        return -1;
    w->fn = fn;
    w->flags = flags;
    memcpy(w->path, path, len + 1);
    w->dev = (flags & FTW_PHYS ? lstat(path, &st) : stat(path, &st)) == 0 ? st.st_dev : 0;
    if ((flags & FTW_CHDIR) && !getcwd(w->cwd, sizeof(w->cwd))) {
        free(w);
        return -1;
    }
    slash = strrchr(path, '/');
    ret = __nftw_walk(w, len, slash && slash[1] ? (int)(slash - path + 1) : 0, 0, NULL);
    free(w);
    return ret;
}
