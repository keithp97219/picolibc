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

/* Parameter names a strict application may use as macros */
#define excepts         1 +
#define flagp           1 +
#define envp            1 +
#define round           1 +
#define mask            1 +
#define modep           1 +
#define rounding_mode   1 +

#define _POSIX_C_SOURCE 202405L
#include <fenv.h>

#include <stdio.h>

/*
 * Strict POSIX: <fenv.h> uses only reserved names for its parameters,
 * and declares no extensions. Each static object here collides with a
 * declaration of the same name.
 */

static int feenableexcept __attribute__((unused));
static int fedisableexcept __attribute__((unused));
static int fegetexcept __attribute__((unused));

/* C23 adds these; POSIX.1-2024 is aligned with C17 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
static int fesetexcept __attribute__((unused));
static int fetestexceptflag __attribute__((unused));
static int fegetmode __attribute__((unused));
static int fesetmode __attribute__((unused));
#ifdef FE_DFL_MODE
#error "FE_DFL_MODE in strict C17 POSIX mode"
#endif
#endif

int
main(void)
{
    printf("success\n");
    return 0;
}
