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

#ifndef _FREEBSD_SIGNAL_H_
#define _FREEBSD_SIGNAL_H_

#define FREEBSD_BUS_ADRALN    1
#define FREEBSD_BUS_ADRERR    2
#define FREEBSD_BUS_OBJERR    3
#define FREEBSD_BUS_OOMERR    100
#define FREEBSD_CLD_CONTINUED 6
#define FREEBSD_CLD_DUMPED    3
#define FREEBSD_CLD_EXITED    1
#define FREEBSD_CLD_KILLED    2
#define FREEBSD_CLD_STOPPED   5
#define FREEBSD_CLD_TRAPPED   4
#define FREEBSD_FPE_FLTDIV    3
#define FREEBSD_FPE_FLTIDO    9
#define FREEBSD_FPE_FLTINV    7
#define FREEBSD_FPE_FLTOVF    4
#define FREEBSD_FPE_FLTRES    6
#define FREEBSD_FPE_FLTSUB    8
#define FREEBSD_FPE_FLTUND    5
#define FREEBSD_FPE_INTDIV    2
#define FREEBSD_FPE_INTOVF    1
#define FREEBSD_ILL_BADSTK    8
#define FREEBSD_ILL_COPROC    7
#define FREEBSD_ILL_ILLADR    3
#define FREEBSD_ILL_ILLOPC    1
#define FREEBSD_ILL_ILLOPN    2
#define FREEBSD_ILL_ILLTRP    4
#define FREEBSD_ILL_PRVOPC    5
#define FREEBSD_ILL_PRVREG    6
#define FREEBSD_MINSIGSTKSZ   2048
#define FREEBSD_SA_NOCLDSTOP  8
#define FREEBSD_SA_NOCLDWAIT  32
#define FREEBSD_SA_NODEFER    16
#define FREEBSD_SA_ONSTACK    1
#define FREEBSD_SA_RESETHAND  4
#define FREEBSD_SA_RESTART    2
#define FREEBSD_SA_SIGINFO    64
#define FREEBSD_SEGV_ACCERR   2
#define FREEBSD_SEGV_MAPERR   1
#define FREEBSD_SEGV_PKUERR   100
#define FREEBSD_SIGABRT       6
#define FREEBSD_SIGALRM       14
#define FREEBSD_SIGBUS        10
#define FREEBSD_SIGCHLD       20
#define FREEBSD_SIGCONT       19
#define FREEBSD_SIGEMT        7
#define FREEBSD_SIGEV_KEVENT  3
#define FREEBSD_SIGEV_NONE    0
#define FREEBSD_SIGEV_SIGNAL  1
#define FREEBSD_SIGEV_THREAD  2
#define FREEBSD_SIGFPE        8
#define FREEBSD_SIGHUP        1
#define FREEBSD_SIGILL        4
#define FREEBSD_SIGINFO       29
#define FREEBSD_SIGINT        2
#define FREEBSD_SIGIO         23
#define FREEBSD_SIGIOT        6
#define FREEBSD_SIGKILL       9
#define FREEBSD_SIGLIBRT      33
#define FREEBSD_SIGLWP        32
#define FREEBSD_SIGPIPE       13
#define FREEBSD_SIGPROF       27
#define FREEBSD_SIGQUIT       3
#define FREEBSD_SIGRTMAX      126
#define FREEBSD_SIGRTMIN      65
#define FREEBSD_SIGSEGV       11
#define FREEBSD_SIGSTOP       17
#define FREEBSD_SIGSYS        12
#define FREEBSD_SIGTERM       15
#define FREEBSD_SIGTHR        32
#define FREEBSD_SIGTRAP       5
#define FREEBSD_SIGTSTP       18
#define FREEBSD_SIGTTIN       21
#define FREEBSD_SIGTTOU       22
#define FREEBSD_SIGURG        16
#define FREEBSD_SIGUSR1       30
#define FREEBSD_SIGUSR2       31
#define FREEBSD_SIGVTALRM     26
#define FREEBSD_SIGWINCH      28
#define FREEBSD_SIGXCPU       24
#define FREEBSD_SIGXFSZ       25
#define FREEBSD_SIG_BLOCK     1
#define FREEBSD_SIG_SETMASK   3
#define FREEBSD_SIG_UNBLOCK   2
#define FREEBSD_SI_ASYNCIO    65540
#define FREEBSD_SI_KERNEL     65542
#define FREEBSD_SI_LWP        65543
#define FREEBSD_SI_MESGQ      65541
#define FREEBSD_SI_NOINFO     0
#define FREEBSD_SI_QUEUE      65538
#define FREEBSD_SI_TIMER      65539
#define FREEBSD_SI_UNDEFINED  0
#define FREEBSD_SI_USER       65537
#define FREEBSD_TRAP_BRKPT    1
#define FREEBSD_TRAP_CAP      4
#define FREEBSD_TRAP_DTRACE   3
#define FREEBSD_TRAP_TRACE    2

#endif /* _FREEBSD_SIGNAL_H_ */
