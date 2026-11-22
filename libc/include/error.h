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

#ifndef _ERROR_H_
#define _ERROR_H_

#include <sys/cdefs.h>

_BEGIN_STD_C

/*
 * GNU error reporting: "program: message[: strerror(errnum)]" on
 * stderr, after flushing stdout, then exit(status) when status is
 * not zero
 */
void error(int __status, int __errnum, const char *__format, ...)
    __picolibc_format(__printf__, 3, 4) __picolibc_export;
void error_at_line(int __status, int __errnum, const char *__filename, unsigned int __linenum,
                   const char *__format, ...) __picolibc_format(__printf__, 5, 6) __picolibc_export;

/* Called in place of printing "program: " when not NULL */
extern __picolibc_export void         (*error_print_progname)(void);

/* How many messages error and error_at_line have printed */
extern __picolibc_export unsigned int error_message_count;

/* Non-zero: error_at_line skips a message for the same place as the last */
extern __picolibc_export int          error_one_per_line;

_END_STD_C

#endif /* _ERROR_H_ */
