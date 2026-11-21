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
#include "local-cancel.h"
#include <freebsd/freebsd-flock-struct.h>
#include <limits.h>
#include <stdarg.h>

static int
_fd_flags_to_freebsd(int flags)
{
    int kflags = 0;

    if (flags & FD_CLOEXEC)
        kflags |= FREEBSD_FD_CLOEXEC;
    if (flags & FD_CLOFORK)
        kflags |= FREEBSD_FD_CLOFORK;
    return kflags;
}

static int
_fd_flags_from_freebsd(int kflags)
{
    int flags = 0;

    if (kflags & FREEBSD_FD_CLOEXEC)
        flags |= FD_CLOEXEC;
    if (kflags & FREEBSD_FD_CLOFORK)
        flags |= FD_CLOFORK;
    return flags;
}

/* Record locks; FreeBSD has no open file description locks */
static int
_fcntl_lock(int fd, int kop, struct flock *fl)
{
    struct __freebsd_flock kfl = { 0 };
    void                  *cancel;
    long                   ret;

    switch (fl->l_type) {
    case F_RDLCK:
        kfl.l_type = FREEBSD_F_RDLCK;
        break;
    case F_WRLCK:
        kfl.l_type = FREEBSD_F_WRLCK;
        break;
    case F_UNLCK:
        kfl.l_type = FREEBSD_F_UNLCK;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    kfl.l_whence = fl->l_whence;
    kfl.l_start = fl->l_start;
    kfl.l_len = fl->l_len;
    kfl.l_pid = fl->l_pid;

    /* Waiting for a lock is a cancellation point */
    cancel = kop == FREEBSD_F_SETLKW ? _cancel_enter() : NULL;
    ret = _syscall(FREEBSD_SYS_fcntl, fd, kop, &kfl);
    if (cancel)
        _cancel_leave(cancel);
    if (ret < 0)
        return -1;
    if (kop == FREEBSD_F_GETLK) {
        switch (kfl.l_type) {
        case FREEBSD_F_RDLCK:
            fl->l_type = F_RDLCK;
            break;
        case FREEBSD_F_WRLCK:
            fl->l_type = F_WRLCK;
            break;
        default:
            fl->l_type = F_UNLCK;
            break;
        }
        fl->l_whence = kfl.l_whence;
        fl->l_start = kfl.l_start;
        fl->l_len = kfl.l_len;
        fl->l_pid = kfl.l_pid;
    }
    return (int)ret;
}

/*
 * The SIGIO/SIGURG owner. FreeBSD's F_GETOWN and F_SETOWN name process
 * groups with negative values, as POSIX does, and the f_owner_ex forms
 * are built on them. F_GETOWN's negative results can look like errors,
 * so read the owner with FIOGETOWN, which is what F_GETOWN does.
 */
static int
_fcntl_getown(int fd, struct f_owner_ex *owner)
{
    int kowner;

    if (_syscall(FREEBSD_SYS_ioctl, fd, FREEBSD_FIOGETOWN, &kowner) < 0)
        return -1;
    if (kowner < 0) {
        owner->type = F_OWNER_PGRP;
        owner->pid = -kowner;
    } else {
        owner->type = F_OWNER_PID;
        owner->pid = kowner;
    }
    return 0;
}

static int
_fcntl_setown(int fd, const struct f_owner_ex *owner)
{
    long pid;

    if (owner->pid < 0) {
        errno = EINVAL;
        return -1;
    }
    switch (owner->type) {
    case F_OWNER_PID:
        pid = owner->pid;
        break;
    case F_OWNER_PGRP:
        pid = -(long)owner->pid;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_SETOWN, pid);
}

int
fcntl(int fd, int op, ...)
{
    va_list ap;
    int     arg = 0;
    void   *parg = NULL;
    long    ret;

    va_start(ap, op);
    switch (op) {
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_DUPFD_CLOFORK:
    case F_SETFD:
    case F_SETFL:
    case F_SETOWN:
        arg = va_arg(ap, int);
        break;
    case F_GETOWN_EX:
    case F_SETOWN_EX:
    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
        parg = va_arg(ap, void *);
        break;
    }
    va_end(ap);

    switch (op) {
    case F_DUPFD:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_DUPFD, arg);
    case F_DUPFD_CLOEXEC:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_DUPFD_CLOEXEC, arg);
    case F_DUPFD_CLOFORK:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_DUPFD_CLOFORK, arg);
    case F_GETFD:
        ret = _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_GETFD);
        if (ret < 0)
            return -1;
        return _fd_flags_from_freebsd((int)ret);
    case F_SETFD:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_SETFD, _fd_flags_to_freebsd(arg));
    case F_GETFL:
        ret = _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_GETFL);
        if (ret < 0)
            return -1;
        return _open_flags_from_freebsd((int)ret);
    case F_SETFL:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_SETFL, _open_flags_to_freebsd(arg));
    case F_GETOWN: {
        struct f_owner_ex owner;

        if (_fcntl_getown(fd, &owner) < 0)
            return -1;
        return owner.type == F_OWNER_PGRP ? -owner.pid : owner.pid;
    }
    case F_SETOWN: {
        struct f_owner_ex owner = { .type = F_OWNER_PID, .pid = arg };

        /* Negative values name process groups */
        if (arg == INT_MIN) {
            errno = EINVAL;
            return -1;
        }
        if (arg < 0) {
            owner.type = F_OWNER_PGRP;
            owner.pid = -arg;
        }
        return _fcntl_setown(fd, &owner);
    }
    case F_GETOWN_EX:
        return _fcntl_getown(fd, parg);
    case F_SETOWN_EX:
        return _fcntl_setown(fd, parg);
    case F_GETLK:
        return _fcntl_lock(fd, FREEBSD_F_GETLK, parg);
    case F_SETLK:
        return _fcntl_lock(fd, FREEBSD_F_SETLK, parg);
    case F_SETLKW:
        return _fcntl_lock(fd, FREEBSD_F_SETLKW, parg);
    default:
        /* Including F_OFD_GETLK, F_OFD_SETLK and F_OFD_SETLKW */
        errno = EINVAL;
        return -1;
    }
}
