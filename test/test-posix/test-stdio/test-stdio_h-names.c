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
#include <stdio.h>
#include <stdlib.h>

/*
 * Strict POSIX.1-2024 <stdio.h> declares nothing outside its namespace:
 * these names, from BSD, GNU, picolibc or earlier issues, stay free for the
 * application. A declaration in the header would conflict with these.
 */

#if defined(P_tmpdir) || defined(fropen) || defined(fwopen) || defined(clearerr_unlocked) \
    || defined(L_cuserid)
#error extension macro visible in strict mode
#endif

static int
asnprintf(void)
{
    return 1;
}
static int
vasnprintf(void)
{
    return 2;
}
static int
setbuffer(void)
{
    return 3;
}
static int
setlinebuf(void)
{
    return 4;
}
static int
funopen(void)
{
    return 5;
}
static int
fopencookie(void)
{
    return 6;
}
static int
clearerr_unlocked(void)
{
    return 7;
}
static int
ferror_unlocked(void)
{
    return 8;
}
static int
feof_unlocked(void)
{
    return 9;
}
static int
fgetln(void)
{
    return 10;
}
static int
tempnam(void)
{
    return 11;
}
static int
getw(void)
{
    return 12;
}
static int
putw(void)
{
    return 13;
}
static int
cuserid(void)
{
    return 14;
}
static int cookie_io_functions_t = 15;
static int
fcloseall(void)
{
    return 16;
}

int
main(void)
{
    int sum = asnprintf() + vasnprintf() + setbuffer() + setlinebuf() + funopen() + fopencookie()
        + clearerr_unlocked() + ferror_unlocked() + feof_unlocked() + fgetln() + tempnam() + getw()
        + putw() + cuserid() + cookie_io_functions_t + fcloseall();

    if (sum != 136) {
        printf("sum %d\n", sum);
        return 1;
    }
    printf("success\n");
    return 0;
}
