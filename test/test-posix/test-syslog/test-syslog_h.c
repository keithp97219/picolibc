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
/* <syslog.h> is an XSI header */
#define _XOPEN_SOURCE 800
#include <syslog.h>
#include <stdio.h>

/* What <syslog.h> defines in strict XSI mode */

/* vsyslog isn't POSIX, so this mustn't clash with a declaration */
static int
vsyslog(void)
{
    return 0;
}

/* The openlog options, facilities and severities are distinct */
static int
check_distinct(int value)
{
    switch (value) {
    case LOG_PID:
    case LOG_CONS:
    case LOG_NDELAY:
    case LOG_ODELAY:
    case LOG_NOWAIT:
        break;
    }
    switch (value) {
    case LOG_KERN:
    case LOG_USER:
    case LOG_MAIL:
    case LOG_NEWS:
    case LOG_UUCP:
    case LOG_DAEMON:
    case LOG_AUTH:
    case LOG_CRON:
    case LOG_LPR:
    case LOG_LOCAL0:
    case LOG_LOCAL1:
    case LOG_LOCAL2:
    case LOG_LOCAL3:
    case LOG_LOCAL4:
    case LOG_LOCAL5:
    case LOG_LOCAL6:
    case LOG_LOCAL7:
        break;
    }
    switch (value) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
    case LOG_WARNING:
    case LOG_NOTICE:
    case LOG_INFO:
    case LOG_DEBUG:
        break;
    }
    return value;
}

/* LOG_MASK and LOG_UPTO give int for an int argument */
_Static_assert(_Generic(LOG_MASK(LOG_ERR), int: 1, default: 0), "LOG_MASK is int");
_Static_assert(_Generic(LOG_UPTO(LOG_ERR), int: 1, default: 0), "LOG_UPTO is int");

/* The example from setlogmask's page: both masks are the same */
_Static_assert((LOG_UPTO(LOG_WARNING) | LOG_MASK(LOG_DEBUG))
                   == (LOG_UPTO(LOG_DEBUG) & ~((LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_INFO)))),
               "mask example");

/* Prototypes */
_Static_assert(_Generic(&closelog, void (*)(void): 1, default: 0), "closelog");
_Static_assert(_Generic(&openlog, void (*)(const char *, int, int): 1, default: 0), "openlog");
_Static_assert(_Generic(&setlogmask, int (*)(int): 1, default: 0), "setlogmask");
_Static_assert(_Generic(&syslog, void (*)(int, const char *, ...): 1, default: 0), "syslog");

#define check(condition)                        \
    do {                                        \
        if (!(condition)) {                     \
            printf("failed: %s\n", #condition); \
            ret = 1;                            \
        }                                       \
    } while (0)

int
main(void)
{
    static const int sev[]
        = { LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG };
    unsigned i, j;
    int      ret = vsyslog();

    (void)check_distinct(0);

    /*
     * LOG_UPTO(pri) holds the mask of every severity from LOG_EMERG
     * through pri in the order listed, and no other
     */
    for (i = 0; i < sizeof(sev) / sizeof(sev[0]); i++) {
        int all = 0;

        check(LOG_MASK(sev[i]) != 0);
        for (j = 0; j < sizeof(sev) / sizeof(sev[0]); j++) {
            if (j <= i)
                check((LOG_UPTO(sev[i]) & LOG_MASK(sev[j])) == LOG_MASK(sev[j]));
            else
                check((LOG_UPTO(sev[i]) & LOG_MASK(sev[j])) == 0);
            if (j != i)
                check((LOG_MASK(sev[i]) & LOG_MASK(sev[j])) == 0);
            if (j <= i)
                all |= LOG_MASK(sev[j]);
        }
        check(LOG_UPTO(sev[i]) == all);
    }
    if (ret == 0)
        printf("success\n");
    return ret;
}
