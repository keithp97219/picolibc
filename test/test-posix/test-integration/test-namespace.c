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
#include <ftw.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
 * In strict POSIX mode, names POSIX doesn't reserve for these headers
 * belong to the application: extensions, XSI interfaces and interfaces
 * POSIX has removed must stay hidden.
 */

#if defined(S_ISVTX) || defined(SS_ONSTACK) || defined(SS_DISABLE) || defined(FTW_PHYS) \
    || defined(NSIG)
#error "XSI or extension macros visible in strict POSIX mode"
#endif

/* Each would conflict with a declaration the headers shouldn't make */
static int
setbuffer(int a)
{
    return a + 1;
}

static int
setlinebuf(int a)
{
    return a + 2;
}

static int
wait3(int a)
{
    return a + 3;
}

static int
sigaltstack(int a)
{
    return a + 4;
}

int
main(void)
{
    if (setbuffer(0) + setlinebuf(0) + wait3(0) + sigaltstack(0) != 10) {
        printf("wrong functions called\n");
        return 1;
    }
    printf("success\n");
    return 0;
}
