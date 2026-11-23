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
/* getresuid and setresuid are XSI_USER_GROUPS interfaces, new in Issue 8 */
#define _XOPEN_SOURCE 800
/* glibc declares getresuid and setresuid only with _GNU_SOURCE */
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

/* getresuid reports and setresuid sets the real, effective and saved ids */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Returns an ID the process holds in none of its real, effective or saved IDs */
static uid_t
other_id(uid_t a, uid_t b, uid_t c)
{
    uid_t id = a + 1;

    while (id == a || id == b || id == c || id == (uid_t)-1 || id == 0)
        id++;
    return id;
}

int
main(void)
{
    uid_t r, e, s, r2, e2, s2, other;

    check(getresuid(&r, &e, &s) == 0, "getresuid");
    check(r == getuid() && e == geteuid(), "real and effective");
    check(setresuid(r, e, s) == 0, "setresuid same IDs");
    check(setresuid((uid_t)-1, (uid_t)-1, (uid_t)-1) == 0, "setresuid unchanged");
    /* Each ID may be set to any of the three */
    check(setresuid(s, r, e) == 0 && setresuid(r, e, s) == 0, "setresuid permuted");
    check(getresuid(&r2, &e2, &s2) == 0 && r2 == r && e2 == e && s2 == s, "kept");
    if (geteuid() == 0) {
        pid_t pid;
        int   status;

        /* seteuid leaves the saved ID alone, so the old effective ID can come back */
        fflush(stdout);
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0) {
            if (setresuid(1000, 0, 0) != 0 || seteuid(3000) != 0)
                _exit(1);
            if (getresuid(&r2, &e2, &s2) != 0 || r2 != 1000 || e2 != 3000 || s2 != 0)
                _exit(2);
            if (seteuid(0) != 0 || seteuid(1000) != 0 || getresuid(&r2, &e2, &s2) != 0 || s2 != 0)
                _exit(3);
            _exit(0);
        }
        check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "waitpid");
        check(WEXITSTATUS(status) == 0, "seteuid kept the saved ID");
        printf("privileged, skipping the EPERM checks\n");
    } else {
        other = other_id(r, e, s);
        /* A failing call changes none of the IDs */
        errno = 0;
        check(setresuid(r, e, other) == -1 && errno == EPERM, "another saved ID");
        errno = 0;
        check(setresuid(other, e, s) == -1 && errno == EPERM, "another real ID");
        errno = 0;
        check(setresuid((uid_t)-1, other, (uid_t)-1) == -1 && errno == EPERM,
              "another effective ID");
        check(getresuid(&r2, &e2, &s2) == 0 && r2 == r && e2 == e && s2 == s,
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
