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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_NETWORKING)

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* in6addr_any and in6addr_loopback hold the IPv6 wildcard and loopback addresses */

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
    const struct in6_addr      any = IN6ADDR_ANY_INIT, loopback = IN6ADDR_LOOPBACK_INIT;
    static const unsigned char one[16] = { [15] = 1 };
    static const unsigned char zero[16];

    check(memcmp(&in6addr_any, zero, 16) == 0, "in6addr_any is ::");
    check(memcmp(&in6addr_any, &any, sizeof(any)) == 0, "IN6ADDR_ANY_INIT");
    check(memcmp(&in6addr_loopback, one, 16) == 0, "in6addr_loopback is ::1");
    check(memcmp(&in6addr_loopback, &loopback, sizeof(loopback)) == 0, "IN6ADDR_LOOPBACK_INIT");
    check(IN6_IS_ADDR_UNSPECIFIED(&in6addr_any) && !IN6_IS_ADDR_LOOPBACK(&in6addr_any), "any");
    check(IN6_IS_ADDR_LOOPBACK(&in6addr_loopback) && !IN6_IS_ADDR_UNSPECIFIED(&in6addr_loopback),
          "loopback");
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
