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

/* Strict POSIX.1-2008 leaves BSD, GNU and XSI names to the application */
#define _POSIX_C_SOURCE 200809L

#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>

#if defined(NSIG) || defined(_PATH_PASSWD) || defined(S_ISVTX)
#error extension macro visible
#endif

#if defined(SS_ONSTACK) || defined(SS_DISABLE) || defined(MINSIGSTKSZ) || defined(SIGSTKSZ)
#error XSI macro visible
#endif

static int clearerr_unlocked = 1;
static int ferror_unlocked = 2;
static int feof_unlocked = 3;
static int setbuffer = 4;
static int setlinebuf = 5;
static int wait3 = 6;
static int sigaltstack = 7;
static int mknod = 8;

int
main(void)
{
    int sum = clearerr_unlocked + ferror_unlocked + feof_unlocked + setbuffer + setlinebuf + wait3
        + sigaltstack + mknod;

    if (sum != 36)
        return 1;
    printf("success\n");
    return 0;
}
