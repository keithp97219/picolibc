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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_NETWORKING) && defined(__PICOLIBC_POSIX_FD_MGMT)    \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE) \
    && defined(__PICOLIBC_POSIX_SIGNALS)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include "../is-qemu.h"

/* recvmsg scatters data and receives addresses, descriptors and flags */

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

/*
 * A child that sends SIGUSR1 every 10ms to a handler installed without
 * SA_RESTART, so a call that blocks is interrupted whenever it starts
 */
static volatile sig_atomic_t interrupts;

static void
on_interrupt(int sig)
{
    (void)sig;
    interrupts++;
}

static pid_t
start_interrupts(void)
{
    struct sigaction act;
    pid_t            parent = getpid(), pid;

    memset(&act, 0, sizeof(act));
    act.sa_handler = on_interrupt;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct timespec ts = { 0, 10 * 1000 * 1000 };

        for (;;) {
            kill(parent, SIGUSR1);
            nanosleep(&ts, NULL);
        }
    }
    return pid;
}

static void
stop_interrupts(pid_t pid)
{
    check(kill(pid, SIGKILL) == 0 && waitpid(pid, NULL, 0) == pid, "stop the interrupter");
    check(signal(SIGUSR1, SIG_IGN) != SIG_ERR, "ignore SIGUSR1");
}

/* A message header for the buffers and control space */
static struct msghdr
header(struct iovec *iov, int iovlen, void *control, size_t controllen)
{
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    msg.msg_control = control;
    msg.msg_controllen = (socklen_t)controllen;
    /* Ignored on input */
    msg.msg_flags = ~0;
    return msg;
}

/* Send a byte along with the descriptors */
static void
send_fds(int fd, const int *fds, int n)
{
    union {
        char           buf[CMSG_SPACE(2 * sizeof(int))];
        struct cmsghdr align;
    } control;
    struct iovec    iov = { .iov_base = "f", .iov_len = 1 };
    struct msghdr   msg = header(&iov, 1, control.buf, CMSG_SPACE((size_t)n * sizeof(int)));
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

    msg.msg_flags = 0;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN((size_t)n * sizeof(int));
    memcpy(CMSG_DATA(cmsg), fds, (size_t)n * sizeof(int));
    check(sendmsg(fd, &msg, 0) == 1, "send descriptors");
}

int
main(void)
{
    static char        big[16];
    struct sockaddr_in a = loopback(), b = loopback(), from;
    struct iovec       iov[3];
    struct msghdr      msg;
    struct cmsghdr    *cmsg;
    union {
        char           buf[CMSG_SPACE(2 * sizeof(int))];
        struct cmsghdr align;
    } control;
    socklen_t len;
    pid_t     interrupter;
    char      x[2], y[3], z[8], buf[16];
    int       sv[2], pfds[2], fd, client, server, sa, sb;

    /* The buffers fill in turn */
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");
    check(send(sv[0], "abcdefg", 7, 0) == 7, "send");
    iov[0].iov_base = x;
    iov[0].iov_len = sizeof(x);
    iov[1].iov_base = y;
    iov[1].iov_len = sizeof(y);
    iov[2].iov_base = z;
    iov[2].iov_len = sizeof(z);
    msg = header(iov, 3, NULL, 0);
    check(recvmsg(sv[1], &msg, MSG_PEEK) == 7, "peek");
    msg = header(iov, 3, NULL, 0);
    check(recvmsg(sv[1], &msg, 0) == 7, "recvmsg");
    check(memcmp(x, "ab", 2) == 0 && memcmp(y, "cde", 3) == 0 && memcmp(z, "fg", 2) == 0,
          "scattered in order");
    check(msg.msg_flags == 0, "no flags");

    /* Descriptors, with and without MSG_CMSG_CLOEXEC */
    check(pipe(pfds) == 0, "pipe");
    send_fds(sv[0], pfds, 1);
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    msg = header(iov, 1, control.buf, sizeof(control.buf));
    check(recvmsg(sv[1], &msg, 0) == 1, "receive a descriptor");
    cmsg = CMSG_FIRSTHDR(&msg);
    check(cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS
              && cmsg->cmsg_len == CMSG_LEN(sizeof(int)),
          "SCM_RIGHTS");
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    check((fcntl(fd, F_GETFD) & FD_CLOEXEC) == 0, "no FD_CLOEXEC");
    check(close(fd) == 0, "close");
    send_fds(sv[0], pfds, 1);
    msg = header(iov, 1, control.buf, sizeof(control.buf));
    check(recvmsg(sv[1], &msg, MSG_CMSG_CLOEXEC) == 1, "receive with MSG_CMSG_CLOEXEC");
    cmsg = CMSG_FIRSTHDR(&msg);
    check(cmsg != NULL, "a control message");
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    check(fcntl(fd, F_GETFD) & FD_CLOEXEC, "FD_CLOEXEC");
    check(close(fd) == 0, "close");
    /* Too little control space */
    send_fds(sv[0], pfds, 2);
    msg = header(iov, 1, control.buf, CMSG_LEN(sizeof(int)));
    check(recvmsg(sv[1], &msg, 0) == 1, "receive two descriptors in space for one");
    /* QEMU's user-mode emulation drops the flag as it converts the control data */
    check((msg.msg_flags & MSG_CTRUNC) || is_qemu(), "MSG_CTRUNC");
    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg) {
        size_t n = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);

        while (n-- > 0) {
            memcpy(&fd, CMSG_DATA(cmsg) + n * sizeof(int), sizeof(int));
            check(close(fd) == 0, "close");
        }
    }
    check(close(pfds[0]) == 0 && close(pfds[1]) == 0, "close pipe");

    /* An empty or oversized array; Linux, and so glibc, accepts an empty one */
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
#ifndef __GLIBC__
    msg = header(iov, 0, NULL, 0);
    errno = 0;
    check(recvmsg(sv[1], &msg, MSG_DONTWAIT) == -1 && errno == EMSGSIZE, "no buffers");
    msg = header(iov, -1, NULL, 0);
    errno = 0;
    check(recvmsg(sv[1], &msg, MSG_DONTWAIT) == -1 && errno == EMSGSIZE, "negative count");
    /* Linux shortens the transfer, and glibc passes that on */
    iov[0].iov_base = big;
    iov[0].iov_len = (size_t)SSIZE_MAX / 2 + 1;
    iov[1].iov_base = big;
    iov[1].iov_len = (size_t)SSIZE_MAX / 2 + 1;
    msg = header(iov, 2, NULL, 0);
    errno = 0;
    check(recvmsg(sv[1], &msg, MSG_DONTWAIT) == -1 && errno == EINVAL, "total overflows");
#else
    (void)big;
#endif
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    msg = header(iov, 1, NULL, 0);
    errno = 0;
    check(recvmsg(sv[1], &msg, MSG_DONTWAIT) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "EAGAIN");
    /* A blocking receive waits until a signal interrupts it */
    interrupter = start_interrupts();
    errno = 0;
    check(recvmsg(sv[1], &msg, 0) == -1 && errno == EINTR, "EINTR");
    stop_interrupts(interrupter);
    /* An orderly shutdown gives 0 */
    check(close(sv[0]) == 0, "close");
    check(recvmsg(sv[1], &msg, 0) == 0, "0 at the end");
    check(close(sv[1]) == 0, "close");

    /* Datagrams: the source address, and truncation */
    sa = make_socket(AF_INET, SOCK_DGRAM);
    sb = make_socket(AF_INET, SOCK_DGRAM);
    len = sizeof(a);
    check(bind(sa, (struct sockaddr *)&a, sizeof(a)) == 0
              && getsockname(sa, (struct sockaddr *)&a, &len) == 0,
          "bind a");
    len = sizeof(b);
    check(bind(sb, (struct sockaddr *)&b, sizeof(b)) == 0
              && getsockname(sb, (struct sockaddr *)&b, &len) == 0,
          "bind b");
    check(sendto(sa, "datagram", 8, 0, (struct sockaddr *)&b, sizeof(b)) == 8
              && sendto(sa, "next", 4, 0, (struct sockaddr *)&b, sizeof(b)) == 4,
          "sendto");
    iov[0].iov_base = buf;
    iov[0].iov_len = 4;
    msg = header(iov, 1, NULL, 0);
    msg.msg_name = &from;
    msg.msg_namelen = sizeof(from);
    check(recvmsg(sb, &msg, 0) == 4 && memcmp(buf, "data", 4) == 0, "truncated datagram");
    check(msg.msg_flags & MSG_TRUNC, "MSG_TRUNC");
    check(msg.msg_namelen == sizeof(from) && from.sin_family == AF_INET
              && from.sin_port == a.sin_port && from.sin_addr.s_addr == a.sin_addr.s_addr,
          "source address");
    iov[0].iov_len = sizeof(buf);
    msg = header(iov, 1, NULL, 0);
    check(recvmsg(sb, &msg, 0) == 4 && memcmp(buf, "next", 4) == 0, "the rest was discarded");
    check((msg.msg_flags & MSG_TRUNC) == 0, "no MSG_TRUNC");
    check(close(sa) == 0 && close(sb) == 0, "close");

    /* Out-of-band data is flagged */
    tcp_pair(&client, &server);
    errno = 0;
    iov[0].iov_len = 1;
    msg = header(iov, 1, NULL, 0);
    check(recvmsg(server, &msg, MSG_OOB) == -1 && errno == EINVAL, "no out-of-band data");
    check(send(client, "a", 1, 0) == 1 && send(client, "!", 1, MSG_OOB) == 1, "send");
    check(recv(server, buf, 1, MSG_WAITALL) == 1, "normal data");
    while (sockatmark(server) == 0)
        ;
    msg = header(iov, 1, NULL, 0);
    check(recvmsg(server, &msg, MSG_OOB) == 1 && buf[0] == '!', "out-of-band byte");
    check(msg.msg_flags & MSG_OOB, "MSG_OOB");
    check(close(client) == 0 && close(server) == 0, "close");
    /* Not connected */
    fd = make_socket(AF_INET, SOCK_STREAM);
    msg = header(iov, 1, NULL, 0);
    errno = 0;
    check(recvmsg(fd, &msg, 0) == -1 && errno == ENOTCONN, "ENOTCONN");
    check(close(fd) == 0, "close");
    check(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) == 0, "socketpair");
    errno = 0;
    check(recvmsg(sv[0], &msg, MSG_OOB) == -1 && errno == EOPNOTSUPP, "EOPNOTSUPP");
    check(close(sv[0]) == 0 && close(sv[1]) == 0, "close");

    check(pipe(pfds) == 0, "pipe");
    errno = 0;
    check(recvmsg(pfds[0], &msg, 0) == -1 && errno == ENOTSOCK, "ENOTSOCK");
    check(close(pfds[0]) == 0 && close(pfds[1]) == 0, "close");
    errno = 0;
    check(recvmsg(-1, &msg, 0) == -1 && errno == EBADF, "EBADF");
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
