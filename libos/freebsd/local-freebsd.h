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

#ifndef _LOCAL_FREEBSD_H_
#define _LOCAL_FREEBSD_H_

#define _GNU_SOURCE

#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <freebsd/freebsd-fcntl.h>
#include <freebsd/freebsd-ioctl.h>
#include <freebsd/freebsd-mman.h>
#include <freebsd/freebsd-syscall.h>
#include <freebsd/freebsd-time.h>

/*
 * Make a system call, setting errno and returning -1 on failure. The
 * kernel reports failure with the carry flag and a positive errno.
 */
long _syscall(long sys_call, ...);

/*
 * As _syscall, but returning the kernel's errno negated on failure,
 * without touching errno; _syscall_errno converts that to picolibc's
 * errno. A system call whose successful results include small negative
 * values (getpriority) can't use either.
 */
long _syscall_raw(long sys_call, ...);
int  _syscall_errno(long ret);

/* The value _syscall_raw returns for a FreeBSD errno */
#define _FREEBSD_ERR(e) (-(long)(e))

/* Translate the directory descriptor of the *at functions */
static inline int
_at_fd_to_freebsd(int fd)
{
    if (fd == AT_FDCWD)
        return (int)FREEBSD_AT_FDCWD;
    /* FreeBSD's AT_FDCWD isn't AT_FDCWD here, so it is just a bad descriptor */
    if (fd == (int)FREEBSD_AT_FDCWD)
        return -1;
    return fd;
}

/* Translate AT_ flags, returning -1 for flags outside 'allowed' */
static inline int
_at_flags_to_freebsd(int flags, int allowed)
{
    int kflags = 0;

    if (flags & ~allowed)
        return -1;
    if (flags & AT_EACCESS)
        kflags |= FREEBSD_AT_EACCESS;
    if (flags & AT_SYMLINK_NOFOLLOW)
        kflags |= FREEBSD_AT_SYMLINK_NOFOLLOW;
    if (flags & AT_SYMLINK_FOLLOW)
        kflags |= FREEBSD_AT_SYMLINK_FOLLOW;
    if (flags & AT_REMOVEDIR)
        kflags |= FREEBSD_AT_REMOVEDIR;
#ifdef AT_EMPTY_PATH
    if (flags & AT_EMPTY_PATH)
        kflags |= FREEBSD_AT_EMPTY_PATH;
#endif
    return kflags;
}

/* Translate open flags */
int _open_flags_to_freebsd(int flags);
int _open_flags_from_freebsd(int kflags);

/* Convert the kernel's struct stat */
struct __freebsd_stat;
int _statbuf(struct stat *statbuf, const struct __freebsd_stat *kstat);

#endif /* _LOCAL_FREEBSD_H_ */
