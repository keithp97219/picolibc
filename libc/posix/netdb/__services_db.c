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

/* Used when /etc/services can't be read */
const char * const __services_builtin[] = {
    "echo 7/tcp",         "echo 7/udp",
    "discard 9/tcp",      "discard 9/udp",
    "daytime 13/tcp",     "daytime 13/udp",
    "ftp-data 20/tcp",    "ftp 21/tcp",
    "ssh 22/tcp",         "telnet 23/tcp",
    "smtp 25/tcp mail",   "time 37/tcp",
    "time 37/udp",        "domain 53/tcp",
    "domain 53/udp",      "tftp 69/udp",
    "http 80/tcp www",    "pop3 110/tcp",
    "ntp 123/udp",        "imap 143/tcp",
    "snmp 161/udp",       "https 443/tcp",
    "https 443/udp",      "syslog 514/udp",
    "submission 587/tcp", NULL,
};

struct __netdb_db __services_db = { .path = "/etc/services", .builtin = __services_builtin };

/* "name port/protocol aliases..." */
struct servent *
__services_entry(struct __netdb_db *db, int n)
{
    static struct servent s;
    char                 *slash, *end;
    long                  port;

    if (n < 2)
        return NULL;
    slash = strchr(db->fields[1], '/');
    if (!slash)
        return NULL;
    *slash = '\0';
    port = strtol(db->fields[1], &end, 10);
    if (*end || port < 0 || port > 65535)
        return NULL;
    s.s_name = db->fields[0];
    s.s_port = htons((uint16_t)port);
    s.s_proto = slash + 1;
    s.s_aliases = &db->fields[2];
    return &s;
}
