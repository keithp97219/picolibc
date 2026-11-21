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

#ifndef _SYSLOG_H_
#define _SYSLOG_H_

#include <sys/cdefs.h>
#if __BSD_VISIBLE || __GNU_VISIBLE
/* Only vsyslog needs va_list; <syslog.h> may not define the va_ names */
#include <stdarg.h>
#endif

_BEGIN_STD_C

/* openlog options */
#define LOG_PID    0x01 /* Log the process ID with each message */
#define LOG_CONS   0x02 /* Log to the console if the message can't be sent */
#define LOG_ODELAY 0x04 /* Delay opening until syslog() is called */
#define LOG_NDELAY 0x08 /* Open the connection immediately */
#define LOG_NOWAIT 0x10 /* Don't wait for child processes */
#if __BSD_VISIBLE || __GNU_VISIBLE
#define LOG_PERROR 0x20 /* Log to standard error as well */
#endif

/* Facilities */
#define LOG_KERN   (0 << 3)
#define LOG_USER   (1 << 3)
#define LOG_MAIL   (2 << 3)
#define LOG_DAEMON (3 << 3)
#define LOG_AUTH   (4 << 3)
#define LOG_LPR    (6 << 3)
#define LOG_NEWS   (7 << 3)
#define LOG_UUCP   (8 << 3)
#define LOG_CRON   (9 << 3)
#define LOG_LOCAL0 (16 << 3)
#define LOG_LOCAL1 (17 << 3)
#define LOG_LOCAL2 (18 << 3)
#define LOG_LOCAL3 (19 << 3)
#define LOG_LOCAL4 (20 << 3)
#define LOG_LOCAL5 (21 << 3)
#define LOG_LOCAL6 (22 << 3)
#define LOG_LOCAL7 (23 << 3)

/* Priorities */
#define LOG_EMERG     0
#define LOG_ALERT     1
#define LOG_CRIT      2
#define LOG_ERR       3
#define LOG_WARNING   4
#define LOG_NOTICE    5
#define LOG_INFO      6
#define LOG_DEBUG     7

#define LOG_MASK(pri) (1 << (pri))
#define LOG_UPTO(pri) ((1 << ((pri) + 1)) - 1)
#if __BSD_VISIBLE || __GNU_VISIBLE
#define LOG_PRIMASK 0x07
#define LOG_FACMASK 0x03f8
#define LOG_PRI(p)  ((p) & LOG_PRIMASK)
#define LOG_FAC(p)  (((p) & LOG_FACMASK) >> 3)
#endif

void closelog(void) __picolibc_export;
void openlog(const char *, int, int) __picolibc_export;
int  setlogmask(int) __picolibc_export;
void syslog(int, const char *, ...) __picolibc_export __picolibc_format(__printf__, 2, 3);
#if __BSD_VISIBLE || __GNU_VISIBLE
void vsyslog(int, const char *, va_list) __picolibc_export __picolibc_format(__printf__, 2, 0);
#endif

_END_STD_C

#endif /* _SYSLOG_H_ */
