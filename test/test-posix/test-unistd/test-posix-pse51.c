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
 * A system with no <sys/_posix.h> of its own claims the minimal
 * realtime profile (PSE51) here, as its <sys/_posix.h> would. <unistd.h>
 * supplies exactly what IEEE 1003.13-2003 Tables 6-1 and 6-2 require of
 * the OS, nothing that only full POSIX.1 or the larger profiles need,
 * and reports a subprofile of POSIX.1-2024.
 */

#define _POSIX_C_SOURCE 202405L

/* Without the system support this test needs, it is skipped */
#if defined(__has_include)
#if !__has_include(<sys/_posix.h>)
#define CLAIM
#define _POSIX_AEP_REALTIME_MINIMAL 200312L
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(CLAIM) && defined(__PICOLIBC__)

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* What the profile requires, from Table 6-2 and the units in Table 6-1 */
#define REQUIRED(option) check(option == 200312L, "PSE51 requires " #option)

int
main(void)
{
    REQUIRED(_POSIX_CLOCK_SELECTION);
    REQUIRED(_POSIX_FSYNC);
    REQUIRED(_POSIX_MEMLOCK);
    REQUIRED(_POSIX_MEMLOCK_RANGE);
    REQUIRED(_POSIX_MONOTONIC_CLOCK);
    REQUIRED(_POSIX_REALTIME_SIGNALS);
    REQUIRED(_POSIX_SEMAPHORES);
    REQUIRED(_POSIX_SHARED_MEMORY_OBJECTS);
    REQUIRED(_POSIX_SYNCHRONIZED_IO);
    REQUIRED(_POSIX_THREAD_ATTR_STACKADDR);
    REQUIRED(_POSIX_THREAD_ATTR_STACKSIZE);
    REQUIRED(_POSIX_THREAD_CPUTIME);
    REQUIRED(_POSIX_THREAD_PRIO_INHERIT);
    REQUIRED(_POSIX_THREAD_PRIO_PROTECT);
    REQUIRED(_POSIX_THREAD_PRIORITY_SCHEDULING);
    REQUIRED(_POSIX_THREAD_SPORADIC_SERVER);
    REQUIRED(_POSIX_THREADS);
    REQUIRED(_POSIX_TIMEOUTS);
    REQUIRED(_POSIX_TIMERS);
    REQUIRED(__PICOLIBC_POSIX_C_LIB_EXT);
    REQUIRED(__PICOLIBC_POSIX_DEVICE_IO);
    REQUIRED(__PICOLIBC_POSIX_SIGNALS);
    REQUIRED(__PICOLIBC_POSIX_SINGLE_PROCESS);
    check(_POSIX_NO_TRUNC == 1, "PSE51 requires _POSIX_NO_TRUNC");

    /* Not required by PSE51, so not claimed for it */
#if defined(_POSIX_BARRIERS) || defined(_POSIX_MAPPED_FILES) || defined(_POSIX_MEMORY_PROTECTION) \
    || defined(_POSIX_READER_WRITER_LOCKS) || defined(_POSIX_SPIN_LOCKS)
    check(0, "full POSIX.1 options claimed for PSE51");
#endif
#if defined(_POSIX_MESSAGE_PASSING) || defined(__PICOLIBC_POSIX_FD_MGMT)                \
    || defined(__PICOLIBC_POSIX_FILE_SYSTEM) || defined(__PICOLIBC_POSIX_MULTI_PROCESS)
    check(0, "larger profiles' options or groups claimed for PSE51");
#endif

    /* picolibc's asynchronous I/O exists only with its own threads */
#if defined(__PICOLIBC_PTHREADS) != defined(_POSIX_ASYNCHRONOUS_IO)
    check(0, "_POSIX_ASYNCHRONOUS_IO follows picolibc's threads");
#endif

    /* A subprofile of POSIX.1-2024, without the shell and utilities */
#ifndef _POSIX_SUBPROFILE
    check(0, "PSE51 alone is a subprofile");
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
