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

#include <wchar.h>
#include <string.h>
#include <stdio.h>

/*
 * Programs may clear only part of an mbstate_t, sized by what they
 * know of the OS's usual C library. gnulib's mbszero() clears this many
 * bytes (lib/wchar.in.h, _GL_MBSTATE_ZERO_SIZE), and its mcel.h
 * initializes the same bytes through a union, so mbstate_t must be at
 * least this large or they write past it.
 */
#if defined(__APPLE__) && defined(__MACH__)
#define CLEARED_SIZE 12
#elif defined(__FreeBSD__)
#define CLEARED_SIZE 12
#elif defined(__NetBSD__)
#define CLEARED_SIZE 28
#elif defined(__OpenBSD__)
#define CLEARED_SIZE 12
#else
#define CLEARED_SIZE sizeof(mbstate_t)
#endif

int
main(void)
{
    mbstate_t state;
    wchar_t   wc = 0;

    if (sizeof(mbstate_t) < CLEARED_SIZE) {
        printf("sizeof(mbstate_t) %zu < %zu\n", sizeof(mbstate_t), (size_t)CLEARED_SIZE);
        return 1;
    }

    /* A state with only those bytes cleared is an initial state */
    memset(&state, 0x55, sizeof(state));
    memset(&state, 0, CLEARED_SIZE);
    if (!mbsinit(&state)) {
        printf("mbsinit after clearing %zu bytes\n", (size_t)CLEARED_SIZE);
        return 1;
    }
    if (mbrtowc(&wc, "a", 1, &state) != 1 || wc != L'a' || !mbsinit(&state)) {
        printf("mbrtowc after clearing %zu bytes\n", (size_t)CLEARED_SIZE);
        return 1;
    }
    printf("success\n");
    return 0;
}
