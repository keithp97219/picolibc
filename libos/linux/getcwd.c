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

/*
 * Linux's getcwd system call builds the name in a page of its own and
 * gives up with ENAMETOOLONG on names longer than that. Then, as glibc
 * does, walk up through ".." (see local-getcwd.h).
 */

#define _GETCWD_DIR_FLAGS (OS_O_DIRECTORY | OS_O_CLOEXEC | LINUX_O_LARGEFILE)

/*
 * A directory outside the process root can't be reached from it; the
 * system call gives "(unreachable)" names for these, which getcwd
 * reports as ENOENT
 */
#define _GETCWD_UNREACHABLE _OS_ERR(LINUX_ENOENT)

#include "../common/local-getcwd.h"

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

char *
getcwd(char *buf, size_t size)
{
    long ret;

    if (!buf)
        return _getcwd_alloc(size);

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
