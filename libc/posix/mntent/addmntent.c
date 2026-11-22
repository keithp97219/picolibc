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

#include <mntent.h>

/* Write a field, escaping what would end it */
static int
put_field(FILE *stream, const char *s)
{
    for (; *s; s++) {
        int r;

        if (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\\')
            r = fprintf(stream, "\\%03o", (unsigned char)*s);
        else
            r = putc(*s, stream);
        if (r < 0)
            return -1;
    }
    return 0;
}

/* Append an entry; 0 on success, 1 on failure */
int
addmntent(FILE *stream, const struct mntent *mnt)
{
    if (fseek(stream, 0, SEEK_END) < 0)
        return 1;
    if (put_field(stream, mnt->mnt_fsname) < 0 || putc(' ', stream) < 0
        || put_field(stream, mnt->mnt_dir) < 0 || putc(' ', stream) < 0
        || put_field(stream, mnt->mnt_type) < 0 || putc(' ', stream) < 0
        || put_field(stream, mnt->mnt_opts) < 0
        || fprintf(stream, " %d %d\n", mnt->mnt_freq, mnt->mnt_passno) < 0)
        return 1;
    return fflush(stream) != 0;
}
