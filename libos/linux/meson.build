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
    'access.c',
    'alarm.c',
    'cfgetispeed.c',
    'cfgetospeed.c',
    'cfsetispeed.c',
    'cfsetospeed.c',
    'cfsetspeed.c',
    'chdir.c',
    'chmod.c',
    'clock_gettime.c',
    'clock_getres.c',
    'clock_nanosleep.c',
    'clock_settime.c',
    'close.c',
    'closedir.c',
    'dup.c',
    'dup2.c',
    'execve.c',
    '_exit.c',
    'fcntl.c',
    'fork.c',
    'fpathconf.c',
    'fstat.c',
    'getcwd.c',
    'getegid.c',
    'getentropy.c',
    'geteuid.c',
    'getgid.c',
    'getgroups.c',
    'gethostname.c',
    'getitimer.c',
    'getpgrp.c',
    'getpid.c',
    'getppid.c',
    'getrlimit.c',
    'gettimeofday.c',
    'getuid.c',
    'getwd.c',
    'iob_stderr.c',
    'iob_stdin.c',
    'iob_stdout.c',
    'ioctl.c',
    'isatty.c',
    'kill.c',
    'killpg.c',
    'link.c',
    'lseek.c',
    'lstat.c',
    'madvise.c',
    'madvise-gnu.c',
    'mkdir.c',
    'mlockall_munlockall.c',
    'mlock.c',
    'mmap.c',
    'mprotect.c',
    'msync.c',
    'munlock.c',
    'munmap.c',
    'nanosleep.c',
    'open.c',
    'opendir.c',
    'pathconf.c',
    'pipe.c',
    'poll.c',
    'raise.c',
    'read.c',
    'readdir.c',
    'readlink.c',
    'rename.c',
    'rewinddir.c',
    'rmdir.c',
    'sbrk.c',
    'sched_yield.c',
    'setegid.c',
    'setgid.c',
    'setgroups.c',
    'setitimer.c',
    'seteuid.c',
    'setpgid.c',
    'setrlimit.c',
    'setuid.c',
    'shm_open.c',
    'shm_unlink.c',
    'sigaction.c',
    'signal.c',
    '_sigmask_from_linux.c',
    '_sigmask_to_linux.c',
    '_signal_from_linux.c',
    '_signal_to_linux.c',
    '_siginfo_from_linux.c',
    'sigpending.c',
    'sigprocmask.c',
    'sigsuspend.c',
    'stat.c',
    '_statbuf.c',
    'statvfs.c',
    '_statvfsbuf.c',
    'fstatvfs.c',
    'symlink.c',
    '_syscall.c',
    '_syscall_errno.c',
    'tcflow.c',
    'tcflush.c',
    'tcgetattr.c',
    'tcgetpgrp.c',
    'tcgetwinsize.c',
    'tcsetattr.c',
    'tcsetpgrp.c',
    'tcsetwinsize.c',
    'times.c',
    'ttyname.c',
    'ttyname_r.c',
    'umask.c',
    'unlink.c',
    'vfork.c',
    'wait3.c',
    'waitpid.c',
    'write.c',
  ]

  if posix_threads
    srcs_os_linux += [
      '_futex_lock_pi.c',
      '_futex_timespec.c',
      '_futex_unlock_pi.c',
      '_futex_wait.c',
      '_futex_wake.c',
      'lock.c',
      'pthread_attr_destroy.c',
      'pthread_attr_getdetachstate.c',
      'pthread_attr_getguardsize.c',
      'pthread_attr_getinheritsched.c',
      'pthread_attr_getschedparam.c',
      'pthread_attr_getschedpolicy.c',
      'pthread_attr_getscope.c',
      'pthread_attr_getstack.c',
      'pthread_attr_getstacksize.c',
      'pthread_attr_init.c',
      'pthread_attr_setdetachstate.c',
      'pthread_attr_setguardsize.c',
      'pthread_attr_setinheritsched.c',
      'pthread_attr_setschedparam.c',
      'pthread_attr_setschedpolicy.c',
      'pthread_attr_setscope.c',
      'pthread_attr_setstack.c',
      'pthread_attr_setstacksize.c',
      'pthread_barrier_destroy.c',
      'pthread_barrier_init.c',
      'pthread_barrier_wait.c',
      'pthread_barrierattr_destroy.c',
      'pthread_barrierattr_getpshared.c',
      'pthread_barrierattr_init.c',
      'pthread_barrierattr_setpshared.c',
      'pthread_cancel.c',
      '_pthread_cancel_act.c',
      '_pthread_cleanup_exit.c',
      '__pthread_cleanup_pop.c',
      '__pthread_cleanup_push.c',
      'pthread_cond_broadcast.c',
      'pthread_cond_clockwait.c',
      'pthread_cond_destroy.c',
      'pthread_cond_init.c',
      'pthread_cond_signal.c',
      'pthread_cond_timedwait.c',
      'pthread_cond_wait.c',
      '_pthread_cond_wait_timed.c',
      'pthread_condattr_destroy.c',
      'pthread_condattr_getclock.c',
      'pthread_condattr_getpshared.c',
      'pthread_condattr_init.c',
      'pthread_condattr_setclock.c',
      'pthread_condattr_setpshared.c',
      'pthread_create.c',
      'pthread_detach.c',
      'pthread_equal.c',
      'pthread_exit.c',
      'pthread_getconcurrency.c',
      '_pthread_getsched.c',
      'pthread_getschedparam.c',
      'pthread_getspecific.c',
      'pthread_join.c',
      'pthread_key_create.c',
      'pthread_key_delete.c',
      '_pthread_keys.c',
      'pthread_kill.c',
      'pthread_mutex_clocklock.c',
      'pthread_mutex_destroy.c',
      'pthread_mutex_getprioceiling.c',
      'pthread_mutex_init.c',
      'pthread_mutex_lock.c',
      '_pthread_mutex_lock_timed.c',
      'pthread_mutex_setprioceiling.c',
      'pthread_mutex_timedlock.c',
      'pthread_mutex_trylock.c',
      'pthread_mutex_unlock.c',
      '_pthread_mutex_unlock_ceiling.c',
      'pthread_mutexattr_destroy.c',
      'pthread_mutexattr_getprioceiling.c',
      'pthread_mutexattr_getprotocol.c',
      'pthread_mutexattr_getpshared.c',
      'pthread_mutexattr_gettype.c',
      'pthread_mutexattr_init.c',
      'pthread_mutexattr_setprioceiling.c',
      'pthread_mutexattr_setprotocol.c',
      'pthread_mutexattr_setpshared.c',
      'pthread_mutexattr_settype.c',
      'pthread_once.c',
      '_pthread_reap.c',
      'pthread_rwlock_clockrdlock.c',
      'pthread_rwlock_clockwrlock.c',
      'pthread_rwlock_destroy.c',
      'pthread_rwlock_init.c',
      '_pthread_rwlock_lock_timed.c',
      'pthread_rwlock_rdlock.c',
      'pthread_rwlock_timedrdlock.c',
      'pthread_rwlock_timedwrlock.c',
      'pthread_rwlock_tryrdlock.c',
      'pthread_rwlock_trywrlock.c',
      'pthread_rwlock_unlock.c',
      'pthread_rwlock_wrlock.c',
      'pthread_rwlockattr_destroy.c',
      'pthread_rwlockattr_getpshared.c',
      'pthread_rwlockattr_init.c',
      'pthread_rwlockattr_setpshared.c',
      'pthread_self.c',
      'pthread_setcancelstate.c',
      'pthread_setcanceltype.c',
      'pthread_setconcurrency.c',
      '_pthread_setsched.c',
      'pthread_setschedparam.c',
      'pthread_setschedprio.c',
      'pthread_setspecific.c',
      'pthread_sigmask.c',
      '_pthread_specific_exit.c',
      'pthread_spin_destroy.c',
      'pthread_spin_init.c',
      'pthread_spin_lock.c',
      'pthread_spin_trylock.c',
      'pthread_spin_unlock.c',
      'pthread_testcancel.c',
      '_pthread_tpp_lower.c',
      '_pthread_tpp_raise.c',
      '_pthread_wait_exit.c',
      '_pthread_zombie.c',
      '_pthread_zombies.c',
      'sem_clockwait.c',
      'sem_close.c',
      'sem_destroy.c',
      'sem_getvalue.c',
      'sem_init.c',
      '_sem_named.c',
      'sem_open.c',
      'sem_post.c',
      'sem_timedwait.c',
      'sem_trywait.c',
      'sem_unlink.c',
      'sem_wait.c',
      '_sem_wait_timed.c',
    ]
  endif

  subdir(machine_dir)

  src_os_linux = files(srcs_os_linux) + src_os_linux_machine

  srcs_libos += srcs_os_linux

  linux_inc = include_directories(machine_dir, 'include')

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
			    include_directories : [linux_inc, inc],
			    c_args : target_c_args + c_args)
    else
      target_lib_prefix = params['lib_prefix']
      _lib = static_library(join_paths(target_dir, target_lib_prefix + liblinux_name),
			    src_os_linux,
			    install : really_install,
			    install_dir : instdir,
			    include_directories : [linux_inc, inc],
			    c_args : target_c_args + c_args)
    endif

    set_variable('lib_linux' + target, _lib)
    
  endforeach

endif
