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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "local-grp.h"
#include "../pwd/local-getent.h"

/*
 * Split a line into the group name, the gid and the member list, in
 * place. A missing member field means no members.
 */
static bool
gr_parse(char *line, struct group *gr, char **members)
{
    char         *field[3];
    unsigned long gid;
    size_t        n;

    /* name:passwd:gid:members, ignoring anything after */
    for (n = 0; n < 3; n++)
        if ((field[n] = __getent_field(&line)) == NULL)
            return false;
    *members = __getent_field(&line);
    if (field[0][0] == '\0' || !__getent_id(field[2], (gid_t) ~(gid_t)0, &gid))
        return false;
    gr->gr_name = field[0];
    gr->gr_gid = (gid_t)gid;
    return true;
}

int
__gr_find(FILE *file, enum __gr_match match, const char *name, gid_t gid, struct group *grbuf,
          char *buf, size_t size, struct group **result)
{
    int          saved_errno = errno;
    char        *line = NULL;
    size_t       cap = 0;
    ssize_t      len;
    off_t        start;
    struct group ent;
    char        *members, *m, **mem;
    size_t       nmem, array, need;
    int          ret;

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
        if (!gr_parse(line, &ent, &members))
            continue;
        if (match == __gr_match_name && strcmp(ent.gr_name, name) != 0)
            continue;
        if (match == __gr_match_gid && ent.gr_gid != gid)
            continue;

        /* An empty member field means no members, not one with an empty name */
        nmem = 0;
        if (members && *members) {
            nmem = 1;
            for (m = members; *m; m++)
                if (*m == ',')
                    nmem++;
        }

        /* The line, then the aligned, null-terminated array of members */
        array = (size_t)len + 1;
        array += (sizeof(char *) - ((uintptr_t)buf + array) % sizeof(char *)) % sizeof(char *);
        need = array + (nmem + 1) * sizeof(char *);

        /* Only the entry found has to fit */
        if (need > size) {
            /* Leave the entry to be read again with a larger buffer */
            if (start >= 0)
                (void)fseeko(file, start, SEEK_SET);
            ret = ERANGE;
            break;
        }
        memcpy(buf, line, (size_t)len + 1);
        grbuf->gr_name = buf + (ent.gr_name - line);
        grbuf->gr_gid = ent.gr_gid;
        mem = (char **)(void *)(buf + array);
        grbuf->gr_mem = mem;
        if (nmem) {
            for (m = buf + (members - line);;) {
                *mem++ = m;
                m = strchr(m, ',');
                if (!m)
                    break;
                *m++ = '\0';
            }
        }
        *mem = NULL;
        *result = grbuf;
        ret = 0;
        break;
    }
    free(line);
    errno = saved_errno;
    return ret;
}
