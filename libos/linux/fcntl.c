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
#include "local-cancel.h"
#include <fcntl.h>
#include <stdarg.h>

/*
 * Record locks use 64-bit offsets everywhere: 32-bit targets need the
 * fcntl64 call with its own lock commands. The open file description
 * lock commands are the same on all targets.
 */
#if __SIZEOF_LONG__ == 8
#define LINUX_SYS_fcntl_lock LINUX_SYS_fcntl
#define LINUX_F_GETLK_64     5
#define LINUX_F_SETLK_64     6
#define LINUX_F_SETLKW_64    7
#else
#define LINUX_SYS_fcntl_lock LINUX_SYS_fcntl64
#define LINUX_F_GETLK_64     12
#define LINUX_F_SETLK_64     13
#define LINUX_F_SETLKW_64    14
#endif
#define LINUX_F_OFD_GETLK_64  36
#define LINUX_F_OFD_SETLK_64  37
#define LINUX_F_OFD_SETLKW_64 38

#define LINUX_F_RDLCK         0
#define LINUX_F_WRLCK         1
#define LINUX_F_UNLCK         2

struct __kernel_flock64 {
    short     l_type;
    short     l_whence;
    __int64_t l_start;
    __int64_t l_len;
    int       l_pid;
};

static int
_fcntl_lock(int fd, int kop, struct flock *fl)
{
    struct __kernel_flock64 kfl;
    void                   *cancel;
    long                    ret;

    switch (fl->l_type) {
    case F_RDLCK:
        kfl.l_type = LINUX_F_RDLCK;
        break;
    case F_WRLCK:
        kfl.l_type = LINUX_F_WRLCK;
        break;
    case F_UNLCK:
        kfl.l_type = LINUX_F_UNLCK;
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
    cancel = (kop == LINUX_F_SETLKW_64 || kop == LINUX_F_OFD_SETLKW_64) ? _cancel_enter() : NULL;
    ret = _syscall(LINUX_SYS_fcntl_lock, fd, kop, &kfl);
    if (cancel)
        _cancel_leave(cancel);
    if (ret < 0)
        return -1;

    if (kop == LINUX_F_GETLK_64 || kop == LINUX_F_OFD_GETLK_64) {
        switch (kfl.l_type) {
        case LINUX_F_RDLCK:
            fl->l_type = F_RDLCK;
            break;
        case LINUX_F_WRLCK:
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

int
fcntl(int fd, int op, ...)
{
    va_list ap;
    int     arg = 0;
    long    ret;

    switch (op) {
    case F_DUPFD:
        op = LINUX_F_DUPFD;
        arg = 1;
        break;
    case F_DUPFD_CLOEXEC:
        op = LINUX_F_DUPFD_CLOEXEC;
        arg = 1;
        break;
    case F_GETFD:
        op = LINUX_F_GETFD;
        break;
    case F_SETFD:
        op = LINUX_F_SETFD;
        arg = 1;
        break;
    case F_GETFL:
        op = LINUX_F_GETFL;
        break;
    case F_SETFL:
        op = LINUX_F_SETFL;
        arg = 1;
        break;
    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
    case F_OFD_GETLK:
    case F_OFD_SETLK:
    case F_OFD_SETLKW: {
        static const int kops[] = {
            [F_GETLK] = LINUX_F_GETLK_64,         [F_SETLK] = LINUX_F_SETLK_64,
            [F_SETLKW] = LINUX_F_SETLKW_64,       [F_OFD_GETLK] = LINUX_F_OFD_GETLK_64,
            [F_OFD_SETLK] = LINUX_F_OFD_SETLK_64, [F_OFD_SETLKW] = LINUX_F_OFD_SETLKW_64,
        };
        struct flock *fl;

        va_start(ap, op);
        fl = va_arg(ap, struct flock *);
        va_end(ap);
        return _fcntl_lock(fd, kops[op], fl);
    }
    default:
        errno = EINVAL;
        return -1;
    }
    va_start(ap, op);
    if (arg)
        arg = va_arg(ap, int);
    va_end(ap);

    /* The file status flags have different values in Linux */
    if (op == LINUX_F_SETFL)
        arg = _open_flags_to_linux(arg);
    ret = _syscall(LINUX_SYS_fcntl, fd, op, arg);
    if (op == LINUX_F_GETFL && ret >= 0)
        ret = _open_flags_from_linux((int)ret);
    return (int)ret;
}
