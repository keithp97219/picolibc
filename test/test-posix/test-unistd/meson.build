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

dir_tests = [
  'test-_Fork',
  'test-_exit',
  'test-access',
  'test-alarm',
  'test-chown',
  'test-confstr',
  'test-crypt',
  'test-dup2',
  'test-dup3',
  'test-execl',
  'test-execle',
  'test-execvp',
  'test-faccessat',
  'test-fchdir',
  'test-fchown',
  'test-fchownat',
  'test-fdatasync',
  'test-fexecve',
  'test-fork',
  'test-fsync',
  'test-ftruncate',
  'test-getcwd',
  'test-getentropy',
  'test-getgid',
  'test-getgroups',
  'test-gethostid',
  'test-gethostname',
  'test-getlogin',
  'test-getlogin_r',
  'test-getpgid',
  'test-getresgid',
  'test-getresuid',
  'test-getsid',
  'test-getuid',
  'test-isatty',
  'test-lchown',
  'test-link',
  'test-linkat',
  'test-lockf',
  'test-lseek',
  'test-nice',
  'test-optind',
  'test-pathconf',
  'test-pause',
  'test-pipe',
  'test-pipe2',
  'test-posix-options',
  'test-posix-profiles',
  'test-posix_close',
  'test-pread',
  'test-pwrite',
  'test-read',
  'test-readlink',
  'test-readlinkat',
  'test-setegid',
  'test-seteuid',
  'test-setgid',
  'test-setpgid',
  'test-setregid',
  'test-setreuid',
  'test-setsid',
  'test-setuid',
  'test-sleep',
  'test-symlink',
  'test-symlinkat',
  'test-swab',
  'test-sync',
  'test-sysconf',
  'test-sysconf-arg_max',
  'test-sysconf-limits',
  'test-tcsetpgrp',
  'test-truncate',
  'test-ttyname_r',
  'test-unistd_h-config',
  'test-unistd_h-files',
  'test-unistd_h-names',
  'test-unistd_h-process',
  'test-unlink',
  'test-unlinkat',
  'test-write',
]

# These check credentials across threads
dir_thread_tests = [
  'test-execl-thread',
  'test-nice-thread',
  'test-setuid-thread',
]

# glibc's sysconf disagrees with its <unistd.h> about robust mutexes
test_skip_native += ['test-sysconf']
# glibc keeps crypt in libcrypt and no longer links encrypt or setkey
test_skip_native += ['test-crypt']
