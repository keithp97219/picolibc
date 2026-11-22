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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* setuid accepts the real ID, refuses an invalid one, and refuses others without privileges */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Returns an ID the process holds in none of its real, effective or saved IDs */
static uid_t
other_id(uid_t a, uid_t b, uid_t c)
{
    uid_t id = a + 1;

    while (id == a || id == b || id == c || id == (uid_t)-1 || id == 0)
        id++;
    return id;
}

int
main(void)
{
    uid_t r = getuid(), e = geteuid();
    gid_t before[256], after[256];
    int   nbefore, nafter;

    nbefore = getgroups(256, before);
    check(nbefore >= 0, "getgroups");
    check(setuid(r) == 0, "setuid to the real ID");
    check(getuid() == r && geteuid() == r, "IDs after setuid");
    /* setuid doesn't touch the supplementary groups */
    nafter = getgroups(256, after);
    check(nafter == nbefore && memcmp(before, after, (size_t)nbefore * sizeof(gid_t)) == 0,
          "supplementary groups kept");

    errno = 0;
    check(setuid((uid_t)-1) == -1 && errno == EINVAL, "an invalid ID gives EINVAL");
    if (geteuid() == 0) {
        printf("privileged, skipping the EPERM check\n");
    } else {
        errno = 0;
        check(setuid(other_id(r, e, r)) == -1 && errno == EPERM, "another ID gives EPERM");
        check(getuid() == r && geteuid() == r, "IDs kept after EPERM");
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
