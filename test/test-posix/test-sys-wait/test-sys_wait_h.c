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
#include <sys/wait.h>

#include <stdio.h>

/*
 * <sys/wait.h> defines the wait options, the status macros, idtype_t,
 * id_t, pid_t, siginfo_t and the prototypes, and in strict mode
 * nothing from <sys/resource.h> or wait3
 */

#if !defined(WEXITSTATUS) || !defined(WIFEXITED) || !defined(WIFSIGNALED) || !defined(WIFSTOPPED) \
    || !defined(WSTOPSIG) || !defined(WTERMSIG)
#error "status macro missing"
#endif

/* New in Issue 8; glibc 2.43 defines it only with _DEFAULT_SOURCE */
#if !defined(WCOREDUMP) && !defined(__GLIBC__)
#error "WCOREDUMP missing"
#endif
#ifndef WCOREDUMP
#define WCOREDUMP(w) 0
#endif

#if !defined(WNOHANG) || !defined(WUNTRACED)
#error "waitpid option missing"
#endif

#if !defined(WEXITED) || !defined(WSTOPPED) || !defined(WNOWAIT)
#error "waitid option missing"
#endif

/* The waitpid options are distinct bits */
_Static_assert(WNOHANG != 0 && WUNTRACED != 0 && (WNOHANG & WUNTRACED) == 0, "waitpid options");

/* The waitid options can be combined */
_Static_assert(WEXITED != 0 && WSTOPPED != 0 && WNOWAIT != 0, "waitid options");
_Static_assert((WEXITED & WSTOPPED) == 0 && (WEXITED & WNOWAIT) == 0 && (WSTOPPED & WNOWAIT) == 0,
               "waitid options");
_Static_assert((WNOHANG & (WEXITED | WSTOPPED | WNOWAIT)) == 0, "WNOHANG with waitid");

#ifdef WCONTINUED
_Static_assert(WCONTINUED != 0 && (WCONTINUED & (WNOHANG | WUNTRACED)) == 0, "WCONTINUED");
_Static_assert((WCONTINUED & (WEXITED | WSTOPPED | WNOWAIT)) == 0, "WCONTINUED with waitid");
#endif

/* idtype_t values */
_Static_assert(P_ALL != P_PID && P_ALL != P_PGID && P_PID != P_PGID, "idtype_t values");

/* pid_t is signed; id_t can hold a pid_t */
_Static_assert((pid_t)-1 < 0, "pid_t signed");
_Static_assert(sizeof(id_t) >= sizeof(pid_t), "id_t size");

#define check_type(f, t) _Generic(&(f), t: 1, default: 0)

/* Nothing from <sys/resource.h>, which isn't one of the headers <sys/wait.h> may expose */
#if defined(RLIMIT_CORE) || defined(RLIM_INFINITY) || defined(PRIO_PROCESS) || defined(RUSAGE_SELF)
#error "<sys/resource.h> constants visible"
#endif
struct rlimit {
    char not_the_header_s;
};
static double getrlimit;
static double setrlimit;
static double getpriority;
static double setpriority;
static double getrusage;

/* wait3 is an extension */
static double wait3;

int
main(void)
{
    int          ok = 1;
    idtype_t     idtype = P_ALL;
    id_t         id = 0;
    siginfo_t    info;
    union sigval value;

    ok &= check_type(wait, pid_t (*)(int *));
    ok &= check_type(waitpid, pid_t (*)(pid_t, int *, int));
    ok &= check_type(waitid, int (*)(idtype_t, id_t, siginfo_t *, int));

    /* siginfo_t has the members waitid fills in */
    info.si_signo = 0;
    info.si_code = 0;
    info.si_pid = (pid_t)0;
    info.si_uid = (uid_t)0;
    info.si_status = 0;
    value.sival_int = 0;
    (void)info;
    (void)value;
    (void)idtype;
    (void)id;

    /* The status macros give integral values */
    ok &= (WIFEXITED(0) | WEXITSTATUS(0) | WIFSIGNALED(0) | WTERMSIG(0) | WIFSTOPPED(0)
           | WSTOPSIG(0) | WCOREDUMP(0) | 1)
        != 0;

    (void)sizeof(struct rlimit);
    (void)getrlimit;
    (void)setrlimit;
    (void)getpriority;
    (void)setpriority;
    (void)getrusage;
    (void)wait3;

    if (!ok) {
        printf("wrong prototype\n");
        return 1;
    }
    printf("success\n");
    return 0;
}
