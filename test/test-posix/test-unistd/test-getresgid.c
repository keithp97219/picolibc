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
/* getresgid and setresgid are XSI_USER_GROUPS interfaces, new in Issue 8 */
#define _XOPEN_SOURCE 800
/* glibc declares getresgid and setresgid only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_XSI_USER_GROUPS >= 202405L && TEST_POSIX_USER_GROUPS && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/* getresgid reports and setresgid sets the real, effective and saved ids */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Returns an ID the process holds in none of its real, effective or saved IDs */
static gid_t
other_id(gid_t a, gid_t b, gid_t c)
{
    gid_t id = a + 1;

    while (id == a || id == b || id == c || id == (gid_t)-1 || id == 0)
        id++;
    return id;
}

int
main(void)
{
    gid_t r, e, s, r2, e2, s2, other;

    check(getresgid(&r, &e, &s) == 0, "getresgid");
    check(r == getgid() && e == getegid(), "real and effective");
    check(setresgid(r, e, s) == 0, "setresgid same IDs");
    check(setresgid((gid_t)-1, (gid_t)-1, (gid_t)-1) == 0, "setresgid unchanged");
    /* Each ID may be set to any of the three */
    check(setresgid(s, r, e) == 0 && setresgid(r, e, s) == 0, "setresgid permuted");
    check(getresgid(&r2, &e2, &s2) == 0 && r2 == r && e2 == e && s2 == s, "kept");
    if (geteuid() == 0) {
        pid_t pid;
        int   status;

        /* setegid leaves the saved ID alone, so the old effective ID can come back */
        fflush(stdout);
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0) {
            if (setresgid(1000, 0, 0) != 0 || setegid(3000) != 0)
                _exit(1);
            if (getresgid(&r2, &e2, &s2) != 0 || r2 != 1000 || e2 != 3000 || s2 != 0)
                _exit(2);
            if (setegid(0) != 0 || setegid(1000) != 0 || getresgid(&r2, &e2, &s2) != 0 || s2 != 0)
                _exit(3);
            _exit(0);
        }
        check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "waitpid");
        check(WEXITSTATUS(status) == 0, "setegid kept the saved ID");
        printf("privileged, skipping the EPERM checks\n");
    } else {
        other = other_id(r, e, s);
        /* A failing call changes none of the IDs */
        errno = 0;
        check(setresgid(r, e, other) == -1 && errno == EPERM, "another saved ID");
        errno = 0;
        check(setresgid(other, e, s) == -1 && errno == EPERM, "another real ID");
        errno = 0;
        check(setresgid((gid_t)-1, other, (gid_t)-1) == -1 && errno == EPERM,
              "another effective ID");
        check(getresgid(&r2, &e2, &s2) == 0 && r2 == r && e2 == e && s2 == s,
              "IDs kept after EPERM");
    }

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
