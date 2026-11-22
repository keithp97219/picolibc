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

#define _GNU_SOURCE
#include <error.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void         (*error_print_progname)(void);
unsigned int error_message_count;
int          error_one_per_line;

/* "program:" and, for error, a space; or whatever error_print_progname prints */
static void
error_prefix(const char *sep)
{
    fflush(stdout);
    if (error_print_progname)
        (*error_print_progname)();
    else if (program_invocation_name)
        fprintf(stderr, "%s:%s", program_invocation_name, sep);
}

static void
error_finish(int status, int errnum, const char *format, va_list ap)
{
    vfprintf(stderr, format, ap);
    error_message_count++;
    if (errnum)
        fprintf(stderr, ": %s", strerror(errnum));
    fputc('\n', stderr);
    fflush(stderr);
    if (status)
        exit(status);
}

void
error(int status, int errnum, const char *format, ...)
{
    va_list ap;

    error_prefix(" ");
    va_start(ap, format);
    error_finish(status, errnum, format, ap);
    va_end(ap);
}

void
error_at_line(int status, int errnum, const char *filename, unsigned int linenum,
              const char *format, ...)
{
    static const char  *last_filename;
    static unsigned int last_linenum;
    va_list             ap;

    if (error_one_per_line) {
        /* The same place as the last message: print nothing */
        if (linenum == last_linenum
            && (filename == last_filename
                || (filename && last_filename && strcmp(filename, last_filename) == 0)))
            return;
        last_filename = filename;
        last_linenum = linenum;
    }
    error_prefix("");
    if (filename)
        fprintf(stderr, "%s:%u: ", filename, linenum);
    else
        fputc(' ', stderr);
    va_start(ap, format);
    error_finish(status, errnum, format, ap);
    va_end(ap);
}
