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

#define _DEFAULT_SOURCE
#include <fmtmsg.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * label: severity: text
 * TO FIX: action tag
 */
static int
__fmtmsg_format(char *buf, size_t size, const char *label, int severity, const char *text,
                const char *action, const char *tag)
{
    static const char * const sevs[] = { "", "HALT", "ERROR", "WARNING", "INFO" };
    const char               *sev = severity > 0 && severity < 5 ? sevs[severity] : NULL;
    char                      sevbuf[16];
    int                       n = 0;

    if (severity >= 5) {
        snprintf(sevbuf, sizeof(sevbuf), "SEV=%d", severity);
        sev = sevbuf;
    }
    buf[0] = '\0';
#define ADD(...)                                                                          \
    do {                                                                                  \
        int _r = snprintf(buf + n, size > (size_t)n ? size - (size_t)n : 0, __VA_ARGS__); \
        if (_r > 0)                                                                       \
            n += _r;                                                                      \
    } while (0)
    if (label)
        ADD("%s", label);
    if (sev)
        ADD("%s%s", n ? ": " : "", sev);
    if (text)
        ADD("%s%s", n ? ": " : "", text);
    if (action || tag) {
        if (n)
            ADD("\n");
        if (action)
            ADD("TO FIX: %s", action);
        if (tag)
            ADD("%s%s", action ? "  " : "", tag);
    }
    ADD("\n");
#undef ADD
    return n;
}

int
fmtmsg(long classification, const char *label, int severity, const char *text, const char *action,
       const char *tag)
{
    char buf[1024];
    int  len, ret = MM_OK, failed = 0, tried = 0;

    len = __fmtmsg_format(buf, sizeof(buf), label, severity, text, action, tag);
    if (len >= (int)sizeof(buf))
        len = sizeof(buf) - 1;
    if (classification & MM_PRINT) {
        tried++;
        if (write(2, buf, (size_t)len) != len) {
            failed++;
            ret = MM_NOMSG;
        }
    }
    if (classification & MM_CONSOLE) {
        int fd = open("/dev/console", O_WRONLY | O_NOCTTY | O_CLOEXEC);

        tried++;
        if (fd < 0 || write(fd, buf, (size_t)len) != len) {
            failed++;
            ret = ret == MM_NOMSG ? MM_NOTOK : MM_NOCON;
        }
        if (fd >= 0)
            close(fd);
    }
    if (tried && failed == tried)
        return MM_NOTOK;
    return ret;
}
