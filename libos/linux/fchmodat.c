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

#include "local-linux.h"
#include <linux/linux-errno.h>
#include <sys/stat.h>
#include <string.h>

/* Older kernel headers lack fchmodat2, which has the same number everywhere */
#ifndef LINUX_SYS_fchmodat2
#define LINUX_SYS_fchmodat2 452
#endif

/*
 * Without fchmodat2, look at the file through an O_PATH descriptor: a
 * symbolic link can't have its mode changed, and anything else is
 * changed through /proc, as glibc does
 */
static int
_fchmodat_nofollow(int kdirfd, const char *path, mode_t mode)
{
    struct stat st;
    char        name[sizeof("/proc/self/fd/") + 10], *s;
    long        fd;
    int         n, ret;

    fd = _syscall_raw(LINUX_SYS_openat, kdirfd, path,
                      LINUX_O_PATH | LINUX_O_NOFOLLOW | LINUX_O_CLOEXEC);
    if (fd < 0) {
        errno = _syscall_errno(fd);
        return -1;
    }
    ret = fstat((int)fd, &st);
    if (ret == 0 && S_ISLNK(st.st_mode)) {
        errno = EOPNOTSUPP;
        ret = -1;
    } else if (ret == 0) {
        s = name + sizeof(name) - 1;
        *s = '\0';
        n = (int)fd;
        do
            *--s = '0' + n % 10;
        while ((n /= 10) != 0);
        s -= sizeof("/proc/self/fd/") - 1;
        memcpy(s, "/proc/self/fd/", sizeof("/proc/self/fd/") - 1);
        ret = (int)_syscall_raw(LINUX_SYS_fchmodat, LINUX_AT_FDCWD, s, mode);
        if (ret < 0) {
            /* Without /proc, the mode can't be changed this way */
            errno = ret == -LINUX_ENOENT ? EOPNOTSUPP : _syscall_errno(ret);
            ret = -1;
        }
    }
    (void)_syscall_raw(LINUX_SYS_close, fd);
    return ret;
}

int
fchmodat(int dirfd, const char *path, mode_t mode, int flags)
{
    int  kflags = _at_flags_to_linux(flags, AT_SYMLINK_NOFOLLOW);
    long ret;

    if (kflags < 0) {
        errno = EINVAL;
        return -1;
    }
    if (!kflags)
        return _syscall(LINUX_SYS_fchmodat, _at_fd_to_linux(dirfd), path, mode);

    /* Only fchmodat2 takes flags */
    ret = _syscall_raw(LINUX_SYS_fchmodat2, _at_fd_to_linux(dirfd), path, mode, kflags);
    if (ret == 0)
        return 0;
    if (ret == -LINUX_ENOSYS)
        return _fchmodat_nofollow(_at_fd_to_linux(dirfd), path, mode);
    errno = _syscall_errno(ret);
    return -1;
}
