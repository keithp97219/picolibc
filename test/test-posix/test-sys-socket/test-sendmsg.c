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

#define _POSIX_C_SOURCE 202405L
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_NETWORKING

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>

/* sendmsg and recvmsg pass gathered data and descriptors */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Skips the test when the system has no sockets of this family */
static int
make_socket(int family, int type)
{
    int fd = socket(family, type, 0);

    if (fd < 0 && (errno == EAFNOSUPPORT || errno == EACCES || errno == EPERM)) {
        printf("no sockets of family %d, skipping\n", family);
        exit(77);
    }
    check(fd >= 0, "socket");
    return fd;
}

/* A loopback IPv4 socket address with port 0 */
static struct sockaddr_in
loopback(void)
{
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return sin;
}

/* A connected pair of TCP sockets over the loopback interface */
static void
tcp_pair(int *client, int *server)
{
    struct sockaddr_in sin = loopback();
    socklen_t          len = sizeof(sin);
    int                listener = make_socket(AF_INET, SOCK_STREAM);

    check(bind(listener, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    check(listen(listener, 1) == 0, "listen");
    check(getsockname(listener, (struct sockaddr *)&sin, &len) == 0, "getsockname");
    *client = make_socket(AF_INET, SOCK_STREAM);
    check(connect(*client, (struct sockaddr *)&sin, sizeof(sin)) == 0, "connect");
    *server = accept(listener, NULL, NULL);
    check(*server >= 0, "accept");
    check(close(listener) == 0, "close listener");
}

/* Fill a non-blocking socket's buffers */
static void
fill(int fd)
{
    static char block[4096];

    while (send(fd, block, sizeof(block), MSG_DONTWAIT) > 0)
        ;
    check(errno == EAGAIN || errno == EWOULDBLOCK, "EAGAIN");
    while (send(fd, block, 1, MSG_DONTWAIT) > 0)
        ;
    check(errno == EAGAIN || errno == EWOULDBLOCK, "EAGAIN for a byte");
}

/* sendmsg with just data; keeps errno */
static ssize_t
send_iov(int fd, struct iovec *iov, int iovlen, int flags)
{
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    return sendmsg(fd, &msg, flags);
}

int
main(void)
{
    struct iovec   *many;
    long            iov_max = sysconf(_SC_IOV_MAX);
    static char     big[70000];
    struct msghdr   msg;
    struct iovec    iov[3];
    struct cmsghdr *cmsg;
    union {
        char           buf[CMSG_SPACE(2 * sizeof(int))];
        struct cmsghdr align;
    } control;
    struct sockaddr_in a = loopback(), b = loopback(), c = loopback();
    socklen_t          len;
    char               buf[16], data[4];
    int                sv[2], fds[2], fd, pair[2], i, sa, sb, sc;
    ssize_t            ret;

    check(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");
    check(pipe(fds) == 0, "pipe");

    /* Callers need not clear the structures; fill them with garbage */
    memset(&msg, 0xa5, sizeof(msg));
    memset(&control, 0xa5, sizeof(control));
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    /* msg_flags is ignored */
    iov[0].iov_base = "two ";
    iov[0].iov_len = 4;
    iov[1].iov_base = "";
    iov[1].iov_len = 0;
    iov[2].iov_base = "parts";
    iov[2].iov_len = 5;
    msg.msg_iov = iov;
    msg.msg_iovlen = 3;
    msg.msg_control = control.buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(int));
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fds[1], sizeof(int));
    check(sendmsg(sv[0], &msg, 0) == 9, "sendmsg");

    memset(&msg, 0xa5, sizeof(msg));
    memset(&control, 0xa5, sizeof(control));
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control.buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(int));
    check(recvmsg(sv[1], &msg, 0) == 9 && memcmp(buf, "two parts", 9) == 0, "recvmsg");
    check(msg.msg_flags == 0 && msg.msg_controllen == CMSG_SPACE(sizeof(int)), "received lengths");
    cmsg = CMSG_FIRSTHDR(&msg);
    check(cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS, "control");
    check(CMSG_NXTHDR(&msg, cmsg) == NULL, "one control message");
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    /* The received descriptor writes to the same pipe */
    check(write(fd, "pipe", 4) == 4 && read(fds[0], data, 4) == 4 && memcmp(data, "pipe", 4) == 0,
          "passed descriptor");
    check(close(fd) == 0, "close");

    /* Two descriptors in one message */
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0, "socketpair");
    memset(&msg, 0, sizeof(msg));
    iov[0].iov_base = "x";
    iov[0].iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control.buf;
    msg.msg_controllen = sizeof(control.buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(2 * sizeof(int));
    memcpy(CMSG_DATA(cmsg), pair, 2 * sizeof(int));
    check(sendmsg(sv[0], &msg, 0) == 1, "send two descriptors");
    check(close(pair[0]) == 0 && close(pair[1]) == 0, "close");
    memset(&msg, 0, sizeof(msg));
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control.buf;
    msg.msg_controllen = sizeof(control.buf);
    check(recvmsg(sv[1], &msg, 0) == 1, "receive two descriptors");
    cmsg = CMSG_FIRSTHDR(&msg);
    check(cmsg && cmsg->cmsg_len == CMSG_LEN(2 * sizeof(int)), "two descriptors");
    memcpy(pair, CMSG_DATA(cmsg), 2 * sizeof(int));
    check(write(pair[0], "z", 1) == 1 && read(pair[1], data, 1) == 1 && data[0] == 'z',
          "the passed pair");
    check(close(pair[0]) == 0 && close(pair[1]) == 0, "close");

    /* The array's dimension and total length; Linux, and so glibc, accepts empty arrays */
    iov[0].iov_base = "x";
    iov[0].iov_len = 1;
#ifndef __GLIBC__
    errno = 0;
    check(send_iov(sv[0], iov, 0, 0) == -1 && errno == EMSGSIZE, "no buffers");
    errno = 0;
    check(send_iov(sv[0], iov, -1, 0) == -1 && errno == EMSGSIZE, "negative count");
#endif
    /* IOV_MAX is an XSI name in <limits.h> */
    if (iov_max > 0) {
        many = calloc((size_t)iov_max + 1, sizeof(*many));
        check(many != NULL, "calloc");
        for (i = 0; i <= iov_max; i++) {
            many[i].iov_base = "x";
            many[i].iov_len = 1;
        }
        errno = 0;
        check(send_iov(sv[0], many, (int)iov_max + 1, 0) == -1 && errno == EMSGSIZE,
              "more than IOV_MAX");
        check(send_iov(sv[0], many, (int)iov_max, 0) == iov_max, "IOV_MAX buffers");
        for (i = 0; i < iov_max; i += (int)ret) {
            ret = recv(sv[1], big, (size_t)(iov_max - i), 0);
            check(ret > 0, "drain");
        }
        free(many);
    }
    /* Linux shortens the transfer, and glibc passes that on */
#ifndef __GLIBC__
    iov[0].iov_base = big;
    iov[0].iov_len = (size_t)SSIZE_MAX / 2 + 1;
    iov[1].iov_base = big;
    iov[1].iov_len = (size_t)SSIZE_MAX / 2 + 1;
    errno = 0;
    check(send_iov(sv[0], iov, 2, 0) == -1 && errno == EINVAL, "total overflows");
#endif

    /* Full buffers */
    fill(sv[0]);
    iov[0].iov_base = "x";
    iov[0].iov_len = 1;
    errno = 0;
    check(send_iov(sv[0], iov, 1, MSG_DONTWAIT) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "EAGAIN");
    /* No longer connected */
    check(close(sv[1]) == 0, "close peer");
    errno = 0;
    check(send_iov(sv[0], iov, 1, MSG_NOSIGNAL) == -1 && errno == EPIPE, "EPIPE");
    check(close(sv[0]) == 0, "close");
    fd = make_socket(AF_UNIX, SOCK_STREAM);
    errno = 0;
    check(send_iov(fd, iov, 1, MSG_NOSIGNAL) == -1 && errno == ENOTCONN, "ENOTCONN");
    check(close(fd) == 0, "close");
    check(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) == 0, "socketpair");
    errno = 0;
    check(send_iov(sv[0], iov, 1, MSG_OOB) == -1 && errno == EOPNOTSUPP, "EOPNOTSUPP");
    check(close(sv[0]) == 0 && close(sv[1]) == 0, "close");

    /* Datagrams go to msg_name; a peer may be overridden or not */
    sa = make_socket(AF_INET, SOCK_DGRAM);
    sb = make_socket(AF_INET, SOCK_DGRAM);
    sc = make_socket(AF_INET, SOCK_DGRAM);
    len = sizeof(a);
    check(bind(sa, (struct sockaddr *)&a, sizeof(a)) == 0
              && getsockname(sa, (struct sockaddr *)&a, &len) == 0,
          "bind a");
    len = sizeof(b);
    check(bind(sb, (struct sockaddr *)&b, sizeof(b)) == 0
              && getsockname(sb, (struct sockaddr *)&b, &len) == 0,
          "bind b");
    len = sizeof(c);
    check(bind(sc, (struct sockaddr *)&c, sizeof(c)) == 0
              && getsockname(sc, (struct sockaddr *)&c, &len) == 0,
          "bind c");
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &b;
    msg.msg_namelen = sizeof(b);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    check(sendmsg(sa, &msg, 0) == 1 && recv(sb, buf, sizeof(buf), 0) == 1, "to msg_name");
    check(connect(sa, (struct sockaddr *)&b, sizeof(b)) == 0, "connect");
    msg.msg_name = &c;
    msg.msg_namelen = sizeof(c);
    ret = sendmsg(sa, &msg, 0);
    if (ret == 1)
        check(recv(sc, buf, sizeof(buf), 0) == 1, "override the peer");
    else
        check(ret == -1 && errno == EISCONN, "EISCONN");
    /* Too large */
    iov[0].iov_base = big;
    iov[0].iov_len = sizeof(big);
    errno = 0;
    check(sendmsg(sa, &msg, 0) == -1 && errno == EMSGSIZE, "EMSGSIZE");
    /* An address of another family */
    {
        struct sockaddr_in6 sin6;

        memset(&sin6, 0, sizeof(sin6));
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = htons(9);
        msg.msg_name = &sin6;
        msg.msg_namelen = sizeof(sin6);
        iov[0].iov_len = 1;
        errno = 0;
        check(sendmsg(sc, &msg, 0) == -1 && errno == EAFNOSUPPORT, "EAFNOSUPPORT");
    }
    check(close(sa) == 0 && close(sb) == 0 && close(sc) == 0, "close");

    iov[0].iov_base = "x";
    iov[0].iov_len = 1;
    errno = 0;
    check(send_iov(fds[1], iov, 1, 0) == -1 && errno == ENOTSOCK, "ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");
    errno = 0;
    check(send_iov(-1, iov, 1, 0) == -1 && errno == EBADF, "EBADF");
    (void)tcp_pair;
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
