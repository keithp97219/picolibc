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
#if defined(__PICOLIBC_POSIX_SINGLE_PROCESS)

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

/*
 * gethostname returns the node name uname reports, null-terminated,
 * no longer than {HOST_NAME_MAX}; a short buffer gets a truncated
 * name and nothing is written past it. The name itself is never
 * printed.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define GUARD 16

int
main(void)
{
    struct utsname u;
    char           name[256 + GUARD];
    size_t         len, size, i;
    long           max;

    check(uname(&u) == 0, "uname");
    memset(name, 'g', sizeof(name));
    check(gethostname(name, 256) == 0, "gethostname");
    check(memchr(name, '\0', 256) != NULL, "null-terminated");
    check(strcmp(name, u.nodename) == 0, "nodename");
    len = strlen(name);

    /* Host names are limited to {HOST_NAME_MAX} bytes */
    max = sysconf(_SC_HOST_NAME_MAX);
    /* glibc reports Linux's 64, below the POSIX minimum */
#ifndef __GLIBC__
    check(max == -1 || max >= _POSIX_HOST_NAME_MAX, "_SC_HOST_NAME_MAX");
#endif
    check(max == -1 || len <= (size_t)max, "within _SC_HOST_NAME_MAX");
#ifdef HOST_NAME_MAX
    check(len <= HOST_NAME_MAX, "within HOST_NAME_MAX");
#endif

    /* Exactly enough room */
    memset(name, 'g', sizeof(name));
    check(gethostname(name, len + 1) == 0, "exact buffer");
    check(strcmp(name, u.nodename) == 0, "exact buffer holds the name");
    for (i = len + 1; i < len + 1 + GUARD; i++)
        check(name[i] == 'g', "nothing past an exact buffer");

    /*
     * Too little room: the name is truncated, perhaps without its null,
     * and nothing past the buffer changes. glibc fails with
     * ENAMETOOLONG instead
     */
    for (size = 1; size <= len; size++) {
        memset(name, 'g', sizeof(name));
#ifndef __GLIBC__
        check(gethostname(name, size) == 0, "short buffer");
        check(strncmp(name, u.nodename, size - 1) == 0, "truncated");
#else
        (void)gethostname(name, size);
#endif
        for (i = size; i < size + GUARD; i++)
            check(name[i] == 'g', "nothing past a short buffer");
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
