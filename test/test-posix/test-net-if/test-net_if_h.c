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

/* What <net/if.h> provides */

#define _POSIX_C_SOURCE 202405L
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_NETWORKING

#include <net/if.h>

#define same_type(a, T) _Generic((a), T: 1, default: 0)

static struct if_nameindex ni;
_Static_assert(same_type(ni.if_index, unsigned) && same_type(ni.if_name, char *),
               "struct if_nameindex");

#ifndef IF_NAMESIZE
#error IF_NAMESIZE
#endif
_Static_assert(IF_NAMESIZE > 1, "IF_NAMESIZE");

static void                 (                *const p_if_freenameindex)(struct if_nameindex *) = if_freenameindex;
static char                *(               *const p_if_indextoname)(unsigned, char *) = if_indextoname;
static struct if_nameindex *(* const p_if_nameindex)(void) = if_nameindex;
static unsigned             (            *const p_if_nametoindex)(const char *) = if_nametoindex;

#include <stdio.h>

int
main(void)
{
    (void)ni;
    (void)p_if_freenameindex;
    (void)p_if_indextoname;
    (void)p_if_nameindex;
    (void)p_if_nametoindex;
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
