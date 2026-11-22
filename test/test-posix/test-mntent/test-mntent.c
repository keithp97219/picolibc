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

#define _DEFAULT_SOURCE
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && (defined(__PICOLIBC__) || defined(TEST_GLIBC))

#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <mntent.h> (BSD/glibc, not POSIX): parse an fstab-format table,
 * with comments, blank lines, octal escapes and missing numbers,
 * write one back with addmntent and find options with hasmntopt, and
 * read the mounted file system table
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static const char table[] = "# comment\n"
                            "\n"
                            "/dev/sda1 / ext4 rw,relatime 0 1\n"
                            "  server:/export\t/mnt/with\\040space nfs ro,soft,vers=4\n"
                            "tmpfs /tmp tmpfs defaults 0 0\n";

int
main(void)
{
    char           name[] = "mntent-XXXXXX";
    struct mntent *m, add;
    FILE          *f;
    int            fd;

    fd = mkstemp(name);
    check(fd >= 0, "mkstemp");
    close(fd);
    f = fopen(name, "w");
    check(f != NULL && fputs(table, f) >= 0 && fclose(f) == 0, "write table");

    f = setmntent(name, "r");
    check(f != NULL, "setmntent");
    m = getmntent(f);
    check(m != NULL && strcmp(m->mnt_fsname, "/dev/sda1") == 0 && strcmp(m->mnt_dir, "/") == 0,
          "first entry");
    check(strcmp(m->mnt_type, "ext4") == 0 && strcmp(m->mnt_opts, "rw,relatime") == 0
              && m->mnt_freq == 0 && m->mnt_passno == 1,
          "first entry fields");
    check(hasmntopt(m, "relatime") != NULL && hasmntopt(m, "rel") == NULL
              && hasmntopt(m, "ro") == NULL,
          "hasmntopt");
    m = getmntent(f);
    check(m != NULL && strcmp(m->mnt_dir, "/mnt/with space") == 0, "octal escape");
    check(m->mnt_freq == 0 && m->mnt_passno == 0, "missing numbers are 0");
    check(hasmntopt(m, "vers") != NULL && strncmp(hasmntopt(m, "vers"), "vers=4", 6) == 0,
          "hasmntopt with a value");
    m = getmntent(f);
    check(m != NULL && strcmp(m->mnt_type, "tmpfs") == 0, "third entry");
    check(getmntent(f) == NULL, "end of table");
    check(endmntent(f) == 1, "endmntent");

    /* addmntent appends, escaping blanks */
    f = setmntent(name, "a+");
    check(f != NULL, "setmntent a+");
    add = (struct mntent) { "a b", "/x\ty", "none", "bind", 1, 2 };
    check(addmntent(f, &add) == 0, "addmntent");
    check(endmntent(f) == 1, "endmntent after add");
    f = setmntent(name, "r");
    check(f != NULL, "setmntent again");
    while ((m = getmntent(f)) != NULL && strcmp(m->mnt_type, "none") != 0)
        ;
    check(m != NULL && strcmp(m->mnt_fsname, "a b") == 0 && strcmp(m->mnt_dir, "/x\ty") == 0
              && m->mnt_freq == 1 && m->mnt_passno == 2,
          "added entry reads back");
    endmntent(f);
    check(unlink(name) == 0, "cleanup");

#if TEST_POSIX_MULTI_PROCESS
    /*
     * The mounted file system table (Linux's /etc/mtab; FreeBSD has
     * none, and the OS layer makes one from getmntinfo) lists /
     */
    {
        int root = 0;

        f = setmntent(MOUNTED, "r");
        check(f != NULL, "setmntent(MOUNTED)");
        while ((m = getmntent(f)) != NULL)
            if (strcmp(m->mnt_dir, "/") == 0 && m->mnt_type[0] != '\0')
                root = 1;
        endmntent(f);
        check(root, "/ is mounted");
    }
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
