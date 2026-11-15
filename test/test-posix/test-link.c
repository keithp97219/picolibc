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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "LINK.TXT"
#endif

#define TEST_LINK_NAME TEST_FILE_NAME ".link"

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(TEST_FILE_NAME);              \
            (void)unlink(TEST_LINK_NAME);              \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    struct stat st, lst;
    char        buf[16];
    int         fd;

    (void)unlink(TEST_FILE_NAME);
    (void)unlink(TEST_LINK_NAME);

    fd = open(TEST_FILE_NAME, O_WRONLY | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create file");
    check(write(fd, "linked", 6) == 6, "write file");
    check(close(fd) == 0, "close file");

    check(link(TEST_FILE_NAME, TEST_LINK_NAME) == 0, "link");
    check(stat(TEST_FILE_NAME, &st) == 0 && stat(TEST_LINK_NAME, &lst) == 0, "stat");
    check(st.st_ino == lst.st_ino && st.st_dev == lst.st_dev, "same file");
    check(st.st_nlink == 2, "link count");
    check(link(TEST_FILE_NAME, TEST_LINK_NAME) == -1 && errno == EEXIST, "link to existing name");

    /* The contents remain reachable through the new name */
    check(unlink(TEST_FILE_NAME) == 0, "unlink original");
    fd = open(TEST_LINK_NAME, O_RDONLY);
    check(fd >= 0, "open link");
    check(read(fd, buf, sizeof(buf)) == 6 && memcmp(buf, "linked", 6) == 0, "read link");
    check(close(fd) == 0, "close link");
    check(link(TEST_FILE_NAME, TEST_LINK_NAME) == -1 && errno == ENOENT, "link missing file");
    check(unlink(TEST_LINK_NAME) == 0, "unlink link");

    printf("success\n");
    return 0;
}
