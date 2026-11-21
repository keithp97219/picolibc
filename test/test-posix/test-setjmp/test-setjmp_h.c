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
#include <setjmp.h>
#include <stdio.h>

/* What <setjmp.h> defines for POSIX */

/* jmp_buf and sigjmp_buf are array types */
static jmp_buf    env __attribute__((unused));
static sigjmp_buf sigenv __attribute__((unused));

#define is_array(a) _Generic((__typeof__(a) *)0, __typeof__(&(a)[0]) *: 0, default: 1)
_Static_assert(is_array(env), "jmp_buf is an array");
_Static_assert(is_array(sigenv), "sigjmp_buf is an array");

/* longjmp and siglongjmp are functions, with prototypes */
_Static_assert(_Generic(&longjmp, void (*)(jmp_buf, int): 1, default: 0), "longjmp");
_Static_assert(_Generic(&siglongjmp, void (*)(sigjmp_buf, int): 1, default: 0), "siglongjmp");

/* Both are _Noreturn: without that, -Werror=return-type would fail */
#pragma GCC diagnostic error "-Wreturn-type"
static int
jump(int which)
{
    if (which)
        (siglongjmp)(sigenv, which);
    (longjmp)(env, which);
}

static void (* volatile jumpers[])(sigjmp_buf, int) = { siglongjmp };

int
main(void)
{
    /* The functions work when called through pointers too */
    switch (sigsetjmp(sigenv, 1)) {
    case 0:
        jumpers[0](sigenv, 3);
        break;
    case 3:
        jump(5);
        break;
    case 5:
        printf("success\n");
        return 0;
    default:
        printf("sigsetjmp returned an unexpected value\n");
        return 1;
    }
    printf("siglongjmp returned\n");
    return 1;
}
