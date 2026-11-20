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
#include <limits.h>
#include <sys/resource.h>
#include <unistd.h>

/* The soft limit of 'resource', or -1 when there isn't one */
static long
_sysconf_rlimit(int resource)
{
    struct rlimit rl;

    if (getrlimit(resource, &rl) < 0 || rl.rlim_cur == RLIM_INFINITY)
        return -1;
    return (long)rl.rlim_cur;
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
        return _sysconf_rlimit(RLIMIT_NOFILE);
    case _SC_ARG_MAX:
        /* The kernel allows a quarter of the stack for arguments */
        value = _sysconf_rlimit(RLIMIT_STACK);
        if (value < 0)
            return -1;
        value /= 4;
        return value < _POSIX_ARG_MAX ? _POSIX_ARG_MAX : value;
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
    case _SC_MQ_PRIO_MAX:
        /* The kernel's MQ_PRIO_MAX */
        return 32768;
    default:
        return __fallback_sysconf(name);
    }
}
