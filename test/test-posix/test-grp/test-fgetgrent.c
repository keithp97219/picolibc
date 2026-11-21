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

/* fgetgrent and fgetgrent_r are GNU extensions */
#define _GNU_SOURCE
#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * fgetgrent and fgetgrent_r parse group database lines the way
 * getgrnam, getgrgid and getgrent do: lines that aren't entries are
 * skipped, ids are unsigned, an empty member field means no members,
 * and long lines work
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Members of the "many" group: "m000" ... */
#define MANY      300
#define MANY_SIZE 5

static const char         head[] = "root:x:0:\n"
                                   "\n"
                                   "short:x\n"
                                   "emptygid:x::a\n"
                                   "badgid:x:7z:a\n"
                                   "high:x:4294967294:a,b\n"
                                   "many:x:9:";
static const char         tail[] = "\n"
                                   "last:x:10:z\n";

static const char * const none[] = { NULL };
static const char * const ab[] = { "a", "b", NULL };
static const char * const z[] = { "z", NULL };

static const struct {
    const char         *name;
    unsigned long       gid;
    const char * const *mem;
} expect[] = {
    { "root", 0,          none },
    { "high", 4294967294, ab   },
    { "many", 9,          NULL },
    { "last", 10,         z    },
};

#define NEXPECT (sizeof(expect) / sizeof(expect[0]))

static char contents[sizeof(head) + MANY * MANY_SIZE + sizeof(tail)];

static void
check_entry(const struct group *gr, size_t e)
{
    size_t m;
    char   member[MANY_SIZE];

    if (strcmp(gr->gr_name, expect[e].name) != 0 || gr->gr_gid != expect[e].gid) {
        printf("entry %zu: got '%s' %lu want '%s' %lu\n", e, gr->gr_name, (unsigned long)gr->gr_gid,
               expect[e].name, expect[e].gid);
        exit(1);
    }
    for (m = 0; gr->gr_mem[m]; m++) {
        if (expect[e].mem) {
            check(expect[e].mem[m] != NULL && strcmp(gr->gr_mem[m], expect[e].mem[m]) == 0,
                  "member");
        } else {
            snprintf(member, sizeof(member), "m%03zu", m);
            check(m < MANY && strcmp(gr->gr_mem[m], member) == 0, "many member");
        }
    }
    if (expect[e].mem)
        check(expect[e].mem[m] == NULL, "member count");
    else
        check(m == MANY, "many member count");
}

static FILE *
open_contents(void)
{
    FILE *f = fmemopen(contents, strlen(contents), "r");

    check(f != NULL, "fmemopen");
    return f;
}

int
main(void)
{
    static char  buf[4096];
    struct group ent, *gr;
    FILE        *f;
    size_t       e, m;
    char        *p;
    int          ret;

    if (sizeof(gid_t) < 4) {
        printf("gid_t narrower than 32 bits, skipping\n");
        return 77;
    }

    p = contents;
    strcpy(p, head);
    p += strlen(head);
    for (m = 0; m < MANY; m++) {
        sprintf(p, "%sm%03zu", m ? "," : "", m);
        p += strlen(p);
    }
    strcpy(p, tail);

    /* fgetgrent_r, with a buffer big enough for every line */
    f = open_contents();
    for (e = 0; e < NEXPECT; e++) {
        ret = fgetgrent_r(f, &ent, buf, sizeof(buf), &gr);
        check(ret == 0 && gr == &ent, "fgetgrent_r");
        check_entry(gr, e);
    }
    gr = &ent;
    check(fgetgrent_r(f, &ent, buf, sizeof(buf), &gr) == ENOENT && gr == NULL, "fgetgrent_r end");
    fclose(f);

    /*
     * A buffer too small for the long entry gives ERANGE, and leaves it
     * to be read again with a bigger one
     */
    f = open_contents();
    check(fgetgrent_r(f, &ent, buf, sizeof(buf), &gr) == 0, "fgetgrent_r");
    check_entry(gr, 0);
    check(fgetgrent_r(f, &ent, buf, sizeof(buf), &gr) == 0, "fgetgrent_r");
    check_entry(gr, 1);
    gr = &ent;
    check(fgetgrent_r(f, &ent, buf, 100, &gr) == ERANGE && gr == NULL, "fgetgrent_r ERANGE");
    check(fgetgrent_r(f, &ent, buf, sizeof(buf), &gr) == 0, "fgetgrent_r after ERANGE");
    check_entry(gr, 2);
    fclose(f);

    /* fgetgrent, whose own buffer must hold the long entry */
    f = open_contents();
    for (e = 0; e < NEXPECT; e++) {
        errno = 0;
        gr = fgetgrent(f);
        check(gr != NULL, "fgetgrent");
        check_entry(gr, e);
    }
#ifdef __PICOLIBC__
    /* picolibc's fgetgrent, like getgetgrent, leaves errno alone at the end */
    errno = 0;
    check(fgetgrent(f) == NULL && errno == 0, "fgetgrent end leaves errno alone");
#endif
    fclose(f);

    printf("success\n");
    return 0;
}
