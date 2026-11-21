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
#if defined(__PICOLIBC_POSIX_USER_GROUPS)

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

/*
 * getpwnam and getpwuid find root and the running user, and leave
 * errno alone when nothing matches
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* ':' separates the fields of the user database, so no name holds one */
#define NO_NAME "no:such:user"

static void
check_entry(const struct passwd *pw)
{
    check(pw->pw_name != NULL && pw->pw_dir != NULL && pw->pw_shell != NULL, "entry strings");
}

/* Look the entry up by name, and expect it to have this uid */
static void
check_by_name(const char *name, uid_t uid)
{
    struct passwd *pw;

    errno = 0;
    pw = getpwnam(name);
    check(pw != NULL, "getpwnam of a name getpwuid returned");
    check_entry(pw);
    check(strcmp(pw->pw_name, name) == 0 && pw->pw_uid == uid, "getpwnam entry");
}

/*
 * With every descriptor in use, the database can't be opened: the
 * lookups may fail with EMFILE, and if they do, they report it
 */
static void
check_emfile(void)
{
    struct rlimit lim, low;
    struct passwd ent, *res;
    char          buf[4096];
    int           fds[64];
    int           n = 0, ret;
    void         *p;

    if (getrlimit(RLIMIT_NOFILE, &lim) != 0)
        return;
    low = lim;
    low.rlim_cur = 32;
    if (lim.rlim_cur != RLIM_INFINITY && lim.rlim_cur < low.rlim_cur)
        low.rlim_cur = lim.rlim_cur;
    check(setrlimit(RLIMIT_NOFILE, &low) == 0, "setrlimit");
    while (n < 64 && (fds[n] = dup(1)) >= 0)
        n++;
    check(n < 64 && errno == EMFILE, "descriptors used up");

    errno = 0;
    p = getpwnam("root");
    check(p != NULL || errno == EMFILE, "getpwnam with no descriptors");
    res = &ent;
    ret = getpwnam_r("root", &ent, buf, sizeof(buf), &res);
    check((ret == 0 && res == &ent) || (ret == EMFILE && res == NULL),
          "getpwnam_r with no descriptors");

    while (n > 0)
        close(fds[--n]);
    check(setrlimit(RLIMIT_NOFILE, &lim) == 0, "setrlimit");
}

int
main(void)
{
    struct passwd *pw;
    char          *name;
    uid_t          uid = getuid();

    /* Every system has root */
    errno = 0;
    pw = getpwuid(0);
    if (!pw) {
        check(errno == 0, "getpwuid(0) found nothing");
        printf("no root entry, skipping\n");
        return 77;
    }
    check_entry(pw);
    check(pw->pw_uid == 0, "getpwuid(0) entry");
    name = strdup(pw->pw_name);
    check(name != NULL, "strdup");
    check_by_name(name, 0);
    free(name);

    /* The running user, when the database has it */
    errno = 0;
    pw = getpwuid(uid);
    if (pw) {
        check_entry(pw);
        check(pw->pw_uid == uid, "getpwuid(getuid()) entry");
        name = strdup(pw->pw_name);
        check(name != NULL, "strdup");
        check_by_name(name, uid);
        free(name);
    } else {
        check(errno == 0, "getpwuid(getuid()) found nothing");
        printf("no entry for the running user\n");
    }

    /* No match leaves errno unchanged */
    errno = 0;
    check(getpwnam(NO_NAME) == NULL && errno == 0, "getpwnam of a missing name");
    errno = 0;
    check(getpwnam("") == NULL && errno == 0, "getpwnam of an empty name");
    if (uid != (uid_t)-2) {
        errno = 0;
        check(getpwuid((uid_t)-2) == NULL && errno == 0, "getpwuid of a missing uid");
    }

    /* And a lookup after one that failed still works */
    check(getpwuid(0) != NULL, "getpwuid(0) again");

    check_emfile();

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
