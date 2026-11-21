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

/*
 * The realtime profiles follow from their members: claim everything the
 * multi-purpose profile (PSE54) needs of the OS, as <sys/_posix.h>
 * would, and check that all four profiles result. picolibc supplies
 * the rest (asynchronous I/O and thread-safe functions need its
 * threads).
 */

#define _POSIX_C_SOURCE 202405L

/* Without the system support this test needs, it is skipped */
#if defined(__has_include)
#if __has_include(<picolibc.h>)
#include <picolibc.h>
#endif
#if __has_include(<sys/_posix.h>)
#include <sys/_posix.h>
#endif
#endif
#if defined(__PICOLIBC_PTHREADS) && !defined(__SINGLE_THREAD)
#define CLAIM 1
#ifndef __PICOLIBC_OS_CLOCK_SELECTION
#define __PICOLIBC_OS_CLOCK_SELECTION 202405L
#endif
#ifndef __PICOLIBC_OS_FSYNC
#define __PICOLIBC_OS_FSYNC 202405L
#endif
#ifndef __PICOLIBC_OS_MEMLOCK
#define __PICOLIBC_OS_MEMLOCK 202405L
#endif
#ifndef __PICOLIBC_OS_MEMLOCK_RANGE
#define __PICOLIBC_OS_MEMLOCK_RANGE 202405L
#endif
#ifndef __PICOLIBC_OS_MONOTONIC_CLOCK
#define __PICOLIBC_OS_MONOTONIC_CLOCK 202405L
#endif
#ifndef __PICOLIBC_OS_REALTIME_SIGNALS
#define __PICOLIBC_OS_REALTIME_SIGNALS 202405L
#endif
#ifndef __PICOLIBC_OS_SEMAPHORES
#define __PICOLIBC_OS_SEMAPHORES 202405L
#endif
#ifndef __PICOLIBC_OS_SHARED_MEMORY_OBJECTS
#define __PICOLIBC_OS_SHARED_MEMORY_OBJECTS 202405L
#endif
#ifndef __PICOLIBC_OS_SYNCHRONIZED_IO
#define __PICOLIBC_OS_SYNCHRONIZED_IO 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_ATTR_STACKADDR
#define __PICOLIBC_OS_THREAD_ATTR_STACKADDR 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_ATTR_STACKSIZE
#define __PICOLIBC_OS_THREAD_ATTR_STACKSIZE 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_CPUTIME
#define __PICOLIBC_OS_THREAD_CPUTIME 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_PRIO_INHERIT
#define __PICOLIBC_OS_THREAD_PRIO_INHERIT 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_PRIO_PROTECT
#define __PICOLIBC_OS_THREAD_PRIO_PROTECT 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_PRIORITY_SCHEDULING
#define __PICOLIBC_OS_THREAD_PRIORITY_SCHEDULING 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_SPORADIC_SERVER
#define __PICOLIBC_OS_THREAD_SPORADIC_SERVER 202405L
#endif
#ifndef __PICOLIBC_OS_THREADS
#define __PICOLIBC_OS_THREADS 202405L
#endif
#ifndef __PICOLIBC_OS_TIMEOUTS
#define __PICOLIBC_OS_TIMEOUTS 202405L
#endif
#ifndef __PICOLIBC_OS_TIMERS
#define __PICOLIBC_OS_TIMERS 202405L
#endif
#ifndef __PICOLIBC_OS_MAPPED_FILES
#define __PICOLIBC_OS_MAPPED_FILES 202405L
#endif
#ifndef __PICOLIBC_OS_MESSAGE_PASSING
#define __PICOLIBC_OS_MESSAGE_PASSING 202405L
#endif
#ifndef __PICOLIBC_OS_CPUTIME
#define __PICOLIBC_OS_CPUTIME 202405L
#endif
#ifndef __PICOLIBC_OS_MEMORY_PROTECTION
#define __PICOLIBC_OS_MEMORY_PROTECTION 202405L
#endif
#ifndef __PICOLIBC_OS_PRIORITIZED_IO
#define __PICOLIBC_OS_PRIORITIZED_IO 202405L
#endif
#ifndef __PICOLIBC_OS_PRIORITY_SCHEDULING
#define __PICOLIBC_OS_PRIORITY_SCHEDULING 202405L
#endif
#ifndef __PICOLIBC_OS_RAW_SOCKETS
#define __PICOLIBC_OS_RAW_SOCKETS 202405L
#endif
#ifndef __PICOLIBC_OS_SPORADIC_SERVER
#define __PICOLIBC_OS_SPORADIC_SERVER 202405L
#endif
#ifndef __PICOLIBC_OS_THREAD_PROCESS_SHARED
#define __PICOLIBC_OS_THREAD_PROCESS_SHARED 202405L
#endif
#ifndef __PICOLIBC_OS_ADVISORY_INFO
#define __PICOLIBC_OS_ADVISORY_INFO 202405L
#endif
#ifndef __PICOLIBC_OS_DEVICE_IO
#define __PICOLIBC_OS_DEVICE_IO 202405L
#endif
#ifndef __PICOLIBC_OS_SIGNALS
#define __PICOLIBC_OS_SIGNALS 202405L
#endif
#ifndef __PICOLIBC_OS_SINGLE_PROCESS
#define __PICOLIBC_OS_SINGLE_PROCESS 202405L
#endif
#ifndef __PICOLIBC_OS_FD_MGMT
#define __PICOLIBC_OS_FD_MGMT 202405L
#endif
#ifndef __PICOLIBC_OS_FILE_SYSTEM
#define __PICOLIBC_OS_FILE_SYSTEM 202405L
#endif
#ifndef __PICOLIBC_OS_MULTI_PROCESS
#define __PICOLIBC_OS_MULTI_PROCESS 202405L
#endif
#ifndef __PICOLIBC_OS_NETWORKING
#define __PICOLIBC_OS_NETWORKING 202405L
#endif
#ifndef __PICOLIBC_OS_PIPE
#define __PICOLIBC_OS_PIPE 202405L
#endif
#ifndef __PICOLIBC_OS_DEVICE_SPECIFIC
#define __PICOLIBC_OS_DEVICE_SPECIFIC 202405L
#endif
#ifndef __PICOLIBC_OS_DYNAMIC_LINKING
#define __PICOLIBC_OS_DYNAMIC_LINKING 202405L
#endif
#ifndef __PICOLIBC_OS_FIFO
#define __PICOLIBC_OS_FIFO 202405L
#endif
#ifndef __PICOLIBC_OS_FILE_ATTRIBUTES
#define __PICOLIBC_OS_FILE_ATTRIBUTES 202405L
#endif
#ifndef __PICOLIBC_OS_FILE_SYSTEM_EXT
#define __PICOLIBC_OS_FILE_SYSTEM_EXT 202405L
#endif
#ifndef __PICOLIBC_OS_SYMBOLIC_LINKS
#define __PICOLIBC_OS_SYMBOLIC_LINKS 202405L
#endif
#ifndef __PICOLIBC_OS_USER_GROUPS
#define __PICOLIBC_OS_USER_GROUPS 202405L
#endif
#ifndef __PICOLIBC_OS_XSI_SYSTEM_LOGGING
#define __PICOLIBC_OS_XSI_SYSTEM_LOGGING 202405L
#endif
#ifndef __PICOLIBC_OS_CHOWN_RESTRICTED
#define __PICOLIBC_OS_CHOWN_RESTRICTED 1
#endif
#ifndef __PICOLIBC_OS_JOB_CONTROL
#define __PICOLIBC_OS_JOB_CONTROL 1
#endif
#ifndef __PICOLIBC_OS_NO_TRUNC
#define __PICOLIBC_OS_NO_TRUNC 1
#endif
#ifndef __PICOLIBC_OS_SAVED_IDS
#define __PICOLIBC_OS_SAVED_IDS 1
#endif
#ifndef __PICOLIBC_OS_SHELL
#define __PICOLIBC_OS_SHELL 1
#endif
#ifndef __PICOLIBC_OS_VDISABLE
#define __PICOLIBC_OS_VDISABLE 0
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef CLAIM

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    check(_POSIX_AEP_REALTIME_MULTI == 200312L, "multi-purpose");
    check(_POSIX_AEP_REALTIME_DEDICATED == 200312L, "dedicated");
    check(_POSIX_AEP_REALTIME_CONTROLLER == 200312L, "controller");
    check(_POSIX_AEP_REALTIME_MINIMAL == 200312L, "minimal");

    /* picolibc's share */
    check(_POSIX_THREAD_SAFE_FUNCTIONS > 0, "thread-safe functions");
    check(_POSIX_ASYNCHRONOUS_IO > 0, "asynchronous I/O");
    check(_POSIX_REGEXP > 0, "regular expressions");
    check(_POSIX_SPAWN > 0, "spawn");

    /* Without all of POSIX.1, the system is a subprofile of POSIX.1-2024 */
#ifdef _POSIX_SUBPROFILE
    check(_POSIX_VERSION == 202405L, "a subprofile names the edition it profiles");
#endif
    printf("_POSIX_VERSION %ld\n", (long)_POSIX_VERSION);
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
