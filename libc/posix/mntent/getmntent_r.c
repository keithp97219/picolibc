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
#include <stdlib.h>
#include <string.h>

/*
 * The next field of a line, in place: blanks separate fields, and
 * \ooo octal escapes (\040 for a space) stand for characters the
 * format can't otherwise hold
 */
static char *
next_field(char **pos)
{
    char *p = *pos, *start, *out;

    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '\0')
        return NULL;
    start = out = p;
    while (*p && *p != ' ' && *p != '\t') {
        if (p[0] == '\\' && p[1] >= '0' && p[1] <= '3' && p[2] >= '0' && p[2] <= '7' && p[3] >= '0'
            && p[3] <= '7') {
            *out++ = (char)(((p[1] - '0') << 6) | ((p[2] - '0') << 3) | (p[3] - '0'));
            p += 4;
        } else {
            *out++ = *p++;
        }
    }
    if (*p)
        p++;
    *out = '\0';
    *pos = p;
    return start;
}

struct mntent *
getmntent_r(FILE *stream, struct mntent *mnt, char *buf, int buflen)
{
    char *pos, *freq, *passno;

    if (buflen < 2)
        return NULL;
    for (;;) {
        size_t len;

        if (!fgets(buf, buflen, stream))
            return NULL;
        len = strlen(buf);
        if (len && buf[len - 1] == '\n') {
            buf[--len] = '\0';
        } else if (!feof(stream)) {
            /* Too long for the buffer: skip the rest of it */
            int c;

            while ((c = getc(stream)) != EOF && c != '\n')
                ;
            continue;
        }
        pos = buf;
        while (*pos == ' ' || *pos == '\t')
            pos++;
        if (*pos == '\0' || *pos == '#')
            continue;
        mnt->mnt_fsname = next_field(&pos);
        mnt->mnt_dir = next_field(&pos);
        mnt->mnt_type = next_field(&pos);
        mnt->mnt_opts = next_field(&pos);
        if (!mnt->mnt_dir)
            continue;
        if (!mnt->mnt_type)
            mnt->mnt_type = "";
        if (!mnt->mnt_opts)
            mnt->mnt_opts = "";
        freq = next_field(&pos);
        passno = next_field(&pos);
        mnt->mnt_freq = freq ? atoi(freq) : 0;
        mnt->mnt_passno = passno ? atoi(passno) : 0;
        return mnt;
    }
}
