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
    '_exit.c',
    '_open_flags_from_freebsd.c',
    '_open_flags_to_freebsd.c',
    '_siginfo.c',
    '_sigmask.c',
    '_statbuf.c',
    '_statvfsbuf.c',
    '_syscall.c',
    '_syscall_errno.c',
    'access.c',
    'clock_getres.c',
    'clock_gettime.c',
    'clock_nanosleep.c',
    'clock_settime.c',
    'close.c',
    'closedir.c',
    'fcntl.c',
    'fstat.c',
    'fstatat.c',
    'fstatvfs.c',
    'getegid.c',
    'getentropy.c',
    'geteuid.c',
    'gethostname.c',
    'getpid.c',
    'getrlimit.c',
    'gettimeofday.c',
    'iob_stderr.c',
    'iob_stdin.c',
    'iob_stdout.c',
    'isatty.c',
    'kill.c',
    'lseek.c',
    'lstat.c',
    'mmap.c',
    'munmap.c',
    'nanosleep.c',
    'open.c',
    'openat.c',
    'opendir.c',
    'pipe.c',
    'raise.c',
    'read.c',
    'readdir.c',
    'rename.c',
    'sbrk.c',
    'sigaction.c',
    'signal.c',
    'sigprocmask.c',
    'stat.c',
    'statvfs.c',
    'times.c',
    'uname.c',
    'unlink.c',
    'write.c',
  ]

  subdir(machine_dir)

  src_os_freebsd = files(srcs_os_freebsd) + src_os_freebsd_machine

  srcs_libos += srcs_os_freebsd

  freebsd_inc = include_directories('machine' / freebsd_machines[host_cpu_family], 'include')

  # The POSIX options and option groups this OS layer provides, and
  # its signal stack sizes
  if really_install
    install_headers(['include/sys/_posix.h', 'include/sys/_signal.h'],
                    install_dir: include_dir / 'sys')
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
                            include_directories : [freebsd_inc, inc],
                            c_args : target_c_args + c_args)
    else
      target_lib_prefix = params['lib_prefix']
      _lib = static_library(join_paths(target_dir, target_lib_prefix + libfreebsd_name),
                            src_os_freebsd,
                            install : really_install,
                            install_dir : instdir,
                            include_directories : [freebsd_inc, inc],
                            c_args : target_c_args + c_args)
    endif

    set_variable('lib_freebsd' + target, _lib)

  endforeach

endif
