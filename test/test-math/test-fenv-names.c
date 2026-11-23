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

/* Strict C: no _GNU_SOURCE, and not C23 */
#define _POSIX_C_SOURCE 200809L
#include <fenv.h>
#include <stdio.h>

/*
 * feenableexcept, fedisableexcept and fegetexcept are GNU extensions,
 * and fesetexcept is new in C23, so in strict C17 mode they are the
 * application's names. Each machine header that defines them inline is
 * checked here once it guards them.
 */

#if defined(__hexagon__)
#define CHECK_FENV_NAMES
#endif

#ifdef CHECK_FENV_NAMES
static int feenableexcept __attribute__((unused));
static int fedisableexcept __attribute__((unused));
static int fegetexcept __attribute__((unused));
#if __STDC_VERSION__ < 202311L
static int fesetexcept __attribute__((unused));
#endif
#endif

int
main(void)
{
#ifdef CHECK_FENV_NAMES
    printf("success\n");
    return 0;
#else
    printf("fenv names not checked on this target, skipping\n");
    return 77;
#endif
}
