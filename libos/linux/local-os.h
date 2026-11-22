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
 * The kernel names libos/common uses, mapped to Linux's. Each OS layer
 * has its own local-os.h; libos/common has none, so its sources find
 * the one of the OS layer building them.
 */

#ifndef _LOCAL_OS_H_
#define _LOCAL_OS_H_

#include "local-linux.h"

/* System calls */
#define OS_SYS__exit LINUX_SYS_exit_group
#ifdef LINUX_SYS_access
#define OS_SYS_access LINUX_SYS_access
#endif
#define OS_SYS_chdir LINUX_SYS_chdir
#ifdef LINUX_SYS_chmod
#define OS_SYS_chmod LINUX_SYS_chmod
#endif
#define OS_SYS_close     LINUX_SYS_close
#define OS_SYS_dup       LINUX_SYS_dup
#define OS_SYS_execve    LINUX_SYS_execve
#define OS_SYS_faccessat LINUX_SYS_faccessat
#ifdef LINUX_SYS_faccessat2
#define OS_SYS_faccessat2 LINUX_SYS_faccessat2
#endif
#define OS_SYS_fchdir    LINUX_SYS_fchdir
#define OS_SYS_fchmod    LINUX_SYS_fchmod
#define OS_SYS_fchmodat  LINUX_SYS_fchmodat
#define OS_SYS_fchownat  LINUX_SYS_fchownat
#define OS_SYS_fcntl     LINUX_SYS_fcntl
#define OS_SYS_fdatasync LINUX_SYS_fdatasync
#define OS_SYS_fsync     LINUX_SYS_fsync
#define OS_SYS_getegid   LINUX_SYS_getegid
#ifdef LINUX_SYS_getegid32
#define OS_SYS_getegid32 LINUX_SYS_getegid32
#endif
#define OS_SYS_geteuid LINUX_SYS_geteuid
#ifdef LINUX_SYS_geteuid32
#define OS_SYS_geteuid32 LINUX_SYS_geteuid32
#endif
#define OS_SYS_getgid LINUX_SYS_getgid
#ifdef LINUX_SYS_getgid32
#define OS_SYS_getgid32 LINUX_SYS_getgid32
#endif
#define OS_SYS_getpgid LINUX_SYS_getpgid
#ifdef LINUX_SYS_getpgrp
#define OS_SYS_getpgrp LINUX_SYS_getpgrp
#endif
#define OS_SYS_getpid    LINUX_SYS_getpid
#define OS_SYS_getppid   LINUX_SYS_getppid
#define OS_SYS_getresgid LINUX_SYS_getresgid
#ifdef LINUX_SYS_getresgid32
#define OS_SYS_getresgid32 LINUX_SYS_getresgid32
#endif
#define OS_SYS_getresuid LINUX_SYS_getresuid
#ifdef LINUX_SYS_getresuid32
#define OS_SYS_getresuid32 LINUX_SYS_getresuid32
#endif
#define OS_SYS_getsid LINUX_SYS_getsid
#define OS_SYS_getuid LINUX_SYS_getuid
#ifdef LINUX_SYS_getuid32
#define OS_SYS_getuid32 LINUX_SYS_getuid32
#endif
#define OS_SYS_ioctl LINUX_SYS_ioctl
#ifdef LINUX_SYS_link
#define OS_SYS_link LINUX_SYS_link
#endif
#define OS_SYS_linkat LINUX_SYS_linkat
#ifdef LINUX_SYS_mkdir
#define OS_SYS_mkdir LINUX_SYS_mkdir
#endif
#define OS_SYS_mkdirat LINUX_SYS_mkdirat
#define OS_SYS_mlock   LINUX_SYS_mlock
#define OS_SYS_msgget  LINUX_SYS_msgget
#define OS_SYS_munlock LINUX_SYS_munlock
#define OS_SYS_openat  LINUX_SYS_openat
#define OS_SYS_pipe2   LINUX_SYS_pipe2
#define OS_SYS_read    LINUX_SYS_read
#ifdef LINUX_SYS_rename
#define OS_SYS_rename LINUX_SYS_rename
#endif
#ifdef LINUX_SYS_renameat
#define OS_SYS_renameat LINUX_SYS_renameat
#endif
#define OS_SYS_renameat2 LINUX_SYS_renameat2
#ifdef LINUX_SYS_rmdir
#define OS_SYS_rmdir LINUX_SYS_rmdir
#endif
#define OS_SYS_sched_getscheduler LINUX_SYS_sched_getscheduler
#define OS_SYS_sched_yield        LINUX_SYS_sched_yield
#define OS_SYS_semget             LINUX_SYS_semget
#define OS_SYS_setgroups          LINUX_SYS_setgroups
#ifdef LINUX_SYS_setgroups32
#define OS_SYS_setgroups32 LINUX_SYS_setgroups32
#endif
#define OS_SYS_setpgid  LINUX_SYS_setpgid
#define OS_SYS_setregid LINUX_SYS_setregid
#ifdef LINUX_SYS_setregid32
#define OS_SYS_setregid32 LINUX_SYS_setregid32
#endif
#define OS_SYS_setresgid LINUX_SYS_setresgid
#ifdef LINUX_SYS_setresgid32
#define OS_SYS_setresgid32 LINUX_SYS_setresgid32
#endif
#define OS_SYS_setresuid LINUX_SYS_setresuid
#ifdef LINUX_SYS_setresuid32
#define OS_SYS_setresuid32 LINUX_SYS_setresuid32
#endif
#define OS_SYS_setreuid LINUX_SYS_setreuid
#ifdef LINUX_SYS_setreuid32
#define OS_SYS_setreuid32 LINUX_SYS_setreuid32
#endif
#define OS_SYS_setsid           LINUX_SYS_setsid
#define OS_SYS_shmat            LINUX_SYS_shmat
#define OS_SYS_shmdt            LINUX_SYS_shmdt
#define OS_SYS_shmget           LINUX_SYS_shmget
#define OS_SYS_symlinkat        LINUX_SYS_symlinkat
#define OS_SYS_sync             LINUX_SYS_sync
#define OS_SYS_timer_delete     LINUX_SYS_timer_delete
#define OS_SYS_timer_getoverrun LINUX_SYS_timer_getoverrun
#define OS_SYS_timer_gettime    LINUX_SYS_timer_gettime_time_t
#define OS_SYS_umask            LINUX_SYS_umask
#define OS_SYS_unlinkat         LINUX_SYS_unlinkat
#define OS_SYS_wait4            LINUX_SYS_wait4
#define OS_SYS_write            LINUX_SYS_write

/* Constants */
#define OS_AT_FDCWD     LINUX_AT_FDCWD
#define OS_AT_REMOVEDIR LINUX_AT_REMOVEDIR
#define OS_EINTR        LINUX_EINTR
#define OS_F_GETFL      LINUX_F_GETFL
#define OS_O_CLOEXEC    LINUX_O_CLOEXEC
#define OS_O_DIRECTORY  LINUX_O_DIRECTORY
#define OS_O_NOREAD     LINUX_O_PATH
#define OS_TCGETATTR    LINUX_TCGETS2
#define OS_TIOCGPGRP    LINUX_TIOCGPGRP
#define OS_TIOCGSID     LINUX_TIOCGSID
#define OS_TIOCLINUX    LINUX_TIOCLINUX

/* The value _syscall_raw returns for a kernel errno, and where those are */
#define _OS_ERR(e)  (-(long)(e))
#define _OS_ERRNO_H <linux/linux-errno.h>

/* Kernel structures */
#define __os_dirent     __kernel_dirent64
#define __os_rusage     __kernel_wait_rusage
#define __os_itimerspec __kernel_itimerspec_time_t
#define __os_termios    __kernel_termios2

/* Translations */
#define _at_fd_to_os          _at_fd_to_linux
#define _at_flags_to_os       _at_flags_to_linux
#define _key_to_os(key)       (key)
#define _wait_options_to_os   _wait_options_to_linux
#define _wait_status_from_os  _wait_status_from_linux
#define _sched_policy_from_os _sched_policy_from_linux

#endif /* _LOCAL_OS_H_ */
