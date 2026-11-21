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
 * A system claiming every option and option group the minimal realtime
 * profile (PSE51) needs gets _POSIX_AEP_REALTIME_MINIMAL, computed by
 * <unistd.h> from IEEE 1003.13-2003 Tables 6-1 and 6-2; one without
 * sporadic server, as most systems are, doesn't. Claim them here, as
 * <sys/_posix.h> would, on a system without threads of its own, and
 * check the profile and that it's a subprofile of POSIX.1-2024.
 */

#define _POSIX_C_SOURCE 202405L

/* Without the system support this test needs, it is skipped */
#if defined(__has_include)
#if __has_include(<sys/_posix.h>)
#include <sys/_posix.h>
#endif
#if __has_include(<picolibc.h>) && !defined(__PICOLIBC_OS_THREADS)
#define CLAIM
#ifndef NO_SPORADIC_SERVER
#define __PICOLIBC_OS_THREAD_SPORADIC_SERVER 202405L
#endif
#ifndef __PICOLIBC_OS_NO_TRUNC
#define __PICOLIBC_OS_NO_TRUNC 1
#endif
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
#ifndef __PICOLIBC_OS_THREADS
#define __PICOLIBC_OS_THREADS 202405L
#endif
#ifndef __PICOLIBC_OS_TIMEOUTS
#define __PICOLIBC_OS_TIMEOUTS 202405L
#endif
#ifndef __PICOLIBC_OS_TIMERS
#define __PICOLIBC_OS_TIMERS 202405L
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
#ifdef NO_SPORADIC_SERVER
    /* One option short of PSE51 */
#ifdef _POSIX_AEP_REALTIME_MINIMAL
    check(0, "PSE51 claimed without sporadic server");
#endif
#else
    check(_POSIX_AEP_REALTIME_MINIMAL == 200312L, "PSE51 from its members");
#endif

    /* PSE52 needs more than PSE51's members */
#if defined(_POSIX_AEP_REALTIME_CONTROLLER) && !defined(__PICOLIBC_OS_FILE_SYSTEM)
    check(0, "PSE52 without the file system");
#endif

    /* A subprofile of POSIX.1-2024, without the shell and utilities */
#ifndef _POSIX_SUBPROFILE
    check(0, "PSE51's members alone are a subprofile");
#endif
    check(_POSIX_VERSION == 202405L, "a subprofile names the edition it profiles");
#ifdef _POSIX2_VERSION
    check(0, "no _POSIX2_VERSION for a subprofile");
#endif

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
