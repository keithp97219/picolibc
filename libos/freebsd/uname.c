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
#include <string.h>
#include <sys/utsname.h>
#include <freebsd/freebsd-sysctl.h>

/* Read a string sysctl into a utsname field */
static int
_uname_field(int top, int what, char *field, size_t size)
{
    int    mib[2] = { top, what };
    size_t len = size - 1;

    memset(field, 0, size);
    /* A value longer than the field is truncated, with ENOMEM */
    if (_syscall_raw(FREEBSD_SYS___sysctl, mib, 2, field, &len, NULL, 0) < 0 && len == 0)
        return -1;
    field[size - 1] = '\0';
    return 0;
}

int
uname(struct utsname *name)
{
    if (_uname_field(FREEBSD_CTL_KERN, FREEBSD_KERN_OSTYPE, name->sysname, sizeof(name->sysname))
            < 0
        || _uname_field(FREEBSD_CTL_KERN, FREEBSD_KERN_HOSTNAME, name->nodename,
                        sizeof(name->nodename))
            < 0
        || _uname_field(FREEBSD_CTL_KERN, FREEBSD_KERN_OSRELEASE, name->release,
                        sizeof(name->release))
            < 0
        || _uname_field(FREEBSD_CTL_KERN, FREEBSD_KERN_VERSION, name->version,
                        sizeof(name->version))
            < 0
        || _uname_field(FREEBSD_CTL_HW, FREEBSD_HW_MACHINE, name->machine, sizeof(name->machine))
            < 0) {
        errno = EFAULT;
        return -1;
    }
    return 0;
}
