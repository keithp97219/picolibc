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

#ifndef _LOCAL_PWD_H_
#define _LOCAL_PWD_H_
#include <stdio.h>
#include <pwd.h>

/* The non-reentrant functions' entry, in memory allocated to fit it */
extern char         *__passwd_buffer;
extern struct passwd __passwd_data;
extern FILE         *__passwd_file;

/* What __pw_find looks for */
enum __pw_match {
    __pw_match_any,
    __pw_match_name,
    __pw_match_uid,
};

/*
 * Read entries from file until one matches, skipping lines that aren't
 * valid entries. Returns 0 with the entry stored in buf, ENOENT at the
 * end of the file, ERANGE when the matching entry doesn't fit (leaving
 * a seekable file positioned to read it again), or another error. errno
 * is left alone. With a null buf, the entry goes in memory allocated to
 * fit it, which replaces the non-reentrant functions' buffer.
 */
int __pw_find(FILE *file, enum __pw_match match, const char *name, uid_t uid, struct passwd *pwbuf,
              char *buf, size_t size, struct passwd **result);

#endif /* _LOCAL_PWD_H_ */
