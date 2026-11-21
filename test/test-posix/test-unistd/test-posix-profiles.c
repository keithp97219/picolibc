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
 * A system claiming a realtime profile gets the profiles below it and
 * the options and option groups the profile requires. Claim the
 * largest one here, with the macro an OS would define in
 * <sys/_posix.h>, where picolibc can back it; the profile macros are
 * picolibc's to check.
 */

#define _POSIX_C_SOURCE 202405L

/* Without the system support this test needs, it is skipped */
#if defined(__has_include)
#if __has_include(<picolibc.h>)
#include <picolibc.h>
#endif
#endif
#if defined(__PICOLIBC_PTHREADS) && !defined(__SINGLE_THREAD)
#define CLAIM                            1
#define __PICOLIBC_OS_AEP_REALTIME_MULTI 200312L
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
    /* The lower profiles */
    check(_POSIX_AEP_REALTIME_DEDICATED == 200312L, "dedicated");
    check(_POSIX_AEP_REALTIME_CONTROLLER == 200312L, "controller");
    check(_POSIX_AEP_REALTIME_MINIMAL == 200312L, "minimal");

    /* Options the OS left out take the profile's value; its own keep theirs */
    check(_POSIX_THREAD_SPORADIC_SERVER == 200312L, "minimal option");
    check(_POSIX_SPORADIC_SERVER == 200312L, "dedicated option");
    check(_POSIX_PRIORITIZED_IO == 200312L, "dedicated option");
#if defined(_POSIX_THREADS) && _POSIX_THREADS != 200312L
    check(_POSIX_THREADS >= 202405L, "OS option");
#endif
    check(__PICOLIBC_POSIX_USER_GROUPS > 0, "multi-purpose group");
    check(_POSIX_ADVISORY_INFO > 0 && _POSIX_SHELL > 0, "multi-purpose options");

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
