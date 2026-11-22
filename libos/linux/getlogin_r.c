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
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>

/*
 * The login name is the session's, not the process's uid: Linux
 * records the uid that logged in as /proc/self/loginuid, (uid_t)-1
 * when there was no login (daemons, containers, user namespaces),
 * as glibc reads it. Without /proc, fall back to the real uid.
 */
static int
_loginuid(uid_t *uid)
{
    char          buf[16];
    long          fd, len, i;
    unsigned long v = 0;

    fd = _syscall_raw(LINUX_SYS_openat, (long)(int)LINUX_AT_FDCWD, "/proc/self/loginuid",
                      LINUX_O_CLOEXEC);
    if (fd < 0)
        return -1;
    len = _syscall_raw(LINUX_SYS_read, fd, buf, sizeof(buf) - 1);
    (void)_syscall_raw(LINUX_SYS_close, fd);
    if (len <= 0)
        return -1;
    for (i = 0; i < len && buf[i] >= '0' && buf[i] <= '9'; i++)
        v = v * 10 + (unsigned long)(buf[i] - '0');
    if (i == 0)
        return -1;
    *uid = (uid_t)v;
    return 0;
}

int
getlogin_r(char *name, size_t namesize)
{
    struct passwd pwd, *result;
    char          buf[1024];
    size_t        len;
    uid_t         uid;
    int           ret;

    if (_loginuid(&uid) < 0)
        uid = getuid();
    else if (uid == (uid_t)-1)
        return ENXIO;
    ret = getpwuid_r(uid, &pwd, buf, sizeof(buf), &result);
    if (ret)
        return ret;
    if (!result)
        return ENXIO;
    len = strlen(pwd.pw_name);
    if (len >= namesize)
        return ERANGE;
    memcpy(name, pwd.pw_name, len + 1);
    return 0;
}
