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
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * getgrgid_r finds an entry by id in the buffer it is given, reporting
 * ERANGE only when that entry doesn't fit
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* ':' separates the fields of the group database, so no name holds one */
#define NO_NAME "no:such:group"

/* The string lies within the first size bytes of buf */
static int
in_buffer(const char *s, const char *buf, size_t size)
{
    return s >= buf && s < buf + size && s + strlen(s) < buf + size;
}

int
main(void)
{
    struct group ent, *result;
    char         buf[4096], small[1];
    char         name[256];
    size_t       need, m;
    long         suggested;
    int          ret;

    /* Every system has a group 0, though its name varies (root, wheel) */
    check(getgrgid_r(0, &ent, buf, sizeof(buf), &result) == 0, "getgrgid_r");
    if (!result) {
        printf("no root entry, skipping\n");
        return 77;
    }
    check(result == &ent && ent.gr_name[0] != '\0' && strlen(ent.gr_name) < sizeof(name)
              && ent.gr_gid == 0,
          "root");
    strcpy(name, ent.gr_name);

    /* Not finding an entry isn't an error */
    result = &ent;
    check(getgrgid_r((gid_t)-2, &ent, buf, sizeof(buf), &result) == 0 && result == NULL, "missing");

    /* A buffer too small: ERANGE and a null result */
    result = &ent;
    check(getgrgid_r(0, &ent, small, sizeof(small), &result) == ERANGE && result == NULL, "ERANGE");

    /*
     * Grow the buffer a byte at a time: each one too small gives
     * ERANGE, and the first that is big enough holds the name, the
     * member array and the member names
     */
    for (need = 1;; need++) {
        check(need <= sizeof(buf), "the root group fits in 4096 bytes");
        result = &ent;
        ret = getgrgid_r(0, &ent, buf, need, &result);
        if (ret == 0)
            break;
        check(ret == ERANGE && result == NULL, "ERANGE below the size needed");
    }
    check(result == &ent && strcmp(ent.gr_name, name) == 0 && ent.gr_gid == 0, "root again");
    check(in_buffer(ent.gr_name, buf, need), "name in the buffer");
    for (m = 0;; m++) {
        check((const char *)&ent.gr_mem[m] >= buf
                  && (const char *)(&ent.gr_mem[m] + 1) <= buf + need,
              "member array in the buffer");
        if (!ent.gr_mem[m])
            break;
        check(in_buffer(ent.gr_mem[m], buf, need), "member in the buffer");
    }

#ifndef __GLIBC__
    /*
     * The size of the buffer only matters for the entry found: a
     * missing entry is still not an error with a small buffer (glibc
     * reports ERANGE for any line of the file that doesn't fit)
     */
    result = &ent;
    check(getgrgid_r((gid_t)-2, &ent, small, sizeof(small), &result) == 0 && result == NULL,
          "missing, small buffer");
#endif

    /* sysconf suggests a size, or returns -1 without changing errno */
    errno = 0;
    suggested = sysconf(_SC_GETGR_R_SIZE_MAX);
    check(suggested > 0 || (suggested == -1 && errno == 0), "sysconf(_SC_GETGR_R_SIZE_MAX)");
    printf("sysconf(_SC_GETGR_R_SIZE_MAX) %ld, the root group needs %zu\n", suggested, need);

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
