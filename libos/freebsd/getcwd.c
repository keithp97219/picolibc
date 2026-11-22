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
#include <freebsd/freebsd-timespec-struct.h>
#include <freebsd/freebsd-stat-struct.h>

/*
 * FreeBSD's __getcwd builds the name from the kernel's name cache,
 * into a buffer of at most MAXPATHLEN (1024) bytes, and gives up on
 * longer names and on directories whose names it can't find. Then, as
 * FreeBSD's libc does, walk up through ".." (see local-getcwd.h). For
 * a directory outside the process root, the kernel's name lookup stops
 * at the system root instead and __getcwd gives the name from there;
 * the walk does the same, so no _GETCWD_UNREACHABLE.
 */

#include "../common/local-getcwd.h"

static long
_getcwd_stat(int fd, const char *name, struct _getcwd_id *id)
{
    struct __freebsd_stat st;
    long                  ret;

    /* fstatat.c relies on AT_EMPTY_PATH too */
    ret = _syscall_raw(FREEBSD_SYS_fstatat, (long)fd, name, &st,
                       (long)(FREEBSD_AT_SYMLINK_NOFOLLOW | FREEBSD_AT_EMPTY_PATH));
    if (ret < 0)
        return ret;
    id->dev = st.st_dev;
    id->ino = st.st_ino;
    return 0;
}

char *
getcwd(char *buf, size_t size)
{
    long ret;

    if (!buf)
        return _getcwd_alloc(size);

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
