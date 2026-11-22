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

#include <errno.h>
#include <mntent.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>

/* Write a field in fstab format, escaping blanks and backslashes */
static void
put_field(FILE *f, const char *s)
{
    for (; *s; s++) {
        if (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\\')
            fprintf(f, "\\%03o", (unsigned char)*s);
        else
            putc(*s, f);
    }
}

/*
 * FreeBSD keeps no /etc/mtab: the mounted file system table is the
 * kernel's, from getmntinfo. When MOUNTED is asked for and doesn't
 * exist, write the current table to a temporary file in fstab
 * format for getmntent to read; other tables are plain files.
 */
FILE *
setmntent(const char *file, const char *mode)
{
    FILE          *f = fopen(file, mode);
    struct statfs *mnt;
    int            n, i;

    if (f || errno != ENOENT || strcmp(file, MOUNTED) != 0 || mode[0] != 'r')
        return f;
    n = getmntinfo(&mnt, MNT_NOWAIT);
    if (n <= 0)
        return NULL;
    f = tmpfile();
    if (!f)
        return NULL;
    for (i = 0; i < n; i++) {
        put_field(f, mnt[i].f_mntfromname);
        putc(' ', f);
        put_field(f, mnt[i].f_mntonname);
        putc(' ', f);
        put_field(f, mnt[i].f_fstypename);
        fprintf(f, " %s%s%s 0 0\n", (mnt[i].f_flags & MNT_RDONLY) ? "ro" : "rw",
                (mnt[i].f_flags & MNT_NOSUID) ? ",nosuid" : "",
                (mnt[i].f_flags & MNT_NOEXEC) ? ",noexec" : "");
    }
    if (fflush(f) != 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    return f;
}
