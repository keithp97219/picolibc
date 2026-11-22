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

#include <stdio.h>

/*
 * On Linux the login name belongs to the session: the user
 * /proc/self/loginuid names, or none when it is (uid_t)-1 (coreutils'
 * misc/user test runs logname in a user namespace, where it's unset).
 */

#if defined(__linux__) && TEST_POSIX_USER_GROUPS && TEST_POSIX_FILE_SYSTEM
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
    char           buf[256], name[256];
    int            fd, ret;
    ssize_t        n;
    unsigned long  loginuid;
    struct passwd *pw;

    fd = open("/proc/self/loginuid", O_RDONLY);
    if (fd < 0) {
        printf("no /proc/self/loginuid, skipping\n");
        return 77;
    }
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        printf("can't read /proc/self/loginuid, skipping\n");
        return 77;
    }
    buf[n] = '\0';
    loginuid = strtoul(buf, NULL, 10);
    ret = getlogin_r(name, sizeof(name));
    if (loginuid == 4294967295UL) {
        if (ret == 0) {
            printf("no login uid, but getlogin_r gave \"%s\"\n", name);
            return 1;
        }
    } else {
        pw = getpwuid((uid_t)loginuid);
        if (pw == NULL) {
            printf("login uid %lu has no passwd entry, skipping\n", loginuid);
            return 77;
        }
        if (ret != 0 || strcmp(name, pw->pw_name) != 0) {
            printf("getlogin_r: %d \"%s\", want \"%s\"\n", ret, ret ? "" : name, pw->pw_name);
            return 1;
        }
    }
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
