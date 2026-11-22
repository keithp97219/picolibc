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
/* sigaltstack and SS_DISABLE are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_THREADS && TEST_POSIX_SEMAPHORES && TEST_POSIX_PIPE && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <fenv.h>
#include <locale.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

/* Joining, thread termination and the state a new thread starts with */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static sem_t started, release;

static void
pause_ms(long ms)
{
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };

    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
        ;
}

/* Waits until released */
static void *
wait_release(void *arg)
{
    sem_post(&started);
    while (sem_wait(&release) != 0)
        ;
    return arg;
}

static void *
join_arg(void *arg)
{
    void *result = NULL;

    sem_post(&started);
    pthread_join(*(pthread_t *)arg, &result);
    return result;
}

/* Signals the joining thread until told to stop */
static volatile int       signals, stop_signals;
static volatile pthread_t signal_target;

static void
on_signal(int sig)
{
    (void)sig;
    signals++;
}

static void *
signaller(void *arg)
{
    (void)arg;
    while (!stop_signals) {
        pthread_kill(signal_target, SIGUSR1);
        pause_ms(1);
    }
    return NULL;
}

static void *
slow_return(void *arg)
{
    pause_ms(100);
    return arg;
}

/* The initial thread can be joined after it calls pthread_exit */
static void *
join_initial(void *arg)
{
    void *result = NULL;

    if (pthread_join(*(pthread_t *)arg, &result) != 0)
        _exit(2);
    _exit(result == (void *)5 ? 0 : 3);
}

static int
initial_child(void)
{
    static pthread_t initial;
    pthread_t        thread;

    initial = pthread_self();
    if (pthread_create(&thread, NULL, join_initial, &initial) != 0)
        return 1;
    pthread_exit((void *)5);
}

/* Thread exit doesn't run atexit handlers or release process resources */
static volatile int    atexit_ran;
static pthread_mutex_t held = PTHREAD_MUTEX_INITIALIZER;
static int             exit_pipe[2];

static void
at_exit(void)
{
    atexit_ran = 1;
}

static void *
lock_and_exit(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&held);
    if (pipe(exit_pipe) != 0)
        return (void *)1;
    pthread_exit(NULL);
}

/* The signal state, alternate stack, locale and floating-point environment of a new thread */
static void *
signal_state(void *arg)
{
    sigset_t set;
    stack_t  ss;

    (void)arg;
    if (sigpending(&set) != 0 || sigismember(&set, SIGUSR2))
        return (void *)1;
    if (pthread_sigmask(SIG_BLOCK, NULL, &set) != 0 || !sigismember(&set, SIGUSR2))
        return (void *)2;
    if (sigaltstack(NULL, &ss) != 0 || !(ss.ss_flags & SS_DISABLE))
        return (void *)3;
    return NULL;
}

static void *
get_locale(void *arg)
{
    (void)arg;
    return uselocale((locale_t)0) == LC_GLOBAL_LOCALE ? NULL : (void *)1;
}

static void *
get_round(void *arg)
{
    (void)arg;
    return (void *)(intptr_t)fegetround();
}

static void *
get_except(void *arg)
{
    return (void *)(intptr_t)fetestexcept((int)(intptr_t)arg);
}

#if TEST_POSIX_THREAD_CPUTIME
static long long
ns(clockid_t id)
{
    struct timespec ts;

    check(clock_gettime(id, &ts) == 0, "clock_gettime");
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void *
cpu_time(void *arg)
{
    (void)arg;
    return (void *)(intptr_t)(ns(CLOCK_THREAD_CPUTIME_ID) / 1000000);
}
#endif

int
main(void)
{
    struct sigaction act;
    pthread_t        thread, joiner, sig_thread;
    void            *result;
    sigset_t         set;
    pid_t            pid;
    int              status, sig;

    check(sem_init(&started, 0, 0) == 0, "sem_init");
    check(sem_init(&release, 0, 0) == 0, "sem_init");

    /* Signals don't interrupt pthread_join */
    memset(&act, 0, sizeof(act));
    act.sa_handler = on_signal;
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    signal_target = pthread_self();
    check(pthread_create(&thread, NULL, slow_return, (void *)6) == 0, "pthread_create");
    check(pthread_create(&sig_thread, NULL, signaller, NULL) == 0, "pthread_create signaller");
    check(pthread_join(thread, &result) == 0, "pthread_join with signals");
    check(result == (void *)6, "pthread_join value with signals");
    stop_signals = 1;
    check(pthread_join(sig_thread, NULL) == 0, "pthread_join signaller");
    check(signals > 0, "signals arrived during pthread_join");

    /* A cancelled pthread_join leaves the target joinable */
    check(pthread_create(&thread, NULL, wait_release, (void *)7) == 0, "pthread_create");
    check(sem_wait(&started) == 0, "sem_wait");
    check(pthread_create(&joiner, NULL, join_arg, &thread) == 0, "pthread_create joiner");
    check(sem_wait(&started) == 0, "sem_wait");
    pause_ms(20); /* let the joiner block */
    check(pthread_cancel(joiner) == 0, "pthread_cancel");
    check(pthread_join(joiner, &result) == 0 && result == PTHREAD_CANCELED, "joiner cancelled");
    check(sem_post(&release) == 0, "sem_post");
    check(pthread_join(thread, &result) == 0, "target still joinable");
    check(result == (void *)7, "target value");

    /* The initial thread, joined after pthread_exit */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(initial_child());
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        printf("child status %#x\n", status);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "join the initial thread");

    /* Thread termination runs no atexit handlers and releases no mutexes or descriptors */
    check(atexit(at_exit) == 0, "atexit");
    check(pthread_create(&thread, NULL, lock_and_exit, NULL) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && result == NULL, "pthread_join");
    check(!atexit_ran, "thread exit ran atexit handlers");
    check(pthread_mutex_trylock(&held) == EBUSY, "mutex still locked");
    check(write(exit_pipe[1], "x", 1) == 1, "descriptor still open");
    check(close(exit_pipe[0]) == 0 && close(exit_pipe[1]) == 0, "close");

    /* The signal mask is inherited; pending signals and the alternate stack aren't */
    {
        stack_t ss;

        sigemptyset(&set);
        sigaddset(&set, SIGUSR2);
        check(pthread_sigmask(SIG_BLOCK, &set, NULL) == 0, "pthread_sigmask");
        check(pthread_kill(pthread_self(), SIGUSR2) == 0, "pthread_kill");
        ss.ss_sp = malloc(65536);
        ss.ss_size = 65536;
        ss.ss_flags = 0;
        check(ss.ss_sp != NULL, "malloc");
        check(sigaltstack(&ss, NULL) == 0, "sigaltstack");
        check(pthread_create(&thread, NULL, signal_state, NULL) == 0, "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result != (void *)1, "pending signals aren't inherited");
        check(result != (void *)2, "the signal mask is inherited");
        check(result != (void *)3, "the alternate stack isn't inherited");
        check(sigwait(&set, &sig) == 0 && sig == SIGUSR2, "signal pending for the creator");
        check(pthread_sigmask(SIG_UNBLOCK, &set, NULL) == 0, "pthread_sigmask");
        ss.ss_flags = SS_DISABLE;
        check(sigaltstack(&ss, NULL) == 0, "sigaltstack disable");
    }

    /* The thread's current locale isn't inherited */
    {
        locale_t loc = newlocale(LC_ALL_MASK, "C", (locale_t)0);

        check(loc != (locale_t)0, "newlocale");
        check(uselocale(loc) != (locale_t)0, "uselocale");
        check(pthread_create(&thread, NULL, get_locale, NULL) == 0, "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result == NULL, "new thread uses the global locale");
        check(uselocale(LC_GLOBAL_LOCALE) == loc, "uselocale");
        freelocale(loc);
    }

    /* The floating-point environment is inherited */
#ifdef FE_UPWARD
    if (fesetround(FE_UPWARD) == 0 && fegetround() == FE_UPWARD) {
        check(pthread_create(&thread, NULL, get_round, NULL) == 0, "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result == (void *)(intptr_t)FE_UPWARD, "rounding mode inherited");
        fesetround(FE_TONEAREST);
    }
#endif
#ifdef FE_INEXACT
    feclearexcept(FE_ALL_EXCEPT);
    if (feraiseexcept(FE_INEXACT) == 0 && fetestexcept(FE_INEXACT)) {
        check(pthread_create(&thread, NULL, get_except, (void *)(intptr_t)FE_INEXACT) == 0,
              "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result != NULL, "exception flags inherited");
        feclearexcept(FE_ALL_EXCEPT);
    }
#endif

    /* A new thread's CPU-time clock starts at zero */
#if TEST_POSIX_THREAD_CPUTIME
    {
        /* Use some CPU time in the creating thread first */
        while (ns(CLOCK_THREAD_CPUTIME_ID) < 200000000LL)
            ;
        check(pthread_create(&thread, NULL, cpu_time, NULL) == 0, "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check((intptr_t)result < 100, "new thread's CPU-time clock starts at zero");
    }
#endif

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
