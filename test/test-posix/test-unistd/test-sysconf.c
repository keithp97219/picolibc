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

#define _POSIX_C_SOURCE 202405L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../posix-groups.h"

/* sysconf reports the options <unistd.h> defines, and system limits */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/*
 * An option defined in <unistd.h> must be reported with its value; one
 * left undefined may still be supported at run time (glibc), so only
 * picolibc is held to -1 there.
 */
#ifdef __PICOLIBC__
#define UNDEFINED(sc) check(sysconf(sc) == -1, #sc " undefined")
#else
#define UNDEFINED(sc) ((void)0)
#endif

int
main(void)
{
    errno = 0;
#if _POSIX_JOB_CONTROL > 0
    check(sysconf(_SC_JOB_CONTROL) == _POSIX_JOB_CONTROL, "_SC_JOB_CONTROL");
#elif !defined(_POSIX_JOB_CONTROL)
    UNDEFINED(_SC_JOB_CONTROL);
#endif
#if _POSIX_SAVED_IDS > 0
    check(sysconf(_SC_SAVED_IDS) == _POSIX_SAVED_IDS, "_SC_SAVED_IDS");
#elif !defined(_POSIX_SAVED_IDS)
    UNDEFINED(_SC_SAVED_IDS);
#endif
#if _POSIX_VERSION > 0
    check(sysconf(_SC_VERSION) == _POSIX_VERSION, "_SC_VERSION");
#elif !defined(_POSIX_VERSION)
    UNDEFINED(_SC_VERSION);
#endif
#if _POSIX_ASYNCHRONOUS_IO > 0
    check(sysconf(_SC_ASYNCHRONOUS_IO) == _POSIX_ASYNCHRONOUS_IO, "_SC_ASYNCHRONOUS_IO");
#elif !defined(_POSIX_ASYNCHRONOUS_IO)
    UNDEFINED(_SC_ASYNCHRONOUS_IO);
#endif
#if _POSIX_FSYNC > 0
    check(sysconf(_SC_FSYNC) == _POSIX_FSYNC, "_SC_FSYNC");
#elif !defined(_POSIX_FSYNC)
    UNDEFINED(_SC_FSYNC);
#endif
#if _POSIX_MAPPED_FILES > 0
    check(sysconf(_SC_MAPPED_FILES) == _POSIX_MAPPED_FILES, "_SC_MAPPED_FILES");
#elif !defined(_POSIX_MAPPED_FILES)
    UNDEFINED(_SC_MAPPED_FILES);
#endif
#if _POSIX_MEMLOCK > 0
    check(sysconf(_SC_MEMLOCK) == _POSIX_MEMLOCK, "_SC_MEMLOCK");
#elif !defined(_POSIX_MEMLOCK)
    UNDEFINED(_SC_MEMLOCK);
#endif
#if _POSIX_MEMLOCK_RANGE > 0
    check(sysconf(_SC_MEMLOCK_RANGE) == _POSIX_MEMLOCK_RANGE, "_SC_MEMLOCK_RANGE");
#elif !defined(_POSIX_MEMLOCK_RANGE)
    UNDEFINED(_SC_MEMLOCK_RANGE);
#endif
#if _POSIX_MEMORY_PROTECTION > 0
    check(sysconf(_SC_MEMORY_PROTECTION) == _POSIX_MEMORY_PROTECTION, "_SC_MEMORY_PROTECTION");
#elif !defined(_POSIX_MEMORY_PROTECTION)
    UNDEFINED(_SC_MEMORY_PROTECTION);
#endif
#if _POSIX_MESSAGE_PASSING > 0
    check(sysconf(_SC_MESSAGE_PASSING) == _POSIX_MESSAGE_PASSING, "_SC_MESSAGE_PASSING");
#elif !defined(_POSIX_MESSAGE_PASSING)
    UNDEFINED(_SC_MESSAGE_PASSING);
#endif
#if _POSIX_PRIORITIZED_IO > 0
    check(sysconf(_SC_PRIORITIZED_IO) == _POSIX_PRIORITIZED_IO, "_SC_PRIORITIZED_IO");
#elif !defined(_POSIX_PRIORITIZED_IO)
    UNDEFINED(_SC_PRIORITIZED_IO);
#endif
#if _POSIX_REALTIME_SIGNALS > 0
    check(sysconf(_SC_REALTIME_SIGNALS) == _POSIX_REALTIME_SIGNALS, "_SC_REALTIME_SIGNALS");
#elif !defined(_POSIX_REALTIME_SIGNALS)
    UNDEFINED(_SC_REALTIME_SIGNALS);
#endif
#if _POSIX_SEMAPHORES > 0
    check(sysconf(_SC_SEMAPHORES) == _POSIX_SEMAPHORES, "_SC_SEMAPHORES");
#elif !defined(_POSIX_SEMAPHORES)
    UNDEFINED(_SC_SEMAPHORES);
#endif
#if _POSIX_SHARED_MEMORY_OBJECTS > 0
    check(sysconf(_SC_SHARED_MEMORY_OBJECTS) == _POSIX_SHARED_MEMORY_OBJECTS,
          "_SC_SHARED_MEMORY_OBJECTS");
#elif !defined(_POSIX_SHARED_MEMORY_OBJECTS)
    UNDEFINED(_SC_SHARED_MEMORY_OBJECTS);
#endif
#if _POSIX_SYNCHRONIZED_IO > 0
    check(sysconf(_SC_SYNCHRONIZED_IO) == _POSIX_SYNCHRONIZED_IO, "_SC_SYNCHRONIZED_IO");
#elif !defined(_POSIX_SYNCHRONIZED_IO)
    UNDEFINED(_SC_SYNCHRONIZED_IO);
#endif
#if _POSIX_TIMERS > 0
    check(sysconf(_SC_TIMERS) == _POSIX_TIMERS, "_SC_TIMERS");
#elif !defined(_POSIX_TIMERS)
    UNDEFINED(_SC_TIMERS);
#endif
#if _POSIX_THREADS > 0
    check(sysconf(_SC_THREADS) == _POSIX_THREADS, "_SC_THREADS");
#elif !defined(_POSIX_THREADS)
    UNDEFINED(_SC_THREADS);
#endif
#if _POSIX_THREAD_ATTR_STACKADDR > 0
    check(sysconf(_SC_THREAD_ATTR_STACKADDR) == _POSIX_THREAD_ATTR_STACKADDR,
          "_SC_THREAD_ATTR_STACKADDR");
#elif !defined(_POSIX_THREAD_ATTR_STACKADDR)
    UNDEFINED(_SC_THREAD_ATTR_STACKADDR);
#endif
#if _POSIX_THREAD_ATTR_STACKSIZE > 0
    check(sysconf(_SC_THREAD_ATTR_STACKSIZE) == _POSIX_THREAD_ATTR_STACKSIZE,
          "_SC_THREAD_ATTR_STACKSIZE");
#elif !defined(_POSIX_THREAD_ATTR_STACKSIZE)
    UNDEFINED(_SC_THREAD_ATTR_STACKSIZE);
#endif
#if _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    check(sysconf(_SC_THREAD_PRIORITY_SCHEDULING) == _POSIX_THREAD_PRIORITY_SCHEDULING,
          "_SC_THREAD_PRIORITY_SCHEDULING");
#elif !defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    UNDEFINED(_SC_THREAD_PRIORITY_SCHEDULING);
#endif
#if _POSIX_THREAD_PRIO_INHERIT > 0
    check(sysconf(_SC_THREAD_PRIO_INHERIT) == _POSIX_THREAD_PRIO_INHERIT,
          "_SC_THREAD_PRIO_INHERIT");
#elif !defined(_POSIX_THREAD_PRIO_INHERIT)
    UNDEFINED(_SC_THREAD_PRIO_INHERIT);
#endif
#if _POSIX_THREAD_PRIO_PROTECT > 0
    check(sysconf(_SC_THREAD_PRIO_PROTECT) == _POSIX_THREAD_PRIO_PROTECT,
          "_SC_THREAD_PRIO_PROTECT");
#elif !defined(_POSIX_THREAD_PRIO_PROTECT)
    UNDEFINED(_SC_THREAD_PRIO_PROTECT);
#endif
#if _POSIX_THREAD_PROCESS_SHARED > 0
    check(sysconf(_SC_THREAD_PROCESS_SHARED) == _POSIX_THREAD_PROCESS_SHARED,
          "_SC_THREAD_PROCESS_SHARED");
#elif !defined(_POSIX_THREAD_PROCESS_SHARED)
    UNDEFINED(_SC_THREAD_PROCESS_SHARED);
#endif
#if _POSIX_THREAD_SAFE_FUNCTIONS > 0
    check(sysconf(_SC_THREAD_SAFE_FUNCTIONS) == _POSIX_THREAD_SAFE_FUNCTIONS,
          "_SC_THREAD_SAFE_FUNCTIONS");
#elif !defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    UNDEFINED(_SC_THREAD_SAFE_FUNCTIONS);
#endif
#if _POSIX_ADVISORY_INFO > 0
    check(sysconf(_SC_ADVISORY_INFO) == _POSIX_ADVISORY_INFO, "_SC_ADVISORY_INFO");
#elif !defined(_POSIX_ADVISORY_INFO)
    UNDEFINED(_SC_ADVISORY_INFO);
#endif
#if _POSIX_BARRIERS > 0
    check(sysconf(_SC_BARRIERS) == _POSIX_BARRIERS, "_SC_BARRIERS");
#elif !defined(_POSIX_BARRIERS)
    UNDEFINED(_SC_BARRIERS);
#endif
#if _POSIX_CLOCK_SELECTION > 0
    check(sysconf(_SC_CLOCK_SELECTION) == _POSIX_CLOCK_SELECTION, "_SC_CLOCK_SELECTION");
#elif !defined(_POSIX_CLOCK_SELECTION)
    UNDEFINED(_SC_CLOCK_SELECTION);
#endif
#if _POSIX_CPUTIME > 0
    check(sysconf(_SC_CPUTIME) == _POSIX_CPUTIME, "_SC_CPUTIME");
#elif !defined(_POSIX_CPUTIME)
    UNDEFINED(_SC_CPUTIME);
#endif
#if _POSIX_IPV6 > 0
    check(sysconf(_SC_IPV6) == _POSIX_IPV6, "_SC_IPV6");
#elif !defined(_POSIX_IPV6)
    UNDEFINED(_SC_IPV6);
#endif
#if _POSIX_MONOTONIC_CLOCK > 0
    check(sysconf(_SC_MONOTONIC_CLOCK) == _POSIX_MONOTONIC_CLOCK, "_SC_MONOTONIC_CLOCK");
#elif !defined(_POSIX_MONOTONIC_CLOCK)
    UNDEFINED(_SC_MONOTONIC_CLOCK);
#endif
#if _POSIX_RAW_SOCKETS > 0
    check(sysconf(_SC_RAW_SOCKETS) == _POSIX_RAW_SOCKETS, "_SC_RAW_SOCKETS");
#elif !defined(_POSIX_RAW_SOCKETS)
    UNDEFINED(_SC_RAW_SOCKETS);
#endif
#if _POSIX_READER_WRITER_LOCKS > 0
    check(sysconf(_SC_READER_WRITER_LOCKS) == _POSIX_READER_WRITER_LOCKS,
          "_SC_READER_WRITER_LOCKS");
#elif !defined(_POSIX_READER_WRITER_LOCKS)
    UNDEFINED(_SC_READER_WRITER_LOCKS);
#endif
#if _POSIX_REGEXP > 0
    check(sysconf(_SC_REGEXP) == _POSIX_REGEXP, "_SC_REGEXP");
#elif !defined(_POSIX_REGEXP)
    UNDEFINED(_SC_REGEXP);
#endif
#if _POSIX_SHELL > 0
    check(sysconf(_SC_SHELL) == _POSIX_SHELL, "_SC_SHELL");
#elif !defined(_POSIX_SHELL)
    UNDEFINED(_SC_SHELL);
#endif
#if _POSIX_SPAWN > 0
    check(sysconf(_SC_SPAWN) == _POSIX_SPAWN, "_SC_SPAWN");
#elif !defined(_POSIX_SPAWN)
    UNDEFINED(_SC_SPAWN);
#endif
#if _POSIX_SPIN_LOCKS > 0
    check(sysconf(_SC_SPIN_LOCKS) == _POSIX_SPIN_LOCKS, "_SC_SPIN_LOCKS");
#elif !defined(_POSIX_SPIN_LOCKS)
    UNDEFINED(_SC_SPIN_LOCKS);
#endif
#if _POSIX_SPORADIC_SERVER > 0
    check(sysconf(_SC_SPORADIC_SERVER) == _POSIX_SPORADIC_SERVER, "_SC_SPORADIC_SERVER");
#elif !defined(_POSIX_SPORADIC_SERVER)
    UNDEFINED(_SC_SPORADIC_SERVER);
#endif
#if _POSIX_THREAD_CPUTIME > 0
    check(sysconf(_SC_THREAD_CPUTIME) == _POSIX_THREAD_CPUTIME, "_SC_THREAD_CPUTIME");
#elif !defined(_POSIX_THREAD_CPUTIME)
    UNDEFINED(_SC_THREAD_CPUTIME);
#endif
#if _POSIX_THREAD_SPORADIC_SERVER > 0
    check(sysconf(_SC_THREAD_SPORADIC_SERVER) == _POSIX_THREAD_SPORADIC_SERVER,
          "_SC_THREAD_SPORADIC_SERVER");
#elif !defined(_POSIX_THREAD_SPORADIC_SERVER)
    UNDEFINED(_SC_THREAD_SPORADIC_SERVER);
#endif
#if _POSIX_TIMEOUTS > 0
    check(sysconf(_SC_TIMEOUTS) == _POSIX_TIMEOUTS, "_SC_TIMEOUTS");
#elif !defined(_POSIX_TIMEOUTS)
    UNDEFINED(_SC_TIMEOUTS);
#endif
#if _POSIX_TRACE > 0
    check(sysconf(_SC_TRACE) == _POSIX_TRACE, "_SC_TRACE");
#elif !defined(_POSIX_TRACE)
    UNDEFINED(_SC_TRACE);
#endif
#if _POSIX_TRACE_EVENT_FILTER > 0
    check(sysconf(_SC_TRACE_EVENT_FILTER) == _POSIX_TRACE_EVENT_FILTER, "_SC_TRACE_EVENT_FILTER");
#elif !defined(_POSIX_TRACE_EVENT_FILTER)
    UNDEFINED(_SC_TRACE_EVENT_FILTER);
#endif
#if _POSIX_TRACE_INHERIT > 0
    check(sysconf(_SC_TRACE_INHERIT) == _POSIX_TRACE_INHERIT, "_SC_TRACE_INHERIT");
#elif !defined(_POSIX_TRACE_INHERIT)
    UNDEFINED(_SC_TRACE_INHERIT);
#endif
#if _POSIX_TRACE_LOG > 0
    check(sysconf(_SC_TRACE_LOG) == _POSIX_TRACE_LOG, "_SC_TRACE_LOG");
#elif !defined(_POSIX_TRACE_LOG)
    UNDEFINED(_SC_TRACE_LOG);
#endif
#if _POSIX_TYPED_MEMORY_OBJECTS > 0
    check(sysconf(_SC_TYPED_MEMORY_OBJECTS) == _POSIX_TYPED_MEMORY_OBJECTS,
          "_SC_TYPED_MEMORY_OBJECTS");
#elif !defined(_POSIX_TYPED_MEMORY_OBJECTS)
    UNDEFINED(_SC_TYPED_MEMORY_OBJECTS);
#endif
#if _POSIX_V7_ILP32_OFF32 > 0
    check(sysconf(_SC_V7_ILP32_OFF32) == _POSIX_V7_ILP32_OFF32, "_SC_V7_ILP32_OFF32");
#elif !defined(_POSIX_V7_ILP32_OFF32)
    UNDEFINED(_SC_V7_ILP32_OFF32);
#endif
#if _POSIX_V7_ILP32_OFFBIG > 0
    check(sysconf(_SC_V7_ILP32_OFFBIG) == _POSIX_V7_ILP32_OFFBIG, "_SC_V7_ILP32_OFFBIG");
#elif !defined(_POSIX_V7_ILP32_OFFBIG)
    UNDEFINED(_SC_V7_ILP32_OFFBIG);
#endif
#if _POSIX_V7_LP64_OFF64 > 0
    check(sysconf(_SC_V7_LP64_OFF64) == _POSIX_V7_LP64_OFF64, "_SC_V7_LP64_OFF64");
#elif !defined(_POSIX_V7_LP64_OFF64)
    UNDEFINED(_SC_V7_LP64_OFF64);
#endif
#if _POSIX_V7_LPBIG_OFFBIG > 0
    check(sysconf(_SC_V7_LPBIG_OFFBIG) == _POSIX_V7_LPBIG_OFFBIG, "_SC_V7_LPBIG_OFFBIG");
#elif !defined(_POSIX_V7_LPBIG_OFFBIG)
    UNDEFINED(_SC_V7_LPBIG_OFFBIG);
#endif
#if _XOPEN_CRYPT > 0
    check(sysconf(_SC_XOPEN_CRYPT) == _XOPEN_CRYPT, "_SC_XOPEN_CRYPT");
#elif !defined(_XOPEN_CRYPT)
    UNDEFINED(_SC_XOPEN_CRYPT);
#endif
#if _XOPEN_ENH_I18N > 0
    check(sysconf(_SC_XOPEN_ENH_I18N) == _XOPEN_ENH_I18N, "_SC_XOPEN_ENH_I18N");
#elif !defined(_XOPEN_ENH_I18N)
    UNDEFINED(_SC_XOPEN_ENH_I18N);
#endif
#if _XOPEN_LEGACY > 0
    check(sysconf(_SC_XOPEN_LEGACY) == _XOPEN_LEGACY, "_SC_XOPEN_LEGACY");
#elif !defined(_XOPEN_LEGACY)
    UNDEFINED(_SC_XOPEN_LEGACY);
#endif
#if _XOPEN_REALTIME > 0
    check(sysconf(_SC_XOPEN_REALTIME) == _XOPEN_REALTIME, "_SC_XOPEN_REALTIME");
#elif !defined(_XOPEN_REALTIME)
    UNDEFINED(_SC_XOPEN_REALTIME);
#endif
#if _POSIX_PRIORITY_SCHEDULING > 0
    check(sysconf(_SC_PRIORITY_SCHEDULING) == _POSIX_PRIORITY_SCHEDULING,
          "_SC_PRIORITY_SCHEDULING");
#elif !defined(_POSIX_PRIORITY_SCHEDULING)
    UNDEFINED(_SC_PRIORITY_SCHEDULING);
#endif
#if _XOPEN_REALTIME_THREADS > 0
    check(sysconf(_SC_XOPEN_REALTIME_THREADS) == _XOPEN_REALTIME_THREADS,
          "_SC_XOPEN_REALTIME_THREADS");
#elif !defined(_XOPEN_REALTIME_THREADS)
    UNDEFINED(_SC_XOPEN_REALTIME_THREADS);
#endif
#if _XOPEN_SHM > 0
    check(sysconf(_SC_XOPEN_SHM) == _XOPEN_SHM, "_SC_XOPEN_SHM");
#elif !defined(_XOPEN_SHM)
    UNDEFINED(_SC_XOPEN_SHM);
#endif
#if _XOPEN_STREAMS > 0
    check(sysconf(_SC_XOPEN_STREAMS) == _XOPEN_STREAMS, "_SC_XOPEN_STREAMS");
#elif !defined(_XOPEN_STREAMS)
    UNDEFINED(_SC_XOPEN_STREAMS);
#endif
#if _XOPEN_UNIX > 0
    check(sysconf(_SC_XOPEN_UNIX) == _XOPEN_UNIX, "_SC_XOPEN_UNIX");
#elif !defined(_XOPEN_UNIX)
    UNDEFINED(_SC_XOPEN_UNIX);
#endif
#if _XOPEN_VERSION > 0
    check(sysconf(_SC_XOPEN_VERSION) == _XOPEN_VERSION, "_SC_XOPEN_VERSION");
#elif !defined(_XOPEN_VERSION)
    UNDEFINED(_SC_XOPEN_VERSION);
#endif
#if _POSIX2_CHAR_TERM > 0
    check(sysconf(_SC_2_CHAR_TERM) == _POSIX2_CHAR_TERM, "_SC_2_CHAR_TERM");
#elif !defined(_POSIX2_CHAR_TERM)
    UNDEFINED(_SC_2_CHAR_TERM);
#endif
#if _POSIX2_C_BIND > 0
    check(sysconf(_SC_2_C_BIND) == _POSIX2_C_BIND, "_SC_2_C_BIND");
#elif !defined(_POSIX2_C_BIND)
    UNDEFINED(_SC_2_C_BIND);
#endif
#if _POSIX2_C_DEV > 0
    check(sysconf(_SC_2_C_DEV) == _POSIX2_C_DEV, "_SC_2_C_DEV");
#elif !defined(_POSIX2_C_DEV)
    UNDEFINED(_SC_2_C_DEV);
#endif
#if _POSIX2_FORT_DEV > 0
    check(sysconf(_SC_2_FORT_DEV) == _POSIX2_FORT_DEV, "_SC_2_FORT_DEV");
#elif !defined(_POSIX2_FORT_DEV)
    UNDEFINED(_SC_2_FORT_DEV);
#endif
#if _POSIX2_FORT_RUN > 0
    check(sysconf(_SC_2_FORT_RUN) == _POSIX2_FORT_RUN, "_SC_2_FORT_RUN");
#elif !defined(_POSIX2_FORT_RUN)
    UNDEFINED(_SC_2_FORT_RUN);
#endif
#if _POSIX2_LOCALEDEF > 0
    check(sysconf(_SC_2_LOCALEDEF) == _POSIX2_LOCALEDEF, "_SC_2_LOCALEDEF");
#elif !defined(_POSIX2_LOCALEDEF)
    UNDEFINED(_SC_2_LOCALEDEF);
#endif
#if _POSIX2_PBS > 0
    check(sysconf(_SC_2_PBS) == _POSIX2_PBS, "_SC_2_PBS");
#elif !defined(_POSIX2_PBS)
    UNDEFINED(_SC_2_PBS);
#endif
#if _POSIX2_PBS_ACCOUNTING > 0
    check(sysconf(_SC_2_PBS_ACCOUNTING) == _POSIX2_PBS_ACCOUNTING, "_SC_2_PBS_ACCOUNTING");
#elif !defined(_POSIX2_PBS_ACCOUNTING)
    UNDEFINED(_SC_2_PBS_ACCOUNTING);
#endif
#if _POSIX2_PBS_CHECKPOINT > 0
    check(sysconf(_SC_2_PBS_CHECKPOINT) == _POSIX2_PBS_CHECKPOINT, "_SC_2_PBS_CHECKPOINT");
#elif !defined(_POSIX2_PBS_CHECKPOINT)
    UNDEFINED(_SC_2_PBS_CHECKPOINT);
#endif
#if _POSIX2_PBS_LOCATE > 0
    check(sysconf(_SC_2_PBS_LOCATE) == _POSIX2_PBS_LOCATE, "_SC_2_PBS_LOCATE");
#elif !defined(_POSIX2_PBS_LOCATE)
    UNDEFINED(_SC_2_PBS_LOCATE);
#endif
#if _POSIX2_PBS_MESSAGE > 0
    check(sysconf(_SC_2_PBS_MESSAGE) == _POSIX2_PBS_MESSAGE, "_SC_2_PBS_MESSAGE");
#elif !defined(_POSIX2_PBS_MESSAGE)
    UNDEFINED(_SC_2_PBS_MESSAGE);
#endif
#if _POSIX2_PBS_TRACK > 0
    check(sysconf(_SC_2_PBS_TRACK) == _POSIX2_PBS_TRACK, "_SC_2_PBS_TRACK");
#elif !defined(_POSIX2_PBS_TRACK)
    UNDEFINED(_SC_2_PBS_TRACK);
#endif
#if _POSIX2_SW_DEV > 0
    check(sysconf(_SC_2_SW_DEV) == _POSIX2_SW_DEV, "_SC_2_SW_DEV");
#elif !defined(_POSIX2_SW_DEV)
    UNDEFINED(_SC_2_SW_DEV);
#endif
#if _POSIX2_UPE > 0
    check(sysconf(_SC_2_UPE) == _POSIX2_UPE, "_SC_2_UPE");
#elif !defined(_POSIX2_UPE)
    UNDEFINED(_SC_2_UPE);
#endif
#if _POSIX2_VERSION > 0
    check(sysconf(_SC_2_VERSION) == _POSIX2_VERSION, "_SC_2_VERSION");
#elif !defined(_POSIX2_VERSION)
    UNDEFINED(_SC_2_VERSION);
#endif
#if _POSIX_THREAD_ROBUST_PRIO_INHERIT > 0
    check(sysconf(_SC_THREAD_ROBUST_PRIO_INHERIT) == _POSIX_THREAD_ROBUST_PRIO_INHERIT,
          "_SC_THREAD_ROBUST_PRIO_INHERIT");
#elif !defined(_POSIX_THREAD_ROBUST_PRIO_INHERIT)
    UNDEFINED(_SC_THREAD_ROBUST_PRIO_INHERIT);
#endif
#if _POSIX_THREAD_ROBUST_PRIO_PROTECT > 0
    check(sysconf(_SC_THREAD_ROBUST_PRIO_PROTECT) == _POSIX_THREAD_ROBUST_PRIO_PROTECT,
          "_SC_THREAD_ROBUST_PRIO_PROTECT");
#elif !defined(_POSIX_THREAD_ROBUST_PRIO_PROTECT)
    UNDEFINED(_SC_THREAD_ROBUST_PRIO_PROTECT);
#endif
#if _XOPEN_UUCP > 0
    check(sysconf(_SC_XOPEN_UUCP) == _XOPEN_UUCP, "_SC_XOPEN_UUCP");
#elif !defined(_XOPEN_UUCP)
    UNDEFINED(_SC_XOPEN_UUCP);
#endif
#if _POSIX_26_VERSION > 0
    check(sysconf(_SC_POSIX_26_VERSION) == _POSIX_26_VERSION, "_SC_POSIX_26_VERSION");
#elif !defined(_POSIX_26_VERSION)
    UNDEFINED(_SC_POSIX_26_VERSION);
#endif
    check(errno == 0, "options leave errno alone");

#if defined(__PICOLIBC_POSIX_SINGLE_PROCESS)
    /* Limits the operating system decides */
    check(sysconf(_SC_OPEN_MAX) == -1 || sysconf(_SC_OPEN_MAX) >= _POSIX_OPEN_MAX, "_SC_OPEN_MAX");
    check(sysconf(_SC_ARG_MAX) == -1 || sysconf(_SC_ARG_MAX) >= _POSIX_ARG_MAX, "_SC_ARG_MAX");
    check(sysconf(_SC_CLK_TCK) > 0, "_SC_CLK_TCK");
    check(sysconf(_SC_PAGESIZE) > 0, "_SC_PAGESIZE");
#endif
    errno = 0;
    check(sysconf(-1) == -1 && errno == EINVAL, "EINVAL");
    printf("success\n");
    return 0;
}
