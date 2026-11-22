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
#include <sys/resource.h>

/*
 * getgrnam and getgrgid find the root group and the running group,
 * with member lists holding only names, and leave errno alone when
 * nothing matches
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

static void
check_entry(const struct group *gr)
{
    size_t m;

    check(gr->gr_name != NULL && gr->gr_mem != NULL, "entry pointers");
    /* A null-terminated array of member names; none is empty */
    for (m = 0; gr->gr_mem[m]; m++)
        check(gr->gr_mem[m][0] != '\0', "member name");
}

/* Look the entry up by name, and expect it to have this gid */
static void
check_by_name(const char *name, gid_t gid)
{
    struct group *gr;

    errno = 0;
    gr = getgrnam(name);
    check(gr != NULL, "getgrnam of a name getgrgid returned");
    check_entry(gr);
    check(strcmp(gr->gr_name, name) == 0 && gr->gr_gid == gid, "getgrnam entry");
}

#if TEST_POSIX_FD_MGMT
/*
 * With every descriptor in use, the database can't be opened: the
 * lookups may fail with EMFILE, and if they do, they report it. Using
 * up the descriptors takes dup, from the descriptor management group.
 */
static void
check_emfile(void)
{
    struct rlimit lim, low;
    struct group  ent, *res;
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
    p = getgrnam("root");
    check(p != NULL || errno == EMFILE, "getgrnam with no descriptors");
    res = &ent;
    ret = getgrnam_r("root", &ent, buf, sizeof(buf), &res);
    check((ret == 0 && res == &ent) || (ret == EMFILE && res == NULL),
          "getgrnam_r with no descriptors");

    while (n > 0)
        close(fds[--n]);
    check(setrlimit(RLIMIT_NOFILE, &lim) == 0, "setrlimit");
}
#endif

int
main(void)
{
    struct group *gr;
    char         *name;
    gid_t         gid = getgid();

    /* Every system has a group 0 */
    errno = 0;
    gr = getgrgid(0);
    if (!gr) {
        check(errno == 0, "getgrgid(0) found nothing");
        printf("no group 0, skipping\n");
        return 77;
    }
    check_entry(gr);
    check(gr->gr_gid == 0, "getgrgid(0) entry");
    name = strdup(gr->gr_name);
    check(name != NULL, "strdup");
    check_by_name(name, 0);
    free(name);

    /* The running group, when the database has it */
    errno = 0;
    gr = getgrgid(gid);
    if (gr) {
        check_entry(gr);
        check(gr->gr_gid == gid, "getgrgid(getgid()) entry");
        name = strdup(gr->gr_name);
        check(name != NULL, "strdup");
        check_by_name(name, gid);
        free(name);
    } else {
        check(errno == 0, "getgrgid(getgid()) found nothing");
        printf("no entry for the running group\n");
    }

    /* No match leaves errno unchanged */
    errno = 0;
    check(getgrnam(NO_NAME) == NULL && errno == 0, "getgrnam of a missing name");
    errno = 0;
    check(getgrnam("") == NULL && errno == 0, "getgrnam of an empty name");
    if (gid != (gid_t)-2) {
        errno = 0;
        check(getgrgid((gid_t)-2) == NULL && errno == 0, "getgrgid of a missing gid");
    }

    /* And a lookup after one that failed still works */
    check(getgrgid(0) != NULL, "getgrgid(0) again");

#if TEST_POSIX_FD_MGMT
    check_emfile();
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
