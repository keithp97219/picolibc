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
/* TRAP_BRKPT, TRAP_TRACE and killpg are XSI interfaces */
#define _XOPEN_SOURCE 800
#include <signal.h>
#include <unistd.h>

/* The types <signal.h> defines */
_Static_assert(sizeof(size_t) > 0 && sizeof(uid_t) > 0 && sizeof(pid_t) > 0
                   && sizeof(sig_atomic_t) > 0 && sizeof(sigset_t) > 0
                   && sizeof(struct timespec) > 0,
               "size_t, uid_t, pid_t, sig_atomic_t, sigset_t, struct timespec");

/* SIG_DFL, SIG_ERR and SIG_IGN have the type signal() takes and returns */
static void (* const handlers[])(int) = { SIG_DFL, SIG_ERR, SIG_IGN };

#define member_is(type, s, m, t) _Generic(((type *)0)->m, t: 1, default: 0)

/* struct sigevent */
typedef void (*notify_function)(union sigval);
_Static_assert(member_is(struct sigevent, s, sigev_notify, int)
                   && member_is(struct sigevent, s, sigev_signo, int)
                   && member_is(struct sigevent, s, sigev_value, union sigval)
                   && member_is(struct sigevent, s, sigev_notify_function, notify_function),
               "struct sigevent members");
_Static_assert(SIGEV_NONE != SIGEV_SIGNAL && SIGEV_NONE != SIGEV_THREAD
                   && SIGEV_SIGNAL != SIGEV_THREAD,
               "SIGEV_ values");

/* union sigval */
_Static_assert(member_is(union sigval, u, sival_int, int)
                   && member_is(union sigval, u, sival_ptr, void *),
               "union sigval members");

/* SIG2STR_MAX can be used in #if; glibc doesn't have it */
#if defined(__PICOLIBC__) && (!defined(SIG2STR_MAX) || SIG2STR_MAX < 6)
#error SIG2STR_MAX
#endif

/* The signals, which are distinct positive int constants */
static const int signals[] = {
    SIGABRT, SIGALRM, SIGBUS,   SIGCHLD, SIGCONT, SIGFPE,  SIGHUP,    SIGILL,  SIGINT,
    SIGKILL, SIGPIPE, SIGQUIT,  SIGSEGV, SIGSTOP, SIGTERM, SIGTSTP,   SIGTTIN, SIGTTOU,
    SIGUSR1, SIGUSR2, SIGWINCH, SIGSYS,  SIGTRAP, SIGURG,  SIGVTALRM, SIGXCPU, SIGXFSZ,
};
#define NSIGNALS (sizeof(signals) / sizeof(signals[0]))
_Static_assert(_Generic(SIGABRT, int: 1, default: 0) && _Generic(SIGRTMIN, int: 1, default: 0)
                   && _Generic(SIGRTMAX, int: 1, default: 0),
               "signal numbers are int");

/* struct sigaction */
typedef void (*sa_handler_type)(int);
typedef void (*sa_sigaction_type)(int, siginfo_t *, void *);
_Static_assert(member_is(struct sigaction, s, sa_handler, sa_handler_type)
                   && member_is(struct sigaction, s, sa_mask, sigset_t)
                   && member_is(struct sigaction, s, sa_flags, int)
                   && member_is(struct sigaction, s, sa_sigaction, sa_sigaction_type),
               "struct sigaction members");

/* sigprocmask how values are integer constants, not necessarily usable in #if */
_Static_assert(SIG_BLOCK != SIG_UNBLOCK && SIG_BLOCK != SIG_SETMASK && SIG_UNBLOCK != SIG_SETMASK,
               "SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK");

/* sa_flags values, which can be combined */
static const int sa_flags[]
    = { SA_NOCLDSTOP, SA_ONSTACK, SA_RESETHAND, SA_RESTART, SA_SIGINFO, SA_NOCLDWAIT, SA_NODEFER };
#define NSA_FLAGS (sizeof(sa_flags) / sizeof(sa_flags[0]))

/* sigaltstack constants */
_Static_assert(SS_ONSTACK != SS_DISABLE && MINSIGSTKSZ > 0 && SIGSTKSZ >= MINSIGSTKSZ,
               "SS_ONSTACK, SS_DISABLE, MINSIGSTKSZ, SIGSTKSZ");

/* stack_t */
_Static_assert(member_is(stack_t, s, ss_sp, void *) && member_is(stack_t, s, ss_size, size_t)
                   && member_is(stack_t, s, ss_flags, int),
               "stack_t members");

/* siginfo_t */
_Static_assert(member_is(siginfo_t, s, si_signo, int) && member_is(siginfo_t, s, si_code, int)
                   && member_is(siginfo_t, s, si_errno, int)
                   && member_is(siginfo_t, s, si_pid, pid_t)
                   && member_is(siginfo_t, s, si_uid, uid_t)
                   && member_is(siginfo_t, s, si_addr, void *)
                   && member_is(siginfo_t, s, si_status, int)
                   && member_is(siginfo_t, s, si_value, union sigval),
               "siginfo_t members");

/* si_code values; the codes for one signal are distinct from each other and from the SI_ codes */
#define SI_CODES SI_USER, SI_QUEUE, SI_TIMER, SI_ASYNCIO, SI_MESGQ
static const int ill_codes[] = { ILL_ILLOPC, ILL_ILLOPN, ILL_ILLADR, ILL_ILLTRP, ILL_PRVOPC,
                                 ILL_PRVREG, ILL_COPROC, ILL_BADSTK, SI_CODES };
static const int fpe_codes[] = { FPE_INTDIV, FPE_INTOVF, FPE_FLTDIV, FPE_FLTOVF, FPE_FLTUND,
                                 FPE_FLTRES, FPE_FLTINV, FPE_FLTSUB, SI_CODES };
static const int segv_codes[] = { SEGV_MAPERR, SEGV_ACCERR, SI_CODES };
static const int bus_codes[] = { BUS_ADRALN, BUS_ADRERR, BUS_OBJERR, SI_CODES };
static const int trap_codes[] = { TRAP_BRKPT, TRAP_TRACE, SI_CODES };
static const int cld_codes[]
    = { CLD_EXITED, CLD_KILLED, CLD_DUMPED, CLD_TRAPPED, CLD_STOPPED, CLD_CONTINUED, SI_CODES };

#include "../posix-groups.h"

/* The thread types and the functions need system support */
#if defined(__PICOLIBC_POSIX_SIGNALS) && _POSIX_THREADS > 0
#define CHECK_PROTOTYPES

/* The thread types, which need thread support */
_Static_assert(sizeof(pthread_t) > 0 && sizeof(pthread_attr_t) > 0, "pthread_t, pthread_attr_t");
_Static_assert(member_is(struct sigevent, s, sigev_notify_attributes, pthread_attr_t *),
               "sigev_notify_attributes is a pthread_attr_t pointer");

/* Each function has the specified prototype */
static int  ( *const p_kill)(pid_t, int) = kill;
static int  ( *const p_killpg)(pid_t, int) = killpg;
static void (* const p_psiginfo)(const siginfo_t *, const char *) = psiginfo;
static void (* const p_psignal)(int, const char *) = psignal;
static int  ( *const p_pthread_kill)(pthread_t, int) = pthread_kill;
static int  ( *const p_pthread_sigmask)(int, const sigset_t  *restrict, sigset_t  *restrict)
    = pthread_sigmask;
static int (* const p_raise)(int) = raise;
#ifndef __GLIBC__
/* glibc doesn't have sig2str and str2sig */
static int (* const p_sig2str)(int, char *) = sig2str;
static int (* const p_str2sig)(const char * restrict, int * restrict) = str2sig;
#endif
static int (* const p_sigaction)(int, const struct sigaction * restrict,
                                 struct sigaction * restrict)
    = sigaction;
static int  ( *const p_sigaddset)(sigset_t *, int) = sigaddset;
static int  ( *const p_sigaltstack)(const stack_t  *restrict, stack_t  *restrict) = sigaltstack;
static int  ( *const p_sigdelset)(sigset_t *, int) = sigdelset;
static int  ( *const p_sigemptyset)(sigset_t *) = sigemptyset;
static int  ( *const p_sigfillset)(sigset_t *) = sigfillset;
static int  ( *const p_sigismember)(const sigset_t *, int) = sigismember;
static void (*(* const p_signal)(int, void (*)(int)))(int) = signal;
static int  ( *const p_sigpending)(sigset_t *) = sigpending;
static int  ( *const p_sigprocmask)(int, const sigset_t  *restrict, sigset_t  *restrict)
    = sigprocmask;
static int (* const p_sigqueue)(pid_t, int, union sigval) = sigqueue;
static int (* const p_sigsuspend)(const sigset_t *) = sigsuspend;
static int (* const p_sigtimedwait)(const sigset_t * restrict, siginfo_t * restrict,
                                    const struct timespec * restrict)
    = sigtimedwait;
static int (* const p_sigwait)(const sigset_t * restrict, int * restrict) = sigwait;
static int (* const p_sigwaitinfo)(const sigset_t * restrict, siginfo_t * restrict) = sigwaitinfo;

static const void * const prototypes[] = {
    &p_kill,         &p_killpg,          &p_psiginfo,   &p_psignal,
    &p_pthread_kill, &p_pthread_sigmask, &p_raise,      &p_sigaction,
    &p_sigaddset,    &p_sigaltstack,     &p_sigdelset,  &p_sigemptyset,
    &p_sigfillset,   &p_sigismember,     &p_signal,     &p_sigpending,
    &p_sigprocmask,  &p_sigqueue,        &p_sigsuspend, &p_sigtimedwait,
    &p_sigwait,      &p_sigwaitinfo,
#ifndef __GLIBC__
    &p_sig2str,      &p_str2sig,
#endif
};
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
distinct(const int *values, size_t n)
{
    size_t i, j;

    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (values[i] == values[j])
                return 0;
    return 1;
}

static void
some_handler(int sig)
{
    (void)sig;
}

#define DISTINCT(a) distinct(a, sizeof(a) / sizeof(a[0]))

int
main(void)
{
    size_t i;

#ifdef CHECK_PROTOTYPES
    (void)prototypes;
#endif

    check(handlers[0] != handlers[1] && handlers[0] != handlers[2] && handlers[1] != handlers[2],
          "SIG_DFL, SIG_ERR and SIG_IGN are distinct");
    check(handlers[0] != some_handler && handlers[1] != some_handler && handlers[2] != some_handler,
          "SIG_DFL and SIG_IGN differ from functions");

    check(DISTINCT(signals), "signal numbers are distinct");
    for (i = 0; i < NSIGNALS; i++) {
        check(signals[i] > 0, "signal numbers are positive");
        check(signals[i] < SIGRTMIN || signals[i] > SIGRTMAX,
              "real-time signals don't overlap the others");
#ifdef NSIG_MAX
        check(signals[i] < NSIG_MAX, "signal numbers are less than NSIG_MAX");
#endif
    }
    check(SIGRTMIN > 0 && SIGRTMAX >= SIGRTMIN, "SIGRTMIN and SIGRTMAX are positive");
    check(SIGRTMAX - SIGRTMIN + 1 >= _POSIX_RTSIG_MAX, "at least _POSIX_RTSIG_MAX signals");
#if defined(RTSIG_MAX) && !defined(__GLIBC__)
    /* glibc's RTSIG_MAX (32) counts the two signals it keeps for itself */
    check(SIGRTMAX - SIGRTMIN + 1 >= RTSIG_MAX, "at least RTSIG_MAX real-time signals");
#endif
#ifndef __GLIBC__
    /* glibc doesn't define NSIG_MAX */
    check(NSIG_MAX > SIGRTMAX, "NSIG_MAX past SIGRTMAX");
#endif
#ifdef _SC_NSIG
    check(sysconf(_SC_NSIG) > SIGRTMAX, "SIGRTMAX less than sysconf(_SC_NSIG)");
#endif

    for (i = 0; i < NSA_FLAGS; i++)
        check(sa_flags[i] != 0, "sa_flags values are non-zero");
    check(DISTINCT(sa_flags), "sa_flags values are distinct");

    check(DISTINCT(ill_codes), "SIGILL codes");
    check(DISTINCT(fpe_codes), "SIGFPE codes");
    check(DISTINCT(segv_codes), "SIGSEGV codes");
    check(DISTINCT(bus_codes), "SIGBUS codes");
    check(DISTINCT(trap_codes), "SIGTRAP codes");
    check(DISTINCT(cld_codes), "SIGCHLD codes");

    printf("success\n");
    return 0;
}
