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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * getopt follows the POSIX rules: optind and its updates, "-" and
 * "--", option-arguments, '?' and ':' with optopt, a leading ':' and
 * opterr silencing the diagnostics, '+', and rescanning from optind 1.
 * GNU argument permutation is not POSIX and isn't expected.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define ARGC(a) ((int)(sizeof(a) / sizeof((a)[0])) - 1)

/* Run getopt once and check what it returned and left behind */
static void
step(int argc, char **argv, const char *optstring, int want, int want_optind, const char *want_arg,
     const char *message)
{
    int c = getopt(argc, argv, optstring);

    if (c != want || optind != want_optind
        || (want_arg && (optarg == NULL || strcmp(optarg, want_arg) != 0))) {
        printf("%s: got %d optind %d optarg %s, want %d optind %d optarg %s\n", message, c, optind,
               (want_arg && optarg) ? optarg : "-", want, want_optind, want_arg ? want_arg : "-");
        exit(1);
    }
}

#if TEST_POSIX_FD_MGMT && TEST_POSIX_FILE_SYSTEM
#include <fcntl.h>
#include <sys/stat.h>

#define HAVE_DIAGNOSTICS
static int  diag_fd = -1, saved_stderr = -1;
static char diag[512];

/* Send stderr to a file, so the diagnostics can be read back */
static void
diag_start(void)
{
    fflush(stderr);
    saved_stderr = dup(2);
    check(saved_stderr >= 0, "dup stderr");
    diag_fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600);
    check(diag_fd >= 0, "open diagnostics file");
    check(dup2(diag_fd, 2) == 2, "dup2 onto stderr");
}

/* Return what has been written to stderr since diag_start */
static const char *
diag_read(void)
{
    ssize_t n;

    fflush(stderr);
    check(lseek(diag_fd, 0, SEEK_SET) == 0, "lseek");
    n = read(diag_fd, diag, sizeof(diag) - 1);
    check(n >= 0, "read diagnostics");
    diag[n] = '\0';
    check(ftruncate(diag_fd, 0) == 0 && lseek(diag_fd, 0, SEEK_SET) == 0, "ftruncate");
    return diag;
}

static void
diag_stop(void)
{
    fflush(stderr);
    check(dup2(saved_stderr, 2) == 2, "restore stderr");
    check(close(saved_stderr) == 0 && close(diag_fd) == 0, "close");
    (void)unlink(TEST_FILE_NAME);
}
#endif

int
main(void)
{
    /* The system initializes optind to 1, and opterr to non-zero */
    check(optind == 1, "initial optind");
    check(opterr != 0, "initial opterr");
    /* Quiet until the diagnostics are checked */
    opterr = 0;

    char *basic[] = { "prog", "-a", "-b", "arg", "-cfoo", "--", "-d", "op", NULL };
    optind = 1;
    step(ARGC(basic), basic, "ab:c:d", 'a', 2, NULL, "-a");
    step(ARGC(basic), basic, "ab:c:d", 'b', 4, "arg", "-b arg: optind moves by 2");
    step(ARGC(basic), basic, "ab:c:d", 'c', 5, "foo", "-cfoo: optind moves by 1");
    step(ARGC(basic), basic, "ab:c:d", -1, 6, NULL, "\"--\" is passed over");

    /* Options grouped in one element; optind moves when it is done */
    char *group[] = { "prog", "-ab", "-c", NULL };
    optind = 1;
    step(ARGC(group), group, "abc", 'a', 1, NULL, "first of a group");
    step(ARGC(group), group, "abc", 'b', 2, NULL, "end of a group");
    step(ARGC(group), group, "abc", 'c', 3, NULL, "after a group");
    step(ARGC(group), group, "abc", -1, 3, NULL, "end of argv");

    /* An option-argument may start with '-', or be in the same element */
    char *dash_arg[] = { "prog", "-a", "-b", "-xbc", NULL };
    optind = 1;
    step(ARGC(dash_arg), dash_arg, "a:bx", 'a', 3, "-b", "option-argument starting with -");
    step(ARGC(dash_arg), dash_arg, "a:bx", 'x', 3, NULL, "-x of -xbc");
    step(ARGC(dash_arg), dash_arg, "a:bx", 'b', 3, NULL, "-b of -xbc");
    step(ARGC(dash_arg), dash_arg, "a:bx", '?', 4, NULL, "-c of -xbc is unknown");
    check(optopt == 'c', "optopt for -c");

    /* The first operand ends the options; getopt doesn't permute */
    char *operand[] = { "prog", "-a", "op", "-b", NULL };
    optind = 1;
    step(ARGC(operand), operand, "ab", 'a', 2, NULL, "-a before an operand");
    step(ARGC(operand), operand, "ab", -1, 2, NULL, "operand ends the options");
    step(ARGC(operand), operand, "ab", -1, 2, NULL, "and stays at the operand");
    check(strcmp(operand[2], "op") == 0 && strcmp(operand[3], "-b") == 0, "argv unchanged");

    /* "-" ends the options without moving optind */
    char *lone[] = { "prog", "-a", "-", "-b", NULL };
    optind = 1;
    step(ARGC(lone), lone, "ab", 'a', 2, NULL, "-a before -");
    step(ARGC(lone), lone, "ab", -1, 2, NULL, "\"-\" ends the options");

    /* "--" ends them after moving optind past it, even before "-" */
    char *ddash[] = { "prog", "--", "-", NULL };
    optind = 1;
    step(ARGC(ddash), ddash, "a", -1, 2, NULL, "\"--\" first");

    /* No arguments at all, and a null argv[optind] */
    char *none[] = { "prog", NULL };
    optind = 1;
    step(ARGC(none), none, "a", -1, 1, NULL, "no arguments");

    /* An unknown option is '?' with optopt set to it */
    char *unknown[] = { "prog", "-x", "-a", NULL };
    optind = 1;
    step(ARGC(unknown), unknown, "a", '?', 2, NULL, "unknown option");
    check(optopt == 'x', "optopt is the unknown option");
    step(ARGC(unknown), unknown, "a", 'a', 3, NULL, "after an unknown option");

    /* ':' is never an option character, though optstring contains it */
    char *colon[] = { "prog", "-:", NULL };
    optind = 1;
    step(ARGC(colon), colon, "a:", '?', 2, NULL, "-: is unknown");
    check(optopt == ':', "optopt is ':'");

    /*
     * A missing option-argument is '?', or ':' after a leading ':', with
     * optopt the option; optind moves by 2, past argc. glibc leaves it at
     * argc.
     */
    char *missing[] = { "prog", "-a", NULL };
    optind = 1;
    optopt = 0;
#ifndef __GLIBC__
#define MISSING_OPTIND 3
#else
#define MISSING_OPTIND 2
#endif
    step(ARGC(missing), missing, "a:", '?', MISSING_OPTIND, NULL, "missing argument");
    check(optopt == 'a', "optopt for a missing argument");
    optind = 1;
    optopt = 0;
    step(ARGC(missing), missing, ":a:", ':', MISSING_OPTIND, NULL, "missing argument with ':'");
    check(optopt == 'a', "optopt for a missing argument with ':'");
    char *missing2[] = { "prog", "-ba", NULL };
    optind = 1;
    step(ARGC(missing2), missing2, ":a:b", 'b', 1, NULL, "-b of -ba");
    step(ARGC(missing2), missing2, ":a:b", ':', MISSING_OPTIND, NULL, "-a at the end of -ba");

    /* A leading '+' has no effect, and the ':' after it still counts */
    optind = 1;
    step(ARGC(operand), operand, "+ab", 'a', 2, NULL, "+: -a");
    step(ARGC(operand), operand, "+ab", -1, 2, NULL, "+: operand ends the options");
    optind = 1;
    step(ARGC(missing), missing, "+:a:", ':', MISSING_OPTIND, NULL, "+: then ':'");
    optind = 1;
    step(ARGC(unknown), unknown, "+a", '?', 2, NULL, "+: unknown option");
    check(optopt == 'x', "+: optopt");

    /* Setting optind back to 1 scans again from the start */
    optind = 1;
    step(ARGC(basic), basic, "ab:c:d", 'a', 2, NULL, "rescan -a");
    step(ARGC(basic), basic, "ab:c:d", 'b', 4, "arg", "rescan -b");
    while (getopt(ARGC(basic), basic, "ab:c:d") != -1)
        ;
    check(optind == 6, "rescan end");
    optind = 1;
    step(ARGC(group), group, "abc", 'a', 1, NULL, "rescan another argv");
    /* Finish the element; restarting inside one is unspecified */
    step(ARGC(group), group, "abc", 'b', 2, NULL, "rescan another argv, -b");

#ifdef HAVE_DIAGNOSTICS
    const char *d;

    diag_start();

    /* With opterr set, unknown options and missing arguments are reported */
    opterr = 1;
    optind = 1;
    step(ARGC(unknown), unknown, "a", '?', 2, NULL, "reported unknown option");
    d = diag_read();
    /* POSIX: the message names the program and the option character */
    check(strstr(d, "prog") != NULL && strchr(d, 'x') != NULL, "unknown option diagnostic");
    optind = 1;
    step(ARGC(missing), missing, "a:", '?', MISSING_OPTIND, NULL, "reported missing argument");
    d = diag_read();
    check(strstr(d, "prog") != NULL && strchr(d, 'a') != NULL, "missing argument diagnostic");

    /* A leading ':' (after an optional '+') silences them */
    optind = 1;
    step(ARGC(unknown), unknown, ":a", '?', 2, NULL, "silent unknown option");
    optind = 1;
    step(ARGC(missing), missing, ":a:", ':', MISSING_OPTIND, NULL, "silent missing argument");
    optind = 1;
    step(ARGC(missing), missing, "+:a:", ':', MISSING_OPTIND, NULL, "silent missing argument +");
    d = diag_read();
    check(d[0] == '\0', "leading ':' silences the diagnostics");

    /* So does opterr = 0 */
    opterr = 0;
    optind = 1;
    step(ARGC(unknown), unknown, "a", '?', 2, NULL, "unknown option, opterr 0");
    optind = 1;
    step(ARGC(missing), missing, "a:", '?', MISSING_OPTIND, NULL, "missing argument, opterr 0");
    d = diag_read();
    check(d[0] == '\0', "opterr 0 silences the diagnostics");

    /* Options parsed without error print nothing */
    opterr = 1;
    optind = 1;
    while (getopt(ARGC(basic), basic, "ab:c:d") != -1)
        ;
    d = diag_read();
    check(d[0] == '\0', "no diagnostics without errors");

    /*
     * A write error while reporting sets stderr's error indicator, and
     * getopt still succeeds
     */
    check(close(2) == 0, "close stderr");
    clearerr(stderr);
    optind = 1;
    step(ARGC(unknown), unknown, "a", '?', 2, NULL, "unknown option, stderr closed");
    check(ferror(stderr), "write error sets the error indicator");
    clearerr(stderr);
    check(dup2(diag_fd, 2) == 2, "reopen stderr");

    diag_stop();
#endif

    printf("success\n");
    return 0;
}
