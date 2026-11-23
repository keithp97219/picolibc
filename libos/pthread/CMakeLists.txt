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
# CMake build has no OS layer with threads itself; an OS layer
# elsewhere (Zephyr's picolibc POSIX support, for instance) supplies
# local-pthread-os.h and its OS headers (see local-pthread.h for that
# contract) and links the os-pthread library defined below, or
# compiles these sources into a library of its own.

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
  ${CMAKE_CURRENT_SOURCE_DIR}/_pthread_stack_min.c
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

# <aio.h> and ISO C <threads.h> on these threads, for an OS layer
# which builds them too
set(PICOLIBC_PTHREAD_AIO_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/__aio_notify.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/__aio_signal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/__aio_submit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/aio_cancel.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/aio_error.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/aio_fsync.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/aio_read.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/aio_return.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/aio_suspend.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/aio_write.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aio/lio_listio.c
  CACHE INTERNAL "libos/pthread aio sources")

set(PICOLIBC_PTHREAD_THREADS_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/cnd_broadcast.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/cnd_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/cnd_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/cnd_signal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/cnd_timedwait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/cnd_wait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/mtx_destroy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/mtx_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/mtx_lock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/mtx_timedlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/mtx_trylock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/mtx_unlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/thrd_current.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/thrd_detach.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/thrd_equal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/thrd_exit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/thrd_join.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/thrd_sleep.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/thrd_yield.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/tss_create.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/tss_delete.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/tss_get.c
  ${CMAKE_CURRENT_SOURCE_DIR}/threads/tss_set.c
  CACHE INTERNAL "libos/pthread ISO C threads sources")

set(PICOLIBC_PTHREAD_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  CACHE INTERNAL "libos/pthread include directories")

# The per-machine thread pointer setup, for OS layers whose core
# places TLS itself
set(PICOLIBC_PTHREAD_MACHINE_SOURCES)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/machine/${CMAKE_SYSTEM_PROCESSOR}/pthread_tls_setup.c)
  list(APPEND PICOLIBC_PTHREAD_MACHINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/machine/${CMAKE_SYSTEM_PROCESSOR}/pthread_tls_setup.c)
endif()

# The thread core as a library, as the meson OS layers build it into
# theirs. Its OS half comes from an INTERFACE library the OS layer
# defines, named picolibc-pthread-os, whose include directories hold
# local-pthread-os.h and the OS headers (<sys/_posix.h>, and its own
# <sys/_pthread.h> if it has one); they come ahead of picolibc's. The
# OS layer links os-pthread with its own code, which provides the
# hooks. Target names resolve when the build is generated, so the OS
# layer may be configured before or after picolibc (Zephyr's lib/
# comes before its modules). Nothing builds os-pthread unless
# something links it, so a build without an OS layer is unaffected.
add_library(os-pthread-headers INTERFACE)
target_include_directories(os-pthread-headers SYSTEM INTERFACE
  ${PICOLIBC_PTHREAD_INCLUDE_DIRECTORIES}
  ${PICOLIBC_INCLUDE_DIRECTORIES})

add_library(os-pthread STATIC EXCLUDE_FROM_ALL
  ${PICOLIBC_PTHREAD_SOURCES}
  ${PICOLIBC_PTHREAD_AIO_SOURCES}
  ${PICOLIBC_PTHREAD_THREADS_SOURCES}
  ${PICOLIBC_PTHREAD_MACHINE_SOURCES})
target_compile_options(os-pthread PRIVATE ${PICOLIBC_COMPILE_OPTIONS})
target_link_libraries(os-pthread PRIVATE picolibc-pthread-os os-pthread-headers)
