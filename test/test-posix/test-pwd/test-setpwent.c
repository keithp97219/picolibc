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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_XSI_USER_GROUPS

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * getpwent walks the user database, leaving errno alone at the end;
 * setpwent rewinds it and endpwent closes it, also leaving errno alone
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The prototypes, with the types POSIX gives */
static void           (          *const p_endpwent)(void) = endpwent;
static struct passwd *(* const p_getpwent)(void) = getpwent;
static void           (          *const p_setpwent)(void) = setpwent;

static char          *first, *second;

/* Read the whole database, returning the number of entries */
static unsigned long
walk(int *found_root, int *found_user)
{
    unsigned long  count = 0;
    struct passwd *pw;
    long           login_max = sysconf(_SC_LOGIN_NAME_MAX);

    *found_root = *found_user = 0;
    for (;;) {
        errno = 0;
        pw = getpwent();
        if (!pw)
            break;
        check(pw->pw_name != NULL && pw->pw_dir != NULL && pw->pw_shell != NULL, "strings");
        /* {LOGIN_NAME_MAX} includes the null */
        if (login_max > 0)
            check(strlen(pw->pw_name) < (size_t)login_max, "name fits {LOGIN_NAME_MAX}");
        if (count == 0) {
            if (!first)
                check((first = strdup(pw->pw_name)) != NULL, "strdup");
            else
                check(strcmp(pw->pw_name, first) == 0, "same first entry");
        } else if (count == 1) {
            if (!second)
                check((second = strdup(pw->pw_name)) != NULL, "strdup");
            else
                check(strcmp(pw->pw_name, second) == 0, "same second entry");
        }
        if (pw->pw_uid == 0)
            *found_root = 1;
        if (pw->pw_uid == getuid())
            *found_user = 1;
        check(++count < 10000000, "the database ends");
    }
    /* The end of the database is not an error */
    check(errno == 0, "errno at the end");
    return count;
}

int
main(void)
{
    unsigned long  count, again;
    int            found_root, found_user, root_again, user_again;
    struct passwd *pw;

    count = walk(&found_root, &found_user);
    if (count == 0) {
        printf("empty user database, skipping\n");
        return 77;
    }
    check(found_root, "root is in the database");

    /* setpwent rewinds; it leaves errno alone */
    errno = 0;
    p_setpwent();
    check(errno == 0, "setpwent errno");
    again = walk(&root_again, &user_again);
    check(again == count && root_again == found_root && user_again == found_user, "same walk");

    /* endpwent closes; the next getpwent starts over */
    errno = 0;
    p_endpwent();
    check(errno == 0, "endpwent errno");
    pw = p_getpwent();
    check(pw != NULL && strcmp(pw->pw_name, first) == 0, "first entry after endpwent");

    /* Lookups by name and uid don't move getpwent along */
    if (count > 1) {
        check(getpwnam(first) != NULL, "getpwnam");
        check(getpwuid(0) != NULL, "getpwuid");
        pw = getpwent();
        check(pw != NULL && strcmp(pw->pw_name, second) == 0, "second entry after lookups");
    }

    /* setpwent on a database open in the middle, and endpwent twice */
    setpwent();
    pw = getpwent();
    check(pw != NULL && strcmp(pw->pw_name, first) == 0, "first entry after setpwent");
    endpwent();
    errno = 0;
    endpwent();
    check(errno == 0, "endpwent when closed");

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
