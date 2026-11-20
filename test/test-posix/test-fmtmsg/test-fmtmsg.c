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
/* fmtmsg is an XSI interface */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                \
    || (defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0 \
        && defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0)

#include <fmtmsg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* fmtmsg writes classified messages to standard error */

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
    char    buf[256];
    int     fds[2], saved;
    ssize_t n;

    check(pipe(fds) == 0, "pipe");
    saved = dup(2);
    check(saved >= 0 && dup2(fds[1], 2) == 2, "redirect stderr");
    check(fmtmsg(MM_PRINT | MM_SOFT | MM_APPL | MM_RECOVER, "picolibc:test", MM_ERROR,
                 "something failed", "try again", "picolibc:1")
              == MM_OK,
          "fmtmsg");
    check(dup2(saved, 2) == 2 && close(saved) == 0, "restore stderr");
    check(close(fds[1]) == 0, "close");
    n = read(fds[0], buf, sizeof(buf) - 1);
    check(n > 0, "read");
    buf[n] = '\0';
    check(strstr(buf, "picolibc:test: ERROR: something failed") != NULL, "first line");
    check(strstr(buf, "TO FIX: try again") != NULL && strstr(buf, "picolibc:1") != NULL,
          "action and tag");
    check(close(fds[0]) == 0, "close");
    check(fmtmsg(MM_NULLMC, MM_NULLLBL, MM_NULLSEV, MM_NULLTXT, MM_NULLACT, MM_NULLTAG) == MM_OK,
          "nothing to print");
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
