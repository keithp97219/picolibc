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

/* What <netdb.h> provides */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                       \
    || (defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING > 0)

#include <netdb.h>

#define same_type(a, T) _Generic((a), T: 1, default: 0)

/* The structures */
static struct hostent  h;
static struct netent   n;
static struct protoent p;
static struct servent  s;
static struct addrinfo ai;
static uint32_t        u32;
static socklen_t       len;
_Static_assert(same_type(h.h_name, char *) && same_type(h.h_aliases, char **)
                   && same_type(h.h_addrtype, int) && same_type(h.h_length, int)
                   && same_type(h.h_addr_list, char **),
               "struct hostent");
_Static_assert(same_type(n.n_name, char *) && same_type(n.n_aliases, char **)
                   && same_type(n.n_addrtype, int) && same_type(n.n_net, uint32_t)
                   && sizeof(u32) == 4,
               "struct netent");
_Static_assert(same_type(p.p_name, char *) && same_type(p.p_aliases, char **)
                   && same_type(p.p_proto, int),
               "struct protoent");
_Static_assert(same_type(s.s_name, char *) && same_type(s.s_aliases, char **)
                   && same_type(s.s_port, int) && same_type(s.s_proto, char *),
               "struct servent");
_Static_assert(same_type(ai.ai_flags, int) && same_type(ai.ai_family, int)
                   && same_type(ai.ai_socktype, int) && same_type(ai.ai_protocol, int)
                   && same_type(ai.ai_addrlen, socklen_t)
                   && same_type(ai.ai_addr, struct sockaddr *) && same_type(ai.ai_canonname, char *)
                   && same_type(ai.ai_next, struct addrinfo *) && sizeof(len) > 0,
               "struct addrinfo");

#ifndef IPPORT_RESERVED
#error IPPORT_RESERVED
#endif

/* The flags are bitwise distinct: no two share a bit */
#define AI_ALL_FLAGS                                                                     \
    ((AI_PASSIVE) | (AI_CANONNAME) | (AI_NUMERICHOST) | (AI_NUMERICSERV) | (AI_V4MAPPED) \
     | (AI_ALL) | (AI_ADDRCONFIG))
#define AI_SUM_FLAGS                                                                     \
    ((AI_PASSIVE) + (AI_CANONNAME) + (AI_NUMERICHOST) + (AI_NUMERICSERV) + (AI_V4MAPPED) \
     + (AI_ALL) + (AI_ADDRCONFIG))
_Static_assert(AI_ALL_FLAGS == AI_SUM_FLAGS && AI_PASSIVE && AI_CANONNAME && AI_NUMERICHOST
                   && AI_NUMERICSERV && AI_V4MAPPED && AI_ALL && AI_ADDRCONFIG,
               "AI flags");

#if defined(__PICOLIBC__) || defined(NI_NUMERICSCOPE)
#define NI_SCOPE NI_NUMERICSCOPE
#else
/* glibc doesn't define NI_NUMERICSCOPE */
#define NI_SCOPE 0
#endif
#define NI_ALL_FLAGS                                                                              \
    ((NI_NOFQDN) | (NI_NUMERICHOST) | (NI_NAMEREQD) | (NI_NUMERICSERV) | (NI_SCOPE) | (NI_DGRAM))
#define NI_SUM_FLAGS                                                                              \
    ((NI_NOFQDN) + (NI_NUMERICHOST) + (NI_NAMEREQD) + (NI_NUMERICSERV) + (NI_SCOPE) + (NI_DGRAM))
_Static_assert(NI_ALL_FLAGS == NI_SUM_FLAGS && NI_NOFQDN && NI_NUMERICHOST && NI_NAMEREQD
                   && NI_NUMERICSERV && NI_DGRAM,
               "NI flags");
#ifndef __GLIBC__
_Static_assert(NI_NUMERICSCOPE != 0, "NI_NUMERICSCOPE");
#endif

/* The errors work in #if, and are distinct (a duplicate case doesn't compile) and non-zero */
#if !EAI_AGAIN || !EAI_BADFLAGS || !EAI_FAIL || !EAI_FAMILY || !EAI_MEMORY || !EAI_NONAME \
    || !EAI_SERVICE || !EAI_SOCKTYPE || !EAI_SYSTEM || !EAI_OVERFLOW
#error the EAI values
#endif

static int
eai(int error)
{
    switch (error) {
    case EAI_AGAIN:
    case EAI_BADFLAGS:
    case EAI_FAIL:
    case EAI_FAMILY:
    case EAI_MEMORY:
    case EAI_NONAME:
    case EAI_SERVICE:
    case EAI_SOCKTYPE:
    case EAI_SYSTEM:
    case EAI_OVERFLOW:
        return 1;
    default:
        return 0;
    }
}

/* Each function has the specified prototype */
static void        (       *const p_endhostent)(void) = endhostent;
static void        (       *const p_endnetent)(void) = endnetent;
static void        (       *const p_endprotoent)(void) = endprotoent;
static void        (       *const p_endservent)(void) = endservent;
static void        (       *const p_freeaddrinfo)(struct addrinfo *) = freeaddrinfo;
static const char *(* const p_gai_strerror)(int) = gai_strerror;
static int         (        *const p_getaddrinfo)(const char         *restrict, const char         *restrict,
                                   const struct addrinfo         *restrict, struct addrinfo         **restrict)
    = getaddrinfo;
static struct hostent *(* const p_gethostent)(void) = gethostent;
static int (* const p_getnameinfo)(const struct sockaddr * restrict, socklen_t, char * restrict,
                                   socklen_t, char * restrict, socklen_t, int)
    = getnameinfo;
static struct netent   *(  *const p_getnetbyaddr)(uint32_t, int) = getnetbyaddr;
static struct netent   *(  *const p_getnetbyname)(const char *) = getnetbyname;
static struct netent   *(  *const p_getnetent)(void) = getnetent;
static struct protoent *(* const p_getprotobyname)(const char *) = getprotobyname;
static struct protoent *(* const p_getprotobynumber)(int) = getprotobynumber;
static struct protoent *(* const p_getprotoent)(void) = getprotoent;
static struct servent  *( *const p_getservbyname)(const char *, const char *) = getservbyname;
static struct servent  *( *const p_getservbyport)(int, const char *) = getservbyport;
static struct servent  *( *const p_getservent)(void) = getservent;
static void             (            *const p_sethostent)(int) = sethostent;
static void             (            *const p_setnetent)(int) = setnetent;
static void             (            *const p_setprotoent)(int) = setprotoent;
static void             (            *const p_setservent)(int) = setservent;

#include <stdio.h>

int
main(void)
{
    (void)h;
    (void)n;
    (void)p;
    (void)s;
    (void)ai;
    (void)u32;
    (void)len;
    (void)eai;
    (void)p_endhostent;
    (void)p_endnetent;
    (void)p_endprotoent;
    (void)p_endservent;
    (void)p_freeaddrinfo;
    (void)p_gai_strerror;
    (void)p_getaddrinfo;
    (void)p_gethostent;
    (void)p_getnameinfo;
    (void)p_getnetbyaddr;
    (void)p_getnetbyname;
    (void)p_getnetent;
    (void)p_getprotobyname;
    (void)p_getprotobynumber;
    (void)p_getprotoent;
    (void)p_getservbyname;
    (void)p_getservbyport;
    (void)p_getservent;
    (void)p_sethostent;
    (void)p_setnetent;
    (void)p_setprotoent;
    (void)p_setservent;
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
