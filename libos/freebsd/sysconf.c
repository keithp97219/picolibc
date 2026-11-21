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

#include "local-freebsd.h"
#include "../fallback/local-sysconf.h"
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <freebsd/freebsd-resource.h>
#include <freebsd/freebsd-rlimit-struct.h>
#include <freebsd/freebsd-sysctl.h>

/*
 * The kernel's own limits: MAXSYMLINKS from sys/param.h, MAXLOGNAME
 * (with the terminating null) from sys/param.h, MAXHOSTNAMELEN - 1
 * from sys/param.h, and MQ_PRIO_MAX from kern/uipc_mqueue.c
 */
#define FREEBSD_MAXSYMLINKS   32
#define FREEBSD_MAXLOGNAME    33
#define FREEBSD_HOST_NAME_MAX 255
#define FREEBSD_MQ_PRIO_MAX   64
/* FreeBSD's devfs names are at most SPECNAMELEN (255) bytes */
#define FREEBSD_TTY_NAME_MAX (5 + 255 + 1)

/*
 * The soft limit of a FreeBSD resource, or -1 when there isn't one.
 * The raw calls keep errno; sysconf is async-signal-safe.
 */
static long
_sysconf_rlimit(int resource)
{
    struct __freebsd_rlimit klim;

    if (_syscall_raw(FREEBSD_SYS_getrlimit, resource, &klim) < 0
        || klim.rlim_cur == FREEBSD_RLIM_INFINITY || klim.rlim_cur > LONG_MAX)
        return -1;
    return (long)klim.rlim_cur;
}

/* An integer sysctl by MIB, or -1 */
static long
_sysconf_sysctl(int top, int what)
{
    int    mib[2] = { top, what };
    int    value = 0;
    size_t len = sizeof(value);

    if (_syscall_raw(FREEBSD_SYS___sysctl, mib, 2, &value, &len, NULL, 0) < 0
        || len != sizeof(value))
        return -1;
    return value;
}

/* An integer sysctl by name, or -1 */
static long
_sysconf_sysctlbyname(const char *name)
{
    int    value = 0;
    size_t len = sizeof(value);

    if (_syscall_raw(FREEBSD_SYS___sysctlbyname, name, strlen(name), &value, &len, NULL, 0) < 0
        || len != sizeof(value))
        return -1;
    return value;
}

/*
 * Values that come from the kernel; __sysconf_common answers the
 * rest, including every option from <unistd.h>.
 */
long
sysconf(int name)
{
    long value;

    switch (name) {
    case _SC_OPEN_MAX:
        return _sysconf_rlimit(FREEBSD_RLIMIT_NOFILE);
    case _SC_ARG_MAX:
        /* The bytes of arguments and environment exec accepts */
        value = _sysconf_sysctlbyname("kern.argmax");
        return value > 0 ? value : ARG_MAX;
    case _SC_CHILD_MAX:
        /* Processes per user */
        return _sysconf_rlimit(FREEBSD_RLIMIT_NPROC);
    case _SC_SIGQUEUE_MAX:
        /* Signals queued per process */
        value = _sysconf_sysctlbyname("p1003_1b.sigqueue_max");
        return value > 0 ? value : _POSIX_SIGQUEUE_MAX;
    case _SC_TIMER_MAX:
        /* Timers per process */
        value = _sysconf_sysctlbyname("p1003_1b.timer_max");
        return value > 0 ? value : _POSIX_TIMER_MAX;
    case _SC_DELAYTIMER_MAX:
        /* The kernel's timer overrun counts saturate at INT_MAX */
        value = _sysconf_sysctlbyname("p1003_1b.delaytimer_max");
        return value > 0 ? value : INT_MAX;
    case _SC_NGROUPS_MAX:
        /* Supplementary groups per process */
        value = _sysconf_sysctlbyname("kern.ngroups");
        return value > 0 ? value : NGROUPS_MAX;
    case _SC_MQ_OPEN_MAX:
        /* Message queue descriptors are file descriptors */
    case _SC_SEM_NSEMS_MAX:
        /* Semaphores are memory, named ones a mapped file each */
    case _SC_THREAD_THREADS_MAX:
        /* Threads count against memory and kern.threads.max_threads_per_proc */
        return -1;
    case _SC_SYMLOOP_MAX:
        return FREEBSD_MAXSYMLINKS;
    case _SC_NPROCESSORS_CONF:
    case _SC_NPROCESSORS_ONLN:
        value = _sysconf_sysctl(FREEBSD_CTL_HW, FREEBSD_HW_NCPU);
        return value > 0 ? value : 1;
    case _SC_IOV_MAX:
        /* The kernel's UIO_MAXIOV */
        return IOV_MAX;
    case _SC_LOGIN_NAME_MAX:
        return FREEBSD_MAXLOGNAME;
    case _SC_HOST_NAME_MAX:
        return FREEBSD_HOST_NAME_MAX;
    case _SC_TTY_NAME_MAX:
        /* Room for the /dev/ names ttyname returns */
        return FREEBSD_TTY_NAME_MAX;
    case _SC_MQ_PRIO_MAX:
        return FREEBSD_MQ_PRIO_MAX;
    case _SC_PAGESIZE:
        value = _sysconf_sysctl(FREEBSD_CTL_HW, FREEBSD_HW_PAGESIZE);
        return value > 0 ? value : 4096;
    default:
        /* _SC_CLK_TCK is CLOCKS_PER_SEC, which times() counts in */
        return __sysconf_common(name);
    }
}
