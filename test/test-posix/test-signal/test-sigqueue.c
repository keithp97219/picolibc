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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if _POSIX_REALTIME_SIGNALS > 0

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* A queued signal delivers its value to the handler */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;
static siginfo_t             info;

static void
handler(int sig, siginfo_t *si, void *context)
{
    (void)context;
    caught = sig;
    info = *si;
}

/* Catch SIGUSR1 and SIGUSR2 with handler */
static void
catch_usr(void)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check(sigaction(SIGUSR2, &act, NULL) == 0, "sigaction");
}

int
main(void)
{
    union sigval value;

    catch_usr();
    value.sival_int = 42;
    check(sigqueue(getpid(), SIGUSR1, value) == 0, "sigqueue");
    check(caught == SIGUSR1, "queued signal caught");
    check(info.si_signo == SIGUSR1 && info.si_code == SI_QUEUE, "queued signal info");
    check(info.si_value.sival_int == 42, "queued signal value");
    check(info.si_pid == getpid(), "queued signal sender");
    check(sigqueue(getpid(), 0, value) == 0, "sigqueue with no signal");
    errno = 0;
    check(sigqueue(getpid(), SIGRTMAX + 1, value) == -1 && errno == EINVAL, "sigqueue bad signal");
    errno = 0;
    check(sigqueue(getpid(), -1, value) == -1 && errno == EINVAL, "sigqueue negative signal");

    /* A pointer value arrives intact */
    value.sival_ptr = &value;
    check(sigqueue(getpid(), SIGUSR2, value) == 0, "sigqueue pointer");
    check(caught == SIGUSR2 && info.si_value.sival_ptr == &value, "queued pointer value");

    /*
     * EPERM for a process of another user. init usually runs as root,
     * but in a container it may be ours, and then the call succeeds.
     */
    if (geteuid() != 0 && getuid() != 0) {
        errno = 0;
        check(sigqueue(1, 0, value) == 0 || errno == EPERM, "EPERM");
    }
    /* ESRCH for a process that doesn't exist; Linux pids are below 2^22 */
    errno = 0;
    check(sigqueue(0x7fffffff, 0, value) == -1 && errno == ESRCH, "ESRCH");

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
