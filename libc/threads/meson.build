#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2024 Keith Packard
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

# ISO C threads, built on POSIX threads

srcs_threads = [
  'cnd_broadcast.c',
  'cnd_destroy.c',
  'cnd_init.c',
  'cnd_signal.c',
  'cnd_timedwait.c',
  'cnd_wait.c',
  'mtx_destroy.c',
  'mtx_init.c',
  'mtx_lock.c',
  'mtx_timedlock.c',
  'mtx_trylock.c',
  'mtx_unlock.c',
  'thrd_create.c',
  'thrd_current.c',
  'thrd_detach.c',
  'thrd_equal.c',
  'thrd_exit.c',
  'thrd_join.c',
  'thrd_sleep.c',
  'thrd_yield.c',
  'tss_create.c',
  'tss_delete.c',
  'tss_get.c',
  'tss_set.c',
]

srcs_threads_use = []
foreach file : srcs_threads
  s_file = fs.replace_suffix(file, '.S')
  if file in srcs_machine
    message('libc/threads/' + file + ': machine overrides generic')
  elif s_file in srcs_machine
    message('libc/threads/' + s_file + ': machine overrides generic')
  elif file in srcs_libos
    message('libc/threads/' + file + ': libos overrides generic')
  else
    srcs_threads_use += file
  endif
endforeach

src_threads = files(srcs_threads_use)
