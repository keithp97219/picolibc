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

#ifndef _LOCAL_SOCKET_H_
#define _LOCAL_SOCKET_H_

#include "local-freebsd.h"
#include <limits.h>
#include <stdbool.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>

#include <freebsd/freebsd-if.h>
#include <freebsd/freebsd-in.h>
#include <freebsd/freebsd-socket.h>
#include <freebsd/freebsd-socket-flags.h>
#include <freebsd/freebsd-tcp.h>
#include <freebsd/freebsd-cmsghdr-struct.h>
#include <freebsd/freebsd-ifconf-struct.h>
#include <freebsd/freebsd-ifreq-struct.h>
#include <freebsd/freebsd-ipv6_mreq-struct.h>
#include <freebsd/freebsd-linger-struct.h>
#include <freebsd/freebsd-msghdr-struct.h>
#include <freebsd/freebsd-sockaddr-struct.h>
#include <freebsd/freebsd-sockaddr_in-struct.h>
#include <freebsd/freebsd-sockaddr_in6-struct.h>
#include <freebsd/freebsd-sockaddr_storage-struct.h>
#include <freebsd/freebsd-sockaddr_un-struct.h>

/*
 * The socket headers in include/ are written out by hand, as public
 * headers can't use the generated ones; check them against the kernel
 */
#define _SOCK_SAME(name) _Static_assert(name == FREEBSD_##name, #name)
#define _SOCK_LAYOUT(type, member)                                                \
    _Static_assert(__builtin_offsetof(struct type, member)                        \
                           == __builtin_offsetof(struct __freebsd_##type, member) \
                       && sizeof(((struct type *)0)->member)                      \
                           == sizeof(((struct __freebsd_##type *)0)->member),     \
                   #type "." #member)
#define _SOCK_SIZE(type)                                                                  \
    _Static_assert(sizeof(struct type) == sizeof(struct __freebsd_##type), #type " size")

_SOCK_SAME(SCM_RIGHTS);
_SOCK_SAME(SOCK_STREAM);
_SOCK_SAME(SOCK_DGRAM);
_SOCK_SAME(SOCK_RAW);
_SOCK_SAME(SOCK_SEQPACKET);
_SOCK_SAME(SOCK_CLOEXEC);
_SOCK_SAME(SOCK_NONBLOCK);
_SOCK_SAME(SOCK_CLOFORK);
_SOCK_SAME(SOL_SOCKET);
_SOCK_SAME(SO_DEBUG);
_SOCK_SAME(SO_ACCEPTCONN);
_SOCK_SAME(SO_REUSEADDR);
_SOCK_SAME(SO_KEEPALIVE);
_SOCK_SAME(SO_DONTROUTE);
_SOCK_SAME(SO_BROADCAST);
_SOCK_SAME(SO_LINGER);
_SOCK_SAME(SO_OOBINLINE);
_SOCK_SAME(SO_SNDBUF);
_SOCK_SAME(SO_RCVBUF);
_SOCK_SAME(SO_SNDLOWAT);
_SOCK_SAME(SO_RCVLOWAT);
_SOCK_SAME(SO_SNDTIMEO);
_SOCK_SAME(SO_RCVTIMEO);
_SOCK_SAME(SO_ERROR);
_SOCK_SAME(SO_TYPE);
_SOCK_SAME(SO_PROTOCOL);
_SOCK_SAME(SO_DOMAIN);
_SOCK_SAME(SOMAXCONN);
_SOCK_SAME(MSG_OOB);
_SOCK_SAME(MSG_PEEK);
_SOCK_SAME(MSG_DONTROUTE);
_SOCK_SAME(MSG_EOR);
_SOCK_SAME(MSG_TRUNC);
_SOCK_SAME(MSG_CTRUNC);
_SOCK_SAME(MSG_WAITALL);
_SOCK_SAME(MSG_DONTWAIT);
_SOCK_SAME(MSG_NOSIGNAL);
_SOCK_SAME(MSG_CMSG_CLOEXEC);
_SOCK_SAME(MSG_CMSG_CLOFORK);
_SOCK_SAME(AF_UNSPEC);
_SOCK_SAME(AF_UNIX);
_SOCK_SAME(AF_LOCAL);
_SOCK_SAME(AF_INET);
_SOCK_SAME(AF_INET6);
_SOCK_SAME(PF_UNSPEC);
_SOCK_SAME(PF_UNIX);
_SOCK_SAME(PF_LOCAL);
_SOCK_SAME(PF_INET);
_SOCK_SAME(PF_INET6);
_SOCK_SAME(SHUT_RD);
_SOCK_SAME(SHUT_WR);
_SOCK_SAME(SHUT_RDWR);
_SOCK_SAME(IPPROTO_IP);
_SOCK_SAME(IPPROTO_ICMP);
_SOCK_SAME(IPPROTO_TCP);
_SOCK_SAME(IPPROTO_UDP);
_SOCK_SAME(IPPROTO_IPV6);
_SOCK_SAME(IPPROTO_ICMPV6);
_SOCK_SAME(IPPROTO_RAW);
_SOCK_SAME(IPV6_UNICAST_HOPS);
_SOCK_SAME(IPV6_MULTICAST_IF);
_SOCK_SAME(IPV6_MULTICAST_HOPS);
_SOCK_SAME(IPV6_MULTICAST_LOOP);
_SOCK_SAME(IPV6_JOIN_GROUP);
_SOCK_SAME(IPV6_LEAVE_GROUP);
_SOCK_SAME(IPV6_V6ONLY);
_SOCK_SAME(INADDR_ANY);
_SOCK_SAME(INADDR_BROADCAST);
_SOCK_SAME(INADDR_LOOPBACK);
_SOCK_SAME(INET_ADDRSTRLEN);
_SOCK_SAME(INET6_ADDRSTRLEN);
_SOCK_SAME(TCP_NODELAY);
_SOCK_SAME(IF_NAMESIZE);

_SOCK_SIZE(sockaddr);
_SOCK_LAYOUT(sockaddr, sa_len);
_SOCK_LAYOUT(sockaddr, sa_family);
_SOCK_LAYOUT(sockaddr, sa_data);
_SOCK_SIZE(sockaddr_storage);
_SOCK_LAYOUT(sockaddr_storage, ss_len);
_SOCK_LAYOUT(sockaddr_storage, ss_family);
_SOCK_SIZE(sockaddr_in);
_SOCK_LAYOUT(sockaddr_in, sin_len);
_SOCK_LAYOUT(sockaddr_in, sin_family);
_SOCK_LAYOUT(sockaddr_in, sin_port);
_SOCK_LAYOUT(sockaddr_in, sin_addr);
_SOCK_LAYOUT(sockaddr_in, sin_zero);
_SOCK_SIZE(sockaddr_in6);
_SOCK_LAYOUT(sockaddr_in6, sin6_len);
_SOCK_LAYOUT(sockaddr_in6, sin6_family);
_SOCK_LAYOUT(sockaddr_in6, sin6_port);
_SOCK_LAYOUT(sockaddr_in6, sin6_flowinfo);
_SOCK_LAYOUT(sockaddr_in6, sin6_addr);
_SOCK_LAYOUT(sockaddr_in6, sin6_scope_id);
_SOCK_SIZE(sockaddr_un);
_SOCK_LAYOUT(sockaddr_un, sun_len);
_SOCK_LAYOUT(sockaddr_un, sun_family);
_SOCK_LAYOUT(sockaddr_un, sun_path);
_SOCK_SIZE(msghdr);
_SOCK_LAYOUT(msghdr, msg_name);
_SOCK_LAYOUT(msghdr, msg_namelen);
_SOCK_LAYOUT(msghdr, msg_iov);
_SOCK_LAYOUT(msghdr, msg_iovlen);
_SOCK_LAYOUT(msghdr, msg_control);
_SOCK_LAYOUT(msghdr, msg_controllen);
_SOCK_LAYOUT(msghdr, msg_flags);
_SOCK_SIZE(cmsghdr);
_SOCK_LAYOUT(cmsghdr, cmsg_len);
_SOCK_LAYOUT(cmsghdr, cmsg_level);
_SOCK_LAYOUT(cmsghdr, cmsg_type);
_SOCK_SIZE(linger);
_SOCK_LAYOUT(linger, l_onoff);
_SOCK_LAYOUT(linger, l_linger);
_SOCK_SIZE(ipv6_mreq);
_SOCK_LAYOUT(ipv6_mreq, ipv6mr_multiaddr);
_SOCK_LAYOUT(ipv6_mreq, ipv6mr_interface);

#undef _SOCK_SAME
#undef _SOCK_LAYOUT
#undef _SOCK_SIZE

/*
 * The socket options with a struct timeval, whose tv_usec here is
 * narrower than the kernel's long
 */
static inline int
_sock_is_timeo(int level, int name)
{
    return level == SOL_SOCKET && (name == SO_RCVTIMEO || name == SO_SNDTIMEO);
}

/*
 * sendmsg and recvmsg say EMSGSIZE for an empty or oversized array,
 * and EINVAL for lengths adding up past SSIZE_MAX, which the kernel
 * would shorten. Returns false with errno set for those.
 */
static inline bool
_msg_iov_valid(const struct iovec *iov, int iovcnt)
{
    size_t total = 0;

    if (iovcnt <= 0 || iovcnt > IOV_MAX) {
        errno = EMSGSIZE;
        return false;
    }
    for (int i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len > (size_t)SSIZE_MAX - total) {
            errno = EINVAL;
            return false;
        }
        total += iov[i].iov_len;
    }
    return true;
}

/*
 * Finish a socket call made with _syscall_raw. FreeBSD has one number
 * for ENOTSUP and EOPNOTSUPP; picolibc has two, and sockets say
 * EOPNOTSUPP.
 */
long _sock_result(long ret);

/* The socket's address family, or -1 */
int  _sock_domain(int fd);

/*
 * Store an address the kernel returned in a full-sized buffer: as much
 * as fits in the caller's, with its whole length, as Linux does
 */
void _sock_addr_out(struct sockaddr *addr, socklen_t *len, const struct sockaddr_storage *ss,
                    socklen_t klen);

/*
 * Common work for the receive calls. FreeBSD doesn't wait for all the
 * data MSG_WAITALL asks for when MSG_PEEK is set too; raising the
 * socket's low water mark to the length makes it. And it leaves a
 * local datagram receive with MSG_OOB waiting rather than refusing it.
 * Returns -1 with errno set to fail the call, 0 to go ahead, or 1 to
 * go ahead and then call _sock_recv_end with *lowat.
 */
int  _sock_recv_start(int fd, int flags, size_t len, int *lowat);
void _sock_recv_end(int fd, int lowat);

#endif /* _LOCAL_SOCKET_H_ */
