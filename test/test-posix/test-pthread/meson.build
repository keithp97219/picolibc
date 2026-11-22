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
  'test-pthread_h_sync',
  'test-pthread_h_threads',
]

# These use threads, so lock-valid.c doesn't apply
dir_thread_tests = [
  'test-pthread_atfork',
  'test-pthread_attr_setguardsize',
  'test-pthread_attr_setstackaddr',
  'test-pthread_barrier_wait',
  'test-pthread_cancel',
  'test-pthread_cond_timedwait',
  'test-pthread_cond_wait',
  'test-pthread_condattr_setpshared',
  'test-pthread_create',
  'test-pthread_exit',
  'test-pthread_getcpuclockid',
  'test-pthread_join',
  'test-pthread_key_create',
  'test-pthread_key_delete',
  'test-pthread_kill',
  'test-pthread_mutex_consistent',
  'test-pthread_mutex_lock',
  'test-pthread_mutex_prio_inherit',
  'test-pthread_mutex_prio_protect',
  'test-pthread_mutex_timedlock',
  'test-pthread_mutexattr_setprotocol',
  'test-pthread_once',
  'test-pthread_rwlock_rdlock',
  'test-pthread_rwlock_timedwrlock',
  'test-pthread_setschedparam',
  'test-pthread_sigmask',
  'test-pthread_spin_lock',
  'test-pthread_testcancel',
]

# QEMU's user-mode emulation has no robust lists, and glibc's robust
# mutexes then wait forever for an owner that has died
if host_cpu_family not in ['x86', 'x86_64']
  test_skip_native += ['test-pthread_mutex_consistent']
endif
