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

#define _DEFAULT_SOURCE
#include <uchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * ISO C: mbrtoc16 and mbrtoc32 with a null s convert "" and store
 * nothing; c16rtomb and c32rtomb with a null s convert a null
 * character, whatever their character argument.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    mbstate_t ps;
    char16_t  c16;
    char32_t  c32;

    memset(&ps, 0, sizeof(ps));

    c16 = 0x1234;
    check(mbrtoc16(&c16, NULL, 0, &ps) == 0, "mbrtoc16 with a null s");
    check(c16 == 0x1234, "mbrtoc16 with a null s stores nothing");
    check(mbsinit(&ps) != 0, "mbrtoc16 with a null s leaves the initial state");
    c32 = 0x1234;
    check(mbrtoc32(&c32, NULL, 0, &ps) == 0, "mbrtoc32 with a null s");
    check(c32 == 0x1234, "mbrtoc32 with a null s stores nothing");

    check(c16rtomb(NULL, 0xd800, &ps) == 1, "c16rtomb with a null s ignores c16");
    check(mbsinit(&ps) != 0, "c16rtomb with a null s leaves the initial state");
    check(c32rtomb(NULL, 0xd800, &ps) == 1, "c32rtomb with a null s ignores c32");

    printf("success\n");
    return 0;
}
