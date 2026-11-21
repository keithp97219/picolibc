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
#include "../posix-groups.h"

#include <fmtmsg.h>

/* What <fmtmsg.h> defines */
_Static_assert(_Generic(MM_NULLLBL, char *: 1, default: 0) && MM_NULLLBL == (char *)0,
               "MM_NULLLBL");
_Static_assert(_Generic(MM_NULLSEV, int: 1, default: 0) && MM_NULLSEV == 0, "MM_NULLSEV");
_Static_assert(_Generic(MM_NULLMC, long: 1, default: 0) && MM_NULLMC == 0L, "MM_NULLMC");
_Static_assert(_Generic(MM_NULLTXT, char *: 1, default: 0) && MM_NULLTXT == (char *)0,
               "MM_NULLTXT");
_Static_assert(_Generic(MM_NULLACT, char *: 1, default: 0) && MM_NULLACT == (char *)0,
               "MM_NULLACT");
_Static_assert(_Generic(MM_NULLTAG, char *: 1, default: 0) && MM_NULLTAG == (char *)0,
               "MM_NULLTAG");
_Static_assert(_Generic(&fmtmsg,
                   int (*)(long, const char *, int, const char *, const char *, const char *): 1,
                   default: 0),
               "fmtmsg prototype");

/* The classification identifiers are distinct, and so are the severities and results */
static int
distinct(long classification, int severity, int result)
{
    switch (classification) {
    case MM_HARD:
    case MM_SOFT:
    case MM_FIRM:
    case MM_APPL:
    case MM_UTIL:
    case MM_OPSYS:
    case MM_RECOVER:
    case MM_NRECOV:
    case MM_PRINT:
    case MM_CONSOLE:
    case MM_NULLMC:
        break;
    }
    switch (severity) {
    case MM_HALT:
    case MM_ERROR:
    case MM_WARNING:
    case MM_INFO:
    case MM_NOSEV:
        break;
    }
    switch (result) {
    case MM_OK:
    case MM_NOTOK:
    case MM_NOMSG:
    case MM_NOCON:
        break;
    }
    return 0;
}

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_PIPE) && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>

/*
 * fmtmsg writes classified messages to standard error, choosing the
 * components with MSGVERB.
 *
 * Each call runs in its own child, so MSGVERB is read afresh, and the
 * child has no descriptor to spare, so fmtmsg can't open the console:
 * MM_CONSOLE is only ever seen failing.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char out[16384];

/* Standard error works, or it is open read-only so writing to it fails */
enum err { ERR_OK, ERR_BAD };

static int
run(const char *verb, enum err err, long classification, const char *label, int severity,
    const char *text, const char *action, const char *tag)
{
    int     fds[2], status;
    size_t  len = 0;
    ssize_t n;
    pid_t   pid;

    fflush(stdout);
    check(pipe(fds) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit rl;

        if (verb)
            setenv("MSGVERB", verb, 1);
        else
            unsetenv("MSGVERB");
        if (err == ERR_OK) {
            check(dup2(fds[1], 2) == 2, "redirect");
        } else {
            /* Not the pipe's read end, which may be open for writing too */
            int ro = open("/dev/null", O_RDONLY);

            check(ro > 2 && dup2(ro, 2) == 2 && close(ro) == 0, "redirect");
        }
        close(fds[0]);
        close(fds[1]);
        /* Descriptors 0, 1 and 2 are all that's left */
        check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
        rl.rlim_cur = 3;
        check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "setrlimit");
        errno = 0;
        check(open("/dev/null", O_RDONLY) == -1 && errno == EMFILE, "no descriptors left");
        _exit(fmtmsg(classification, label, severity, text, action, tag) + 1);
    }
    close(fds[1]);
    while ((n = read(fds[0], out + len, sizeof(out) - 1 - len)) > 0)
        len += (size_t)n;
    out[len] = '\0';
    close(fds[0]);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    return WEXITSTATUS(status) - 1;
}

#define CLASS (MM_PRINT | MM_SOFT | MM_UTIL | MM_RECOVER)
#define EXAMPLE \
    "XSI:cat", MM_ERROR, "illegal option", "refer to cat in user's reference manual", "XSI:cat:001"
#define FIRST  "XSI:cat: ERROR: illegal option\n"
#define SECOND "TO FIX: refer to cat in user's reference manual"

/* The whole example message, give or take the spaces before the tag */
static int
complete(void)
{
    const char *p = out;

    if (strncmp(p, FIRST SECOND, strlen(FIRST SECOND)) != 0)
        return 0;
    p += strlen(FIRST SECOND);
    if (*p != ' ')
        return 0;
    while (*p == ' ')
        p++;
    return strcmp(p, "XSI:cat:001\n") == 0;
}

int
main(void)
{
    static char big[5000], bigaction[5000];
    int         ret;

    (void)distinct(0, 0, 0);

    /* The example on fmtmsg's page, with MSGVERB unset */
    check(run(NULL, ERR_OK, CLASS, EXAMPLE) == MM_OK, "fmtmsg");
    check(complete(), "complete message");

    /* MSGVERB selects components, in any order */
    check(run("severity:text:action", ERR_OK, CLASS, EXAMPLE) == MM_OK, "MSGVERB");
    check(strcmp(out, "ERROR: illegal option\n" SECOND "\n") == 0, "severity:text:action");
    check(run("tag:label", ERR_OK, CLASS, EXAMPLE) == MM_OK, "MSGVERB");
    check(strstr(out, "XSI:cat") == out && strstr(out, "XSI:cat:001") != NULL, "tag:label");
    check(strstr(out, "ERROR") == NULL && strstr(out, "illegal") == NULL
              && strstr(out, "TO FIX") == NULL,
          "only the label and tag");
    check(run("text", ERR_OK, CLASS, EXAMPLE) == MM_OK, "MSGVERB");
    check(strcmp(out, "illegal option\n") == 0, "text");

    /* An empty, malformed or unknown MSGVERB selects everything */
    check(run("", ERR_OK, CLASS, EXAMPLE) == MM_OK && complete(), "empty MSGVERB");
    check(run("label:bogus", ERR_OK, CLASS, EXAMPLE) == MM_OK && complete(), "unknown keyword");
    check(run("label::text", ERR_OK, CLASS, EXAMPLE) == MM_OK && complete(), "empty keyword");
    check(run("LABEL", ERR_OK, CLASS, EXAMPLE) == MM_OK && complete(), "upper case");

    /* A component with its null value is left out */
    check(run(NULL, ERR_OK, CLASS, MM_NULLLBL, MM_ERROR, "text only", MM_NULLACT, MM_NULLTAG)
              == MM_OK,
          "null components");
    check(strcmp(out, "ERROR: text only\n") == 0, "null label, action and tag");
    check(run(NULL, ERR_OK, CLASS, "a:b", MM_NOSEV, "no severity", MM_NULLACT, MM_NULLTAG) == MM_OK,
          "MM_NOSEV");
    check(strcmp(out, "a:b: no severity\n") == 0, "no severity string");

    /* The severity strings */
    check(run(NULL, ERR_OK, CLASS, MM_NULLLBL, MM_HALT, "t", MM_NULLACT, MM_NULLTAG) == MM_OK
              && strcmp(out, "HALT: t\n") == 0,
          "HALT");
    check(run(NULL, ERR_OK, CLASS, MM_NULLLBL, MM_WARNING, "t", MM_NULLACT, MM_NULLTAG) == MM_OK
              && strcmp(out, "WARNING: t\n") == 0,
          "WARNING");
    check(run(NULL, ERR_OK, CLASS, MM_NULLLBL, MM_INFO, "t", MM_NULLACT, MM_NULLTAG) == MM_OK
              && strcmp(out, "INFO: t\n") == 0,
          "INFO");

    /* The action is preceded by "TO FIX:" */
    check(run(NULL, ERR_OK, CLASS, MM_NULLLBL, MM_NULLSEV, MM_NULLTXT, "act", MM_NULLTAG) == MM_OK
              && strncmp(out, "TO FIX:", 7) == 0 && strstr(out, "act") != NULL,
          "TO FIX:");

    /* The text and the action aren't limited in size */
    memset(big, 't', sizeof(big) - 1);
    memset(bigaction, 'a', sizeof(bigaction) - 1);
    check(run(NULL, ERR_OK, CLASS, "big:text", MM_INFO, big, bigaction, MM_NULLTAG) == MM_OK,
          "long components");
    check(strncmp(out, "big:text: INFO: ", 16) == 0 && strncmp(out + 16, big, sizeof(big) - 1) == 0
              && strncmp(out + 16 + sizeof(big) - 1, "\nTO FIX: ", 9) == 0
              && strcmp(out + 25 + sizeof(big) - 1 + sizeof(bigaction) - 1, "\n") == 0
              && strncmp(out + 25 + sizeof(big) - 1, bigaction, sizeof(bigaction) - 1) == 0,
          "whole text and action");

    /* No display classification: nothing written */
    check(run(NULL, ERR_OK, MM_SOFT | MM_APPL, EXAMPLE) == MM_OK && out[0] == '\0', "no display");
    check(run(NULL, ERR_OK, MM_NULLMC, MM_NULLLBL, MM_NULLSEV, MM_NULLTXT, MM_NULLACT, MM_NULLTAG)
              == MM_OK,
          "nothing to print");

    /*
     * Standard error can't be written, and nothing else was asked for:
     * "failed completely", or "unable to generate a message on standard
     * error, but otherwise succeeded" (glibc)
     */
    ret = run(NULL, ERR_BAD, CLASS, EXAMPLE);
    check(ret == MM_NOTOK || ret == MM_NOMSG, "standard error fails");
#ifdef __PICOLIBC__
    check(ret == MM_NOTOK, "picolibc reports MM_NOTOK");
#endif

    /* The console can't be opened */
    ret = run(NULL, ERR_OK, CLASS | MM_CONSOLE, EXAMPLE);
    check(complete(), "standard error still written");
#ifndef __GLIBC__
    /* glibc logs console messages with syslog and doesn't see them fail */
    check(ret == MM_NOCON, "MM_NOCON");
    check(run(NULL, ERR_OK, MM_CONSOLE, EXAMPLE) == MM_NOTOK, "console only");
    check(run(NULL, ERR_BAD, CLASS | MM_CONSOLE, EXAMPLE) == MM_NOTOK, "neither");
#else
    (void)ret;
#endif
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    (void)distinct(0, 0, 0);
    printf("no system support, skipping\n");
    return 77;
}

#endif
