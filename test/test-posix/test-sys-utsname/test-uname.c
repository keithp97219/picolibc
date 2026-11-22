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
#if TEST_POSIX_SINGLE_PROCESS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

/*
 * uname fills in the system's names, each terminated within its
 * array. The values are the host's own, so only their form and their
 * agreement with other interfaces are checked, and nothing is printed.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define terminated(field) (memchr((field), '\0', sizeof(field)) != NULL)
#define same(field)       (strcmp(name.field, again.field) == 0)

int
main(void)
{
    struct utsname name, again;
    char           host[256];

    /* Every byte starts non-zero, so a missing terminator shows */
    memset(&name, 0xa5, sizeof(name));
    check(uname(&name) >= 0, "uname");
    check(terminated(name.sysname) && terminated(name.nodename) && terminated(name.release)
              && terminated(name.version) && terminated(name.machine),
          "null-terminated fields");
    check(name.sysname[0] && name.release[0] && name.version[0] && name.machine[0], "uname fields");

    /* The same system gives the same names */
    memset(&again, 0, sizeof(again));
    check(uname(&again) >= 0, "uname again");
    check(same(sysname) && same(nodename) && same(release) && same(version) && same(machine),
          "same names twice");

#if defined(__linux__) && !defined(__PICOLIBC_BAREMETAL)
    /* On Linux both come from the same kernel name; gethostname gives the node name */
    check(strcmp(name.sysname, "Linux") == 0, "Linux sysname");
    memset(host, 0, sizeof(host));
    check(gethostname(host, sizeof(host) - 1) == 0, "gethostname");
    check(strcmp(host, name.nodename) == 0, "nodename matches gethostname");
#else
    (void)host;
#endif

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
