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
  'test-access',
  'test-alarm',
  'test-chown',
  'test-confstr',
  'test-dup3',
  'test-execle',
  'test-faccessat',
  'test-fchdir',
  'test-fchown',
  'test-fchownat',
  'test-fdatasync',
  'test-fexecve',
  'test-fsync',
  'test-ftruncate',
  'test-getcwd',
  'test-getgid',
  'test-getgroups',
  'test-gethostname',
  'test-getlogin_r',
  'test-getpgid',
  'test-getsid',
  'test-getuid',
  'test-lchown',
  'test-link',
  'test-linkat',
  'test-lockf',
  'test-pause',
  'test-pipe2',
  'test-posix-options',
  'test-posix_close',
  'test-pread',
  'test-pwrite',
  'test-readlinkat',
  'test-setegid',
  'test-seteuid',
  'test-setgid',
  'test-setsid',
  'test-setuid',
  'test-symlinkat',
  'test-sysconf',
  'test-truncate',
  'test-unlinkat',
]

if oslib_test_variant == 'linux'
  test_skip += ['test-ftruncate']
endif

# glibc's sysconf disagrees with its <unistd.h> about robust mutexes
test_skip_native += ['test-sysconf']
