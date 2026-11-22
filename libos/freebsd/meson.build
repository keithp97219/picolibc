#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2026 Keith Packard
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

# The generated kernel interface headers for each machine
freebsd_machines = { 'x86': 'x86_64' }

machine_dir = 'machine' / host_cpu_family

if fs.is_dir(machine_dir) and host_cpu_family in freebsd_machines

  has_os_freebsd = true

  has_os_library = true

  has_os_posix = true

  has_os_fileio = true

  srcs_os_freebsd = [
    '_Fork.c',
    '_exit.c',
    '_fdevname.c',
    '_fork_raw.c',
    '_mq_notify_thread.c',
    '_mq_supported.c',
    '_open_flags_from_freebsd.c',
    '_open_flags_to_freebsd.c',
    '_pathconf.c',
    '_ppoll.c',
    '_pselect.c',
    '_range_mapped.c',
    '_siginfo.c',
    '_sigmask.c',
    '_sigtimedwait.c',
    '_sock_addr_out.c',
    '_sock_domain.c',
    '_sock_recv.c',
    '_sock_result.c',
    '_statbuf.c',
    '_statvfsbuf.c',
    '_syscall.c',
    '_syscall_errno.c',
    '_termios_freebsd.c',
    'accept.c',
    'accept4.c',
    'access.c',
    'alarm.c',
    'bind.c',
    'cfgetispeed.c',
    'cfgetospeed.c',
    'cfsetispeed.c',
    'cfsetospeed.c',
    'cfsetspeed.c',
    'chdir.c',
    'chmod.c',
    'chown.c',
    'clock_getcpuclockid.c',
    'clock_getres.c',
    'clock_gettime.c',
    'clock_nanosleep.c',
    'clock_settime.c',
    'close.c',
    'closedir.c',
    'connect.c',
    'dup.c',
    'dup2.c',
    'dup3.c',
    'execve.c',
    'faccessat.c',
    'fchdir.c',
    'fchmod.c',
    'fchmodat.c',
    'fchown.c',
    'fchownat.c',
    'fcntl.c',
    'fdatasync.c',
    'fdopendir.c',
    'fexecve.c',
    'fork.c',
    'fpathconf.c',
    'fstat.c',
    'fstatat.c',
    'fstatvfs.c',
    'fsync.c',
    'ftruncate.c',
    'futimens.c',
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
    'getwd.c',
    'grantpt.c',
    'if_freenameindex.c',
    'if_indextoname.c',
    'if_nameindex.c',
    'if_nametoindex.c',
    'iob_stderr.c',
    'iob_stdin.c',
    'iob_stdout.c',
    'ioctl.c',
    'isatty.c',
    'kill.c',
    'killpg.c',
    'lchown.c',
    'link.c',
    'linkat.c',
    'listen.c',
    'lseek.c',
    'lstat.c',
    'madvise.c',
    'mkdir.c',
    'mkdirat.c',
    'mkfifo.c',
    'mkfifoat.c',
    'mknod.c',
    'mknodat.c',
    'mlock.c',
    'mlockall.c',
    'mmap.c',
    'mprotect.c',
    'mq_close.c',
    'mq_getattr.c',
    'mq_notify.c',
    'mq_open.c',
    'mq_receive.c',
    'mq_send.c',
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
    'munlockall.c',
    'munmap.c',
    'nanosleep.c',
    'nice.c',
    'open.c',
    'openat.c',
    'opendir.c',
    'pathconf.c',
    'pause.c',
    'pipe.c',
    'pipe2.c',
    'poll.c',
    'posix_fadvise.c',
    'posix_fallocate.c',
    'posix_getdents.c',
    'posix_madvise.c',
    'posix_openpt.c',
    'ppoll.c',
    'pread.c',
    'pselect.c',
    'ptsname.c',
    'ptsname_r.c',
    'pwrite.c',
    'read.c',
    'readdir.c',
    'readlink.c',
    'readlinkat.c',
    'readv.c',
    'recv.c',
    'recvfrom.c',
    'recvmsg.c',
    'remove.c',
    'rename.c',
    'renameat.c',
    'rewinddir.c',
    'rmdir.c',
    'sbrk.c',
    'sched_get_priority_max.c',
    'sched_get_priority_min.c',
    'sched_getparam.c',
    'sched_getscheduler.c',
    'sched_rr_get_interval.c',
    'sched_setparam.c',
    'sched_setscheduler.c',
    'sched_yield.c',
    'seekdir.c',
    'select.c',
    'semctl.c',
    'semget.c',
    'semop.c',
    'send.c',
    'sendmsg.c',
    'sendto.c',
    'setegid.c',
    'seteuid.c',
    'setgid.c',
    'setgroups.c',
    'setitimer.c',
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
    'sigpending.c',
    'sigprocmask.c',
    'sigqueue.c',
    'sigsuspend.c',
    'sigtimedwait.c',
    'sockatmark.c',
    'socket.c',
    'socketpair.c',
    'stat.c',
    'statvfs.c',
    'symlink.c',
    'symlinkat.c',
    'sync.c',
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
    'timer_create.c',
    'timer_delete.c',
    'timer_getoverrun.c',
    'timer_gettime.c',
    'timer_settime.c',
    'times.c',
    'truncate.c',
    'ttyname.c',
    'ttyname_r.c',
    'umask.c',
    'uname.c',
    'unlink.c',
    'unlinkat.c',
    'unlockpt.c',
    'utimensat.c',
    'utimes.c',
    'vfork.c',
    'wait3.c',
    'waitid.c',
    'waitpid.c',
    'write.c',
    'writev.c',
  ]

  # POSIX threads, built with the shared code in libos/pthread
  if posix_threads
    srcs_os_freebsd += [
      '__aio_signal.c',
      '_futex_wait.c',
      '_futex_wake.c',
      'pthread_getcpuclockid.c',
      '_pthread_getsched.c',
      '_pthread_os_create.c',
      '_pthread_os_exit.c',
      '_pthread_os_set_tid.c',
      '_pthread_os_sigcancel.c',
      '_pthread_setsched.c',
      '_pthread_tkill.c',
      '_sigprocmask.c',
      '_timer_thread.c',
    ]
  endif

  subdir(machine_dir)

  # The shared POSIX threads code goes into this library too
  src_os_freebsd = files(srcs_os_freebsd) + src_os_freebsd_machine + src_os_pthread

  srcs_libos += srcs_os_freebsd + srcs_os_pthread

  freebsd_inc = include_directories('machine' / freebsd_machines[host_cpu_family], 'include')

  # The POSIX options and option groups this OS layer provides, and
  # its signal stack sizes
  if really_install
    install_headers(['include/sys/_posix.h', 'include/sys/_signal.h'],
                    install_dir: include_dir / 'sys')
    # The socket ABI for picolibc's networking headers
    if network == 'freebsd'
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

    libfreebsd_name = 'freebsd'

    if meson.version().version_compare('>=1.10')
      _lib = static_library(libfreebsd_name,
                            src_os_freebsd,
                            build_subdir : target_dir,
                            install : really_install,
                            install_dir : instdir,
                            include_directories : [freebsd_inc, pthread_inc, inc],
                            c_args : target_c_args + c_args)
    else
      target_lib_prefix = params['lib_prefix']
      _lib = static_library(join_paths(target_dir, target_lib_prefix + libfreebsd_name),
                            src_os_freebsd,
                            install : really_install,
                            install_dir : instdir,
                            include_directories : [freebsd_inc, pthread_inc, inc],
                            c_args : target_c_args + c_args)
    endif

    set_variable('lib_freebsd' + target, _lib)

  endforeach

endif
