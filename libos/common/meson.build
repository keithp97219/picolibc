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

# Code shared by the OS layers. Each one lists the files it uses in its
# own meson.build and builds them into its library, with its own
# directory first on the include path, so "local-os.h" (which this
# directory doesn't have) maps the neutral OS_SYS_*, OS_* and __os_*
# names to that kernel's. Some files build an older interface on a
# newer one (stat on fstatat, say), for an OS layer to use when it has
# no better way.

srcs_os_common = [
  '_Fork.c',
  '_exit.c',
  '_syscall.c',
  'access.c',
  'chdir.c',
  'chmod.c',
  'close.c',
  'closedir.c',
  'dup.c',
  'execve.c',
  'faccessat.c',
  'fchdir.c',
  'fchmod.c',
  'fchownat.c',
  'fdatasync.c',
  'fdopendir.c',
  'fork.c',
  'fsync.c',
  'getegid.c',
  'geteuid.c',
  'getgid.c',
  'getpgid.c',
  'getpgrp.c',
  'getpid.c',
  'getppid.c',
  'getresgid.c',
  'getresuid.c',
  'getsid.c',
  'getuid.c',
  'iob_stderr.c',
  'iob_stdin.c',
  'iob_stdout.c',
  'ioctl.c',
  'isatty.c',
  'link.c',
  'linkat.c',
  'lstat.c',
  'mkdir.c',
  'mkdirat.c',
  'mlock.c',
  'msgget.c',
  'munlock.c',
  'opendir.c',
  'pipe.c',
  'posix_getdents.c',
  'read.c',
  'readdir.c',
  'remove.c',
  'rename.c',
  'renameat.c',
  'rewinddir.c',
  'rmdir.c',
  'sched_getscheduler.c',
  'sched_yield.c',
  'seekdir.c',
  'semget.c',
  'setgroups.c',
  'setpgid.c',
  'setregid.c',
  'setresgid.c',
  'setresuid.c',
  'setreuid.c',
  'setsid.c',
  'shmat.c',
  'shmdt.c',
  'shmget.c',
  'sigtimedwait.c',
  'stat.c',
  'symlink.c',
  'symlinkat.c',
  'sync.c',
  'tcgetpgrp.c',
  'tcgetsid.c',
  'telldir.c',
  'timer_delete.c',
  'timer_getoverrun.c',
  'timer_gettime.c',
  'umask.c',
  'unlink.c',
  'vfork.c',
  'wait3.c',
  'waitpid.c',
  'write.c',
]

src_os_common = {}
foreach file : srcs_os_common
  src_os_common += { file : files(file) }
endforeach
