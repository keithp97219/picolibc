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

/* fgetpwent and fgetpwent_r are GNU extensions */
#define _GNU_SOURCE
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * fgetpwent and fgetpwent_r parse user database lines the way getpwnam,
 * getpwuid and getpwent do: lines that aren't entries are skipped, ids
 * are unsigned, empty fields are kept, and long lines work
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define LONG_GECOS 2000

static const char head[] = "root:x:0:0:root:/root:/bin/sh\n"
                           "\n"
                           "short:x\n"
                           "emptyuid:x::1::/:/bin/sh\n"
                           "baduid:x:12a:1::/:/bin/sh\n"
                           "badgid:x:1:1b::/:/bin/sh\n"
                           "high:x:4294967294:4294967293::/home/high:/bin/sh\n"
                           "long:x:5:5:";
static const char tail[] = ":/home/long:/bin/long\n"
                           "noshell:x:6:6::/:\n";

static const struct {
    const char   *name;
    unsigned long uid, gid;
    const char   *dir, *shell;
} expect[] = {
    { "root",    0,          0,          "/root",      "/bin/sh"   },
    { "high",    4294967294, 4294967293, "/home/high", "/bin/sh"   },
    { "long",    5,          5,          "/home/long", "/bin/long" },
    { "noshell", 6,          6,          "/",          ""          },
};

#define NEXPECT (sizeof(expect) / sizeof(expect[0]))

static char contents[sizeof(head) + LONG_GECOS + sizeof(tail)];

static void
check_entry(const struct passwd *pw, size_t e)
{
    if (strcmp(pw->pw_name, expect[e].name) != 0 || pw->pw_uid != expect[e].uid
        || pw->pw_gid != expect[e].gid || strcmp(pw->pw_dir, expect[e].dir) != 0
        || strcmp(pw->pw_shell, expect[e].shell) != 0) {
        printf("entry %zu: got '%s' %lu %lu '%s' '%s'\n", e, pw->pw_name, (unsigned long)pw->pw_uid,
               (unsigned long)pw->pw_gid, pw->pw_dir, pw->pw_shell);
        printf("    want '%s' %lu %lu '%s' '%s'\n", expect[e].name, expect[e].uid, expect[e].gid,
               expect[e].dir, expect[e].shell);
        exit(1);
    }
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
    static char   buf[4096];
    struct passwd ent, *pw;
    FILE         *f;
    size_t        e;
    int           ret;

    if (sizeof(uid_t) < 4 || sizeof(gid_t) < 4) {
        printf("uid_t or gid_t narrower than 32 bits, skipping\n");
        return 77;
    }

    strcpy(contents, head);
    memset(contents + strlen(head), 'g', LONG_GECOS);
    strcpy(contents + strlen(head) + LONG_GECOS, tail);

    /* fgetpwent_r, with a buffer big enough for every line */
    f = open_contents();
    for (e = 0; e < NEXPECT; e++) {
        ret = fgetpwent_r(f, &ent, buf, sizeof(buf), &pw);
        check(ret == 0 && pw == &ent, "fgetpwent_r");
        check_entry(pw, e);
    }
    pw = &ent;
    check(fgetpwent_r(f, &ent, buf, sizeof(buf), &pw) == ENOENT && pw == NULL, "fgetpwent_r end");
    fclose(f);

    /*
     * A buffer too small for the long entry gives ERANGE, and leaves it
     * to be read again with a bigger one
     */
    f = open_contents();
    check(fgetpwent_r(f, &ent, buf, sizeof(buf), &pw) == 0, "fgetpwent_r");
    check_entry(pw, 0);
    check(fgetpwent_r(f, &ent, buf, sizeof(buf), &pw) == 0, "fgetpwent_r");
    check_entry(pw, 1);
    pw = &ent;
    check(fgetpwent_r(f, &ent, buf, 100, &pw) == ERANGE && pw == NULL, "fgetpwent_r ERANGE");
    check(fgetpwent_r(f, &ent, buf, sizeof(buf), &pw) == 0, "fgetpwent_r after ERANGE");
    check_entry(pw, 2);
    fclose(f);

    /* fgetpwent, whose own buffer must hold the long entry */
    f = open_contents();
    for (e = 0; e < NEXPECT; e++) {
        errno = 0;
        pw = fgetpwent(f);
        check(pw != NULL, "fgetpwent");
        check_entry(pw, e);
    }
#ifdef __PICOLIBC__
    /* picolibc's fgetpwent, like getgetpwent, leaves errno alone at the end */
    errno = 0;
    check(fgetpwent(f) == NULL && errno == 0, "fgetpwent end leaves errno alone");
#endif
    fclose(f);

    printf("success\n");
    return 0;
}
