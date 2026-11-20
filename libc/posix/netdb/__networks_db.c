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

/* Used when /etc/networks can't be read */
const char * const __networks_builtin[] = {
    "loopback 127",
    NULL,
};

struct __netdb_db __networks_db = { .path = "/etc/networks", .builtin = __networks_builtin };

/*
 * "name number aliases...", the number in dotted form with up to four
 * parts, each the next eight bits (127 is 0x7f, 10.1 is 0x0a01).
 */
struct netent *
__networks_entry(struct __netdb_db *db, int n)
{
    static struct netent net;
    uint32_t             value = 0;
    char                *p, *end;
    int                  parts = 0;

    if (n < 2)
        return NULL;
    for (p = db->fields[1];; p = end + 1) {
        unsigned long part = strtoul(p, &end, 0);

        if (end == p || part > 255 || ++parts > 4)
            return NULL;
        value = value << 8 | (uint32_t)part;
        if (*end != '.')
            break;
    }
    if (*end)
        return NULL;
    net.n_name = db->fields[0];
    net.n_net = value;
    net.n_addrtype = AF_INET;
    net.n_aliases = &db->fields[2];
    return &net;
}
