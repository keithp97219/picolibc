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

#include "local-linux.h"
#include "local-resource.h"
#include <limits.h>
#include <sys/auxv.h>
#include <unistd.h>

#define LINUX_AT_PAGESZ 6

/*
 * The kernel's own limits: MAXSYMLINKS from include/linux/namei.h,
 * and ARG_MAX and _STK_LIM, which bound exec arguments in
 * fs/exec.c
 */
#define LINUX_MAXSYMLINKS 40
#define LINUX_ARG_MAX     131072
#define LINUX_STK_LIM     (8 * 1024 * 1024)

/*
 * The soft limit of a Linux resource, or -1 when there isn't one.
 * The raw call keeps errno and makes no cancellation point; sysconf
 * is async-signal-safe.
 */
static long
_sysconf_rlimit(int resource)
{
    struct __kernel_rlimit_size klim;
    long                        ret;

#ifdef LINUX_SYS_prlimit_size
    ret = _syscall_raw(LINUX_SYS_prlimit_size, 0, resource, NULL, &klim);
#elif defined(LINUX_SYS_ugetrlimit)
    ret = _syscall_raw(LINUX_SYS_ugetrlimit, resource, &klim);
#else
    ret = _syscall_raw(LINUX_SYS_getrlimit, resource, &klim);
#endif
    if (ret < 0 || klim.rlim_cur == LINUX_RLIM_INFINITY_SIZE || klim.rlim_cur > LONG_MAX)
        return -1;
    return (long)klim.rlim_cur;
}

/* Count the CPUs in a list such as "0-3,8,10-11" */
static long
_sysconf_cpu_list(const char *name)
{
    char    buf[256];
    long    fd, len, count = 0, first = -1, cur = -1;
    ssize_t i;

    /* Read-only is 0 */
    fd = _syscall_raw(LINUX_SYS_openat, (long)(int)LINUX_AT_FDCWD, name, LINUX_O_CLOEXEC);
    if (fd < 0)
        return -1;
    len = _syscall_raw(LINUX_SYS_read, fd, buf, sizeof(buf) - 1);
    (void)_syscall_raw(LINUX_SYS_close, fd);
    if (len <= 0)
        return -1;
    buf[len] = '\0';
    for (i = 0; i <= len; i++) {
        char c = buf[i];

        if (c >= '0' && c <= '9') {
            cur = (cur < 0 ? 0 : cur * 10) + (c - '0');
        } else if (c == '-') {
            first = cur;
            cur = -1;
        } else {
            if (cur >= 0)
                count += first >= 0 && first <= cur ? cur - first + 1 : 1;
            first = cur = -1;
        }
    }
    return count > 0 ? count : -1;
}

/* The CPUs this thread may run on, which are all online */
static long
_sysconf_cpu_affinity(void)
{
    unsigned long mask[1024 / (8 * sizeof(unsigned long))];
    long          len, count = 0;
    size_t        i;

    len = _syscall_raw(LINUX_SYS_sched_getaffinity, 0, sizeof(mask), mask);
    if (len <= 0)
        return 1;
    for (i = 0; i < (size_t)len / sizeof(mask[0]); i++)
        count += __builtin_popcountl(mask[i]);
    return count > 0 ? count : 1;
}

/*
 * Values that come from the kernel; __fallback_sysconf answers the
 * rest, including every option from <unistd.h>.
 */
long
sysconf(int name)
{
    long value;

    switch (name) {
    case _SC_OPEN_MAX:
        return _sysconf_rlimit(LINUX_RLIMIT_NOFILE);
    case _SC_ARG_MAX:
        /*
         * The kernel allows a quarter of the stack for arguments, but
         * no more than three quarters of _STK_LIM and no less than
         * its ARG_MAX, whatever the stack limit
         */
        value = _sysconf_rlimit(LINUX_RLIMIT_STACK);
        if (value < 0 || value / 4 > LINUX_STK_LIM / 4 * 3)
            return LINUX_STK_LIM / 4 * 3;
        value /= 4;
        return value < LINUX_ARG_MAX ? LINUX_ARG_MAX : value;
    case _SC_CHILD_MAX:
        /* Processes per user */
        return _sysconf_rlimit(LINUX_RLIMIT_NPROC);
    case _SC_SIGQUEUE_MAX:
    case _SC_TIMER_MAX:
        /*
         * Queued signals per user; each timer takes one of them as
         * it is created
         */
        return _sysconf_rlimit(LINUX_RLIMIT_SIGPENDING);
    case _SC_MQ_OPEN_MAX:
        /* Message queue descriptors are file descriptors */
    case _SC_SEM_NSEMS_MAX:
        /* Semaphores are memory, named ones a mapped file each */
    case _SC_THREAD_THREADS_MAX:
        /* Threads count against _SC_CHILD_MAX and memory */
        return -1;
    case _SC_SYMLOOP_MAX:
        return LINUX_MAXSYMLINKS;
    case _SC_NPROCESSORS_CONF:
        value = _sysconf_cpu_list("/sys/devices/system/cpu/possible");
        return value > 0 ? value : sysconf(_SC_NPROCESSORS_ONLN);
    case _SC_NPROCESSORS_ONLN:
        value = _sysconf_cpu_list("/sys/devices/system/cpu/online");
        return value > 0 ? value : _sysconf_cpu_affinity();
    case _SC_IOV_MAX:
        /* The kernel's UIO_MAXIOV */
        return IOV_MAX;
    case _SC_CLK_TCK:
        /* The kernel's USER_HZ, which times() counts in */
        return 100;
    case _SC_NGROUPS_MAX:
        return 65536;
    case _SC_DELAYTIMER_MAX:
        /* The kernel's timer overrun counts saturate at INT_MAX */
        return INT_MAX;
    case _SC_LOGIN_NAME_MAX:
        /*
         * Linux names users with up to 32 characters (utmp's ut_user),
         * or more where useradd allows; glibc's LOGIN_NAME_MAX is 256
         */
        return 256;
    case _SC_TTY_NAME_MAX:
        /* Room for the /dev/pts/N and /dev/ttyXXX names ttyname returns */
        return 32;
    case _SC_MQ_PRIO_MAX:
        /* The kernel's MQ_PRIO_MAX */
        return 32768;
    case _SC_PAGESIZE:
        /* Pages need not be 4096 bytes; the kernel says how big they are */
        value = (long)getauxval(LINUX_AT_PAGESZ);
        return value > 0 ? value : 4096;
    default:
        return __fallback_sysconf(name);
    }
}
