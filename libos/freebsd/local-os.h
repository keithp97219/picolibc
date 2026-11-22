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

/*
 * The kernel names libos/common uses, mapped to FreeBSD's. Each OS layer
 * has its own local-os.h; libos/common has none, so its sources find
 * the one of the OS layer building them.
 */

#ifndef _LOCAL_OS_H_
#define _LOCAL_OS_H_

#include "local-freebsd.h"

/* System calls */
#define OS_SYS__exit              FREEBSD_SYS__exit
#define OS_SYS_chdir              FREEBSD_SYS_chdir
#define OS_SYS_close              FREEBSD_SYS_close
#define OS_SYS_dup                FREEBSD_SYS_dup
#define OS_SYS_execve             FREEBSD_SYS_execve
#define OS_SYS_faccessat          FREEBSD_SYS_faccessat
#define OS_SYS_fchdir             FREEBSD_SYS_fchdir
#define OS_SYS_fchmod             FREEBSD_SYS_fchmod
#define OS_SYS_fchmodat           FREEBSD_SYS_fchmodat
#define OS_SYS_fchownat           FREEBSD_SYS_fchownat
#define OS_SYS_fcntl              FREEBSD_SYS_fcntl
#define OS_SYS_fdatasync          FREEBSD_SYS_fdatasync
#define OS_SYS_fsync              FREEBSD_SYS_fsync
#define OS_SYS_getegid            FREEBSD_SYS_getegid
#define OS_SYS_geteuid            FREEBSD_SYS_geteuid
#define OS_SYS_getgid             FREEBSD_SYS_getgid
#define OS_SYS_getpgid            FREEBSD_SYS_getpgid
#define OS_SYS_getpgrp            FREEBSD_SYS_getpgrp
#define OS_SYS_getpid             FREEBSD_SYS_getpid
#define OS_SYS_getppid            FREEBSD_SYS_getppid
#define OS_SYS_getresgid          FREEBSD_SYS_getresgid
#define OS_SYS_getresuid          FREEBSD_SYS_getresuid
#define OS_SYS_getsid             FREEBSD_SYS_getsid
#define OS_SYS_getuid             FREEBSD_SYS_getuid
#define OS_SYS_ioctl              FREEBSD_SYS_ioctl
#define OS_SYS_linkat             FREEBSD_SYS_linkat
#define OS_SYS_mkdirat            FREEBSD_SYS_mkdirat
#define OS_SYS_mlock              FREEBSD_SYS_mlock
#define OS_SYS_msgget             FREEBSD_SYS_msgget
#define OS_SYS_munlock            FREEBSD_SYS_munlock
#define OS_SYS_openat             FREEBSD_SYS_openat
#define OS_SYS_pipe2              FREEBSD_SYS_pipe2
#define OS_SYS_read               FREEBSD_SYS_read
#define OS_SYS_renameat           FREEBSD_SYS_renameat
#define OS_SYS_sched_getscheduler FREEBSD_SYS_sched_getscheduler
#define OS_SYS_sched_yield        FREEBSD_SYS_sched_yield
#define OS_SYS_semget             FREEBSD_SYS_semget
#define OS_SYS_setgroups          FREEBSD_SYS_setgroups
#define OS_SYS_setpgid            FREEBSD_SYS_setpgid
#define OS_SYS_setregid           FREEBSD_SYS_setregid
#define OS_SYS_setresgid          FREEBSD_SYS_setresgid
#define OS_SYS_setresuid          FREEBSD_SYS_setresuid
#define OS_SYS_setreuid           FREEBSD_SYS_setreuid
#define OS_SYS_setsid             FREEBSD_SYS_setsid
#define OS_SYS_shmat              FREEBSD_SYS_shmat
#define OS_SYS_shmdt              FREEBSD_SYS_shmdt
#define OS_SYS_shmget             FREEBSD_SYS_shmget
#define OS_SYS_symlinkat          FREEBSD_SYS_symlinkat
#define OS_SYS_sync               FREEBSD_SYS_sync
#define OS_SYS_timer_delete       FREEBSD_SYS_ktimer_delete
#define OS_SYS_timer_getoverrun   FREEBSD_SYS_ktimer_getoverrun
#define OS_SYS_timer_gettime      FREEBSD_SYS_ktimer_gettime
#define OS_SYS_umask              FREEBSD_SYS_umask
#define OS_SYS_unlinkat           FREEBSD_SYS_unlinkat
#define OS_SYS_write              FREEBSD_SYS_write

/* Constants */
#define OS_AT_FDCWD     FREEBSD_AT_FDCWD
#define OS_AT_REMOVEDIR FREEBSD_AT_REMOVEDIR
#define OS_EINTR        FREEBSD_EINTR
#define OS_F_GETFL      FREEBSD_F_GETFL
#define OS_O_CLOEXEC    FREEBSD_O_CLOEXEC
#define OS_O_DIRECTORY  FREEBSD_O_DIRECTORY
#define OS_O_NOREAD     (FREEBSD_O_EXEC | FREEBSD_O_PATH)
#define OS_TCGETATTR    FREEBSD_TIOCGETA
#define OS_TIOCGPGRP    FREEBSD_TIOCGPGRP
#define OS_TIOCGSID     FREEBSD_TIOCGSID

/* The value _syscall_raw returns for a kernel errno, and where those are */
#define _OS_ERR(e)  _FREEBSD_ERR(e)
#define _OS_ERRNO_H <freebsd/freebsd-errno.h>

/* The kernel changes the IDs of every thread */
#define _syscall_setxid(nr, a, b, c) _syscall(nr, a, b, c)

/* Kernel structures */
#define __os_dirent     __freebsd_dirent
#define __os_itimerspec __freebsd_itimerspec
#define __os_termios    __freebsd_termios

/* Translations */
#define _at_fd_to_os          _at_fd_to_freebsd
#define _at_flags_to_os       _at_flags_to_freebsd
#define _key_to_os            _key_to_freebsd
#define _sched_policy_from_os _sched_policy_from_freebsd

#endif /* _LOCAL_OS_H_ */
