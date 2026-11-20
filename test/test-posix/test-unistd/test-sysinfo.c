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

#define _GNU_SOURCE /* glibc declares secure_getenv only for _GNU_SOURCE */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

/* Allow testing readdir_r, which other C libraries mark deprecated */
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 2) || __GNUC__ > 4)
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "SYSINFO.DIR"
#endif

/* uname, confstr, secure_getenv, getlogin_r, readdir_r and posix_close */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    struct utsname name;
    char           buf[256];
    size_t         len;
    int            ret;
    DIR           *dir;
    struct dirent  entry, *result;
    int            seen = 0;

    check(uname(&name) == 0, "uname");
    check(name.sysname[0] && name.release[0] && name.machine[0], "uname fields");

    len = confstr(_CS_PATH, NULL, 0);
    check(len > 1, "confstr length");
    check(confstr(_CS_PATH, buf, sizeof(buf)) == len && strlen(buf) == len - 1, "confstr");
    check(strstr(buf, "/bin") != NULL, "confstr _CS_PATH");
    check(confstr(_CS_PATH, buf, 2) == len && strlen(buf) == 1, "confstr truncates");
    errno = 0;
    check(confstr(-1, buf, sizeof(buf)) == 0 && errno == EINVAL, "confstr bad name");

    check(setenv("PICOLIBC_TEST", "value", 1) == 0, "setenv");
    check(secure_getenv("PICOLIBC_TEST") == getenv("PICOLIBC_TEST"), "secure_getenv");

    /* The user may have no login name here */
    ret = getlogin_r(buf, sizeof(buf));
    if (ret == 0) {
        check(buf[0] != '\0', "login name");
        check(getlogin_r(buf, 1) == ERANGE, "getlogin_r short buffer");
    } else {
        check(ret == ENXIO || ret == ENOENT || ret == ENOTTY, "getlogin_r error");
    }

    /* readdir_r finds a file in a new directory */
    (void)unlink(TEST_FILE_NAME "/file");
    (void)rmdir(TEST_FILE_NAME);
    check(mkdir(TEST_FILE_NAME, 0700) == 0, "mkdir");
    ret = open(TEST_FILE_NAME "/file", O_WRONLY | O_CREAT, 0600);
    check(ret >= 0 && close(ret) == 0, "create file");
    dir = opendir(TEST_FILE_NAME);
    check(dir != NULL, "opendir");
    while ((ret = readdir_r(dir, &entry, &result)) == 0 && result) {
        check(result == &entry, "readdir_r result");
        if (strcmp(entry.d_name, "file") == 0)
            seen++;
    }
    check(ret == 0, "readdir_r");
    check(seen == 1, "readdir_r found the file");
    check(closedir(dir) == 0, "closedir");

#ifdef POSIX_CLOSE_RESTART /* defined with posix_close; older C libraries lack both */
    /* posix_close closes, and rejects a bad flag without closing */
    ret = open(TEST_FILE_NAME "/file", O_RDONLY);
    check(ret >= 0, "open for posix_close");
    errno = 0;
    check(posix_close(ret, -1) == -1 && errno == EINVAL, "posix_close bad flag");
    check(posix_close(ret, 0) == 0, "posix_close");
    errno = 0;
    check(posix_close(ret, 0) == -1 && errno == EBADF, "posix_close closed descriptor");
#endif

    check(unlink(TEST_FILE_NAME "/file") == 0 && rmdir(TEST_FILE_NAME) == 0, "cleanup");

    printf("success\n");
    return 0;
}
