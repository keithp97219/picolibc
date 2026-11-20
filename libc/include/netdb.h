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

#ifndef _NETDB_H_
#define _NETDB_H_

#include <sys/cdefs.h>
#include <netinet/in.h>
#include <sys/socket.h>

_BEGIN_STD_C

struct hostent {
    char  *h_name;      /* Official name of the host */
    char **h_aliases;   /* Alternative host names */
    int    h_addrtype;  /* Address type */
    int    h_length;    /* The length, in bytes, of the address */
    char **h_addr_list; /* Network addresses for the host */
};

struct netent {
    char    *n_name;     /* Official, fully-qualified name of the network */
    char   **n_aliases;  /* Alternative network names */
    int      n_addrtype; /* The address type of the network */
    uint32_t n_net;      /* The network number, in host byte order */
};

struct protoent {
    char  *p_name;    /* Official name of the protocol */
    char **p_aliases; /* Alternative protocol names */
    int    p_proto;   /* The protocol number */
};

struct servent {
    char  *s_name;    /* Official name of the service */
    char **s_aliases; /* Alternative service names */
    int    s_port;    /* The port number, in network byte order */
    char  *s_proto;   /* The name of the protocol to use */
};

#define IPPORT_RESERVED 1024

struct addrinfo {
    int              ai_flags;     /* Input flags */
    int              ai_family;    /* Address family of socket */
    int              ai_socktype;  /* Socket type */
    int              ai_protocol;  /* Protocol of socket */
    socklen_t        ai_addrlen;   /* Length of socket address */
    struct sockaddr *ai_addr;      /* Socket address of socket */
    char            *ai_canonname; /* Canonical name of service location */
    struct addrinfo *ai_next;      /* Pointer to next in list */
};

#define AI_PASSIVE      0x0001
#define AI_CANONNAME    0x0002
#define AI_NUMERICHOST  0x0004
#define AI_V4MAPPED     0x0008
#define AI_ALL          0x0010
#define AI_ADDRCONFIG   0x0020
#define AI_NUMERICSERV  0x0400

#define NI_NUMERICHOST  0x0001
#define NI_NUMERICSERV  0x0002
#define NI_NOFQDN       0x0004
#define NI_NAMEREQD     0x0008
#define NI_DGRAM        0x0010
#define NI_NUMERICSCOPE 0x0100

#if __BSD_VISIBLE || __GNU_VISIBLE
#define NI_MAXHOST 1025
#define NI_MAXSERV 32
#endif

#define EAI_BADFLAGS -1
#define EAI_NONAME   -2
#define EAI_AGAIN    -3
#define EAI_FAIL     -4
#define EAI_FAMILY   -6
#define EAI_SOCKTYPE -7
#define EAI_SERVICE  -8
#define EAI_MEMORY   -10
#define EAI_SYSTEM   -11
#define EAI_OVERFLOW -12

void        endhostent(void) __picolibc_export;
void        endnetent(void) __picolibc_export;
void        endprotoent(void) __picolibc_export;
void        endservent(void) __picolibc_export;
void        freeaddrinfo(struct addrinfo *) __picolibc_export;
const char *gai_strerror(int) __picolibc_export;
int         getaddrinfo(const char         *__restrict, const char         *__restrict,
                        const struct addrinfo         *__restrict, struct addrinfo         **__restrict)
    __picolibc_export;
struct hostent *gethostent(void) __picolibc_export;
int getnameinfo(const struct sockaddr * __restrict, socklen_t, char * __restrict, socklen_t,
                char * __restrict, socklen_t, int) __picolibc_export;
struct netent   *getnetbyaddr(uint32_t, int) __picolibc_export;
struct netent   *getnetbyname(const char *) __picolibc_export;
struct netent   *getnetent(void) __picolibc_export;
struct protoent *getprotobyname(const char *) __picolibc_export;
struct protoent *getprotobynumber(int) __picolibc_export;
struct protoent *getprotoent(void) __picolibc_export;
struct servent  *getservbyname(const char *, const char *) __picolibc_export;
struct servent  *getservbyport(int, const char *) __picolibc_export;
struct servent  *getservent(void) __picolibc_export;
void             sethostent(int) __picolibc_export;
void             setnetent(int) __picolibc_export;
void             setprotoent(int) __picolibc_export;
void             setservent(int) __picolibc_export;

_END_STD_C

#endif /* _NETDB_H_ */
