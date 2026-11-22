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
 * The Linux socket ABI, for picolibc's <sys/socket.h>. The values are
 * the same on every architecture libos/linux supports.
 */

#ifndef _SYS__SOCKET_H_
#define _SYS__SOCKET_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

#ifndef _SA_FAMILY_T_DECLARED
#define _SA_FAMILY_T_DECLARED
typedef unsigned short sa_family_t;
#endif

#ifndef _SOCKLEN_T_DECLARED
#define _SOCKLEN_T_DECLARED
typedef unsigned int socklen_t;
#endif

struct sockaddr {
    sa_family_t sa_family;
    char        sa_data[14];
};

struct sockaddr_storage {
    sa_family_t   ss_family;
    char          __ss_padding[128 - sizeof(sa_family_t) - sizeof(unsigned long)];
    unsigned long __ss_align;
};

/*
 * POSIX makes msg_iovlen an int and msg_controllen a socklen_t, while
 * the 64-bit kernel reads size_t; pad them to match.
 */
/* The compiler's byte order; <machine/endian.h> would define more names */
#if __SIZEOF_POINTER__ == 8 && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __SOCKET_PAD_BEFORE(name)
#define __SOCKET_PAD_AFTER(name) int name;
#elif __SIZEOF_POINTER__ == 8
#define __SOCKET_PAD_BEFORE(name) int name;
#define __SOCKET_PAD_AFTER(name)
#else
#define __SOCKET_PAD_BEFORE(name)
#define __SOCKET_PAD_AFTER(name)
#endif
#if __SIZEOF_POINTER__ == 8
/* sendmsg and recvmsg clear the padding before handing it to the kernel */
#define __SOCKET_PADDED 1
#endif

struct msghdr {
    void         *msg_name;
    socklen_t     msg_namelen;
    struct iovec *msg_iov;
    __SOCKET_PAD_BEFORE(__pad1)
    int msg_iovlen;
    __SOCKET_PAD_AFTER(__pad1)
    void *msg_control;
    __SOCKET_PAD_BEFORE(__pad2)
    socklen_t msg_controllen;
    __SOCKET_PAD_AFTER(__pad2)
    int msg_flags;
};

struct cmsghdr {
    __SOCKET_PAD_BEFORE(__pad1)
    socklen_t cmsg_len;
    __SOCKET_PAD_AFTER(__pad1)
    int cmsg_level;
    int cmsg_type;
};

#undef __SOCKET_PAD_BEFORE
#undef __SOCKET_PAD_AFTER

#define __CMSG_ALIGN(len) (((len) + sizeof(long) - 1) & ~(sizeof(long) - 1))
#define CMSG_DATA(cmsg)   ((unsigned char *)((struct cmsghdr *)(cmsg) + 1))
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
        || __CMSG_ALIGN(__cmsg->cmsg_len) + sizeof(struct cmsghdr)
            >= (__picolibc_size_t)(__end - (unsigned char *)__cmsg))
        return (struct cmsghdr *)0;
    return (struct cmsghdr *)((unsigned char *)__cmsg + __CMSG_ALIGN(__cmsg->cmsg_len));
}
#define CMSG_NXTHDR(mhdr, cmsg) __cmsg_nxthdr((mhdr), (cmsg))

struct linger {
    int l_onoff;
    int l_linger;
};

#define SCM_RIGHTS     1

#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3
#define SOCK_SEQPACKET 5
#define SOCK_NONBLOCK  04000
#define SOCK_CLOEXEC   02000000

#define SOL_SOCKET     1

#define SO_DEBUG       1
#define SO_REUSEADDR   2
#define SO_TYPE        3
#define SO_ERROR       4
#define SO_DONTROUTE   5
#define SO_BROADCAST   6
#define SO_SNDBUF      7
#define SO_RCVBUF      8
#define SO_KEEPALIVE   9
#define SO_OOBINLINE   10
#define SO_LINGER      13
#define SO_RCVLOWAT    18
#define SO_SNDLOWAT    19
#define SO_ACCEPTCONN  30
#define SO_PROTOCOL    38
#define SO_DOMAIN      39
/* With a 64-bit time_t, 32-bit targets use the kernel's 64-bit versions */
#if __SIZEOF_LONG__ == 8
#define SO_RCVTIMEO 20
#define SO_SNDTIMEO 21
#else
#define SO_RCVTIMEO 66
#define SO_SNDTIMEO 67
#endif

#define SOMAXCONN        4096

#define MSG_OOB          0x1
#define MSG_PEEK         0x2
#define MSG_DONTROUTE    0x4
#define MSG_CTRUNC       0x8
#define MSG_TRUNC        0x20
#define MSG_DONTWAIT     0x40
#define MSG_EOR          0x80
#define MSG_WAITALL      0x100
#define MSG_NOSIGNAL     0x4000
#define MSG_CMSG_CLOEXEC 0x40000000

#define AF_UNSPEC        0
#define AF_UNIX          1
#define AF_LOCAL         AF_UNIX
#define AF_INET          2
#define AF_INET6         10
#define PF_UNSPEC        AF_UNSPEC
#define PF_UNIX          AF_UNIX
#define PF_LOCAL         AF_LOCAL
#define PF_INET          AF_INET
#define PF_INET6         AF_INET6

#define SHUT_RD          0
#define SHUT_WR          1
#define SHUT_RDWR        2

#endif /* _SYS__SOCKET_H_ */
