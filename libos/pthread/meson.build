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

# POSIX threads code shared by the OS layers which provide them. Each
# OS layer builds these sources into its own library.

srcs_os_pthread = []
pthread_machine_srcs = []

if posix_threads
  srcs_os_pthread = [
    'lock.c',
    'pthread_atfork.c',
    '_pthread_all.c',
    '_pthread_atfork.c',
    'pthread_attr_destroy.c',
    'pthread_attr_getdetachstate.c',
    'pthread_attr_getguardsize.c',
    'pthread_attr_getinheritsched.c',
    'pthread_attr_getschedparam.c',
    'pthread_attr_getschedpolicy.c',
    'pthread_attr_getscope.c',
    'pthread_attr_getstack.c',
    'pthread_attr_getstackaddr.c',
    'pthread_attr_getstacksize.c',
    'pthread_attr_init.c',
    'pthread_attr_setdetachstate.c',
    'pthread_attr_setguardsize.c',
    'pthread_attr_setinheritsched.c',
    'pthread_attr_setschedparam.c',
    'pthread_attr_setschedpolicy.c',
    'pthread_attr_setscope.c',
    'pthread_attr_setstack.c',
    'pthread_attr_setstackaddr.c',
    'pthread_attr_setstacksize.c',
    'pthread_barrier_destroy.c',
    'pthread_barrier_init.c',
    'pthread_barrier_wait.c',
    'pthread_barrierattr_destroy.c',
    'pthread_barrierattr_getpshared.c',
    'pthread_barrierattr_init.c',
    'pthread_barrierattr_setpshared.c',
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
    'pthread_detach.c',
    'pthread_equal.c',
    '_pthread_fork_child.c',
    '_pthread_fork_parent.c',
    '_pthread_fork_prepare.c',
    'pthread_getconcurrency.c',
    'pthread_getschedparam.c',
    'pthread_getspecific.c',
    'pthread_join.c',
    'pthread_key_create.c',
    'pthread_key_delete.c',
    '_pthread_keys.c',
    'pthread_mutex_clocklock.c',
    'pthread_mutex_destroy.c',
    'pthread_mutex_getprioceiling.c',
    'pthread_mutex_lock.c',
    '_pthread_mutex_lock_timed.c',
    'pthread_mutex_timedlock.c',
    'pthread_mutex_trylock.c',
    'pthread_mutex_unlock.c',
    '_pthread_mutex_unlock_ceiling.c',
    'pthread_mutexattr_destroy.c',
    'pthread_mutexattr_getprioceiling.c',
    'pthread_mutexattr_getprotocol.c',
    'pthread_mutexattr_getpshared.c',
    'pthread_mutexattr_getrobust.c',
    'pthread_mutexattr_gettype.c',
    'pthread_mutexattr_setprotocol.c',
    'pthread_mutexattr_setpshared.c',
    'pthread_mutexattr_setrobust.c',
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
    'pthread_setcancelstate.c',
    'pthread_setcanceltype.c',
    'pthread_setconcurrency.c',
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
    'call_once.c',
    'thrd_create.c',
  ]

  # The thread pointer setup for new threads depends only on the machine
  pthread_machine_tls = 'machine' / host_cpu_family / 'pthread_tls_setup.c'
  if fs.is_file(pthread_machine_tls)
    pthread_machine_srcs += pthread_machine_tls
  endif

  # The types and limits behind picolibc's thread headers
  if really_install
    install_headers('include/sys/_pthread.h', install_dir: include_dir / 'sys')
  endif
endif

src_os_pthread = files(srcs_os_pthread + pthread_machine_srcs)

pthread_inc = include_directories('.')
