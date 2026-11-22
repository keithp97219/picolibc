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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * strsignal: a string for every signal number, distinct messages for
 * distinct signals, and the same messages psignal prints.
 */

#define S(name) { name, #name }

static const struct {
    int         value;
    const char *name;
} signals[] = {
    S(SIGABRT),   S(SIGALRM), S(SIGBUS),  S(SIGCHLD), S(SIGCONT), S(SIGFPE),  S(SIGHUP),
    S(SIGILL),    S(SIGINT),  S(SIGKILL), S(SIGPIPE), S(SIGQUIT), S(SIGSEGV), S(SIGSTOP),
    S(SIGTERM),   S(SIGTSTP), S(SIGTTIN), S(SIGTTOU), S(SIGUSR1), S(SIGUSR2), S(SIGWINCH),
#ifdef SIGPROF
    S(SIGPROF),
#endif
#ifdef SIGSYS
    S(SIGSYS),
#endif
#ifdef SIGTRAP
    S(SIGTRAP),
#endif
#ifdef SIGURG
    S(SIGURG),
#endif
#ifdef SIGVTALRM
    S(SIGVTALRM),
#endif
#ifdef SIGXCPU
    S(SIGXCPU),
#endif
#ifdef SIGXFSZ
    S(SIGXFSZ),
#endif
};

#define N_SIGNALS (sizeof(signals) / sizeof(signals[0]))

#define MSG_MAX   256

static char messages[N_SIGNALS][MSG_MAX];
static int  ret;

#define fail(...)            \
    do {                     \
        printf(__VA_ARGS__); \
        printf("\n");        \
        ret = 1;             \
    } while (0)

#if TEST_POSIX_PIPE
/* What psignal(sig, prefix) writes to standard error */
static void
psignal_output(int sig, const char *prefix, char *out, size_t size)
{
    int     fds[2], saved;
    ssize_t n;

    fflush(stderr);
    if (pipe(fds) != 0 || (saved = dup(2)) < 0 || dup2(fds[1], 2) != 2) {
        printf("pipe and dup2 failed\n");
        exit(1);
    }
    psignal(sig, prefix);
    fflush(stderr);
    (void)dup2(saved, 2);
    close(saved);
    close(fds[1]);
    n = read(fds[0], out, size - 1);
    out[n > 0 ? n : 0] = '\0';
    close(fds[0]);
}
#endif

int
main(void)
{
    char  *p;
    size_t i, j;
    int    sig;

    for (i = 0; i < N_SIGNALS; i++) {
        p = strsignal(signals[i].value);
        if (p == NULL || *p == '\0') {
            fail("strsignal(%s) gave no message", signals[i].name);
            continue;
        }
        snprintf(messages[i], sizeof(messages[i]), "%s", p);
    }

    /* Distinct signals have distinct messages */
    for (i = 0; i < N_SIGNALS; i++)
        for (j = i + 1; j < N_SIGNALS; j++)
            if (signals[i].value != signals[j].value && strcmp(messages[i], messages[j]) == 0)
                fail("%s and %s share the message \"%s\"", signals[i].name, signals[j].name,
                     messages[i]);

    /* Every number up to SIGRTMAX gives a string */
    for (sig = 1; sig <= SIGRTMAX; sig++) {
        p = strsignal(sig);
        if (p == NULL || *p == '\0')
            fail("strsignal(%d) gave no message", sig);
    }

    /* Each real-time signal has its own message, unlike the others */
    for (sig = SIGRTMIN; sig <= SIGRTMAX; sig++) {
        char rt[MSG_MAX];
        int  other;

        snprintf(rt, sizeof(rt), "%s", strsignal(sig));
        for (i = 0; i < N_SIGNALS; i++)
            if (strcmp(rt, messages[i]) == 0)
                fail("SIGRTMIN+%d shares %s's message \"%s\"", sig - SIGRTMIN, signals[i].name, rt);
        for (other = SIGRTMIN; other < sig; other++)
            if (strcmp(rt, strsignal(other)) == 0)
                fail("SIGRTMIN+%d and SIGRTMIN+%d share the message \"%s\"", sig - SIGRTMIN,
                     other - SIGRTMIN, rt);
    }

#ifdef __PICOLIBC__
    /* picolibc's choice for numbers that aren't signals: a string, not NULL */
    {
        static const int bad[] = { 0, -1, -1000, 1000, SIGRTMAX + 1 };

        for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
            p = strsignal(bad[i]);
            if (p == NULL || *p == '\0')
                fail("strsignal(%d) gave no message", bad[i]);
        }
    }
#endif

#if TEST_POSIX_PIPE
    /* psignal uses the same messages */
    for (i = 0; i < N_SIGNALS; i++) {
        char out[MSG_MAX + 16];
        char want[MSG_MAX + 16];

        psignal_output(signals[i].value, "prefix", out, sizeof(out));
        snprintf(want, sizeof(want), "prefix: %.200s\n", messages[i]);
        if (strcmp(out, want) != 0)
            fail("psignal(%s) wrote \"%s\", not \"%s\"", signals[i].name, out, want);
    }
    /*
     * No function calls strsignal: psignal leaves the
     * string it returned alone, even one built from a number
     */
    {
        /* SIGRTMIN and SIGRTMAX needn't be constants */
        const int pairs[][2] = {
            { SIGRTMIN, SIGRTMAX },
            { SIGRTMAX, SIGRTMIN },
            { 1000,     999      },
            { -5,       -6       }
        };

        for (i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++) {
            int  first = pairs[i][0], second = pairs[i][1];
            char copy[MSG_MAX], out[MSG_MAX + 16];

            p = strsignal(first);
            snprintf(copy, sizeof(copy), "%s", p);
            psignal_output(second, "other", out, sizeof(out));
            if (strcmp(p, copy) != 0)
                fail("psignal(%d) changed strsignal(%d)'s string from \"%s\" to \"%s\"", second,
                     first, copy, p);
        }
    }
    /* glibc 2.43's psignal calls real-time signals "Unknown signal", unlike its strsignal */
#ifndef __GLIBC__
    for (sig = SIGRTMIN; sig <= SIGRTMAX; sig++) {
        char out[MSG_MAX + 16];
        char want[MSG_MAX + 16];

        snprintf(want, sizeof(want), "%s\n", strsignal(sig));
        psignal_output(sig, NULL, out, sizeof(out));
        if (strcmp(out, want) != 0)
            fail("psignal(SIGRTMIN+%d) wrote \"%s\", not \"%s\"", sig - SIGRTMIN, out, want);
    }
#endif
#endif

    if (ret == 0)
        printf("success\n");
    return ret;
}
