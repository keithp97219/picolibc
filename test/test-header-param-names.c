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

#define _GNU_SOURCE
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>

/*
 * These machines' fenv headers still name their parameters; check
 * <fenv.h> elsewhere, adding machines as they are fixed.
 */
#if defined(__hexagon__) || defined(__loongarch__) || defined(__m68k__) || defined(__mips__) \
    || defined(__powerpc__) || defined(__sh__) || defined(__sparc__) || defined(__SPU__)     \
    || defined(__xtensa__)
#include <fenv.h>
#endif

/* time.h still names other parameters; check timespec_getres alone */
#define base 1
#include <time.h>

/*
 * An application may define macros with any name the standards don't
 * reserve before including a header; prototypes and inline functions
 * mustn't use them.
 */
#define a             1
#define addr          1
#define advice        1
#define alignment     1
#define c             1
#define c16           1
#define c32           1
#define c8            1
#define dst           1
#define envp          1
#define errnum        1
#define error         1
#define except        1
#define excepts       1
#define fd            1
#define flagp         1
#define flags         1
#define format        1
#define fp            1
#define h             1
#define h_n           1
#define handler       1
#define l             1
#define len           1
#define modep         1
#define msg           1
#define n             1
#define n_n           1
#define offset        1
#define path          1
#define pc16          1
#define pc32          1
#define pc8           1
#define prot          1
#define ps            1
#define ptr           1
#define resolved_path 1
#define round         1
#define rounding_mode 1
#define s             1
#define sig           1
#define size          1
#define src           1
#define str           1
#define value         1
#define wc            1

#include <ctype.h>
#include <fenv.h>
#include <stdbit.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <uchar.h>
#include <wchar.h>

int
main(void)
{
    printf("success\n");
    return 0;
}
