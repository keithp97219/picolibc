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

#include "local-netdb.h"
#include <string.h>

void
__netdb_rewind(struct __netdb_db *db)
{
    db->next = 0;
    if (db->file) {
        rewind(db->file);
    } else if (!db->use_builtin) {
        db->file = fopen(db->path, "re");
        if (!db->file)
            db->use_builtin = true;
    }
    db->open = true;
}

void
__netdb_close(struct __netdb_db *db)
{
    if (db->file) {
        fclose(db->file);
        db->file = NULL;
    }
    db->use_builtin = false;
    db->open = false;
    db->stayopen = false;
}

int
__netdb_next(struct __netdb_db *db)
{
    if (!db->open)
        __netdb_rewind(db);
    for (;;) {
        char *p, *hash;
        int   n = 0;

        if (db->use_builtin) {
            if (!db->builtin[db->next])
                return 0;
            strncpy(db->line, db->builtin[db->next++], sizeof(db->line) - 1);
            db->line[sizeof(db->line) - 1] = '\0';
        } else if (!db->file || !fgets(db->line, sizeof(db->line), db->file)) {
            return 0;
        } else if (!strchr(db->line, '\n') && !feof(db->file)) {
            /* Skip a line too long to hold, rather than reading its end as another */
            int c;

            while ((c = getc(db->file)) != EOF && c != '\n')
                ;
            continue;
        }
        hash = strchr(db->line, '#');
        if (hash)
            *hash = '\0';
        p = db->line;
        while (n < __NETDB_FIELDS) {
            p += strspn(p, " \t\r\n");
            if (!*p)
                break;
            db->fields[n++] = p;
            p += strcspn(p, " \t\r\n");
            if (*p)
                *p++ = '\0';
        }
        db->fields[n] = NULL;
        if (n)
            return n;
    }
}
