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

# POSIX threads code shared by the OS layers which provide them. The
# CMake build has no OS layer with threads itself, so nothing is built
# here: an OS layer elsewhere (Zephyr's picolibc POSIX support, for
# instance) compiles these sources into its own library, with its own
# local-pthread-os.h first on the include path; see local-pthread.h
# for that contract. The per-machine pthread_tls_setup.c files under
# machine/ are optional for such a layer.

set(PICOLIBC_PTHREAD_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/call_once.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_all.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_alloc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_atfork.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_atfork.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getdetachstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getguardsize.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getinheritsched.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getschedparam.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getschedpolicy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getscope.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getstack.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getstackaddr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_getstacksize.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setdetachstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setguardsize.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setinheritsched.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setschedparam.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setschedpolicy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setscope.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setstack.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setstackaddr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_attr_setstacksize.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_barrier_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_barrier_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_barrier_wait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_barrierattr_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_barrierattr_getpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_barrierattr_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_barrierattr_setpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cancel.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_cancel_act.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_cleanup_exit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/__pthread_cleanup_pop.c
  ${CMAKE_CURRENT_SOURCE_DIR}/__pthread_cleanup_push.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cond_broadcast.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cond_clockwait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cond_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cond_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cond_signal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cond_timedwait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_cond_wait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_cond_wait_timed.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_condattr_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_condattr_getclock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_condattr_getpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_condattr_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_condattr_setclock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_condattr_setpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_create.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_detach.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_equal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_exit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_fork_child.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_fork_parent.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_fork_prepare.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_fork_reset.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_getconcurrency.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_getschedparam.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_getspecific.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_join.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_key_create.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_key_delete.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_keys.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_kill.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_clocklock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_consistent.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_getprioceiling.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_lock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_mutex_lock_timed.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_setprioceiling.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_timedlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_trylock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutex_unlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_mutex_unlock_ceiling.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_getprioceiling.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_getprotocol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_getpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_getrobust.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_gettype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_setprioceiling.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_setprotocol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_setpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_setrobust.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_mutexattr_settype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_once.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_reap.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_clockrdlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_clockwrlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_rwlock_lock_timed.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_rdlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_timedrdlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_timedwrlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_tryrdlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_trywrlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_unlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_wrlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlockattr_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlockattr_getpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlockattr_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlockattr_setpshared.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_self.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_setcancelstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_setcanceltype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_setconcurrency.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_setschedparam.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_setschedprio.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_setspecific.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_sigmask.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_specific_exit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_spin_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_spin_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_spin_lock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_spin_trylock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_spin_unlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pthread_testcancel.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_tpp_lower.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_tpp_raise.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_wait_exit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_zombie.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_zombies.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_clockwait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_close.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_getvalue.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_sem_named.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_open.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_post.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_timedwait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_trywait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_unlink.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_wait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/_sem_wait_timed.c
  ${CMAKE_CURRENT_SOURCE_DIR}/thrd_create.c
  CACHE INTERNAL "libos/pthread sources")

set(PICOLIBC_PTHREAD_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  CACHE INTERNAL "libos/pthread include directories")
