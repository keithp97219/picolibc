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

#include "local-signal.h"
#include <string.h>

/* Translate the si_code values picolibc defines */
static int
_si_code_from_freebsd(int ksig, int kcode)
{
    switch (kcode) {
    case FREEBSD_SI_USER:
        return SI_USER;
    case FREEBSD_SI_QUEUE:
        return SI_QUEUE;
    case FREEBSD_SI_TIMER:
        return SI_TIMER;
    case FREEBSD_SI_ASYNCIO:
        return SI_ASYNCIO;
    case FREEBSD_SI_MESGQ:
        return SI_MESGQ;
    }
    switch (ksig) {
    case FREEBSD_SIGCHLD:
        switch (kcode) {
        case FREEBSD_CLD_EXITED:
            return CLD_EXITED;
        case FREEBSD_CLD_KILLED:
            return CLD_KILLED;
        case FREEBSD_CLD_DUMPED:
            return CLD_DUMPED;
        case FREEBSD_CLD_TRAPPED:
            return CLD_TRAPPED;
        case FREEBSD_CLD_STOPPED:
            return CLD_STOPPED;
        case FREEBSD_CLD_CONTINUED:
            return CLD_CONTINUED;
        }
        break;
    case FREEBSD_SIGSEGV:
        switch (kcode) {
        case FREEBSD_SEGV_MAPERR:
            return SEGV_MAPERR;
        case FREEBSD_SEGV_ACCERR:
            return SEGV_ACCERR;
        }
        break;
    case FREEBSD_SIGBUS:
        switch (kcode) {
        case FREEBSD_BUS_ADRALN:
            return BUS_ADRALN;
        case FREEBSD_BUS_ADRERR:
            return BUS_ADRERR;
        case FREEBSD_BUS_OBJERR:
            return BUS_OBJERR;
        }
        break;
    case FREEBSD_SIGILL:
        switch (kcode) {
        case FREEBSD_ILL_ILLOPC:
            return ILL_ILLOPC;
        case FREEBSD_ILL_ILLOPN:
            return ILL_ILLOPN;
        case FREEBSD_ILL_ILLADR:
            return ILL_ILLADR;
        case FREEBSD_ILL_ILLTRP:
            return ILL_ILLTRP;
        case FREEBSD_ILL_PRVOPC:
            return ILL_PRVOPC;
        case FREEBSD_ILL_PRVREG:
            return ILL_PRVREG;
        case FREEBSD_ILL_COPROC:
            return ILL_COPROC;
        case FREEBSD_ILL_BADSTK:
            return ILL_BADSTK;
        }
        break;
    case FREEBSD_SIGFPE:
        switch (kcode) {
        case FREEBSD_FPE_INTDIV:
            return FPE_INTDIV;
        case FREEBSD_FPE_INTOVF:
            return FPE_INTOVF;
        case FREEBSD_FPE_FLTDIV:
            return FPE_FLTDIV;
        case FREEBSD_FPE_FLTOVF:
            return FPE_FLTOVF;
        case FREEBSD_FPE_FLTUND:
            return FPE_FLTUND;
        case FREEBSD_FPE_FLTRES:
            return FPE_FLTRES;
        case FREEBSD_FPE_FLTINV:
            return FPE_FLTINV;
        case FREEBSD_FPE_FLTSUB:
            return FPE_FLTSUB;
        }
        break;
    case FREEBSD_SIGTRAP:
        switch (kcode) {
        case FREEBSD_TRAP_BRKPT:
            return TRAP_BRKPT;
        case FREEBSD_TRAP_TRACE:
            return TRAP_TRACE;
        }
        break;
    }
    return kcode;
}

void
_siginfo_from_freebsd(siginfo_t *info, const struct __freebsd_siginfo *kinfo)
{
    memset(info, 0, sizeof(*info));
    info->si_signo = _signal_from_freebsd(kinfo->si_signo);
    info->si_code = _si_code_from_freebsd(kinfo->si_signo, kinfo->si_code);
    info->si_errno = kinfo->si_errno ? _syscall_errno(-(long)kinfo->si_errno) : 0;
    info->si_pid = kinfo->si_pid;
    info->si_uid = kinfo->si_uid;
    info->si_addr = kinfo->si_addr;
    info->si_status = kinfo->si_status;
    /* A child stopped or killed by a signal reports the signal number */
    if (kinfo->si_signo == FREEBSD_SIGCHLD && kinfo->si_code != FREEBSD_CLD_EXITED
        && kinfo->si_code > 0 && kinfo->si_code <= FREEBSD_CLD_CONTINUED)
        info->si_status = _signal_from_freebsd(kinfo->si_status);
    info->si_band = kinfo->si_band;
    memcpy(&info->si_value, &kinfo->si_value, sizeof(info->si_value));
}
