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

/* What <sys/socket.h> provides */

#define _POSIX_C_SOURCE 202405L
/* glibc declares accept4 only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_NETWORKING)

#include <sys/socket.h>

#define same_type(a, T) _Generic((a), T: 1, default: 0)

/* The types */
static socklen_t   slen;
static sa_family_t family;
_Static_assert(sizeof(slen) * 8 >= 32 && (socklen_t)0.5 == 0,
               "socklen_t: an integer type of at least 32 bits");
_Static_assert((sa_family_t)-1 > 0 && (sa_family_t)0.5 == 0,
               "sa_family_t: an unsigned integer type");

static struct sockaddr         sa;
static struct sockaddr_storage ss;
static struct msghdr           msg;
static struct cmsghdr          cmsg;
static struct linger           linger;
static struct iovec            iov;

_Static_assert(same_type(sa.sa_family, sa_family_t) && same_type(sa.sa_data[0], char), "sockaddr");
_Static_assert(same_type(ss.ss_family, sa_family_t), "sockaddr_storage");
_Static_assert(same_type(msg.msg_name, void *) && same_type(msg.msg_namelen, socklen_t)
                   && same_type(msg.msg_iov, struct iovec *) && same_type(msg.msg_control, void *)
                   && same_type(msg.msg_flags, int),
               "msghdr");
_Static_assert(same_type(cmsg.cmsg_level, int) && same_type(cmsg.cmsg_type, int), "cmsghdr");
/* glibc follows the kernel: msg_iovlen, msg_controllen and cmsg_len are size_t */
#ifndef __GLIBC__
_Static_assert(same_type(msg.msg_iovlen, int) && same_type(msg.msg_controllen, socklen_t)
                   && same_type(cmsg.cmsg_len, socklen_t),
               "msghdr and cmsghdr lengths");
#endif
_Static_assert(same_type(linger.l_onoff, int) && same_type(linger.l_linger, int), "linger");
_Static_assert(same_type(iov.iov_base, void *) && same_type(iov.iov_len, size_t), "iovec");
_Static_assert(same_type((ssize_t)0, ssize_t) && (ssize_t)-1 < 0, "ssize_t");

/* sockaddr_storage holds and aligns the other addresses */
_Static_assert(sizeof(struct sockaddr_storage) >= sizeof(struct sockaddr)
                   && _Alignof(struct sockaddr_storage) >= _Alignof(struct sockaddr)
                   && _Alignof(struct sockaddr_storage) >= _Alignof(long),
               "sockaddr_storage size and alignment");

/* CMSG_SPACE and CMSG_LEN of constants are constants */
static char control[CMSG_SPACE(sizeof(int)) + CMSG_SPACE(2 * sizeof(int))];
_Static_assert(CMSG_LEN(0) >= sizeof(struct cmsghdr) && CMSG_SPACE(1) >= CMSG_LEN(1)
                   && CMSG_LEN(4) == CMSG_LEN(0) + 4,
               "CMSG_LEN and CMSG_SPACE");

/* SCM_RIGHTS, SOL_SOCKET, SOMAXCONN */
#if !defined(SCM_RIGHTS) || !defined(SOL_SOCKET) || !defined(SOMAXCONN)
#error SCM_RIGHTS, SOL_SOCKET and SOMAXCONN
#endif

/* The socket types are distinct (a duplicate case doesn't compile) */
static int
socket_type(int type)
{
    switch (type) {
    case SOCK_DGRAM:
    case SOCK_RAW:
    case SOCK_SEQPACKET:
    case SOCK_STREAM:
        return 1;
    default:
        return 0;
    }
}

/* The creation flags are bitwise distinct from each other and the types */
#define TYPES (SOCK_DGRAM | SOCK_RAW | SOCK_SEQPACKET | SOCK_STREAM)
_Static_assert(SOCK_NONBLOCK != 0 && SOCK_CLOEXEC != 0 && (SOCK_NONBLOCK & SOCK_CLOEXEC) == 0
                   && (SOCK_NONBLOCK & TYPES) == 0 && (SOCK_CLOEXEC & TYPES) == 0,
               "SOCK_NONBLOCK and SOCK_CLOEXEC");
#ifdef SOCK_CLOFORK
_Static_assert(SOCK_CLOFORK != 0 && (SOCK_CLOFORK & (SOCK_NONBLOCK | SOCK_CLOEXEC | TYPES)) == 0,
               "SOCK_CLOFORK");
#endif

/* The socket options are distinct */
static int
socket_option(int option)
{
    switch (option) {
    case SO_ACCEPTCONN:
    case SO_BROADCAST:
    case SO_DEBUG:
    case SO_DOMAIN:
    case SO_DONTROUTE:
    case SO_ERROR:
    case SO_KEEPALIVE:
    case SO_LINGER:
    case SO_OOBINLINE:
    case SO_PROTOCOL:
    case SO_RCVBUF:
    case SO_RCVLOWAT:
    case SO_RCVTIMEO:
    case SO_REUSEADDR:
    case SO_SNDBUF:
    case SO_SNDLOWAT:
    case SO_SNDTIMEO:
    case SO_TYPE:
        return 1;
    default:
        return 0;
    }
}

/* The message flags are distinct */
static int
message_flag(int flag)
{
    switch (flag) {
    case MSG_CMSG_CLOEXEC:
#ifdef MSG_CMSG_CLOFORK
    case MSG_CMSG_CLOFORK:
#endif
    case MSG_CTRUNC:
    case MSG_DONTROUTE:
    case MSG_EOR:
    case MSG_OOB:
    case MSG_NOSIGNAL:
    case MSG_PEEK:
    case MSG_TRUNC:
    case MSG_WAITALL:
        return 1;
    default:
        return 0;
    }
}

/* The address families and shutdown values are distinct */
static int
family_or_how(int value, int which)
{
    if (which == 0) {
        switch (value) {
        case AF_INET:
        case AF_INET6:
        case AF_UNIX:
        case AF_UNSPEC:
            return 1;
        default:
            return 0;
        }
    }
    switch (value) {
    case SHUT_RD:
    case SHUT_RDWR:
    case SHUT_WR:
        return 1;
    default:
        return 0;
    }
}
_Static_assert(AF_UNSPEC == 0, "AF_UNSPEC");

/* The prototypes */
static int (* const p_accept)(int, struct sockaddr * restrict, socklen_t * restrict) = accept;
static int (* const p_accept4)(int, struct sockaddr * restrict, socklen_t * restrict, int)
    = accept4;
static int (* const p_bind)(int, const struct sockaddr *, socklen_t) = bind;
static int (* const p_connect)(int, const struct sockaddr *, socklen_t) = connect;
static int (* const p_getpeername)(int, struct sockaddr * restrict, socklen_t * restrict)
    = getpeername;
static int (* const p_getsockname)(int, struct sockaddr * restrict, socklen_t * restrict)
    = getsockname;
static int (* const p_getsockopt)(int, int, int, void * restrict, socklen_t * restrict)
    = getsockopt;
static int     (    *const p_listen)(int, int) = listen;
static ssize_t (* const p_recv)(int, void *, size_t, int) = recv;
static ssize_t (* const p_recvfrom)(int, void * restrict, size_t, int, struct sockaddr * restrict,
                                    socklen_t * restrict)
    = recvfrom;
static ssize_t (* const p_recvmsg)(int, struct msghdr *, int) = recvmsg;
static ssize_t (* const p_send)(int, const void *, size_t, int) = send;
static ssize_t (* const p_sendmsg)(int, const struct msghdr *, int) = sendmsg;
static ssize_t (* const p_sendto)(int, const void *, size_t, int, const struct sockaddr *,
                                  socklen_t)
    = sendto;
static int (* const p_setsockopt)(int, int, int, const void *, socklen_t) = setsockopt;
static int (* const p_shutdown)(int, int) = shutdown;
static int (* const p_sockatmark)(int) = sockatmark;
static int (* const p_socket)(int, int, int) = socket;
static int (* const p_socketpair)(int, int, int, int[2]) = socketpair;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    union {
        struct cmsghdr align;
        char           buf[sizeof(control)];
    } u;
    struct msghdr   m;
    struct cmsghdr *c;

    check(socket_type(SOCK_STREAM) && socket_option(SO_TYPE) && message_flag(MSG_PEEK)
              && family_or_how(AF_UNIX, 0) && family_or_how(SHUT_WR, 1),
          "constants");
    check(p_accept && p_accept4 && p_bind && p_connect && p_getpeername && p_getsockname
              && p_getsockopt && p_listen && p_recv && p_recvfrom && p_recvmsg && p_send
              && p_sendmsg && p_sendto && p_setsockopt && p_shutdown && p_sockatmark && p_socket
              && p_socketpair,
          "functions");

    /* sa_family, ss_family and the family of another address overlay */
    ss.ss_family = AF_UNIX;
    check(((struct sockaddr *)&ss)->sa_family == AF_UNIX, "ss_family maps onto sa_family");

    /* Walk two control messages */
    memset(&m, 0, sizeof(m));
    memset(&u, 0, sizeof(u));
    m.msg_control = u.buf;
    m.msg_controllen = sizeof(u.buf);
    c = CMSG_FIRSTHDR(&m);
    check(c == (struct cmsghdr *)u.buf, "CMSG_FIRSTHDR");
    /* glibc 2.43's CMSG_NXTHDR dereferences a null pointer (new in POSIX.1-2024) */
#ifndef __GLIBC__
    check(CMSG_NXTHDR(&m, (struct cmsghdr *)NULL) == c, "CMSG_NXTHDR of a null pointer");
#endif
    c->cmsg_len = CMSG_LEN(sizeof(int));
    c->cmsg_level = SOL_SOCKET;
    c->cmsg_type = SCM_RIGHTS;
    check(CMSG_DATA(c) == (unsigned char *)c + (CMSG_LEN(0)), "CMSG_DATA");
    check((unsigned char *)CMSG_DATA(c) + sizeof(int)
              <= (unsigned char *)c + CMSG_SPACE(sizeof(int)),
          "CMSG_SPACE covers the data");
    c = CMSG_NXTHDR(&m, c);
    check(c == (struct cmsghdr *)(u.buf + CMSG_SPACE(sizeof(int))), "second header");
    c->cmsg_len = CMSG_LEN(2 * sizeof(int));
    check(CMSG_NXTHDR(&m, c) == NULL, "last header");
    /* No control data, or too little for a header */
    m.msg_controllen = 0;
    check(CMSG_FIRSTHDR(&m) == NULL, "CMSG_FIRSTHDR without control data");
    m.msg_controllen = sizeof(struct cmsghdr) - 1;
    check(CMSG_FIRSTHDR(&m) == NULL, "CMSG_FIRSTHDR with a short buffer");
    (void)family;
    (void)msg;
    (void)cmsg;
    (void)linger;
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
