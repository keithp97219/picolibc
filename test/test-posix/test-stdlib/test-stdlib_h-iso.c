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

/*
 * No feature-test macros: with a strict ISO C compiler mode,
 * <stdlib.h> declares only the ISO C names, not the POSIX ones.
 */
#include <stdlib.h>

#include <stdio.h>

#ifdef __STRICT_ANSI__

#if defined(WNOHANG) || defined(WEXITSTATUS) || defined(WIFEXITED) || defined(WCOREDUMP)
#error "<sys/wait.h> macros in ISO C mode"
#endif

#if defined(O_RDWR) || defined(O_APPEND) || defined(O_CLOEXEC)
#error "open flags in ISO C mode"
#endif

static int setenv __attribute__((unused));
static int unsetenv __attribute__((unused));
static int mkstemp __attribute__((unused));
static int rand_r __attribute__((unused));
static int posix_memalign __attribute__((unused));
static int valloc __attribute__((unused));

int
main(void)
{
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("not a strict ISO C compilation, skipping\n");
    return 77;
}

#endif
