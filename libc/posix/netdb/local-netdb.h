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

#ifndef _LOCAL_NETDB_H_
#define _LOCAL_NETDB_H_

#define _DEFAULT_SOURCE
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define __NETDB_LINE_MAX 512
#define __NETDB_FIELDS   34 /* the name or address, then up to 32 more */

/*
 * One of the text databases: a file, or the built-in lines in the same
 * format when the file can't be opened.
 */
struct __netdb_db {
    const char         *path;
    const char * const *builtin;
    FILE               *file;
    size_t              next;
    bool                open;
    bool                use_builtin;
    bool                stayopen;
    char                line[__NETDB_LINE_MAX];
    char               *fields[__NETDB_FIELDS + 1];
};

/* Starts reading from the beginning */
void                      __netdb_rewind(struct __netdb_db *db);

/* Stops reading, closing the file */
void                      __netdb_close(struct __netdb_db *db);

/* Splits the next entry into db->fields; returns their count, or 0 at the end */
int                       __netdb_next(struct __netdb_db *db);

extern struct __netdb_db  __hosts_db, __networks_db, __protocols_db, __services_db;
extern const char * const __hosts_builtin[], * const __networks_builtin[];
extern const char * const __protocols_builtin[], * const __services_builtin[];

/* Converts the current entry of each database, or returns NULL if it is malformed */
struct hostent  *__hosts_entry(struct __netdb_db *db, int n);
struct netent   *__networks_entry(struct __netdb_db *db, int n);
struct protoent *__protocols_entry(struct __netdb_db *db, int n);
struct servent  *__services_entry(struct __netdb_db *db, int n);

/* An address from the hosts database */
struct __netdb_addr {
    int           family;
    unsigned char addr[16];
};

/*
 * Thread-safe lookups for getaddrinfo and getnameinfo, each reading its
 * own copy of the database.
 */

/* The port, in host byte order, of service 'name' for 'proto', or -1 */
int  __netdb_service_port(const char *name, const char *proto);

/* The name of the service on port (host byte order) for 'proto' */
bool __netdb_service_name(int port, const char *proto, char *buf, size_t len);

/*
 * The addresses of host 'name', up to 'max' of them, and its official
 * name in 'canon'; returns the number found.
 */
int  __netdb_host_addrs(const char *name, struct __netdb_addr *addrs, int max, char *canon,
                        size_t canonlen);

/* The name of the host with 'addr' */
bool __netdb_host_name(int family, const void *addr, char *buf, size_t len);

#endif /* _LOCAL_NETDB_H_ */
