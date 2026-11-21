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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "local-pwd.h"
#include "local-getent.h"

/* Split a line into the passwd fields, in place */
static bool
pw_parse(char *line, struct passwd *pw)
{
    char         *field[7];
    unsigned long uid, gid;
    size_t        n;

    /* name:passwd:uid:gid:gecos:dir:shell, ignoring anything after */
    for (n = 0; n < 7; n++)
        if ((field[n] = __getent_field(&line)) == NULL)
            return false;
    if (field[0][0] == '\0' || !__getent_id(field[2], (uid_t) ~(uid_t)0, &uid)
        || !__getent_id(field[3], (gid_t) ~(gid_t)0, &gid))
        return false;
    pw->pw_name = field[0];
    pw->pw_uid = (uid_t)uid;
    pw->pw_gid = (gid_t)gid;
    pw->pw_dir = field[5];
    pw->pw_shell = field[6];
    return true;
}

int
__pw_find(FILE *file, enum __pw_match match, const char *name, uid_t uid, struct passwd *pwbuf,
          char *buf, size_t size, struct passwd **result)
{
    int           saved_errno = errno;
    char         *line = NULL;
    size_t        cap = 0;
    ssize_t       len;
    off_t         start;
    struct passwd ent;
    int           ret;

    *result = NULL;
    for (;;) {
        start = ftello(file);
        errno = 0;
        len = getline(&line, &cap, file);
        if (len < 0) {
            if (errno == 0)
                ret = ferror(file) ? EIO : ENOENT;
            else
                ret = errno;
            break;
        }
        if (len > 0 && line[len - 1] == '\n')
            line[--len] = '\0';
        if (!pw_parse(line, &ent))
            continue;
        if (match == __pw_match_name && strcmp(ent.pw_name, name) != 0)
            continue;
        if (match == __pw_match_uid && ent.pw_uid != uid)
            continue;
        if (!buf) {
            /* Keep the line for the non-reentrant functions */
            free(__passwd_buffer);
            __passwd_buffer = line;
            line = NULL;
            *pwbuf = ent;
            *result = pwbuf;
            ret = 0;
            break;
        }
        /* Only the entry found has to fit */
        if ((size_t)len >= size) {
            /* Leave the entry to be read again with a larger buffer */
            if (start >= 0)
                (void)fseeko(file, start, SEEK_SET);
            ret = ERANGE;
            break;
        }
        memcpy(buf, line, (size_t)len + 1);
        pwbuf->pw_name = buf + (ent.pw_name - line);
        pwbuf->pw_uid = ent.pw_uid;
        pwbuf->pw_gid = ent.pw_gid;
        pwbuf->pw_dir = buf + (ent.pw_dir - line);
        pwbuf->pw_shell = buf + (ent.pw_shell - line);
        *result = pwbuf;
        ret = 0;
        break;
    }
    free(line);
    errno = saved_errno;
    return ret;
}
