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
/* nice, the setre and the res ID functions are XSI interfaces */
#define _XOPEN_SOURCE 800
/* glibc declares _Fork, getresuid, getresgid, setresuid and setresgid only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <unistd.h>

#include <stdio.h>

/*
 * <unistd.h> declares the process functions with the types POSIX gives
 * them, and with XSI no interfaces that Issue 8 removed
 */

#ifndef TEST_GLIBC
/* setpgrp was removed in Issue 8, ualarm, usleep and vfork in Issue 7 */
static int setpgrp;
static int ualarm;
static int usleep;
static int vfork;
#endif

/* The ID types: pid_t is signed */
_Static_assert((pid_t)-1 < 0, "pid_t is signed");
_Static_assert(sizeof(uid_t) > 0 && sizeof(gid_t) > 0, "uid_t and gid_t");

/* Each declaration has exactly the type POSIX gives */
#define check_type(f, type)                                                    \
    _Static_assert(_Generic(&(f), type: 1, default: 0), #f " has type " #type)

check_type(alarm, unsigned (*)(unsigned));
check_type(_exit, void (*)(int));
check_type(execl, int (*)(const char *, const char *, ...));
check_type(execle, int (*)(const char *, const char *, ...));
check_type(execlp, int (*)(const char *, const char *, ...));
check_type(execv, int (*)(const char *, char * const[]));
check_type(execve, int (*)(const char *, char * const[], char * const[]));
check_type(execvp, int (*)(const char *, char * const[]));
check_type(fexecve, int (*)(int, char * const[], char * const[]));
check_type(_Fork, pid_t (*)(void));
check_type(fork, pid_t (*)(void));
check_type(getegid, gid_t (*)(void));
check_type(geteuid, uid_t (*)(void));
check_type(getgid, gid_t (*)(void));
check_type(getgroups, int (*)(int, gid_t[]));
check_type(getlogin, char *(*)(void));
check_type(getlogin_r, int (*)(char *, size_t));
check_type(getpgid, pid_t (*)(pid_t));
check_type(getpgrp, pid_t (*)(void));
check_type(getpid, pid_t (*)(void));
check_type(getppid, pid_t (*)(void));
check_type(getresgid, int (*)(gid_t * restrict, gid_t * restrict, gid_t * restrict));
check_type(getresuid, int (*)(uid_t * restrict, uid_t * restrict, uid_t * restrict));
check_type(getsid, pid_t (*)(pid_t));
check_type(getuid, uid_t (*)(void));
check_type(nice, int (*)(int));
check_type(pause, int (*)(void));
check_type(setegid, int (*)(gid_t));
check_type(seteuid, int (*)(uid_t));
check_type(setgid, int (*)(gid_t));
check_type(setpgid, int (*)(pid_t, pid_t));
check_type(setregid, int (*)(gid_t, gid_t));
check_type(setresgid, int (*)(gid_t, gid_t, gid_t));
check_type(setresuid, int (*)(uid_t, uid_t, uid_t));
check_type(setreuid, int (*)(uid_t, uid_t));
check_type(setsid, pid_t (*)(void));
check_type(setuid, int (*)(uid_t));
check_type(sleep, unsigned (*)(unsigned));
check_type(tcgetpgrp, pid_t (*)(int));
check_type(tcsetpgrp, int (*)(int, pid_t));

int
main(void)
{
#ifndef TEST_GLIBC
    (void)setpgrp;
    (void)ualarm;
    (void)usleep;
    (void)vfork;
#endif
    printf("success\n");
    return 0;
}
