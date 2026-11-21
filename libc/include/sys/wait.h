/*
Copyright (c) 1982, 1986, 1993
The Regents of the University of California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the University nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
 */
#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/cdefs.h>

_BEGIN_STD_C

#include <sys/types.h>
#include <sys/_wait.h>

pid_t wait(int *);
pid_t waitpid(pid_t, int *, int);
#if __MISC_VISIBLE
struct rusage;
pid_t wait3(int *, int, struct rusage *);
#endif

#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE >= 500
#define WIFCONTINUED(w) ((w) == 0xffff)
#endif

#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE
#include <signal.h>

/* waitid options */
#define WEXITED  8
#define WSTOPPED WUNTRACED
#define WNOWAIT  0x10

typedef enum {
    P_ALL,
    P_PID,
    P_PGID,
} idtype_t;

int waitid(idtype_t, id_t, siginfo_t *, int);
#endif

_END_STD_C

#endif
