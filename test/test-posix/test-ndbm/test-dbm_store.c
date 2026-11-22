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
/* The ndbm functions are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_XSI_DBM) && defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * dbm_store inserts records, and with DBM_INSERT returns 1 and leaves
 * an existing record alone, while DBM_REPLACE replaces it; dbm_fetch
 * returns the content, or a null dptr for a missing key; dbm_delete
 * removes a record, failing with a negative value for a missing one;
 * dbm_firstkey and dbm_nextkey return each key once and then a null
 * dptr; keys and contents are arbitrary bytes, with pairs of 1023 bytes
 * supported; dbm_error is 0 until an error, and dbm_clearerr clears it.
 * A database of 20000 records (about 2MB) is written, read back after
 * reopening, walked and half deleted. Everything happens in a fresh
 * directory.
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

static datum
mem_datum(const void *p, size_t len)
{
    datum d;

    d.dptr = (void *)p;
    d.dsize = len;
    return d;
}

static datum
str_datum(const char *s)
{
    return mem_datum(s, strlen(s));
}

static int
fetch_is(DBM *db, datum key, const void *value, size_t len)
{
    datum d = dbm_fetch(db, key);

    if (value == NULL)
        return d.dptr == NULL;
    return d.dptr != NULL && d.dsize == len && memcmp(d.dptr, value, len) == 0;
}

#define NREC    20000
#define REC_LEN 100

static void
make_key(char *buf, size_t size, int i)
{
    snprintf(buf, size, "key%d", i);
}

static void
make_content(unsigned char *buf, int i)
{
    int j;

    for (j = 0; j < REC_LEN; j++)
        buf[j] = (unsigned char)(i * 31 + j * 7);
}

/* Walks the keys: each must be one of ours, seen once; returns the count or -1 */
static int
walk_keys(DBM *db, unsigned char *seen, int want_odd_only)
{
    datum key;
    int   n = 0, i;
    char  buf[32];

    memset(seen, 0, NREC);
    for (key = dbm_firstkey(db); key.dptr != NULL; key = dbm_nextkey(db)) {
        if (key.dsize < 4 || key.dsize >= sizeof(buf))
            return -1;
        memcpy(buf, key.dptr, key.dsize);
        buf[key.dsize] = '\0';
        if (strncmp(buf, "key", 3) != 0)
            return -1;
        i = atoi(buf + 3);
        if (i < 0 || i >= NREC || seen[i])
            return -1;
        if (want_odd_only && i % 2 == 0)
            return -1;
        seen[i] = 1;
        n++;
        if (n > NREC)
            return -1;
    }
    return n;
}

int
main(void)
{
    DBM                 *db, *db2;
    datum                key, d;
    static unsigned char seen[NREC];
    static unsigned char big[2][1023];
    unsigned char        content[REC_LEN];
    char                 kbuf[32];
    int                  i, n, ret, bad;
    static const char    bin1[] = { 'q', '\0', 'b' }, bin2[] = { 'q', '\0', 'c' };

    if (mkdtemp(dir) == NULL) {
        printf("mkdtemp failed\n");
        return 1;
    }
    check(chdir(dir) == 0, "chdir");

    db = dbm_open("db", O_RDWR | O_CREAT, 0600);
    check(db != NULL, "dbm_open");
    if (db == NULL)
        return 1;

    /* A new database: no error, no keys, nothing to fetch */
    check(dbm_error(db) == 0, "dbm_error initially");
    check(dbm_firstkey(db).dptr == NULL, "no first key");
    check(fetch_is(db, str_datum("a"), NULL, 0), "fetch missing");
    check(dbm_delete(db, str_datum("a")) < 0, "delete missing");

    /* DBM_INSERT adds a new record */
    check(dbm_store(db, str_datum("a"), str_datum("1"), DBM_INSERT) == 0, "insert new");
    check(fetch_is(db, str_datum("a"), "1", 1), "fetch new");

    /* DBM_INSERT of an existing key returns 1 and leaves the record */
    ret = dbm_store(db, str_datum("a"), str_datum("2"), DBM_INSERT);
    check(ret == 1, "insert existing returns 1");
    check(fetch_is(db, str_datum("a"), "1", 1), "insert existing keeps the record");

    /* DBM_REPLACE replaces an existing record, and adds a new one */
    check(dbm_store(db, str_datum("a"), str_datum("three"), DBM_REPLACE) == 0, "replace");
    check(fetch_is(db, str_datum("a"), "three", 5), "fetch replaced");
    check(dbm_store(db, str_datum("b"), str_datum("4"), DBM_REPLACE) == 0, "replace new");
    check(fetch_is(db, str_datum("b"), "4", 1), "fetch replace new");

    /* Keys are byte strings of dsize bytes, including null bytes */
    check(dbm_store(db, mem_datum(bin1, 3), str_datum("x"), DBM_INSERT) == 0, "binary key");
    check(dbm_store(db, mem_datum(bin2, 3), mem_datum(bin1, 3), DBM_INSERT) == 0, "binary 2");
    check(fetch_is(db, mem_datum(bin1, 3), "x", 1), "fetch binary");
    check(fetch_is(db, mem_datum(bin2, 3), bin1, 3), "fetch binary content");
    check(fetch_is(db, mem_datum(bin1, 1), NULL, 0), "a prefix is another key");
    check(fetch_is(db, mem_datum(bin1, 2), NULL, 0), "with its null, another key");

    /* Pairs of 1023 bytes, either way round */
    for (i = 0; i < 1023; i++) {
        big[0][i] = (unsigned char)i;
        big[1][i] = (unsigned char)(255 - i);
    }
    check(dbm_store(db, mem_datum(big[0], 23), mem_datum(big[1], 1000), DBM_INSERT) == 0,
          "short key, long content");
    check(dbm_store(db, mem_datum(big[1], 1000), mem_datum(big[0], 23), DBM_INSERT) == 0,
          "long key, short content");
    check(fetch_is(db, mem_datum(big[0], 23), big[1], 1000), "fetch long content");
    check(fetch_is(db, mem_datum(big[1], 1000), big[0], 23), "fetch long key");

    /* dbm_delete removes the record */
    check(dbm_delete(db, str_datum("b")) == 0, "delete");
    check(fetch_is(db, str_datum("b"), NULL, 0), "deleted");
    check(dbm_delete(db, str_datum("b")) < 0, "delete again");
    check(fetch_is(db, str_datum("a"), "three", 5), "others stay");

    /* The keys, each once */
    n = 0;
    bad = 0;
    for (key = dbm_firstkey(db); key.dptr != NULL; key = dbm_nextkey(db)) {
        n++;
        if (!(key.dsize == 1 && memcmp(key.dptr, "a", 1) == 0)
            && !(key.dsize == 3
                 && (memcmp(key.dptr, bin1, 3) == 0 || memcmp(key.dptr, bin2, 3) == 0))
            && !(key.dsize == 23 && memcmp(key.dptr, big[0], 23) == 0)
            && !(key.dsize == 1000 && memcmp(key.dptr, big[1], 1000) == 0))
            bad++;
        if (n > 10)
            break;
    }
    check(n == 5 && bad == 0, "keys");
    /* At the end, dbm_nextkey keeps returning a null dptr */
    check(dbm_nextkey(db).dptr == NULL, "past the end");
    check(dbm_error(db) == 0, "no error");

    /* Two databases at once are separate */
    db2 = dbm_open("db2", O_RDWR | O_CREAT, 0600);
    check(db2 != NULL, "second database");
    if (db2) {
        check(dbm_store(db2, str_datum("a"), str_datum("other"), DBM_INSERT) == 0, "store db2");
        check(fetch_is(db2, str_datum("a"), "other", 5), "fetch db2");
        check(fetch_is(db, str_datum("a"), "three", 5), "db unchanged");
        check(fetch_is(db2, mem_datum(bin1, 3), NULL, 0), "db2 separate");
        dbm_close(db2);
    }
    dbm_close(db);

    /* A larger database, kept across dbm_close */
    db = dbm_open("big", O_RDWR | O_CREAT, 0600);
    check(db != NULL, "dbm_open big");
    if (db == NULL)
        return 1;
    bad = 0;
    for (i = 0; i < NREC; i++) {
        make_key(kbuf, sizeof(kbuf), i);
        make_content(content, i);
        if (dbm_store(db, str_datum(kbuf), mem_datum(content, REC_LEN), DBM_INSERT) != 0)
            bad++;
    }
    check(bad == 0, "store many");
    check(dbm_error(db) == 0, "no error after storing many");
    dbm_close(db);

    db = dbm_open("big", O_RDONLY, 0);
    check(db != NULL, "reopen big");
    if (db == NULL)
        return 1;
    bad = 0;
    for (i = 0; i < NREC; i++) {
        make_key(kbuf, sizeof(kbuf), i);
        make_content(content, i);
        if (!fetch_is(db, str_datum(kbuf), content, REC_LEN))
            bad++;
    }
    check(bad == 0, "fetch many");
    check(walk_keys(db, seen, 0) == NREC, "walk many");

    /* Writing a read-only database fails; dbm_clearerr clears any error it set */
    check(dbm_store(db, str_datum("key0"), str_datum("z"), DBM_REPLACE) < 0, "store O_RDONLY");
    dbm_clearerr(db);
    check(dbm_error(db) == 0, "dbm_clearerr");
    dbm_close(db);

    db = dbm_open("big", O_RDWR, 0);
    check(db != NULL, "reopen big O_RDWR");
    if (db == NULL)
        return 1;
    bad = 0;
    for (i = 0; i < NREC; i += 2) {
        make_key(kbuf, sizeof(kbuf), i);
        if (dbm_delete(db, str_datum(kbuf)) != 0)
            bad++;
    }
    check(bad == 0, "delete half");
    check(walk_keys(db, seen, 1) == NREC / 2, "walk half");
    bad = 0;
    for (i = 0; i < NREC; i++) {
        make_key(kbuf, sizeof(kbuf), i);
        make_content(content, i);
        d = dbm_fetch(db, str_datum(kbuf));
        if (i % 2 == 0 ? d.dptr != NULL
                       : d.dptr == NULL || d.dsize != REC_LEN || memcmp(d.dptr, content, REC_LEN))
            bad++;
    }
    check(bad == 0, "fetch half");
    dbm_close(db);

    /* Clean up */
    {
        static const char * const names[] = { "db", "db2", "big" };
        static const char * const suffixes[] = { ".db", ".dir", ".pag" };
        unsigned                  a, b;

        for (a = 0; a < sizeof(names) / sizeof(names[0]); a++)
            for (b = 0; b < sizeof(suffixes) / sizeof(suffixes[0]); b++) {
                snprintf(kbuf, sizeof(kbuf), "%s%s", names[a], suffixes[b]);
                unlink(kbuf);
            }
        check(chdir("..") == 0, "chdir ..");
        check(rmdir(dir) == 0, "only the database files");
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
