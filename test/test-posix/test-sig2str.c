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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* sig2str and str2sig translate between signal numbers and names */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static const struct {
    const char *name;
    int         sig;
} signals[] = {
    { "ABRT",   SIGABRT   },
    { "ALRM",   SIGALRM   },
    { "BUS",    SIGBUS    },
    { "CHLD",   SIGCHLD   },
    { "CONT",   SIGCONT   },
    { "FPE",    SIGFPE    },
    { "HUP",    SIGHUP    },
    { "ILL",    SIGILL    },
    { "INT",    SIGINT    },
    { "KILL",   SIGKILL   },
    { "PIPE",   SIGPIPE   },
    { "QUIT",   SIGQUIT   },
    { "SEGV",   SIGSEGV   },
    { "STOP",   SIGSTOP   },
    { "TERM",   SIGTERM   },
    { "TSTP",   SIGTSTP   },
    { "TTIN",   SIGTTIN   },
    { "TTOU",   SIGTTOU   },
    { "USR1",   SIGUSR1   },
    { "USR2",   SIGUSR2   },
    { "WINCH",  SIGWINCH  },
    { "SYS",    SIGSYS    },
    { "TRAP",   SIGTRAP   },
    { "URG",    SIGURG    },
    { "VTALRM", SIGVTALRM },
    { "XCPU",   SIGXCPU   },
    { "XFSZ",   SIGXFSZ   },
};

#define NSIGNALS (sizeof(signals) / sizeof(signals[0]))

#ifdef SIGRTMAX
#define LAST_SIG SIGRTMAX
#else
#define LAST_SIG 64
#endif

static int
rejected(const char *str)
{
    int sig = -12345;

    return str2sig(str, &sig) == -1 && sig == -12345;
}

int
main(void)
{
    char   str[SIG2STR_MAX];
    char   want[SIG2STR_MAX + 8];
    int    sig, n;
    size_t i;

    /* The table's signals are named without the SIG prefix */
    for (i = 0; i < NSIGNALS; i++) {
        check(sig2str(signals[i].sig, str) == 0, "sig2str");
        check(strcmp(str, signals[i].name) == 0, "signal name");
        check(str2sig(signals[i].name, &sig) == 0 && sig == signals[i].sig, "str2sig name");
        snprintf(want, sizeof(want), "%d", signals[i].sig);
        check(str2sig(want, &sig) == 0 && sig == signals[i].sig, "str2sig decimal");
    }

#ifdef SIGRTMIN
    /* Real-time signals */
    check(sig2str(SIGRTMIN, str) == 0 && strcmp(str, "RTMIN") == 0, "RTMIN");
    check(sig2str(SIGRTMAX, str) == 0 && strcmp(str, "RTMAX") == 0, "RTMAX");
    check(str2sig("RTMIN", &sig) == 0 && sig == SIGRTMIN, "str2sig RTMIN");
    check(str2sig("RTMAX", &sig) == 0 && sig == SIGRTMAX, "str2sig RTMAX");
    for (sig = SIGRTMIN + 1; sig <= (SIGRTMIN + SIGRTMAX) / 2; sig++) {
        snprintf(want, sizeof(want), "RTMIN+%d", sig - SIGRTMIN);
        check(sig2str(sig, str) == 0 && strcmp(str, want) == 0, "lower half is RTMIN+n");
    }
    for (; sig < SIGRTMAX; sig++) {
        check(sig2str(sig, str) == 0, "upper half");
        snprintf(want, sizeof(want), "RTMAX-%d", SIGRTMAX - sig);
        if (strcmp(str, want) != 0)
            snprintf(want, sizeof(want), "RTMIN+%d", sig - SIGRTMIN);
        check(strcmp(str, want) == 0, "upper half is RTMAX-m or RTMIN+n");
    }
    for (n = 1; n <= SIGRTMAX - SIGRTMIN - 1; n++) {
        snprintf(want, sizeof(want), "RTMIN+%d", n);
        check(str2sig(want, &sig) == 0 && sig == SIGRTMIN + n, "str2sig RTMIN+n");
        snprintf(want, sizeof(want), "RTMAX-%d", n);
        check(str2sig(want, &sig) == 0 && sig == SIGRTMAX - n, "str2sig RTMAX-n");
    }

#endif

    /* Every string sig2str returns fits SIG2STR_MAX and translates back */
    for (sig = 1; sig <= LAST_SIG; sig++) {
        memset(str, 0, sizeof(str));
        if (sig2str(sig, str) != 0)
            continue;
        check(strlen(str) < SIG2STR_MAX, "name fits SIG2STR_MAX");
        check(str2sig(str, &n) == 0 && n == sig, "sig2str result translates back");
    }

    /* Invalid numbers and strings */
    check(sig2str(-1, str) == -1, "sig2str negative");
#ifdef SIGRTMAX
    check(sig2str(SIGRTMAX + 1, str) == -1, "sig2str past SIGRTMAX");
#endif
    check(rejected("NOSUCHSIGNAL"), "unknown name");
    check(rejected(""), "empty string");
    check(rejected("-1"), "negative number");
#ifdef SIGRTMAX
    snprintf(want, sizeof(want), "%d", SIGRTMAX + 1);
    check(rejected(want), "number past SIGRTMAX");
    snprintf(want, sizeof(want), "RTMIN+%d", SIGRTMAX - SIGRTMIN + 5);
    check(rejected(want), "RTMIN+n past SIGRTMAX");
    check(rejected("RTMIN+-1") && rejected("RTMAX-x"), "RTMIN+ and RTMAX- without a number");
    check(rejected("RTMIN+ 1"), "white space after RTMIN+");
#endif
    check(rejected(" 2") && rejected("+2"), "white space and signs aren't decimal numbers");

    printf("success\n");
    return 0;
}
