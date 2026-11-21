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
/* The application's own macros, defined before the header */
#define name     +
#define namesize +
#include <unistd.h>

#include <stdio.h>

/*
 * In strict POSIX mode, without XSI, <unistd.h> declares none of the
 * XSI process functions, nothing Issue 7 or 8 removed and no
 * extensions, and its prototypes don't use the application's names
 */

/* XSI interfaces */
static int nice;
static int setregid;
static int setreuid;
static int getresgid;
static int getresuid;
static int setresgid;
static int setresuid;

/* Removed interfaces */
static int setpgrp;
static int ualarm;
static int usleep;
static int vfork;

/* Extensions */
static int execvpe;
static int execlpe;
static int setgroups;
static int issetugid;
static int daemon;

int
main(void)
{
    (void)nice;
    (void)setregid;
    (void)setreuid;
    (void)getresgid;
    (void)getresuid;
    (void)setresgid;
    (void)setresuid;
    (void)setpgrp;
    (void)ualarm;
    (void)usleep;
    (void)vfork;
    (void)execvpe;
    (void)execlpe;
    (void)setgroups;
    (void)issetugid;
    (void)daemon;
    printf("success\n");
    return 0;
}
