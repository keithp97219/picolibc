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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_SIGNALS) && defined(__PICOLIBC_POSIX_USER_GROUPS) \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Installing, querying and resetting signal actions */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;
static volatile sig_atomic_t plain_caught;
static volatile sig_atomic_t self_blocked;
static volatile sig_atomic_t other_blocked;
static int                   pipe_fds[2];
static siginfo_t             info;

static void
handler(int sig, siginfo_t *si, void *context)
{
    (void)context;
    caught = sig;
    info = *si;
}

/* Records the signal mask in effect while the handler runs */
static void
mask_handler(int sig)
{
    sigset_t mask;

    plain_caught = sig;
    sigprocmask(SIG_BLOCK, NULL, &mask);
    self_blocked = sigismember(&mask, sig);
    other_blocked = sigismember(&mask, SIGUSR2);
}

static void
other_handler(int sig)
{
    plain_caught = sig;
}

/*
 * Queries the action with several function pointers live in registers,
 * so that a sigaction which picked up a stale handler from a register
 * or stack slot would likely install one of them
 */
static void __attribute__((noinline, noclone))
query_action(int sig, struct sigaction *old, void (*a)(int), void (*b)(int), void (*c)(int),
             void (*d)(int), void (*e)(int))
{
    check(sigaction(sig, NULL, old) == 0, "query");
    a(0);
    b(0);
    c(0);
    d(0);
    e(0);
}

/* Makes the interrupted read return something once restarted */
static void
alarm_handler(int sig)
{
    plain_caught = sig;
    (void)write(pipe_fds[1], "x", 1);
}

/* Blocks in read on a pipe until SIGALRM arrives, returning what read returned */
static ssize_t
read_interrupted(int flags, int *err)
{
    struct sigaction act;
    char             c;
    ssize_t          ret;

    memset(&act, 0, sizeof(act));
    act.sa_handler = alarm_handler;
    act.sa_flags = flags;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGALRM, &act, NULL) == 0, "sigaction SIGALRM");
    check(pipe(pipe_fds) == 0, "pipe");
    plain_caught = 0;
    alarm(1);
    errno = 0;
    ret = read(pipe_fds[0], &c, 1);
    *err = errno;
    check(plain_caught == SIGALRM, "alarm delivered");
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return ret;
}

/*
 * qemu's user-mode emulation reports BogoMIPS as 100.00 for every Arm
 * CPU; real hardware reports twice its timer frequency
 */
static bool
is_qemu(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    char  line[256];
    bool  ret = false;

    if (!f)
        return false;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "BogoMIPS", 8) == 0) {
            ret = strstr(line, "100.00") != NULL;
            break;
        }
    }
    fclose(f);
    return ret;
}

int
main(void)
{
    struct sigaction act, old;
    sigset_t         set, pending;
    pid_t            child;
    int              stop_signalled;
    bool             qemu = is_qemu();
    int              status, err;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR2);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");

    check(kill(getpid(), SIGUSR1) == 0, "kill");
    check(caught == SIGUSR1, "handler signal number");
    check(info.si_signo == SIGUSR1, "si_signo");
    check(info.si_code == SI_USER, "si_code from kill");
    check(info.si_pid == getpid(), "si_pid");
    check(info.si_uid == getuid(), "si_uid");

    /* A null act leaves the handling alone and reports it in oact */
    old.sa_handler = other_handler;
    old.sa_flags = 0;
    sigemptyset(&old.sa_mask);
    check(sigaction(SIGUSR2, &old, NULL) == 0, "sigaction on another signal");
    memset(&old, 0, sizeof(old));
    query_action(SIGUSR1, &old, other_handler, other_handler, other_handler, other_handler,
                 other_handler);
    check(old.sa_sigaction == handler, "query reports the handler");
    check(old.sa_flags & SA_SIGINFO, "query reports SA_SIGINFO");
    check(sigismember(&old.sa_mask, SIGUSR2) && !sigismember(&old.sa_mask, SIGINT),
          "query reports the mask");
    caught = 0;
    check(raise(SIGUSR1) == 0 && caught == SIGUSR1, "handler still installed after a query");
    check(sigaction(SIGUSR1, NULL, NULL) == 0, "query with neither act nor oact");
    caught = 0;
    check(raise(SIGUSR1) == 0 && caught == SIGUSR1, "handler still installed after queries");

    /* oact gets the previous action when installing a new one */
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    check(sigaction(SIGUSR1, &act, &old) == 0, "ignore");
    check(old.sa_sigaction == handler && (old.sa_flags & SA_SIGINFO), "previous action");
    caught = 0;
    check(raise(SIGUSR1) == 0 && caught == 0, "SIG_IGN ignores the signal");
    act.sa_handler = SIG_DFL;
    check(sigaction(SIGUSR1, &act, &old) == 0 && old.sa_handler == SIG_IGN, "SIG_IGN reported");
    check(sigaction(SIGUSR1, NULL, &old) == 0 && old.sa_handler == SIG_DFL, "SIG_DFL reported");

    /* The signal and sa_mask are blocked in the handler, then the mask is restored */
    act.sa_handler = mask_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR2);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "mask handler");
    check(raise(SIGUSR1) == 0 && plain_caught == SIGUSR1, "mask handler ran");
    check(self_blocked && other_blocked, "signal and sa_mask blocked in the handler");
    sigprocmask(SIG_BLOCK, NULL, &set);
    check(!sigismember(&set, SIGUSR1) && !sigismember(&set, SIGUSR2), "mask restored");

    /* SA_NODEFER leaves the signal itself unblocked */
    act.sa_flags = SA_NODEFER;
    check(sigaction(SIGUSR1, &act, NULL) == 0, "SA_NODEFER");
    plain_caught = 0;
    check(raise(SIGUSR1) == 0 && plain_caught == SIGUSR1, "SA_NODEFER handler ran");
    check(!self_blocked && other_blocked, "SA_NODEFER");

    /* SA_RESETHAND restores SIG_DFL on entry to the handler */
    act.sa_flags = SA_RESETHAND;
    check(sigaction(SIGUSR1, &act, NULL) == 0, "SA_RESETHAND");
    plain_caught = 0;
    check(raise(SIGUSR1) == 0 && plain_caught == SIGUSR1, "SA_RESETHAND handler ran");
    check(sigaction(SIGUSR1, NULL, &old) == 0 && old.sa_handler == SIG_DFL,
          "SA_RESETHAND resets the disposition");

    /* Errors */
    act.sa_handler = mask_handler;
    act.sa_flags = 0;
    errno = 0;
    check(sigaction(-1, &act, NULL) == -1 && errno == EINVAL, "negative signal");
    errno = 0;
    check(sigaction(SIGRTMAX + 1, &act, NULL) == -1 && errno == EINVAL, "past SIGRTMAX");
    errno = 0;
    check(sigaction(SIGKILL, &act, NULL) == -1 && errno == EINVAL, "catching SIGKILL");
    act.sa_handler = SIG_IGN;
    errno = 0;
    check(sigaction(SIGSTOP, &act, NULL) == -1 && errno == EINVAL, "ignoring SIGSTOP");
    check(sigaction(SIGKILL, NULL, &old) == 0 && old.sa_handler == SIG_DFL,
          "querying SIGKILL succeeds");
    check(sigaction(SIGSTOP, NULL, &old) == 0 && old.sa_handler == SIG_DFL,
          "failed sigaction installs nothing");

    /* SA_RESTART restarts an interrupted read; without it, read fails with EINTR */
    check(read_interrupted(SA_RESTART, &err) == 1, "SA_RESTART");
    check(read_interrupted(0, &err) == -1 && err == EINTR, "EINTR without SA_RESTART");

    /* SA_NOCLDSTOP: no SIGCHLD when a child stops */
    act.sa_handler = mask_handler;
    act.sa_flags = SA_NOCLDSTOP;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGCHLD, &act, NULL) == 0, "SA_NOCLDSTOP");
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "block SIGCHLD");
    child = fork();
    check(child >= 0, "fork");
    if (child == 0) {
        raise(SIGSTOP);
        _exit(0);
    }
    check(waitpid(child, &status, WUNTRACED) == child && WIFSTOPPED(status), "child stopped");
    check(sigpending(&pending) == 0, "sigpending");
    stop_signalled = sigismember(&pending, SIGCHLD);
    /* Reap the child first, so a failure can't leave it stopped */
    check(kill(child, SIGKILL) == 0, "kill child");
    check(waitpid(child, &status, 0) == child, "reap child");
    /* qemu's user-mode emulation ignores SA_NOCLDSTOP */
    if (qemu && stop_signalled)
        printf("qemu: SIGCHLD for a stopped child despite SA_NOCLDSTOP\n");
    else
        check(!stop_signalled, "no SIGCHLD for a stopped child");
    check(sigpending(&pending) == 0 && sigismember(&pending, SIGCHLD),
          "SIGCHLD for a terminated child");
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    check(sigaction(SIGCHLD, &act, NULL) == 0, "default SIGCHLD");
    check(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0, "unblock SIGCHLD");

    /* SA_NOCLDWAIT: terminated children leave no status to wait for */
    act.sa_flags = SA_NOCLDWAIT;
    check(sigaction(SIGCHLD, &act, NULL) == 0, "SA_NOCLDWAIT");
    child = fork();
    check(child >= 0, "fork");
    if (child == 0)
        _exit(0);
    errno = 0;
    /* qemu's user-mode emulation ignores SA_NOCLDWAIT too */
    if (qemu && wait(&status) == child)
        printf("qemu: zombie despite SA_NOCLDWAIT\n");
    else
        check(wait(&status) == -1 && errno == ECHILD, "SA_NOCLDWAIT leaves no zombie");

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
