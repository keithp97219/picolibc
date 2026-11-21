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
#define _XOPEN_SOURCE   800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_XSI_USER_GROUPS)

#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * getgrent walks the group database, leaving errno alone at the end;
 * setgrent rewinds it and endgrent closes it, also leaving errno alone
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The prototypes, with the types POSIX gives */
static void          (         *const p_endgrent)(void) = endgrent;
static struct group *(* const p_getgrent)(void) = getgrent;
static void          (         *const p_setgrent)(void) = setgrent;

static char         *first, *second;

/* Read the whole database, returning the number of entries */
static unsigned long
walk(int *found_root, int *found_group)
{
    unsigned long count = 0;
    struct group *gr;
    size_t        m;

    *found_root = *found_group = 0;
    for (;;) {
        errno = 0;
        gr = getgrent();
        if (!gr)
            break;
        check(gr->gr_name != NULL && gr->gr_mem != NULL, "pointers");
        for (m = 0; gr->gr_mem[m]; m++)
            check(gr->gr_mem[m][0] != '\0', "member name");
        if (count == 0) {
            if (!first)
                check((first = strdup(gr->gr_name)) != NULL, "strdup");
            else
                check(strcmp(gr->gr_name, first) == 0, "same first entry");
        } else if (count == 1) {
            if (!second)
                check((second = strdup(gr->gr_name)) != NULL, "strdup");
            else
                check(strcmp(gr->gr_name, second) == 0, "same second entry");
        }
        if (gr->gr_gid == 0)
            *found_root = 1;
        if (gr->gr_gid == getgid())
            *found_group = 1;
        check(++count < 10000000, "the database ends");
    }
    /* The end of the database is not an error */
    check(errno == 0, "errno at the end");
    return count;
}

int
main(void)
{
    unsigned long count, again;
    int           found_root, found_group, root_again, group_again;
    struct group *gr;

    count = walk(&found_root, &found_group);
    if (count == 0) {
        printf("empty group database, skipping\n");
        return 77;
    }
    check(found_root, "group 0 is in the database");

    /* setgrent rewinds; it leaves errno alone */
    errno = 0;
    p_setgrent();
    check(errno == 0, "setgrent errno");
    again = walk(&root_again, &group_again);
    check(again == count && root_again == found_root && group_again == found_group, "same walk");

    /* endgrent closes; the next getgrent starts over */
    errno = 0;
    p_endgrent();
    check(errno == 0, "endgrent errno");
    gr = p_getgrent();
    check(gr != NULL && strcmp(gr->gr_name, first) == 0, "first entry after endgrent");

    /* Lookups by name and uid don't move getgrent along */
    if (count > 1) {
        check(getgrnam(first) != NULL, "getgrnam");
        check(getgrgid(0) != NULL, "getgrgid");
        gr = getgrent();
        check(gr != NULL && strcmp(gr->gr_name, second) == 0, "second entry after lookups");
    }

    /* setgrent on a database open in the middle, and endgrent twice */
    setgrent();
    gr = getgrent();
    check(gr != NULL && strcmp(gr->gr_name, first) == 0, "first entry after setgrent");
    endgrent();
    errno = 0;
    endgrent();
    check(errno == 0, "endgrent when closed");

    free(first);
    free(second);
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
