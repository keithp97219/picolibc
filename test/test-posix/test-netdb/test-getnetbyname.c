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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                       \
    || (defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING > 0)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* getnetbyname, getnetbyaddr and getnetent read the networks database */

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

#define MAX_FDS 64

/* Records which descriptors are open */
static void
open_fds(unsigned char *open)
{
    int fd;

    for (fd = 0; fd < MAX_FDS; fd++)
        open[fd] = fcntl(fd, F_GETFD) >= 0;
}

/* Counts the descriptors opened since 'before', failing if any lacks FD_CLOEXEC */
static int
new_fds(const unsigned char *before)
{
    int fd, n = 0;

    for (fd = 0; fd < MAX_FDS; fd++) {
        int flags = fcntl(fd, F_GETFD);

        if (flags < 0 || before[fd])
            continue;
        check(flags & FD_CLOEXEC, "FD_CLOEXEC");
        n++;
    }
    return n;
}

int
main(void)
{
    struct netent *n;

    /* Not every system lists any networks */
    setnetent(0);
    while ((n = getnetent()) != NULL) {
        struct netent copy = *n;
        char          name[64];

        check(n->n_addrtype == AF_INET, "address type");
        snprintf(name, sizeof(name), "%s", n->n_name);
        n = getnetbyname(name);
        check(n && n->n_net == copy.n_net, "getnetbyname");
        n = getnetbyaddr(copy.n_net, AF_INET);
        check(n != NULL, "getnetbyaddr");
        break;
    }
    endnetent();
    check(getnetbyname("no-such-network-here") == NULL, "missing");

    /* The network number is in host byte order */
    n = getnetbyname("loopback");
    if (n)
        check(n->n_net == 127 || n->n_net == 0x7f000000, "loopback number");

    /* The address type must match too */
    setnetent(0);
    n = getnetent();
    if (n) {
        uint32_t net = n->n_net;

        check(getnetbyaddr(net, AF_INET6) == NULL, "address type");
    }

    /* setnetent and endnetent start again from the first entry, with FD_CLOEXEC */
    {
        unsigned char before[MAX_FDS];
        char          first[256];

        endnetent();
        open_fds(before);
        setnetent(1);
        n = getnetent();
        if (n) {
            snprintf(first, sizeof(first), "%s", n->n_name);
            (void)getnetent();
            (void)new_fds(before);
            setnetent(0);
            n = getnetent();
            check(n && strcmp(n->n_name, first) == 0, "setnetent rewinds");
            endnetent();
            check(new_fds(before) == 0, "endnetent closes");
            n = getnetent();
            check(n && strcmp(n->n_name, first) == 0, "endnetent rewinds");
        }
        endnetent();
    }
    (void)make_socket;
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
