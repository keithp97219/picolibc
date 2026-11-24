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

/* Strict C17 and POSIX.1-2008: no _GNU_SOURCE, and not C23 */
#define _POSIX_C_SOURCE 200809L

#include <fenv.h>

/* <fenv.h> doesn't define the E names */
#ifdef EDOM
#error <fenv.h> defined EDOM
#endif

#include <string.h>
#include <uchar.h>
#include <wchar.h>

#include <stdio.h>

/* free is reserved only with <stdlib.h> */
static int free = 1;

/* char8_t, mbrtoc8 and c8rtomb are C23 */
static int mbrtoc8 = 2;
static int c8rtomb = 3;
#ifndef __cplusplus
typedef int char8_t;
#endif

/*
 * GNU extensions and C23. These machines' fenv headers still define
 * them inline in every mode; they join the check as they are fixed.
 */
#if !(defined(__hexagon__) || defined(__loongarch__) || defined(__m68k__) || defined(__mips__) \
      || defined(__powerpc__) || defined(__sh__) || defined(__sparc__) || defined(__SPU__)     \
      || defined(__xtensa__))
static int feenableexcept = 4;
static int fedisableexcept = 5;
static int fegetexcept = 6;
static int fesetexcept = 7;
#else
#define feenableexcept  4
#define fedisableexcept 5
#define fegetexcept     6
#define fesetexcept     7
#endif

int
main(void)
{
    char8_t c = 8;
    int     sum = free + mbrtoc8 + c8rtomb + feenableexcept + fedisableexcept + fegetexcept
        + fesetexcept + c;

    if (sum != 36)
        return 1;
    printf("success\n");
    return 0;
}
