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

#ifndef _LOCAL_LINUX_H_
#define _LOCAL_LINUX_H_

#define _GNU_SOURCE

#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <linux/linux-fcntl.h>
#include <linux/linux-ioctl.h>
#include <linux/linux-mman.h>
#include <linux/linux-poll.h>
#include <linux/linux-syscall.h>

#include <linux/linux-time.h>
#include <linux/linux-wait.h>

long _syscall(long sys_call, ...);

/*
 * As _syscall, but returning the kernel's result, a negative Linux
 * errno value on failure, without touching errno. _syscall_errno
 * converts such a value to picolibc's errno.
 */
long _syscall_raw(long sys_call, ...);
int  _syscall_errno(long ret);

/* "/proc/self/fd/" and a descriptor number, for O_PATH descriptors */
#define _OPATH_NAME_MAX 32
char *_opath_name(int fd, char *buf);

/*
 * Credential changes: Linux makes them per thread, so with threads
 * every thread repeats the change. Returns like _syscall.
 */
#ifdef __PICOLIBC_PTHREADS
long _pthread_setxid(long nr, long a, long b, long c);
#define _syscall_setxid(nr, a, b, c) _pthread_setxid(nr, a, b, c)
#else
#define _syscall_setxid(nr, a, b, c) _syscall(nr, a, b, c)
#endif

/* Translate the directory descriptor of the *at functions */
static inline int
_at_fd_to_linux(int fd)
{
    if (fd == AT_FDCWD)
        return (int)LINUX_AT_FDCWD;
    /* Linux's AT_FDCWD isn't AT_FDCWD here, so it is just a bad descriptor */
    if (fd == (int)LINUX_AT_FDCWD)
        return -1;
    return fd;
}

/* Translate AT_ flags, returning -1 for flags outside 'allowed' */
static inline int
_at_flags_to_linux(int flags, int allowed)
{
    int kflags = 0;

    if (flags & ~allowed)
        return -1;
    if (flags & AT_EACCESS)
        kflags |= LINUX_AT_EACCESS;
    if (flags & AT_SYMLINK_NOFOLLOW)
        kflags |= LINUX_AT_SYMLINK_NOFOLLOW;
    if (flags & AT_SYMLINK_FOLLOW)
        kflags |= LINUX_AT_SYMLINK_FOLLOW;
    if (flags & AT_REMOVEDIR)
        kflags |= LINUX_AT_REMOVEDIR;
#ifdef AT_EMPTY_PATH
    if (flags & AT_EMPTY_PATH)
        kflags |= LINUX_AT_EMPTY_PATH;
#endif
    return kflags;
}

/* Translate open flags */
int   _open_flags_to_linux(int flags);
int   _open_flags_from_linux(int kflags);

/* Create a child process; fork and _Fork add their bookkeeping */
pid_t _fork_raw(void);

struct fd_set;
int _pselect(int nfds, struct fd_set *readfds, struct fd_set *writefds, struct fd_set *exceptfds,
             const struct timespec *timeout, const __sigset_t *sigmask);

#endif /* _LOCAL_LINUX_H_ */
