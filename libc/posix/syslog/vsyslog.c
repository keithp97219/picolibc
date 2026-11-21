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

#include "local-syslog.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __PICOLIBC_NETWORK
#include <sys/socket.h>
#include <sys/un.h>
#endif

const char *__syslog_ident;
int         __syslog_option;
int         __syslog_facility = LOG_USER;
int         __syslog_mask = 0xff;
int         __syslog_fd = -1;

int
__syslog_connect(void)
{
#ifdef __PICOLIBC_NETWORK
    static const struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = "/dev/log" };
    int                             fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);

    if (fd >= 0 && connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        fd = -1;
    }
    return fd;
#else
    return -1;
#endif
}

/* Copies fmt, replacing %m with the message for errno */
static void
__syslog_expand(char *out, size_t size, const char *fmt, int error)
{
    const char *msg = strerror(error);
    size_t      n = 0;

    while (*fmt && n + 1 < size) {
        if (fmt[0] == '%' && fmt[1] == '%') {
            if (n + 2 >= size)
                break;
            out[n++] = *fmt++;
            out[n++] = *fmt++;
        } else if (fmt[0] == '%' && fmt[1] == 'm') {
            /* Doubling any % keeps the message from being a format */
            const char *m;

            for (m = msg; *m && n + 2 < size; m++) {
                if (*m == '%')
                    out[n++] = '%';
                out[n++] = *m;
            }
            fmt += 2;
        } else {
            out[n++] = *fmt++;
        }
    }
    out[n] = '\0';
}

/* The length after adding n bytes at len, kept within the buffer */
static int
__syslog_append(size_t size, int len, int n)
{
    if (n < 0)
        return len;
    if ((size_t)n >= size - (size_t)len)
        return (int)size - 1;
    return len + n;
}

void
vsyslog(int priority, const char *fmt, va_list ap)
{
    char      format[512], buf[1024], stamp[32];
    int       error = errno, len, head, sent = 0, cancel;
    time_t    now;
    struct tm tm;

    if (!(__syslog_mask & LOG_MASK(priority & LOG_PRIMASK)))
        return;
    if (!(priority & LOG_FACMASK))
        priority |= __syslog_facility;

    now = time(NULL);
    localtime_r(&now, &tm);
    strftime(stamp, sizeof(stamp), "%b %e %H:%M:%S", &tm);
    __syslog_expand(format, sizeof(format), fmt, error);

    cancel = __syslog_cancel_disable();
    __LIBC_LOCK();
    head = __syslog_append(sizeof(buf), 0, snprintf(buf, sizeof(buf), "<%d>%s ", priority, stamp));
    len = head;
    if (__syslog_ident)
        len = __syslog_append(sizeof(buf), len,
                              snprintf(buf + len, sizeof(buf) - (size_t)len, "%s", __syslog_ident));
    if (__syslog_option & LOG_PID)
        len = __syslog_append(
            sizeof(buf), len,
            snprintf(buf + len, sizeof(buf) - (size_t)len, "[%d]", (int)getpid()));
    if (__syslog_ident || (__syslog_option & LOG_PID))
        len = __syslog_append(sizeof(buf), len,
                              snprintf(buf + len, sizeof(buf) - (size_t)len, ": "));
    len = __syslog_append(sizeof(buf), len,
                          vsnprintf(buf + len, sizeof(buf) - (size_t)len, format, ap));

    if (__syslog_fd < 0)
        __syslog_fd = __syslog_connect();
#ifdef __PICOLIBC_NETWORK
    if (__syslog_fd >= 0) {
        if (send(__syslog_fd, buf, (size_t)len, MSG_NOSIGNAL) == len) {
            sent = 1;
        } else {
            /* The logger may have restarted; try once more */
            close(__syslog_fd);
            __syslog_fd = __syslog_connect();
            if (__syslog_fd >= 0 && send(__syslog_fd, buf, (size_t)len, MSG_NOSIGNAL) == len)
                sent = 1;
        }
    }
#endif
    if (!sent && (__syslog_option & LOG_CONS)) {
        int fd = open("/dev/console", O_WRONLY | O_NOCTTY | O_CLOEXEC);

        if (fd >= 0) {
            (void)!write(fd, buf + head, (size_t)(len - head));
            (void)!write(fd, "\r\n", 2);
            close(fd);
        }
    }
    if (__syslog_option & LOG_PERROR) {
        (void)!write(2, buf + head, (size_t)(len - head));
        (void)!write(2, "\n", 1);
    }
    __LIBC_UNLOCK();
    __syslog_cancel_restore(cancel);
    errno = error;
}
