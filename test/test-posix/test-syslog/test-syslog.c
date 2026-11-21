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
/* syslog is an XSI interface */
#define _XOPEN_SOURCE 800
/*
 * LOG_PERROR is an extension, and the only way to see a message
 * without reaching the system logger
 */
#define _DEFAULT_SOURCE
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_XSI_SYSTEM_LOGGING) && defined(__PICOLIBC_POSIX_PIPE) \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/wait.h>

/*
 * syslog formats messages, honouring the mask, and openlog sets the
 * identity and options.
 *
 * The test must never write to the system log, so it runs in a child
 * whose descriptor limit leaves no room for another descriptor: syslog
 * can't open its socket or the console, and LOG_PERROR copies each
 * message to standard error, which is a pipe the child reads back.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char long_ident[4096];

/* Collects what syslog wrote to standard error (descriptor 0 reads it) */
static void
capture(char *buf, size_t size)
{
    ssize_t n;

    n = read(0, buf, size - 1);
    buf[n > 0 ? n : 0] = '\0';
}

/* Makes standard error a pipe read through standard input, then uses up every descriptor */
static void
isolate(void)
{
    struct rlimit rl;
    int           fds[2];

    check(pipe(fds) == 0, "pipe");
    check(dup2(fds[0], 0) == 0 && dup2(fds[1], 2) == 2, "redirect");
    if (fds[0] > 2)
        close(fds[0]);
    if (fds[1] > 2)
        close(fds[1]);
    check(fcntl(0, F_SETFL, O_NONBLOCK) == 0, "non-blocking");
    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
    rl.rlim_cur = 3;
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "setrlimit");
    /* Stop here unless syslog is sure to find no descriptor */
    errno = 0;
    check(open("/dev/null", O_RDONLY) == -1 && errno == EMFILE, "no descriptors left");
}

static void
child(void)
{
    char buf[512], want[64];
    int  mask;

    isolate();

    /* The default mask allows every priority */
    mask = setlogmask(0);
    check((mask & LOG_UPTO(LOG_DEBUG)) == LOG_UPTO(LOG_DEBUG), "default mask");

    /* openlog isn't needed first; nothing reaches standard error yet */
    syslog(LOG_INFO, "before openlog");
    capture(buf, sizeof(buf));
    check(strstr(buf, "before openlog") == NULL, "no LOG_PERROR yet");

    /* LOG_NDELAY and LOG_CONS find no descriptor, and syslog goes on */
    openlog("picolibc-test", LOG_PID | LOG_PERROR | LOG_NDELAY | LOG_CONS, LOG_USER);
    errno = ENOENT;
    syslog(LOG_ERR, "value %d: %m", 42);
    capture(buf, sizeof(buf));
    snprintf(want, sizeof(want), "picolibc-test[%d]: value 42: ", (int)getpid());
    check(strstr(buf, want) != NULL, "ident, pid and message");
    check(strstr(buf, strerror(ENOENT)) != NULL, "%m");

    /* %m uses errno on entry, and mixes with %n$ conversions */
    errno = EACCES;
    syslog(LOG_ERR, "%2$s %m %1$d", 7, "pos");
    capture(buf, sizeof(buf));
    snprintf(want, sizeof(want), "pos %s 7", strerror(EACCES));
    check(strstr(buf, want) != NULL, "%m with %n$");

    /* %% is still a percent sign, even before m */
    errno = EPERM;
    syslog(LOG_ERR, "100%% %%m");
    capture(buf, sizeof(buf));
    check(strstr(buf, "100% %m") != NULL, "%%");

    /* A severity outside the mask is rejected, whatever the facility */
    check(setlogmask(LOG_MASK(LOG_ERR)) == mask, "setlogmask returns the old mask");
    syslog(LOG_INFO, "hidden");
    syslog(LOG_INFO | LOG_LOCAL3, "hidden");
    capture(buf, sizeof(buf));
    check(strstr(buf, "hidden") == NULL, "masked");
    syslog(LOG_ERR | LOG_LOCAL3, "shown");
    capture(buf, sizeof(buf));
    check(strstr(buf, "shown") != NULL, "not masked");
    check(setlogmask(0) == LOG_MASK(LOG_ERR), "zero leaves the mask");

    /* LOG_UPTO */
    check(setlogmask(LOG_UPTO(LOG_WARNING)) == LOG_MASK(LOG_ERR), "setlogmask");
    syslog(LOG_EMERG, "emerg");
    syslog(LOG_WARNING, "warning");
    syslog(LOG_NOTICE, "notice");
    capture(buf, sizeof(buf));
    check(strstr(buf, "emerg") != NULL && strstr(buf, "warning") != NULL, "up to LOG_WARNING");
    check(strstr(buf, "notice") == NULL, "not LOG_NOTICE");
    setlogmask(LOG_UPTO(LOG_DEBUG));
    syslog(LOG_DEBUG, "debug");
    capture(buf, sizeof(buf));
    check(strstr(buf, "debug") != NULL, "LOG_DEBUG");

    /* A later openlog replaces the identity and the options */
    openlog("second", LOG_PERROR, 0);
    syslog(LOG_ERR, "again");
    capture(buf, sizeof(buf));
    check(strstr(buf, "second: again") != NULL, "new identity");
    check(strchr(buf, '[') == NULL, "no pid without LOG_PID");

    /* An identity longer than any message is cut short, not overrun */
    memset(long_ident, 'i', sizeof(long_ident) - 1);
    openlog(long_ident, LOG_PID | LOG_PERROR, LOG_USER);
    syslog(LOG_ERR, "long");
    capture(buf, sizeof(buf));
    check(strncmp(buf, "iiii", 4) == 0, "long identity");
    while (read(0, buf, sizeof(buf)) > 0)
        ;

    /* closelog, then logging starts again */
    closelog();
    openlog("third", LOG_PERROR, LOG_USER);
    syslog(LOG_ERR, "after closelog");
    capture(buf, sizeof(buf));
    check(strstr(buf, "third: after closelog") != NULL, "after closelog");
    closelog();
    exit(0);
}

int
main(void)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        child();
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child");
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
