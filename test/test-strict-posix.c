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

/* Strict POSIX.1-2024 leaves extensions and removed names alone */
#define _POSIX_C_SOURCE 202405L

#include <inttypes.h>

/* POSIX <inttypes.h> defines wchar_t */
static wchar_t inttypes_wchar;

/* Parameter names don't collide with application macros */
#define flags 1
#include <unistd.h>
#undef flags

#ifndef O_CLOEXEC
#error <unistd.h> must define O_CLOEXEC
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>

#ifdef alloca
#error alloca defined
#endif

#if !defined(MAP_ANON) || !defined(O_RDONLY) || !defined(O_RDWR) || !defined(O_CREAT) \
    || !defined(O_EXCL) || !defined(O_TRUNC)
#error <sys/mman.h> must define MAP_ANON and the shm_open flags
#endif

static int rand_r = 1;
static int valloc = 2;
static int ecvt = 3;
static int fcvt = 4;
static int gcvt = 5;
static int ecvt_r = 6;
static int suboptarg = 7;
static int asnprintf = 8;
static int vasnprintf = 9;
static int memccpy = 10;
static int ffsl = 11;
static int ffsll = 12;
static int ualarm = 13;
static int optreset = 14;

int
main(void)
{
    int (*asp)(char **, const char *, ...) = asprintf;
    int sum = rand_r + valloc + ecvt + fcvt + gcvt + ecvt_r + suboptarg + asnprintf + vasnprintf
        + memccpy + ffsl + ffsll + ualarm + optreset;

    (void)inttypes_wchar;
    if (sum != 105 || !asp)
        return 1;
    printf("success\n");
    return 0;
}
