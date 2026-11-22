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
/* dbm_open is an XSI interface */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_XSI_DBM && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_ATTRIBUTES && TEST_POSIX_FD_MGMT

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * dbm_open keeps a database in file.db, or in file.dir and file.pag,
 * with the file argument naming where; O_CREAT makes it with the
 * process's owner, a group and the given mode (less the umask); without
 * O_CREAT a missing database is an error; O_EXCL fails for an existing
 * one; O_TRUNC empties it, keeping the mode; O_RDONLY databases can't
 * be written; O_WRONLY ones open their files for reading and writing;
 * O_CLOEXEC sets FD_CLOEXEC. Everything happens in a fresh directory.
 */

static int errors;

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            errors++;                                  \
        }                                              \
    } while (0)

static char dir[] = "tmp-dbm-XXXXXX";

#define MAX_FILES 4

struct db_files {
    int         n;
    struct stat st[MAX_FILES];
};

/*
 * Finds the database files of 'base' in 'path', which must be either
 * base.db alone or base.dir and base.pag. Every other name starting with
 * base is an error. Returns the number of files, or -1.
 */
static int
db_files(const char *path, const char *base, struct db_files *f)
{
    DIR           *d = opendir(path);
    struct dirent *de;
    size_t         len = strlen(base);
    int            db = 0, dirf = 0, pag = 0, bad = 0;
    char           name[PATH_MAX];

    f->n = 0;
    if (!d)
        return -1;
    while ((de = readdir(d)) != NULL) {
        if (strncmp(de->d_name, base, len) != 0)
            continue;
        if (strcmp(de->d_name + len, ".db") == 0)
            db++;
        else if (strcmp(de->d_name + len, ".dir") == 0)
            dirf++;
        else if (strcmp(de->d_name + len, ".pag") == 0)
            pag++;
        else {
            printf("unexpected file %s/%s\n", path, de->d_name);
            bad++;
            continue;
        }
        snprintf(name, sizeof(name), "%s/%s", path, de->d_name);
        if (f->n < MAX_FILES && stat(name, &f->st[f->n]) == 0)
            f->n++;
        else
            bad++;
    }
    closedir(d);
    if (bad)
        return -1;
    if (db == 1 && dirf == 0 && pag == 0)
        return 1;
    if (db == 0 && dirf == 1 && pag == 1)
        return 2;
    if (db == 0 && dirf == 0 && pag == 0)
        return 0;
    return -1;
}

/* Every other entry of 'path' must be . or .. or start with one of the names */
static int
only_entries(const char *path, const char * const *names, int nnames)
{
    DIR           *d = opendir(path);
    struct dirent *de;
    int            i, bad = 0;

    if (!d)
        return 0;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        for (i = 0; i < nnames; i++)
            if (strncmp(de->d_name, names[i], strlen(names[i])) == 0)
                break;
        if (i == nnames) {
            printf("unexpected entry %s/%s\n", path, de->d_name);
            bad++;
        }
    }
    closedir(d);
    return bad == 0;
}

#define MAX_FD 256

/* Which descriptors are open */
static void
open_fds(char *fds)
{
    int fd;

    for (fd = 0; fd < MAX_FD; fd++)
        fds[fd] = fcntl(fd, F_GETFD) != -1;
}

static datum
str_datum(const char *s)
{
    datum d;

    d.dptr = (void *)s;
    d.dsize = strlen(s);
    return d;
}

static int
fetch_is(DBM *db, const char *key, const char *value)
{
    datum d = dbm_fetch(db, str_datum(key));

    if (value == NULL)
        return d.dptr == NULL;
    return d.dptr != NULL && d.dsize == strlen(value) && memcmp(d.dptr, value, d.dsize) == 0;
}

/* The mode of each file of the database */
static int
modes_are(const char *path, const char *base, mode_t mode)
{
    struct db_files f;
    int             i;

    if (db_files(path, base, &f) <= 0)
        return 0;
    for (i = 0; i < f.n; i++)
        if ((f.st[i].st_mode & 07777) != mode || !S_ISREG(f.st[i].st_mode))
            return 0;
    return 1;
}

static int
chmod_db(const char *path, const char *base, mode_t mode)
{
    static const char * const suffixes[] = { ".db", ".dir", ".pag" };
    char                      name[PATH_MAX];
    unsigned                  i;
    int                       n = 0;

    for (i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        snprintf(name, sizeof(name), "%s/%s%s", path, base, suffixes[i]);
        if (chmod(name, mode) == 0)
            n++;
    }
    return n > 0;
}

int
main(void)
{
    DBM                      *db;
    struct db_files           f;
    struct stat               dst;
    char                      before[MAX_FD], after[MAX_FD];
    char                      longname[NAME_MAX + 1];
    int                       fd, n, i, found;
    static const char * const top[] = { "sub" };
    static const char * const sub[] = { "data", "excl", "wr", "ce", "x" };

    check(mkdtemp(dir) != NULL, "mkdtemp");
    if (errors)
        return 1;
    /* The scratch directory may inherit set-group-ID; files take the process's group */
    check(chmod(dir, 0700) == 0, "chmod");
    check(chdir(dir) == 0, "chdir");
    check(mkdir("sub", 0700) == 0, "mkdir");
    umask(027);

    /* O_CREAT makes the database where the file argument says */
    db = dbm_open("sub/data", O_RDWR | O_CREAT, 0666);
    check(db != (DBM *)0, "dbm_open O_CREAT");
    if (db == NULL)
        return 1;
    check(dbm_store(db, str_datum("k1"), str_datum("v1"), DBM_INSERT) == 0, "store");
    check(fetch_is(db, "k1", "v1"), "fetch");
    dbm_close(db);

    n = db_files("sub", "data", &f);
    check(n == 1 || n == 2, "data.db, or data.dir and data.pag");
    check(only_entries(".", top, 1), "nothing outside sub");
    printf("database in %d file%s\n", n, n == 1 ? "" : "s");

    /* Owner, group and mode */
    check(stat("sub", &dst) == 0, "stat sub");
    for (i = 0; i < f.n; i++) {
        check(S_ISREG(f.st[i].st_mode), "regular file");
        check((f.st[i].st_mode & 07777) == 0640, "mode less umask");
        check(f.st[i].st_uid == geteuid(), "owner");
        check(f.st[i].st_gid == getegid() || f.st[i].st_gid == dst.st_gid, "group");
    }

    /* O_RDONLY reads the database back, and can't change it */
    db = dbm_open("sub/data", O_RDONLY, 0);
    check(db != NULL, "dbm_open O_RDONLY");
    if (db) {
        check(fetch_is(db, "k1", "v1"), "fetch after reopening");
        check(dbm_store(db, str_datum("k2"), str_datum("v2"), DBM_INSERT) < 0, "store O_RDONLY");
        check(dbm_store(db, str_datum("k1"), str_datum("v9"), DBM_REPLACE) < 0, "replace O_RDONLY");
        check(dbm_delete(db, str_datum("k1")) < 0, "delete O_RDONLY");
        dbm_clearerr(db);
        check(dbm_error(db) == 0, "dbm_clearerr");
        check(fetch_is(db, "k1", "v1") && fetch_is(db, "k2", NULL), "unchanged O_RDONLY");
        dbm_close(db);
    }

    /* Without O_CREAT, a missing database is an error and nothing is made */
    db = dbm_open("sub/none", O_RDWR, 0666);
    check(db == (DBM *)0, "missing database");
    if (db)
        dbm_close(db);
    check(db_files("sub", "none", &f) == 0, "no files for a missing database");

    /* O_CREAT | O_EXCL fails for an existing database and leaves it alone */
    db = dbm_open("sub/data", O_RDWR | O_CREAT | O_EXCL, 0666);
    check(db == (DBM *)0, "O_EXCL existing");
    if (db)
        dbm_close(db);
    db = dbm_open("sub/data", O_RDONLY, 0);
    check(db != NULL && fetch_is(db, "k1", "v1"), "intact after O_EXCL");
    if (db)
        dbm_close(db);

    /* O_CREAT | O_EXCL makes a new one */
    db = dbm_open("sub/excl", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(db != NULL, "O_EXCL new");
    if (db) {
        check(dbm_store(db, str_datum("e"), str_datum("1"), DBM_INSERT) == 0, "store O_EXCL");
        dbm_close(db);
    }
    check(modes_are("sub", "excl", 0600), "O_EXCL mode");

    /* O_CREAT on an existing database has no effect */
    db = dbm_open("sub/data", O_RDWR | O_CREAT, 0600);
    check(db != NULL && fetch_is(db, "k1", "v1"), "O_CREAT existing");
    if (db)
        dbm_close(db);
    check(modes_are("sub", "data", 0640), "O_CREAT existing keeps the mode");

    /* O_TRUNC empties the database and keeps the mode */
    check(chmod_db("sub", "data", 0600), "chmod data");
    db = dbm_open("sub/data", O_RDWR | O_TRUNC, 0666);
    check(db != NULL, "dbm_open O_TRUNC");
    if (db) {
        check(dbm_firstkey(db).dptr == NULL, "empty after O_TRUNC");
        check(fetch_is(db, "k1", NULL), "k1 gone after O_TRUNC");
        check(dbm_store(db, str_datum("t"), str_datum("2"), DBM_INSERT) == 0, "store O_TRUNC");
        dbm_close(db);
    }
    check(modes_are("sub", "data", 0600), "O_TRUNC keeps the mode");
    db = dbm_open("sub/data", O_RDONLY, 0);
    check(db != NULL && fetch_is(db, "t", "2") && fetch_is(db, "k1", NULL), "after O_TRUNC");
    if (db)
        dbm_close(db);

    /*
     * O_WRONLY opens the database for writing, and its files for
     * reading and writing
     */
    open_fds(before);
    db = dbm_open("sub/wr", O_WRONLY | O_CREAT, 0600);
    check(db != NULL, "dbm_open O_WRONLY");
    if (db) {
        open_fds(after);
        found = 0;
        for (fd = 0; fd < MAX_FD; fd++) {
            if (after[fd] && !before[fd]) {
                found++;
                check((fcntl(fd, F_GETFL) & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDWR,
                      "O_WRONLY files opened O_RDWR");
            }
        }
        check(found > 0, "O_WRONLY descriptor");
        check(dbm_store(db, str_datum("w"), str_datum("3"), DBM_INSERT) == 0, "store O_WRONLY");
        dbm_close(db);
    }
    db = dbm_open("sub/wr", O_RDONLY, 0);
    check(db != NULL && fetch_is(db, "w", "3"), "after O_WRONLY");
    if (db)
        dbm_close(db);

    /* O_CLOEXEC sets FD_CLOEXEC on every descriptor the database uses */
    open_fds(before);
    db = dbm_open("sub/ce", O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    check(db != NULL, "dbm_open O_CLOEXEC");
    if (db) {
        open_fds(after);
        found = 0;
        for (fd = 0; fd < MAX_FD; fd++) {
            if (after[fd] && !before[fd]) {
                found++;
                check((fcntl(fd, F_GETFD) & FD_CLOEXEC) != 0, "FD_CLOEXEC");
            }
        }
        check(found > 0, "O_CLOEXEC descriptor");
        dbm_close(db);
    }
    /* dbm_close closes them */
    open_fds(after);
    check(memcmp(before, after, sizeof(before)) == 0, "dbm_close closes the descriptors");

    /* A last component of {NAME_MAX}-4 bytes is fine */
    memset(longname, 'x', NAME_MAX - 4);
    longname[NAME_MAX - 4] = '\0';
    {
        char path[PATH_MAX];

        snprintf(path, sizeof(path), "sub/%s", longname);
        db = dbm_open(path, O_RDWR | O_CREAT, 0600);
        check(db != NULL, "NAME_MAX - 4 component");
        if (db) {
            check(dbm_store(db, str_datum("l"), str_datum("4"), DBM_INSERT) == 0, "store long");
            dbm_close(db);
        }
        n = db_files("sub", longname, &f);
        check(n == 1 || n == 2, "long name files");
    }

    /* Without write permission in the directory, nothing can be created */
    if (geteuid() != 0) {
        check(chmod("sub", 0500) == 0, "chmod sub");
        errno = 0;
        db = dbm_open("sub/ro", O_RDWR | O_CREAT, 0600);
        check(db == (DBM *)0, "read-only directory");
        if (db)
            dbm_close(db);
        check(chmod("sub", 0700) == 0, "chmod sub back");
    }
    check(only_entries("sub", sub, sizeof(sub) / sizeof(sub[0])), "only the databases in sub");

#if TEST_POSIX_USER_GROUPS
    /*
     * The files may take the directory's group: with set-group-ID on
     * the directory, they do on Linux. Needs a second group to see it.
     */
    {
        gid_t groups[64], other = (gid_t)-1;
        int   ng = getgroups(sizeof(groups) / sizeof(groups[0]), groups), k;

        for (k = 0; k < ng; k++)
            if (groups[k] != getegid()) {
                other = groups[k];
                break;
            }
        if (other == (gid_t)-1) {
            printf("no other group: skipping the directory group check\n");
        } else {
            check(mkdir("sub/sg", 0700) == 0, "mkdir sg");
            check(chown("sub/sg", (uid_t)-1, other) == 0, "chown sg");
            check(chmod("sub/sg", 02700) == 0, "chmod sg");
            db = dbm_open("sub/sg/g", O_RDWR | O_CREAT, 0600);
            check(db != NULL, "dbm_open in a set-group-ID directory");
            if (db)
                dbm_close(db);
            n = db_files("sub/sg", "g", &f);
            check(n == 1 || n == 2, "files in sg");
            for (i = 0; i < f.n; i++)
                check(f.st[i].st_gid == other, "the directory's group");
            unlink("sub/sg/g.db");
            unlink("sub/sg/g.dir");
            unlink("sub/sg/g.pag");
            check(rmdir("sub/sg") == 0, "rmdir sg");
        }
    }
#endif

    /* Clean up */
    {
        DIR           *d = opendir("sub");
        struct dirent *de;
        char           name[PATH_MAX];

        if (d) {
            while ((de = readdir(d)) != NULL) {
                if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                    continue;
                snprintf(name, sizeof(name), "sub/%s", de->d_name);
                unlink(name);
            }
            closedir(d);
        }
        rmdir("sub");
        check(chdir("..") == 0, "chdir ..");
        rmdir(dir);
    }

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("skipping: no file system support\n");
    return 77;
}

#endif
