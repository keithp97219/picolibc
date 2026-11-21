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
#include <sys/resource.h>

#include <stdio.h>

/*
 * In strict POSIX mode, without XSI, <sys/resource.h> declares
 * getrlimit and setrlimit (in the base since Issue 8) with their
 * structure and constants, but not the XSI functions
 */

#if !defined(RLIMIT_CORE) || !defined(RLIMIT_DATA) || !defined(RLIMIT_FSIZE)    \
    || !defined(RLIMIT_NOFILE) || !defined(RLIMIT_STACK) || !defined(RLIMIT_AS)
#error "RLIMIT_ constant missing"
#endif

#ifndef RLIM_INFINITY
#error "RLIM_INFINITY missing"
#endif

#ifndef __GLIBC__
/* glibc declares the XSI functions whatever the feature-test macros */
static double getpriority;
static double setpriority;
static double getrusage;
#endif

#define check_type(f, t) _Generic(&(f), t: 1, default: 0)

int
main(void)
{
    struct rlimit rl = { .rlim_cur = RLIM_INFINITY, .rlim_max = RLIM_INFINITY };
    rlim_t        lim = rl.rlim_cur;

    (void)lim;
#ifndef __GLIBC__
    (void)getpriority;
    (void)setpriority;
    (void)getrusage;
#endif
    if (!check_type(getrlimit, int (*)(int, struct rlimit *))
        || !check_type(setrlimit, int (*)(int, const struct rlimit *))) {
        printf("wrong prototype\n");
        return 1;
    }
    printf("success\n");
    return 0;
}
