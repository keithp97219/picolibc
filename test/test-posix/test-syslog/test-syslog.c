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
/* syslog is an XSI interface; LOG_PERROR is an extension */
#define _DEFAULT_SOURCE
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_FD_MGMT) && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/* syslog formats messages, honouring the mask, and closelog ends logging */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int  fds[2], saved;
static char long_ident[4096];

/* Collects what syslog wrote to standard error */
static void
capture(char *buf, size_t size)
{
    ssize_t n;

    n = read(fds[0], buf, size - 1);
    buf[n > 0 ? n : 0] = '\0';
}

int
main(void)
{
    char buf[512], want[64];

    check(pipe(fds) == 0, "pipe");
    check(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0, "non-blocking");
    saved = dup(2);
    check(saved >= 0 && dup2(fds[1], 2) == 2, "redirect stderr");

    openlog("picolibc-test", LOG_PID | LOG_PERROR, LOG_USER);
    errno = ENOENT;
    syslog(LOG_ERR, "value %d: %m", 42);
    capture(buf, sizeof(buf));
    snprintf(want, sizeof(want), "picolibc-test[%d]: value 42: ", (int)getpid());
    check(strstr(buf, want) != NULL, "ident, pid and message");
    check(strstr(buf, strerror(ENOENT)) != NULL, "%m");

    check(setlogmask(LOG_MASK(LOG_ERR)) == 0xff, "setlogmask");
    syslog(LOG_INFO, "hidden");
    capture(buf, sizeof(buf));
    check(strstr(buf, "hidden") == NULL, "masked");
    syslog(LOG_ERR, "shown");
    capture(buf, sizeof(buf));
    check(strstr(buf, "shown") != NULL, "not masked");
    check(setlogmask(0) == LOG_MASK(LOG_ERR), "zero leaves the mask");
    setlogmask(0xff);

    /* An identity longer than any message is cut short, not overrun */
    memset(long_ident, 'i', sizeof(long_ident) - 1);
    openlog(long_ident, LOG_PID | LOG_PERROR, LOG_USER);
    syslog(LOG_ERR, "long");
    capture(buf, sizeof(buf));
    check(strncmp(buf, "iiii", 4) == 0, "long identity");
    while (read(fds[0], buf, sizeof(buf)) > 0)
        ;

    closelog();
    check(dup2(saved, 2) == 2 && close(saved) == 0, "restore stderr");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
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
