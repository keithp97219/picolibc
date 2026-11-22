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
#if TEST_POSIX_NETWORKING && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_PIPE \
    && TEST_POSIX_SYMBOLIC_LINKS && TEST_POSIX_USER_GROUPS

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

/* bind assigns addresses, including ephemeral ports */

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

/* A fresh directory for the AF_UNIX names */
static char dir[] = "/tmp/test-bind-XXXXXX";

static char *
in_dir(const char *name)
{
    static char path[sizeof(dir) + 32];

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    return path;
}

/* An AF_UNIX address for the name in the directory */
static struct sockaddr_un
local_name(const char *name)
{
    struct sockaddr_un sun;

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    check(strlen(in_dir(name)) < sizeof(sun.sun_path), "short name");
    strcpy(sun.sun_path, in_dir(name));
    return sun;
}

/* bind an AF_UNIX stream socket to the name; returns the result and keeps errno */
static int
bind_local(const char *name)
{
    struct sockaddr_un sun = local_name(name);
    int                fd = make_socket(AF_UNIX, SOCK_STREAM), ret, err;

    ret = bind(fd, (struct sockaddr *)&sun, sizeof(sun));
    err = errno;
    check(close(fd) == 0, "close");
    errno = err;
    return ret;
}

/* Whether the kernel lets programs bind to addresses that aren't local */
static int
nonlocal_bind(void)
{
    FILE *f = fopen("/proc/sys/net/ipv4/ip_nonlocal_bind", "r");
    int   c = '0';

    if (f) {
        c = getc(f);
        fclose(f);
    }
    return c == '1';
}

int
main(void)
{
    struct sockaddr_in  sin = loopback(), other;
    struct sockaddr_in6 sin6;
    struct sockaddr_un  sun, got;
    struct stat         st;
    socklen_t           len = sizeof(sin);
    int a = make_socket(AF_INET, SOCK_STREAM), b = make_socket(AF_INET, SOCK_STREAM);
    int fd6, fd, fds[2];

    check(bind(a, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    check(getsockname(a, (struct sockaddr *)&sin, &len) == 0 && ntohs(sin.sin_port) != 0,
          "ephemeral port");
    check(listen(a, 1) == 0, "listen");
    check(bind(b, (struct sockaddr *)&sin, sizeof(sin)) == -1 && errno == EADDRINUSE, "EADDRINUSE");
    /* Already bound */
    other = loopback();
    errno = 0;
    check(bind(a, (struct sockaddr *)&other, sizeof(other)) == -1 && errno == EINVAL,
          "already bound");
    check(close(a) == 0 && close(b) == 0, "close");

    /* An address of another family */
    a = make_socket(AF_INET, SOCK_DGRAM);
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    errno = 0;
    check(bind(a, (struct sockaddr *)&sin6, sizeof(sin6)) == -1 && errno == EAFNOSUPPORT,
          "EAFNOSUPPORT");
    /* An address this machine doesn't have (TEST-NET-1) */
    if (!nonlocal_bind()) {
        other = loopback();
        other.sin_addr.s_addr = htonl(0xc0000201);
        errno = 0;
        check(bind(a, (struct sockaddr *)&other, sizeof(other)) == -1 && errno == EADDRNOTAVAIL,
              "EADDRNOTAVAIL");
    }
    check(close(a) == 0, "close");
    check(pipe(fds) == 0, "pipe");
    sin = loopback();
    errno = 0;
    check(bind(fds[0], (struct sockaddr *)&sin, sizeof(sin)) == -1 && errno == ENOTSOCK,
          "ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(bind(-1, (struct sockaddr *)&sin, sizeof(sin)) == -1 && errno == EBADF, "EBADF");

    fd6 = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd6 >= 0) {
        memset(&sin6, 0, sizeof(sin6));
        sin6.sin6_family = AF_INET6;
        sin6.sin6_addr = in6addr_loopback;
        /* IPv6 may be present but not configured */
        if (bind(fd6, (struct sockaddr *)&sin6, sizeof(sin6)) == 0) {
            len = sizeof(sin6);
            check(getsockname(fd6, (struct sockaddr *)&sin6, &len) == 0, "IPv6 name");
            check(IN6_IS_ADDR_LOOPBACK(&sin6.sin6_addr), "IPv6 loopback");
        }
        check(close(fd6) == 0, "close IPv6");
    }

    /* AF_UNIX names are files */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    fd = make_socket(AF_UNIX, SOCK_STREAM);
    sun = local_name("sock");
    check(bind(fd, (struct sockaddr *)&sun, sizeof(sun)) == 0, "bind a name");
    check(stat(in_dir("sock"), &st) == 0 && S_ISSOCK(st.st_mode), "the name is a socket");
    len = sizeof(got);
    check(getsockname(fd, (struct sockaddr *)&got, &len) == 0 && got.sun_family == AF_UNIX
              && strcmp(got.sun_path, sun.sun_path) == 0,
          "bound name");
    errno = 0;
    sun = local_name("other");
    check(bind(fd, (struct sockaddr *)&sun, sizeof(sun)) == -1 && errno == EINVAL,
          "already bound to a name");
    /* Linux 5.10 leaves the second name behind */
    (void)unlink(in_dir("other"));
    check(close(fd) == 0, "close");
    /* An existing file, even a dangling symbolic link, is in use */
    check(bind_local("sock") == -1 && errno == EADDRINUSE, "socket file in use");
    fd = open(in_dir("file"), O_WRONLY | O_CREAT | O_EXCL, 0600);
    check(fd >= 0 && close(fd) == 0, "create a file");
    check(bind_local("file") == -1 && errno == EADDRINUSE, "regular file in use");
    check(symlink("nothing", in_dir("dangling")) == 0, "symlink");
    check(bind_local("dangling") == -1 && errno == EADDRINUSE, "dangling link in use");
    /* Path errors */
    check(bind_local("missing/sock") == -1 && errno == ENOENT, "ENOENT");
    check(bind_local("file/sock") == -1 && errno == ENOTDIR, "ENOTDIR");
    check(bind_local("missing/") == -1 && (errno == ENOENT || errno == ENOTDIR), "trailing slash");
    /* The name exists, so it may also be in use */
    check(bind_local("file/") == -1 && (errno == ENOTDIR || errno == EADDRINUSE),
          "trailing slash on a file");
    check(symlink("loop", in_dir("loop")) == 0, "symlink loop");
    check(bind_local("loop/sock") == -1 && errno == ELOOP, "ELOOP");
    check(mkdir(in_dir("ro"), 0500) == 0, "mkdir");
    /* Privileges override the directory's mode */
    if (geteuid() != 0)
        check(bind_local("ro/sock") == -1 && errno == EACCES, "EACCES");
    /* No address; Linux says EINVAL or EFAULT, as glibc reports */
#ifndef __GLIBC__
    fd = make_socket(AF_UNIX, SOCK_STREAM);
    errno = 0;
    check(bind(fd, NULL, 0) == -1 && (errno == EDESTADDRREQ || errno == EISDIR), "null address");
    errno = 0;
    check(bind(fd, NULL, sizeof(sun)) == -1 && (errno == EDESTADDRREQ || errno == EISDIR),
          "null address with a length");
    check(close(fd) == 0, "close");
#endif
    check(unlink(in_dir("sock")) == 0 && unlink(in_dir("file")) == 0
              && unlink(in_dir("dangling")) == 0 && unlink(in_dir("loop")) == 0
              && rmdir(in_dir("ro")) == 0 && rmdir(dir) == 0,
          "clean up");
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
