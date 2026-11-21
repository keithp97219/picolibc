/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 Keith Packard
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <sys/cdefs.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>

#ifndef __weak_reference
#define __fallback_sysconf sysconf
#endif

long
__fallback_sysconf(int name)
{
    switch (name) {
    /* libc/posix/aio's limits; each request is a thread, with no count limit */
    case _SC_AIO_LISTIO_MAX:
        return AIO_LISTIO_MAX;
    case _SC_AIO_MAX:
        return -1;
    case _SC_AIO_PRIO_DELTA_MAX:
        return AIO_PRIO_DELTA_MAX;
    /*
     * Limits <limits.h> defines are reported with that value, which
     * sysconf mustn't contradict; the others are the POSIX minimums
     */
    case _SC_ARG_MAX:
        return ARG_MAX;
    case _SC_ATEXIT_MAX:
        return ATEXIT_MAX;
    case _SC_CHILD_MAX:
        return CHILD_MAX;
    case _SC_DELAYTIMER_MAX:
        return _POSIX_DELAYTIMER_MAX;
    /*
     * A suggested initial size for the getpw*_r and getgr*_r buffers;
     * entries in libc/posix/pwd and grp have no fixed limit
     */
    case _SC_GETGR_R_SIZE_MAX:
    case _SC_GETPW_R_SIZE_MAX:
        return 1024;
    case _SC_HOST_NAME_MAX:
        return _POSIX_HOST_NAME_MAX;
    case _SC_IOV_MAX:
        return IOV_MAX;
    case _SC_LOGIN_NAME_MAX:
        return _POSIX_LOGIN_NAME_MAX;
    case _SC_MQ_OPEN_MAX:
        return _POSIX_MQ_OPEN_MAX;
    case _SC_MQ_PRIO_MAX:
        return _POSIX_MQ_PRIO_MAX;
    case _SC_NPROCESSORS_CONF:
    case _SC_NPROCESSORS_ONLN:
        return 1;
    case _SC_NSIG:
        /* Signal numbers run up to _NSIG - 1 */
        return _NSIG;
    case _SC_OPEN_MAX:
        return OPEN_MAX;
    case _SC_PAGE_SIZE:
        return 4096;
    /* Without threads, these depend on an option that isn't there */
    case _SC_THREAD_DESTRUCTOR_ITERATIONS:
#ifdef PTHREAD_DESTRUCTOR_ITERATIONS
        return PTHREAD_DESTRUCTOR_ITERATIONS;
#else
        return -1;
#endif
    case _SC_THREAD_KEYS_MAX:
#ifdef PTHREAD_KEYS_MAX
        return PTHREAD_KEYS_MAX;
#else
        return -1;
#endif
    case _SC_THREAD_STACK_MIN:
#ifdef PTHREAD_STACK_MIN
        return PTHREAD_STACK_MIN;
#else
        return -1;
#endif
    case _SC_THREAD_THREADS_MAX:
        /* Threads are limited only by memory */
        return -1;
    case _SC_RTSIG_MAX:
        return RTSIG_MAX;
    case _SC_SEM_NSEMS_MAX:
        return _POSIX_SEM_NSEMS_MAX;
    case _SC_SEM_VALUE_MAX:
#ifdef SEM_VALUE_MAX
        return SEM_VALUE_MAX;
#else
        return _POSIX_SEM_VALUE_MAX;
#endif
    case _SC_SIGQUEUE_MAX:
        return _POSIX_SIGQUEUE_MAX;
    case _SC_SS_REPL_MAX:
        return _POSIX_SS_REPL_MAX;
    case _SC_STREAM_MAX:
        /* stdio has no limit of its own beyond the C minimum */
        return FOPEN_MAX;
    case _SC_SYMLOOP_MAX:
        return _POSIX_SYMLOOP_MAX;
    case _SC_TIMER_MAX:
        return _POSIX_TIMER_MAX;
    case _SC_TRACE_EVENT_NAME_MAX:
        return _POSIX_TRACE_EVENT_NAME_MAX;
    case _SC_TRACE_NAME_MAX:
        return _POSIX_TRACE_NAME_MAX;
    case _SC_TRACE_SYS_MAX:
        return _POSIX_TRACE_SYS_MAX;
    case _SC_TRACE_USER_EVENT_MAX:
        return _POSIX_TRACE_USER_EVENT_MAX;
    case _SC_TTY_NAME_MAX:
        return _POSIX_TTY_NAME_MAX;
    case _SC_TZNAME_MAX:
        /* What tzset accepts */
        return TZNAME_MAX;
#if 0
        /* pathconf bits */
    case _PC_FILESIZEBITS:
        return sizeof(off_t) * 8;
    case _PC_LINK_MAX:
        return _POSIX_LINK_MAX;
    case _PC_MAX_CANON:
        return _POSIX_MAX_CANON;
    case _PC_MAX_INPUT:
        return _POSIX_MAX_INPUT;
    case _PC_NAME_MAX:
        return _POSIX_NAME_MAX;
    case _PC_PATH_MAX:
        return _POSIX_PATH_MAX;
    case _PC_PIPE_BUF:
        return _POSIX_PIPE_BUF;
    case _PC_POSIX_ALLOC_SIZE_MIN:
        return 0;
    case _PC_POSIX_REC_INCR_XFER_SIZE:
        return 1;
    case _PC_POSIX_REC_MAX_XFER_SIZE:
        return INT64_MAX;
    case _PC_POSIX_REC_MIN_XFER_SIZE:
        return 0;
    case _PC_POSIX_REC_XFER_ALIGN:
        return 1;
    case _PC_SYMLINK_MAX:
        return _POSIX_SYMLINK_MAX;
#endif
    case _SC_BC_BASE_MAX:
        return BC_BASE_MAX;
    case _SC_BC_DIM_MAX:
        return BC_DIM_MAX;
    case _SC_BC_SCALE_MAX:
        return BC_SCALE_MAX;
    case _SC_BC_STRING_MAX:
        return BC_STRING_MAX;
    case _SC_CHARCLASS_NAME_MAX:
        return CHARCLASS_NAME_MAX;
    case _SC_COLL_WEIGHTS_MAX:
        return COLL_WEIGHTS_MAX;
    case _SC_EXPR_NEST_MAX:
        return EXPR_NEST_MAX;
    case _SC_LINE_MAX:
        return LINE_MAX;
    case _SC_NGROUPS_MAX:
        return NGROUPS_MAX;
    case _SC_RE_DUP_MAX:
        return RE_DUP_MAX;
    /* Options report their <unistd.h> value, or -1 when not supported */
    case _SC_JOB_CONTROL:
#ifdef _POSIX_JOB_CONTROL
        return _POSIX_JOB_CONTROL;
#else
        return -1;
#endif
    case _SC_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
        return _POSIX_SAVED_IDS;
#else
        return -1;
#endif
    case _SC_VERSION:
#ifdef _POSIX_VERSION
        return _POSIX_VERSION;
#else
        return -1;
#endif
    case _SC_ASYNCHRONOUS_IO:
#ifdef _POSIX_ASYNCHRONOUS_IO
        return _POSIX_ASYNCHRONOUS_IO;
#else
        return -1;
#endif
    case _SC_FSYNC:
#ifdef _POSIX_FSYNC
        return _POSIX_FSYNC;
#else
        return -1;
#endif
    case _SC_MAPPED_FILES:
#ifdef _POSIX_MAPPED_FILES
        return _POSIX_MAPPED_FILES;
#else
        return -1;
#endif
    case _SC_MEMLOCK:
#ifdef _POSIX_MEMLOCK
        return _POSIX_MEMLOCK;
#else
        return -1;
#endif
    case _SC_MEMLOCK_RANGE:
#ifdef _POSIX_MEMLOCK_RANGE
        return _POSIX_MEMLOCK_RANGE;
#else
        return -1;
#endif
    case _SC_MEMORY_PROTECTION:
#ifdef _POSIX_MEMORY_PROTECTION
        return _POSIX_MEMORY_PROTECTION;
#else
        return -1;
#endif
    case _SC_MESSAGE_PASSING:
#ifdef _POSIX_MESSAGE_PASSING
        return _POSIX_MESSAGE_PASSING;
#else
        return -1;
#endif
    case _SC_PRIORITIZED_IO:
#ifdef _POSIX_PRIORITIZED_IO
        return _POSIX_PRIORITIZED_IO;
#else
        return -1;
#endif
    case _SC_REALTIME_SIGNALS:
#ifdef _POSIX_REALTIME_SIGNALS
        return _POSIX_REALTIME_SIGNALS;
#else
        return -1;
#endif
    case _SC_SEMAPHORES:
#ifdef _POSIX_SEMAPHORES
        return _POSIX_SEMAPHORES;
#else
        return -1;
#endif
    case _SC_SHARED_MEMORY_OBJECTS:
#ifdef _POSIX_SHARED_MEMORY_OBJECTS
        return _POSIX_SHARED_MEMORY_OBJECTS;
#else
        return -1;
#endif
    case _SC_SYNCHRONIZED_IO:
#ifdef _POSIX_SYNCHRONIZED_IO
        return _POSIX_SYNCHRONIZED_IO;
#else
        return -1;
#endif
    case _SC_TIMERS:
#ifdef _POSIX_TIMERS
        return _POSIX_TIMERS;
#else
        return -1;
#endif
    case _SC_THREADS:
#ifdef _POSIX_THREADS
        return _POSIX_THREADS;
#else
        return -1;
#endif
    case _SC_THREAD_ATTR_STACKADDR:
#ifdef _POSIX_THREAD_ATTR_STACKADDR
        return _POSIX_THREAD_ATTR_STACKADDR;
#else
        return -1;
#endif
    case _SC_THREAD_ATTR_STACKSIZE:
#ifdef _POSIX_THREAD_ATTR_STACKSIZE
        return _POSIX_THREAD_ATTR_STACKSIZE;
#else
        return -1;
#endif
    case _SC_THREAD_PRIORITY_SCHEDULING:
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
        return _POSIX_THREAD_PRIORITY_SCHEDULING;
#else
        return -1;
#endif
    case _SC_THREAD_PRIO_INHERIT:
#ifdef _POSIX_THREAD_PRIO_INHERIT
        return _POSIX_THREAD_PRIO_INHERIT;
#else
        return -1;
#endif
    case _SC_THREAD_PRIO_PROTECT:
#ifdef _POSIX_THREAD_PRIO_PROTECT
        return _POSIX_THREAD_PRIO_PROTECT;
#else
        return -1;
#endif
    case _SC_THREAD_PROCESS_SHARED:
#ifdef _POSIX_THREAD_PROCESS_SHARED
        return _POSIX_THREAD_PROCESS_SHARED;
#else
        return -1;
#endif
    case _SC_THREAD_SAFE_FUNCTIONS:
#ifdef _POSIX_THREAD_SAFE_FUNCTIONS
        return _POSIX_THREAD_SAFE_FUNCTIONS;
#else
        return -1;
#endif
    case _SC_ADVISORY_INFO:
#ifdef _POSIX_ADVISORY_INFO
        return _POSIX_ADVISORY_INFO;
#else
        return -1;
#endif
    case _SC_BARRIERS:
#ifdef _POSIX_BARRIERS
        return _POSIX_BARRIERS;
#else
        return -1;
#endif
    case _SC_CLOCK_SELECTION:
#ifdef _POSIX_CLOCK_SELECTION
        return _POSIX_CLOCK_SELECTION;
#else
        return -1;
#endif
    case _SC_CPUTIME:
#ifdef _POSIX_CPUTIME
        return _POSIX_CPUTIME;
#else
        return -1;
#endif
    case _SC_IPV6:
#ifdef _POSIX_IPV6
        return _POSIX_IPV6;
#else
        return -1;
#endif
    case _SC_MONOTONIC_CLOCK:
#ifdef _POSIX_MONOTONIC_CLOCK
        return _POSIX_MONOTONIC_CLOCK;
#else
        return -1;
#endif
    case _SC_RAW_SOCKETS:
#ifdef _POSIX_RAW_SOCKETS
        return _POSIX_RAW_SOCKETS;
#else
        return -1;
#endif
    case _SC_READER_WRITER_LOCKS:
#ifdef _POSIX_READER_WRITER_LOCKS
        return _POSIX_READER_WRITER_LOCKS;
#else
        return -1;
#endif
    case _SC_REGEXP:
#ifdef _POSIX_REGEXP
        return _POSIX_REGEXP;
#else
        return -1;
#endif
    case _SC_SHELL:
#ifdef _POSIX_SHELL
        return _POSIX_SHELL;
#else
        return -1;
#endif
    case _SC_SPAWN:
#ifdef _POSIX_SPAWN
        return _POSIX_SPAWN;
#else
        return -1;
#endif
    case _SC_SPIN_LOCKS:
#ifdef _POSIX_SPIN_LOCKS
        return _POSIX_SPIN_LOCKS;
#else
        return -1;
#endif
    case _SC_SPORADIC_SERVER:
#ifdef _POSIX_SPORADIC_SERVER
        return _POSIX_SPORADIC_SERVER;
#else
        return -1;
#endif
    case _SC_THREAD_CPUTIME:
#ifdef _POSIX_THREAD_CPUTIME
        return _POSIX_THREAD_CPUTIME;
#else
        return -1;
#endif
    case _SC_THREAD_SPORADIC_SERVER:
#ifdef _POSIX_THREAD_SPORADIC_SERVER
        return _POSIX_THREAD_SPORADIC_SERVER;
#else
        return -1;
#endif
    case _SC_TIMEOUTS:
#ifdef _POSIX_TIMEOUTS
        return _POSIX_TIMEOUTS;
#else
        return -1;
#endif
    case _SC_TRACE:
#ifdef _POSIX_TRACE
        return _POSIX_TRACE;
#else
        return -1;
#endif
    case _SC_TRACE_EVENT_FILTER:
#ifdef _POSIX_TRACE_EVENT_FILTER
        return _POSIX_TRACE_EVENT_FILTER;
#else
        return -1;
#endif
    case _SC_TRACE_INHERIT:
#ifdef _POSIX_TRACE_INHERIT
        return _POSIX_TRACE_INHERIT;
#else
        return -1;
#endif
    case _SC_TRACE_LOG:
#ifdef _POSIX_TRACE_LOG
        return _POSIX_TRACE_LOG;
#else
        return -1;
#endif
    case _SC_TYPED_MEMORY_OBJECTS:
#ifdef _POSIX_TYPED_MEMORY_OBJECTS
        return _POSIX_TYPED_MEMORY_OBJECTS;
#else
        return -1;
#endif
    case _SC_V7_ILP32_OFF32:
#ifdef _POSIX_V7_ILP32_OFF32
        return _POSIX_V7_ILP32_OFF32;
#else
        return -1;
#endif
    case _SC_V7_ILP32_OFFBIG:
#ifdef _POSIX_V7_ILP32_OFFBIG
        return _POSIX_V7_ILP32_OFFBIG;
#else
        return -1;
#endif
    case _SC_V7_LP64_OFF64:
#ifdef _POSIX_V7_LP64_OFF64
        return _POSIX_V7_LP64_OFF64;
#else
        return -1;
#endif
    case _SC_V7_LPBIG_OFFBIG:
#ifdef _POSIX_V7_LPBIG_OFFBIG
        return _POSIX_V7_LPBIG_OFFBIG;
#else
        return -1;
#endif
    case _SC_XOPEN_CRYPT:
#ifdef _XOPEN_CRYPT
        return _XOPEN_CRYPT;
#else
        return -1;
#endif
    case _SC_XOPEN_ENH_I18N:
#ifdef _XOPEN_ENH_I18N
        return _XOPEN_ENH_I18N;
#else
        return -1;
#endif
    case _SC_XOPEN_LEGACY:
#ifdef _XOPEN_LEGACY
        return _XOPEN_LEGACY;
#else
        return -1;
#endif
    case _SC_XOPEN_REALTIME:
#ifdef _XOPEN_REALTIME
        return _XOPEN_REALTIME;
#else
        return -1;
#endif
    case _SC_PRIORITY_SCHEDULING:
#ifdef _POSIX_PRIORITY_SCHEDULING
        return _POSIX_PRIORITY_SCHEDULING;
#else
        return -1;
#endif
    case _SC_XOPEN_REALTIME_THREADS:
#ifdef _XOPEN_REALTIME_THREADS
        return _XOPEN_REALTIME_THREADS;
#else
        return -1;
#endif
    case _SC_XOPEN_SHM:
#ifdef _XOPEN_SHM
        return _XOPEN_SHM;
#else
        return -1;
#endif
    case _SC_XOPEN_STREAMS:
#ifdef _XOPEN_STREAMS
        return _XOPEN_STREAMS;
#else
        return -1;
#endif
    case _SC_XOPEN_UNIX:
#ifdef _XOPEN_UNIX
        return _XOPEN_UNIX;
#else
        return -1;
#endif
    case _SC_XOPEN_VERSION:
#ifdef _XOPEN_VERSION
        return _XOPEN_VERSION;
#else
        return -1;
#endif
    case _SC_2_CHAR_TERM:
#ifdef _POSIX2_CHAR_TERM
        return _POSIX2_CHAR_TERM;
#else
        return -1;
#endif
    case _SC_2_C_BIND:
#ifdef _POSIX2_C_BIND
        return _POSIX2_C_BIND;
#else
        return -1;
#endif
    case _SC_2_C_DEV:
#ifdef _POSIX2_C_DEV
        return _POSIX2_C_DEV;
#else
        return -1;
#endif
    case _SC_2_FORT_DEV:
#ifdef _POSIX2_FORT_DEV
        return _POSIX2_FORT_DEV;
#else
        return -1;
#endif
    case _SC_2_FORT_RUN:
#ifdef _POSIX2_FORT_RUN
        return _POSIX2_FORT_RUN;
#else
        return -1;
#endif
    case _SC_2_LOCALEDEF:
#ifdef _POSIX2_LOCALEDEF
        return _POSIX2_LOCALEDEF;
#else
        return -1;
#endif
    case _SC_2_PBS:
#ifdef _POSIX2_PBS
        return _POSIX2_PBS;
#else
        return -1;
#endif
    case _SC_2_PBS_ACCOUNTING:
#ifdef _POSIX2_PBS_ACCOUNTING
        return _POSIX2_PBS_ACCOUNTING;
#else
        return -1;
#endif
    case _SC_2_PBS_CHECKPOINT:
#ifdef _POSIX2_PBS_CHECKPOINT
        return _POSIX2_PBS_CHECKPOINT;
#else
        return -1;
#endif
    case _SC_2_PBS_LOCATE:
#ifdef _POSIX2_PBS_LOCATE
        return _POSIX2_PBS_LOCATE;
#else
        return -1;
#endif
    case _SC_2_PBS_MESSAGE:
#ifdef _POSIX2_PBS_MESSAGE
        return _POSIX2_PBS_MESSAGE;
#else
        return -1;
#endif
    case _SC_2_PBS_TRACK:
#ifdef _POSIX2_PBS_TRACK
        return _POSIX2_PBS_TRACK;
#else
        return -1;
#endif
    case _SC_2_SW_DEV:
#ifdef _POSIX2_SW_DEV
        return _POSIX2_SW_DEV;
#else
        return -1;
#endif
    case _SC_2_UPE:
#ifdef _POSIX2_UPE
        return _POSIX2_UPE;
#else
        return -1;
#endif
    case _SC_2_VERSION:
#ifdef _POSIX2_VERSION
        return _POSIX2_VERSION;
#else
        return -1;
#endif
    case _SC_THREAD_ROBUST_PRIO_INHERIT:
#ifdef _POSIX_THREAD_ROBUST_PRIO_INHERIT
        return _POSIX_THREAD_ROBUST_PRIO_INHERIT;
#else
        return -1;
#endif
    case _SC_THREAD_ROBUST_PRIO_PROTECT:
#ifdef _POSIX_THREAD_ROBUST_PRIO_PROTECT
        return _POSIX_THREAD_ROBUST_PRIO_PROTECT;
#else
        return -1;
#endif
    case _SC_XOPEN_UUCP:
#ifdef _XOPEN_UUCP
        return _XOPEN_UUCP;
#else
        return -1;
#endif
    case _SC_DEVICE_CONTROL:
#ifdef _POSIX_DEVICE_CONTROL
        return _POSIX_DEVICE_CONTROL;
#else
        return -1;
#endif
    case _SC_POSIX_26_VERSION:
#ifdef _POSIX_26_VERSION
        return _POSIX_26_VERSION;
#else
        return -1;
#endif
    default:
        errno = EINVAL;
        return -1;
    }
}

#ifdef __weak_reference
__weak_reference(__fallback_sysconf, sysconf);
#endif
