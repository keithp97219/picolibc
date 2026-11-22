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
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_ATTRIBUTES

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * tmpfile and tmpnam: an update stream on a file that is already gone, and
 * TMP_MAX names that differ and don't name existing files
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
compare(const void *a, const void *b)
{
    return strcmp(a, b);
}

#define NAMES (TMP_MAX < 10000 ? TMP_MAX : 10000)

static char names[NAMES][L_tmpnam];

int
main(void)
{
    FILE       *f;
    struct stat st;
    char        buf[8];
    int         i;
    char       *s;

    f = tmpfile();
    check(f != NULL, "tmpfile");
    check(fstat(fileno(f), &st) == 0 && S_ISREG(st.st_mode) && st.st_nlink == 0,
          "the file is already removed");
    check(fputs("temp", f) >= 0 && fseek(f, 0, SEEK_SET) == 0, "write and seek");
    check(fgets(buf, sizeof(buf), f) != NULL && strcmp(buf, "temp") == 0, "read back");
    check(fclose(f) == 0, "fclose");

    /* tmpnam(NULL) uses an internal object */
    s = tmpnam(NULL);
    check(s != NULL && tmpnam(NULL) == s, "tmpnam(NULL)");

    for (i = 0; i < NAMES; i++) {
        check(tmpnam(names[i]) == names[i], "tmpnam returns its argument");
        errno = 0;
        check(access(names[i], F_OK) != 0 && errno == ENOENT, "the name is not in use");
    }
    qsort(names, NAMES, sizeof(names[0]), compare);
    for (i = 1; i < NAMES; i++)
        check(strcmp(names[i - 1], names[i]) != 0, "names differ");

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
