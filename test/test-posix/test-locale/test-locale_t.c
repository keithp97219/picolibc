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
#include <unistd.h>
#include "../test-flags.h"

#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/*
 * locale_t is a pointer type, as on other systems: programs (gnulib's
 * configure checks among them) pass NULL for "no locale object", as
 * in uselocale(NULL) and newlocale(mask, name, NULL), and compare
 * locale objects with NULL
 */

int
main(void)
{
    locale_t loc, cur;

    loc = newlocale(LC_ALL_MASK, "C", NULL);
    if (loc == NULL) {
        printf("newlocale(\"C\") failed\n");
        return 1;
    }
    cur = uselocale(NULL);
    if (cur == NULL || cur != LC_GLOBAL_LOCALE) {
        printf("uselocale(NULL) should report the global locale\n");
        return 1;
    }
    if (uselocale(loc) != LC_GLOBAL_LOCALE || uselocale(NULL) != loc) {
        printf("uselocale(loc)\n");
        return 1;
    }
#ifdef __PICOLIBC__
    /* POSIX.1-2024; not in every C library yet */
    if (strcmp(getlocalename_l(LC_CTYPE, loc), "C") != 0) {
        printf("getlocalename_l\n");
        return 1;
    }
#endif
    uselocale(LC_GLOBAL_LOCALE);
    freelocale(loc);
    printf("success\n");
    return 0;
}
