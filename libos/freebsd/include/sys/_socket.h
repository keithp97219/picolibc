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

/*
 * The FreeBSD socket ABI, for picolibc's <sys/socket.h>. The
 * addresses start with a length byte; the kernel sets it from the
 * length passed with the address, so callers needn't.
 */

#ifndef _SYS__SOCKET_H_
#define _SYS__SOCKET_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

#ifndef _SA_FAMILY_T_DECLARED
#define _SA_FAMILY_T_DECLARED
typedef __uint8_t sa_family_t;
#endif

#ifndef _SOCKLEN_T_DECLARED
#define _SOCKLEN_T_DECLARED
typedef __uint32_t socklen_t;
#endif

struct sockaddr {
    unsigned char sa_len;
    sa_family_t   sa_family;
    char          sa_data[14];
};

struct sockaddr_storage {
    unsigned char ss_len;
    sa_family_t   ss_family;
    char          __ss_pad1[6];
    __int64_t     __ss_align;
    char          __ss_pad2[112];
};

struct msghdr {
    void         *msg_name;
    socklen_t     msg_namelen;
    struct iovec *msg_iov;
    int           msg_iovlen;
    void         *msg_control;
    socklen_t     msg_controllen;
    int           msg_flags;
};

struct cmsghdr {
    socklen_t cmsg_len;
    int       cmsg_level;
    int       cmsg_type;
};

/* Control data is aligned to a long, which pads the 12-byte header */
#define __CMSG_ALIGN(len) (((len) + sizeof(long) - 1) & ~(sizeof(long) - 1))
#define CMSG_DATA(cmsg)   ((unsigned char *)(cmsg) + __CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_LEN(len)     (__CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#define CMSG_SPACE(len)   (__CMSG_ALIGN(sizeof(struct cmsghdr)) + __CMSG_ALIGN(len))
#define CMSG_FIRSTHDR(mhdr)                                              \
    ((__picolibc_size_t)(mhdr)->msg_controllen >= sizeof(struct cmsghdr) \
         ? (struct cmsghdr *)(mhdr)->msg_control                         \
         : (struct cmsghdr *)0)
/*
 * A null cmsg gives the first header. A function rather than a macro
 * body, so that a null constant isn't seen in pointer arithmetic
 */
static __inline struct cmsghdr *
__cmsg_nxthdr(struct msghdr *__mhdr, struct cmsghdr *__cmsg)
{
    unsigned char *__end = (unsigned char *)__mhdr->msg_control + __mhdr->msg_controllen;

    if (!__cmsg)
        return CMSG_FIRSTHDR(__mhdr);
    if (__cmsg->cmsg_len < sizeof(struct cmsghdr)
        || __CMSG_ALIGN(__cmsg->cmsg_len) + __CMSG_ALIGN(sizeof(struct cmsghdr))
            > (__picolibc_size_t)(__end - (unsigned char *)__cmsg))
        return (struct cmsghdr *)0;
    return (struct cmsghdr *)((unsigned char *)__cmsg + __CMSG_ALIGN(__cmsg->cmsg_len));
}
#define CMSG_NXTHDR(mhdr, cmsg) __cmsg_nxthdr((mhdr), (cmsg))

struct linger {
    int l_onoff;
    int l_linger;
};

#define SCM_RIGHTS       1

#define SOCK_STREAM      1
#define SOCK_DGRAM       2
#define SOCK_RAW         3
#define SOCK_SEQPACKET   5
#define SOCK_CLOEXEC     0x10000000
#define SOCK_NONBLOCK    0x20000000
#define SOCK_CLOFORK     0x40000000

#define SOL_SOCKET       0xffff

#define SO_DEBUG         0x0001
#define SO_ACCEPTCONN    0x0002
#define SO_REUSEADDR     0x0004
#define SO_KEEPALIVE     0x0008
#define SO_DONTROUTE     0x0010
#define SO_BROADCAST     0x0020
#define SO_LINGER        0x0080
#define SO_OOBINLINE     0x0100
#define SO_SNDBUF        0x1001
#define SO_RCVBUF        0x1002
#define SO_SNDLOWAT      0x1003
#define SO_RCVLOWAT      0x1004
#define SO_SNDTIMEO      0x1005
#define SO_RCVTIMEO      0x1006
#define SO_ERROR         0x1007
#define SO_TYPE          0x1008
#define SO_PROTOCOL      0x1016
#define SO_DOMAIN        0x1019

#define SOMAXCONN        128

#define MSG_OOB          0x00000001
#define MSG_PEEK         0x00000002
#define MSG_DONTROUTE    0x00000004
#define MSG_EOR          0x00000008
#define MSG_TRUNC        0x00000010
#define MSG_CTRUNC       0x00000020
#define MSG_WAITALL      0x00000040
#define MSG_DONTWAIT     0x00000080
#define MSG_NOSIGNAL     0x00020000
#define MSG_CMSG_CLOEXEC 0x00040000
#define MSG_CMSG_CLOFORK 0x00400000

#define AF_UNSPEC        0
#define AF_UNIX          1
#define AF_LOCAL         AF_UNIX
#define AF_INET          2
#define AF_INET6         28
#define PF_UNSPEC        AF_UNSPEC
#define PF_UNIX          AF_UNIX
#define PF_LOCAL         AF_LOCAL
#define PF_INET          AF_INET
#define PF_INET6         AF_INET6

#define SHUT_RD          0
#define SHUT_WR          1
#define SHUT_RDWR        2

#endif /* _SYS__SOCKET_H_ */
