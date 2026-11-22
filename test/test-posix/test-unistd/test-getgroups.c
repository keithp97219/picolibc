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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_USER_GROUPS

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* getgroups reports the count, fills in every entry completely and checks the size */

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
    gid_t groups[256];
    int   ngroups;

    /*
     * A call returning 16-bit ids would leave parts of the array
     * untouched, whatever the ids are.
     */
    ngroups = getgroups(0, NULL);
    check(ngroups >= 0 && ngroups <= 256, "number of groups");
    memset(groups, 0xff, sizeof(groups));
    check(getgroups(ngroups, groups) == ngroups, "getgroups");
    for (int i = 0; i < ngroups; i++)
        check((groups[i] >> 16) != 0xffff, "getgroups entry filled in");

    /* A size of zero only counts, leaving the array alone */
    memset(groups, 0xab, sizeof(groups));
    check(getgroups(0, groups) == ngroups, "count");
    for (size_t i = 0; i < sizeof(groups); i++)
        check(((unsigned char *)groups)[i] == 0xab, "array untouched");

    /* An array too small for the list */
    if (ngroups >= 2) {
        errno = 0;
        check(getgroups(ngroups - 1, groups) == -1 && errno == EINVAL, "EINVAL");
    } else {
        printf("%d groups, skipping the EINVAL check\n", ngroups);
    }

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
