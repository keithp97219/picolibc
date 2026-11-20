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

/*
 * A name server: an IPv4 address with an optional :port, or an IPv6
 * address, in brackets when it has a :port
 */
static bool
dns_server(struct __dns_server *srv, char *text)
{
    char *port = NULL;
    long  p;

    srv->port = 53;
    if (*text == '[') {
        char *close = strchr(text, ']');

        if (!close)
            return false;
        *close = '\0';
        text++;
        if (close[1] == ':')
            port = close + 2;
        else if (close[1])
            return false;
    } else if (strchr(text, '.') && (port = strchr(text, ':')) != NULL) {
        *port++ = '\0';
    }
    if (port) {
        char *end;

        p = strtol(port, &end, 10);
        if (*end || p <= 0 || p > 65535)
            return false;
        srv->port = (unsigned short)p;
    }
    if (inet_pton(AF_INET, text, srv->addr) == 1)
        srv->family = AF_INET;
    else if (inet_pton(AF_INET6, text, srv->addr) == 1)
        srv->family = AF_INET6;
    else
        return false;
    return true;
}

/* A number in an options entry, clamped as other resolvers do */
static int
dns_option(const char *value, int max)
{
    long v = strtol(value, NULL, 10);

    if (v < 0)
        return 0;
    return v > max ? max : (int)v;
}

void
__dns_conf(struct __dns_conf *conf)
{
    char  line[__NETDB_LINE_MAX];
    FILE *f;

    memset(conf, 0, sizeof(*conf));
    conf->timeout = 5;
    conf->attempts = 2;
    conf->ndots = 1;

    f = fopen(__picolibc_resolv_conf, "re");
    while (f && fgets(line, sizeof(line), f)) {
        char *save, *word = strtok_r(line, " \t\r\n", &save);
        char *arg;

        if (!word || *word == '#' || *word == ';')
            continue;
        if (strcmp(word, "nameserver") == 0) {
            arg = strtok_r(NULL, " \t\r\n", &save);
            if (arg && conf->nservers < __DNS_SERVERS_MAX
                && dns_server(&conf->servers[conf->nservers], arg))
                conf->nservers++;
        } else if (strcmp(word, "search") == 0 || strcmp(word, "domain") == 0) {
            /* The last search or domain line wins */
            conf->nsearch = 0;
            while ((arg = strtok_r(NULL, " \t\r\n", &save)) != NULL
                   && conf->nsearch < __DNS_SEARCH_MAX) {
                if (strlen(arg) <= __DNS_NAME_MAX)
                    strcpy(conf->search[conf->nsearch++], arg);
                if (*word == 'd')
                    break;
            }
        } else if (strcmp(word, "options") == 0) {
            while ((arg = strtok_r(NULL, " \t\r\n", &save)) != NULL) {
                if (strncmp(arg, "timeout:", 8) == 0)
                    conf->timeout = dns_option(arg + 8, 30);
                else if (strncmp(arg, "attempts:", 9) == 0)
                    conf->attempts = dns_option(arg + 9, 5);
                else if (strncmp(arg, "ndots:", 6) == 0)
                    conf->ndots = dns_option(arg + 6, 15);
            }
        }
    }
    if (f)
        fclose(f);

    if (conf->timeout < 1)
        conf->timeout = 1;
    if (conf->attempts < 1)
        conf->attempts = 1;
    /* Without a server, ask one on this machine */
    if (!conf->nservers) {
        conf->servers[0].family = AF_INET;
        conf->servers[0].port = 53;
        (void)inet_pton(AF_INET, "127.0.0.1", conf->servers[0].addr);
        conf->nservers = 1;
    }
}
