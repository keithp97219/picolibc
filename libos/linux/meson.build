#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2025 Keith Packard
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#

machine_dir = 'machine' / host_cpu_family

if fs.is_dir(machine_dir)

  has_os_linux = true

  has_os_library = true

  has_os_posix = true

  has_os_fileio = true

  has_os_process = true

  has_os_id = true

  srcs_os_linux = [
    '_Fork.c',
    'accept.c',
    'accept4.c',
    'alarm.c',
    'bind.c',
    'chown.c',
    'clock.c',
    'clock_gettime.c',
    'clock_getcpuclockid.c',
    'clock_getres.c',
    'clock_nanosleep.c',
    'clock_settime.c',
    'connect.c',
    'dup2.c',
    'dup3.c',
    'execve.c',
    '_exit.c',
    'fchmodat.c',
    'fchown.c',
    'fcntl.c',
    'fexecve.c',
    'fork.c',
    '_fork_raw.c',
    'fpathconf.c',
    'fstatat.c',
    'fstat.c',
    'ftruncate.c',
    'futimens.c',
    'getauxval.c',
    'getcwd.c',
    'getegid.c',
    'getentropy.c',
    'geteuid.c',
    'getgid.c',
    'getgroups.c',
    'gethostname.c',
    'getitimer.c',
    'getpeername.c',
    'getpgid.c',
    'getpgrp.c',
    'getpid.c',
    'getppid.c',
    'getpriority.c',
    'getresgid.c',
    'getresuid.c',
    'getrlimit.c',
    'getrusage.c',
    'getsid.c',
    'getsockname.c',
    'getsockopt.c',
    'gettimeofday.c',
    'getuid.c',
    'grantpt.c',
    'if_indextoname.c',
    'if_nameindex.c',
    'if_nametoindex.c',
    'ioctl.c',
    'isatty.c',
    'kill.c',
    'killpg.c',
    'lchown.c',
    'listen.c',
    'lseek.c',
    'madvise.c',
    'madvise-gnu.c',
    'mkfifo.c',
    'mkfifoat.c',
    'mknod.c',
    'mknodat.c',
    'mlockall_munlockall.c',
    'mlock.c',
    'mmap.c',
    'mprotect.c',
    'mq_close.c',
    'mq_getattr.c',
    'mq_notify.c',
    'mq_open.c',
    'mq_setattr.c',
    'mq_timedreceive.c',
    'mq_timedsend.c',
    'mq_unlink.c',
    'msgctl.c',
    'msgget.c',
    'msgrcv.c',
    'msgsnd.c',
    'msync.c',
    'munlock.c',
    'munmap.c',
    'nanosleep.c',
    'nice.c',
    'open.c',
    '_open_flags_from_linux.c',
    '_open_flags_to_linux.c',
    'openat.c',
    'pathconf.c',
    'pause.c',
    'pipe2.c',
    'poll.c',
    'posix_fadvise.c',
    'posix_fallocate.c',
    'posix_getdents.c',
    'posix_openpt.c',
    'ppoll.c',
    'pread.c',
    'pselect.c',
    '_pselect.c',
    'ptsname_r.c',
    'pwrite.c',
    'readdir.c',
    'readlink.c',
    'readlinkat.c',
    'readv.c',
    'recv.c',
    'recvfrom.c',
    'recvmsg.c',
    'sbrk.c',
    'sched_get_priority_max.c',
    'sched_get_priority_min.c',
    'sched_getparam.c',
    'sched_getscheduler.c',
    'sched_rr_get_interval.c',
    'sched_setparam.c',
    'sched_setscheduler.c',
    'seekdir.c',
    'sched_yield.c',
    'semctl.c',
    'semget.c',
    'semop.c',
    'send.c',
    'sendmsg.c',
    'sendto.c',
    'setegid.c',
    'setgid.c',
    'setgroups.c',
    'setitimer.c',
    'seteuid.c',
    'setpgid.c',
    'setpriority.c',
    'setregid.c',
    'setresgid.c',
    'setresuid.c',
    'setreuid.c',
    'setrlimit.c',
    'setsid.c',
    'setsockopt.c',
    'setuid.c',
    'shm_open.c',
    'shm_unlink.c',
    'shmat.c',
    'shmctl.c',
    'shmdt.c',
    'shmget.c',
    'shutdown.c',
    'sigaction.c',
    'sigaltstack.c',
    'sigqueue.c',
    '_sigmask_from_linux.c',
    '_sigmask_to_linux.c',
    '_signal_from_linux.c',
    '_signal_to_linux.c',
    '_siginfo_from_linux.c',
    'sigpending.c',
    'sigprocmask.c',
    '_sigprocmask.c',
    'sigsuspend.c',
    'sigtimedwait.c',
    '_sigtimedwait.c',
    'sockatmark.c',
    'socket.c',
    'socketpair.c',
    '_statbuf.c',
    'statvfs.c',
    '_statvfsbuf.c',
    'fstatvfs.c',
    '_syscall_errno.c',
    'sysconf.c',
    'tcdrain.c',
    'tcflow.c',
    'tcflush.c',
    'tcgetattr.c',
    'tcgetpgrp.c',
    'tcgetsid.c',
    'tcgetwinsize.c',
    'tcsendbreak.c',
    'tcsetattr.c',
    'tcsetpgrp.c',
    'tcsetwinsize.c',
    'telldir.c',
    'times.c',
    'timer_create.c',
    'timer_delete.c',
    'timer_getoverrun.c',
    'timer_gettime.c',
    'timer_settime.c',
    'truncate.c',
    'ttyname_r.c',
    'uname.c',
    'unlinkat.c',
    'unlockpt.c',
    'utimensat.c',
    'utimes.c',
    'vfork.c',
    'wait3.c',
    'waitid.c',
    'waitpid.c',
    'writev.c',
  ]

  if posix_threads
    srcs_os_linux += [
      '__aio_signal.c',
      '_futex_lock_pi.c',
      '_futex_timespec.c',
      '_futex_unlock_pi.c',
      '_futex_wait.c',
      '_futex_wake.c',
      '_mq_notify_thread.c',
      'pthread_getcpuclockid.c',
      '_pthread_getsched.c',
      '_pthread_os_create.c',
      '_pthread_os_exit.c',
      '_pthread_os_set_tid.c',
      '_pthread_os_sigcancel.c',
      '_pthread_pi_available.c',
      '_pthread_robust.c',
      '_pthread_setsched.c',
      '_pthread_setxid.c',
      '_pthread_tkill.c',
      '_timer_thread.c',
    ]
  endif

  subdir(machine_dir)

  # The code shared with other OS layers this one uses (see libos/common)
  srcs_os_linux_common = [
    '_syscall.c',
    'access.c',
    'chdir.c',
    'chmod.c',
    'close.c',
    'closedir.c',
    'dup.c',
    'faccessat.c',
    'fchdir.c',
    'fchmod.c',
    'fchownat.c',
    'fdatasync.c',
    'fdopendir.c',
    'fsync.c',
    'iob_stderr.c',
    'iob_stdin.c',
    'iob_stdout.c',
    'link.c',
    'linkat.c',
    'lstat.c',
    'mkdir.c',
    'mkdirat.c',
    'opendir.c',
    'pipe.c',
    'read.c',
    'remove.c',
    'rename.c',
    'renameat.c',
    'rewinddir.c',
    'rmdir.c',
    'stat.c',
    'symlink.c',
    'symlinkat.c',
    'sync.c',
    'umask.c',
    'unlink.c',
    'write.c',
  ]

  src_os_linux_common = []
  foreach file : srcs_os_linux_common
    src_os_linux_common += src_os_common[file]
  endforeach

  # The shared POSIX threads code goes into this library too
  src_os_linux = (files(srcs_os_linux) + src_os_linux_common + src_os_linux_machine
                  + src_os_pthread)

  srcs_libos += srcs_os_linux + srcs_os_linux_common + srcs_os_pthread

  linux_inc = include_directories(machine_dir, 'include')

  # The POSIX options and option groups this OS layer provides, and
  # its signal stack sizes
  if really_install
    install_headers(['include/sys/_posix.h', 'include/sys/_signal.h'],
                    install_dir: include_dir / 'sys')
    # The socket ABI for picolibc's networking headers
    if network == 'linux'
      install_headers(['include/sys/_socket.h', 'include/sys/_un.h'],
                      install_dir: include_dir / 'sys')
      install_headers(['include/netinet/_in.h', 'include/netinet/_tcp.h'],
                      install_dir: include_dir / 'netinet')
      install_headers('include/net/_if.h',
                      install_dir: include_dir / 'net')
    endif
  endif

  foreach params : targets
    target = params['name']
    target_dir = params['dir']
    target_c_args = params['c_args']

    instdir = join_paths(lib_dir, target_dir)

    liblinux_name = 'linux'

    if meson.version().version_compare('>=1.10')
      _lib = static_library(liblinux_name,
			    src_os_linux,
                            build_subdir : target_dir,
			    install : really_install,
			    install_dir : instdir,
			    include_directories : [linux_inc, pthread_inc, inc],
			    c_args : target_c_args + c_args)
    else
      target_lib_prefix = params['lib_prefix']
      _lib = static_library(join_paths(target_dir, target_lib_prefix + liblinux_name),
			    src_os_linux,
			    install : really_install,
			    install_dir : instdir,
			    include_directories : [linux_inc, pthread_inc, inc],
			    c_args : target_c_args + c_args)
    endif

    set_variable('lib_linux' + target, _lib)
    
  endforeach

endif
