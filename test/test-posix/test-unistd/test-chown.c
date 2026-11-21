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
#if defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES) && defined(__PICOLIBC_POSIX_FILE_SYSTEM)       \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS_FD) && defined(__PICOLIBC_POSIX_USER_GROUPS)  \
    && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES_FD)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * chown, fchown, lchown and fchownat: what an owner without privileges
 * may change (its own uid, its own groups), the set-ID bits, links,
 * descriptors and the errors, which change nothing
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)chmod("sub", 0700);
    (void)unlink("file");
    (void)unlink("link");
    (void)unlink("loop");
    (void)unlink("sub/f");
    (void)rmdir("sub");
    if (chdir("..") == 0)
        (void)rmdir(dir);
}

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            cleanup();                                                                 \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#define check_err(expr, err, message) check((errno = 0, (expr) == -1) && errno == (err), message)

static struct stat
info(const char *path, int nofollow)
{
    struct stat st;

    check(fstatat(AT_FDCWD, path, &st, nofollow ? AT_SYMLINK_NOFOLLOW : 0) == 0, "fstatat");
    return st;
}

static gid_t *groups;

/* A group the process is in, other than its effective group, or -1 */
static gid_t
other_group(int n)
{
    int i;

    for (i = 0; i < n; i++)
        if (groups[i] != getegid())
            return groups[i];
    return (gid_t)-1;
}

/* A group the process isn't in, or -1 */
static gid_t
foreign_group(int n)
{
    gid_t g;
    int   i;

    for (g = 1; g < 100000; g++) {
        if (g == getegid())
            continue;
        for (i = 0; i < n; i++)
            if (groups[i] == g)
                break;
        if (i == n)
            return g;
    }
    return (gid_t)-1;
}

static char long_name[NAME_MAX + 2];

int
main(void)
{
    struct stat     st;
    struct timespec old[2] = {
        { 1000, 0 },
        { 1000, 0 }
    };
    uid_t uid = geteuid();
    gid_t gid = getegid(), mine, theirs, link_gid;
    int   fd, dfd, n, root = uid == 0;

    n = getgroups(0, NULL);
    check(n >= 0, "count groups");
    groups = calloc((size_t)n + 1, sizeof(gid_t));
    check(groups != NULL, "calloc");
    n = getgroups(n, groups);
    check(n >= 0, "getgroups");
    mine = other_group(n);
    theirs = foreign_group(n);

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    /*
     * A set-group-ID parent passes its bit on. New files take the
     * effective group or, on BSD, the directory's; the link keeps the
     * one it got
     */
    check(chmod(".", 0700) == 0, "clear set-group-ID");
    fd = open("file", O_RDWR | O_CREAT | O_EXCL, 0700);
    check(fd >= 0, "create");
    check(symlink("file", "link") == 0, "symlink");
    link_gid = info("link", 1).st_gid;

    /* The owner may set its own uid and group, and -1 changes nothing */
    check(chown("file", uid, gid) == 0, "chown to the same owner");
    check(chown("file", (uid_t)-1, (gid_t)-1) == 0, "chown -1, -1");
    st = info("file", 0);
    check(st.st_uid == uid && st.st_gid == gid, "owner unchanged");

    /* The change time is marked for update */
    check(futimens(fd, old) == 0, "set old times");
    check(chown("file", uid, gid) == 0 && info("file", 0).st_ctime > 1000, "change time updated");

    /* Any supplementary group is allowed; -1 keeps the uid */
    if (mine != (gid_t)-1) {
        check(chown("file", (uid_t)-1, mine) == 0 && info("file", 0).st_gid == mine,
              "chown to a supplementary group");
        check(fchown(fd, uid, gid) == 0 && info("file", 0).st_gid == gid, "fchown back");
        /* chown follows the link, lchown changes the link itself */
        check(chown("link", (uid_t)-1, mine) == 0, "chown through a link");
        check(info("file", 0).st_gid == mine && info("link", 1).st_gid == link_gid,
              "target changed");
        check(chown("file", (uid_t)-1, gid) == 0, "restore");
        check(lchown("link", (uid_t)-1, mine) == 0, "lchown");
        check(info("link", 1).st_gid == mine && info("file", 0).st_gid == gid, "link changed");
        check(fchownat(AT_FDCWD, "link", (uid_t)-1, gid, AT_SYMLINK_NOFOLLOW) == 0
                  && info("link", 1).st_gid == gid,
              "fchownat AT_SYMLINK_NOFOLLOW");
        check(fchownat(AT_FDCWD, "link", (uid_t)-1, mine, 0) == 0 && info("file", 0).st_gid == mine
                  && info("link", 1).st_gid == gid,
              "fchownat follows links");
        check(chown("file", (uid_t)-1, gid) == 0, "restore");
    }

    /* Without privileges, the set-user-ID and set-group-ID bits go */
    if (!root) {
        check(chmod("file", 06755) == 0, "set-ID bits");
        if ((info("file", 0).st_mode & 06000) == 06000) {
            /*
             * Change the group when there's another to change to;
             * FreeBSD's UFS clears the bits only on a real change
             */
            gid_t to = mine != (gid_t)-1 ? mine : gid;

            check(chown("file", (uid_t)-1, to) == 0, "chown");
            check((info("file", 0).st_mode & 06000) == 0, "chown clears the set-ID bits");
            check(chown("file", (uid_t)-1, gid) == 0, "restore the group");
        }
        check(chmod("file", 06755) == 0
                  && fchown(fd, uid, mine != (gid_t)-1 ? mine : (gid_t)-1) == 0,
              "fchown");
        check((info("file", 0).st_mode & S_ISUID) == 0, "fchown clears set-user-ID");
        check(fchown(fd, (uid_t)-1, gid) == 0, "restore the group");
        check(chmod("file", 0700) == 0, "chmod");
    }

    /* EPERM: another user, or a group the process isn't in (not as root) */
    if (!root) {
        check_err(chown("file", uid + 1, (gid_t)-1), EPERM, "chown to another user");
        check_err(chown("file", 0, (gid_t)-1), EPERM, "chown to root");
        check_err(fchown(fd, uid + 1, (gid_t)-1), EPERM, "fchown to another user");
        check_err(lchown("link", uid + 1, (gid_t)-1), EPERM, "lchown to another user");
        check_err(fchownat(AT_FDCWD, "file", uid + 1, (gid_t)-1, 0), EPERM,
                  "fchownat to another user");
        if (theirs != (gid_t)-1) {
            check_err(chown("file", (uid_t)-1, theirs), EPERM, "chown to a foreign group");
            check_err(fchown(fd, (uid_t)-1, theirs), EPERM, "fchown to a foreign group");
        }
        /*
         * A file owned by someone else. (Linux lets anyone "change" it
         * with -1 and -1, which changes nothing; that isn't checked.)
         */
        if (stat("/", &st) == 0 && st.st_uid != uid)
            check_err(chown("/", (uid_t)-1, st.st_gid), EPERM, "chown of another user's file");
        st = info("file", 0);
        check(st.st_uid == uid && st.st_gid == gid, "nothing changed");
    }

    /* EBADF from fchown for descriptors that aren't open, AT_FDCWD's value too */
    check_err(fchown(-1, uid, gid), EBADF, "fchown(-1)");
    check_err(fchown(AT_FDCWD, (uid_t)-1, (gid_t)-1), EBADF, "fchown(AT_FDCWD)");
    check_err(fchown(-100, (uid_t)-1, (gid_t)-1), EBADF, "fchown(-100)");

    /* Pathname errors */
    check_err(chown("missing", uid, gid), ENOENT, "missing");
    check_err(chown("", uid, gid), ENOENT, "empty path");
    check_err(lchown("", uid, gid), ENOENT, "lchown empty path");
    check_err(chown("file/x", uid, gid), ENOTDIR, "prefix not a directory");
    check_err(chown("file/", uid, gid), ENOTDIR, "trailing slash on a file");
    check_err(lchown("file/", uid, gid), ENOTDIR, "lchown trailing slash on a file");
    check(symlink("loop", "loop") == 0, "symlink loop");
    check_err(chown("loop", uid, gid), ELOOP, "loop");
    check(lchown("loop", uid, gid) == 0, "lchown doesn't follow the loop");
    memset(long_name, 'n', NAME_MAX + 1);
    check_err(chown(long_name, uid, gid), ENAMETOOLONG, "long component");
    check(mkdir("sub", 0700) == 0 && close(open("sub/f", O_WRONLY | O_CREAT, 0600)) == 0, "sub/f");
    if (!root) {
        check(chmod("sub", 0600) == 0, "no search permission");
        check_err(chown("sub/f", uid, gid), EACCES, "no search permission in the prefix");
        check(chmod("sub", 0700) == 0, "restore");
    }

    /* fchownat: relative to the descriptor; EBADF, ENOTDIR, EINVAL */
    dfd = open("sub", O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open directory");
    check(fchownat(dfd, "f", uid, gid, 0) == 0, "fchownat relative");
    check_err(fchownat(-1, "f", uid, gid, 0), EBADF, "bad descriptor");
    check_err(fchownat(fd, "f", uid, gid, 0), ENOTDIR, "file descriptor");
    check_err(fchownat(dfd, "f", uid, gid, 0x40000000), EINVAL, "bad flag");
    if (!root) {
        check(chmod("sub", 0600) == 0, "no search permission");
        check_err(fchownat(dfd, "f", uid, gid, 0), EACCES,
                  "no search permission on the descriptor");
        check(chmod("sub", 0700) == 0, "restore");
    }
    check(close(dfd) == 0 && close(fd) == 0, "close");

    free(groups);
    cleanup();
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
