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

/* POSIX.1-2024 provides the case-insensitive flags in strict mode */
#define _POSIX_C_SOURCE 202405L
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(FNM_CASEFOLD) && defined(FNM_IGNORECASE)

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
    check(FNM_IGNORECASE == FNM_CASEFOLD, "FNM_IGNORECASE");
    check(fnmatch("aB*", "Abc", FNM_CASEFOLD) == 0, "FNM_CASEFOLD");
    check(fnmatch("aB*", "Abc", FNM_IGNORECASE) == 0, "FNM_IGNORECASE");
    check(fnmatch("aB*", "Abc", 0) == FNM_NOMATCH, "case sensitive");
    printf("success\n");
    return 0;
}

#elif defined(__PICOLIBC__)
#error FNM_CASEFOLD and FNM_IGNORECASE missing
#else

int
main(void)
{
    /* glibc offers FNM_CASEFOLD only to GNU code, and lacks FNM_IGNORECASE */
    printf("no FNM_CASEFOLD, skipping\n");
    return 77;
}

#endif
