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
#include <freebsd/freebsd-resource.h>
#include <freebsd/freebsd-rlimit-struct.h>

/*
 * A stopgap until the full FreeBSD sysconf: the options and groups
 * from <sys/_posix.h>, as __sysconf_common reports them in this
 * library, the descriptor limit and MAXSYMLINKS (sys/param.h)
 * symbolic links in a path.
 */
#define FREEBSD_MAXSYMLINKS 32

long
sysconf(int name)
{
    struct __freebsd_rlimit klim;

    switch (name) {
    case _SC_OPEN_MAX:
        if (_syscall_raw(FREEBSD_SYS_getrlimit, FREEBSD_RLIMIT_NOFILE, &klim) < 0
            || klim.rlim_cur == FREEBSD_RLIM_INFINITY || klim.rlim_cur > LONG_MAX)
            return -1;
        return (long)klim.rlim_cur;
    case _SC_SYMLOOP_MAX:
        return FREEBSD_MAXSYMLINKS;
    }
    return __sysconf_common(name);
}
