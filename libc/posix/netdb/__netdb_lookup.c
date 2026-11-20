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
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int
__netdb_service_port(const char *name, const char *proto)
{
    struct __netdb_db db = { .path = "/etc/services", .builtin = __services_builtin };
    int               n, port = -1;

    while (port < 0 && (n = __netdb_next(&db)) != 0) {
        char *slash, *end;
        long  p;
        int   i;

        if (n < 2 || !(slash = strchr(db.fields[1], '/')))
            continue;
        *slash = '\0';
        if (proto && strcmp(slash + 1, proto) != 0)
            continue;
        for (i = 0; i < n; i++)
            if (i != 1 && strcmp(db.fields[i], name) == 0)
                break;
        if (i == n)
            continue;
        p = strtol(db.fields[1], &end, 10);
        if (!*end && p >= 0 && p <= 65535)
            port = (int)p;
    }
    __netdb_close(&db);
    return port;
}

bool
__netdb_service_name(int port, const char *proto, char *buf, size_t len)
{
    struct __netdb_db db = { .path = "/etc/services", .builtin = __services_builtin };
    bool              found = false;
    int               n;

    while (!found && (n = __netdb_next(&db)) != 0) {
        char *slash, *end;

        if (n < 2 || !(slash = strchr(db.fields[1], '/')))
            continue;
        *slash = '\0';
        if (strcmp(slash + 1, proto) != 0 || strtol(db.fields[1], &end, 10) != port || *end)
            continue;
        if (strlen(db.fields[0]) < len) {
            strcpy(buf, db.fields[0]);
            found = true;
        }
    }
    __netdb_close(&db);
    return found;
}

int
__netdb_host_addrs(const char *name, struct __netdb_addr *addrs, int max, char *canon,
                   size_t canonlen)
{
    struct __netdb_db db = { .path = "/etc/hosts", .builtin = __hosts_builtin };
    int               n, count = 0;

    while (count < max && (n = __netdb_next(&db)) != 0) {
        struct __netdb_addr *a = &addrs[count];
        int                  i;

        for (i = 1; i < n; i++)
            if (strcasecmp(db.fields[i], name) == 0)
                break;
        if (n < 2 || i == n)
            continue;
        if (inet_pton(AF_INET, db.fields[0], a->addr) == 1)
            a->family = AF_INET;
        else if (inet_pton(AF_INET6, db.fields[0], a->addr) == 1)
            a->family = AF_INET6;
        else
            continue;
        if (count == 0 && canon && strlen(db.fields[1]) < canonlen)
            strcpy(canon, db.fields[1]);
        count++;
    }
    __netdb_close(&db);
    return count;
}

bool
__netdb_host_name(int family, const void *addr, char *buf, size_t len)
{
    struct __netdb_db db = { .path = "/etc/hosts", .builtin = __hosts_builtin };
    size_t            alen = family == AF_INET ? 4 : 16;
    bool              found = false;
    int               n;

    while (!found && (n = __netdb_next(&db)) != 0) {
        unsigned char a[16];

        if (n < 2 || inet_pton(family, db.fields[0], a) != 1 || memcmp(a, addr, alen) != 0)
            continue;
        if (strlen(db.fields[1]) < len) {
            strcpy(buf, db.fields[1]);
            found = true;
        }
    }
    __netdb_close(&db);
    return found;
}
