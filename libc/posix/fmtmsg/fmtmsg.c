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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MM_VERB_LABEL    0x01
#define MM_VERB_SEVERITY 0x02
#define MM_VERB_TEXT     0x04
#define MM_VERB_ACTION   0x08
#define MM_VERB_TAG      0x10
#define MM_VERB_ALL      0x1f

/*
 * The components MSGVERB selects for standard error: all of them
 * unless it is a colon-separated list of valid keywords
 */
static int
__fmtmsg_verb(void)
{
    static const char * const keywords[] = { "label", "severity", "text", "action", "tag" };
    const char               *verb = getenv("MSGVERB");
    int                       mask = 0;

    if (!verb || !*verb)
        return MM_VERB_ALL;
    for (;;) {
        const char *end = strchr(verb, ':');
        size_t      len = end ? (size_t)(end - verb) : strlen(verb);
        size_t      k;

        for (k = 0; k < sizeof(keywords) / sizeof(keywords[0]); k++)
            if (strlen(keywords[k]) == len && memcmp(keywords[k], verb, len) == 0)
                break;
        if (k == sizeof(keywords) / sizeof(keywords[0]))
            return MM_VERB_ALL;
        mask |= 1 << k;
        if (!end)
            return mask;
        verb = end + 1;
    }
}

/*
 * label: severity: text
 * TO FIX: action tag
 *
 * Returns the length of the whole message, which may exceed size
 */
static size_t
__fmtmsg_format(char *buf, size_t size, int verb, const char *label, int severity, const char *text,
                const char *action, const char *tag)
{
    static const char * const sevs[] = { "", "HALT", "ERROR", "WARNING", "INFO" };
    const char               *sev = severity > 0 && severity < 5 ? sevs[severity] : NULL;
    char                      sevbuf[16];
    size_t                    n = 0;

    if (severity >= 5) {
        snprintf(sevbuf, sizeof(sevbuf), "SEV=%d", severity);
        sev = sevbuf;
    }
    if (!(verb & MM_VERB_LABEL))
        label = NULL;
    if (!(verb & MM_VERB_SEVERITY))
        sev = NULL;
    if (!(verb & MM_VERB_TEXT))
        text = NULL;
    if (!(verb & MM_VERB_ACTION))
        action = NULL;
    if (!(verb & MM_VERB_TAG))
        tag = NULL;
    buf[0] = '\0';
#define ADD(...)                                                                              \
    do {                                                                                      \
        int _r = snprintf(buf + (n < size ? n : size), n < size ? size - n : 0, __VA_ARGS__); \
        if (_r > 0)                                                                           \
            n += (size_t)_r;                                                                  \
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

/* Writes the message; text and action may be any length */
static int
__fmtmsg_write(int fd, int verb, const char *label, int severity, const char *text,
               const char *action, const char *tag)
{
    char    local[1024], *buf = local;
    size_t  len;
    ssize_t ret;

    len = __fmtmsg_format(buf, sizeof(local), verb, label, severity, text, action, tag);
    if (len >= sizeof(local)) {
        buf = malloc(len + 1);
        if (!buf)
            return -1;
        __fmtmsg_format(buf, len + 1, verb, label, severity, text, action, tag);
    }
    ret = write(fd, buf, len);
    if (buf != local)
        free(buf);
    return ret == (ssize_t)len ? 0 : -1;
}

int
fmtmsg(long classification, const char *label, int severity, const char *text, const char *action,
       const char *tag)
{
    int ret = MM_OK, failed = 0, tried = 0;

    if (classification & MM_PRINT) {
        tried++;
        if (__fmtmsg_write(2, __fmtmsg_verb(), label, severity, text, action, tag) < 0) {
            failed++;
            ret = MM_NOMSG;
        }
    }
    if (classification & MM_CONSOLE) {
        /* The console gets every component */
        int fd = open("/dev/console", O_WRONLY | O_NOCTTY | O_CLOEXEC);

        tried++;
        if (fd < 0 || __fmtmsg_write(fd, MM_VERB_ALL, label, severity, text, action, tag) < 0) {
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
