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
#if defined(__PICOLIBC_POSIX_NETWORKING)

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
#include <sys/stat.h>

/* writev writes the buffers in order */

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

#include <sys/uio.h>

int
main(void)
{
    static char  big[16];
    char         path[] = "/tmp/test-writev-XXXXXX";
    struct iovec iov[3], *many;
    struct stat  st;
    char         buf[16];
    int          fds[2], fd, i;
    long         iov_max = sysconf(_SC_IOV_MAX);
    volatile int negative = -1;

    check(pipe(fds) == 0, "pipe");
    iov[0].iov_base = "one ";
    iov[0].iov_len = 4;
    iov[1].iov_base = "";
    iov[1].iov_len = 0;
    iov[2].iov_base = "two";
    iov[2].iov_len = 3;
    check(writev(fds[1], iov, 3) == 7, "writev");
    check(read(fds[0], buf, sizeof(buf)) == 7 && memcmp(buf, "one two", 7) == 0, "contents");
    check(writev(-1, iov, 3) == -1 && errno == EBADF, "EBADF");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    /* A regular file */
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp");
    check(writev(fd, iov, 3) == 7 && lseek(fd, 0, SEEK_CUR) == 7, "file writev");
    /* Nothing to write: 0 and no other effect */
    iov[0].iov_len = 0;
    iov[2].iov_len = 0;
    check(writev(fd, iov, 3) == 0, "empty writev");
    check(lseek(fd, 0, SEEK_CUR) == 7 && fstat(fd, &st) == 0 && st.st_size == 7, "no effect");
    /* Bad counts */
    /* volatile, as the compiler rejects a constant negative count */
    errno = 0;
    check(writev(fd, iov, negative) == -1 && errno == EINVAL, "negative count");
    /* IOV_MAX is an XSI name in <limits.h> */
    if (iov_max > 0) {
        many = calloc((size_t)iov_max + 1, sizeof(*many));
        check(many != NULL, "calloc");
        for (i = 0; i <= iov_max; i++) {
            many[i].iov_base = "m";
            many[i].iov_len = 1;
        }
        errno = 0;
        check(writev(fd, many, (int)iov_max + 1) == -1 && errno == EINVAL, "more than IOV_MAX");
        check(writev(fd, many, (int)iov_max) == iov_max, "IOV_MAX buffers");
        check(ftruncate(fd, 7) == 0 && lseek(fd, 7, SEEK_SET) == 7, "trim");
        free(many);
    }
    /* Too much in total: nothing is written; Linux shortens it, and glibc passes that on */
#ifndef __GLIBC__
    iov[0].iov_base = big;
    iov[0].iov_len = (size_t)SSIZE_MAX / 2 + 1;
    iov[1].iov_base = big;
    iov[1].iov_len = (size_t)SSIZE_MAX / 2 + 1;
    errno = 0;
    check(writev(fd, iov, 2) == -1 && errno == EINVAL, "total overflows");
    check(lseek(fd, 0, SEEK_CUR) == 7 && fstat(fd, &st) == 0 && st.st_size == 7, "nothing written");
#else
    (void)big;
#endif
    check(close(fd) == 0 && unlink(path) == 0, "clean up");
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
