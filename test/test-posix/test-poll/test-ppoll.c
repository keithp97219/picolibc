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

#define _GNU_SOURCE /* glibc declares ppoll only for _GNU_SOURCE */
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* ppoll reports a readable pipe, times out and rejects a bad timeout */

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
    int             fds[2];
    struct pollfd   pfd[2];
    struct timespec ts;
    sigset_t        mask;
    char            c;

    check(pipe(fds) == 0, "pipe");
    check(write(fds[1], "x", 1) == 1, "write");

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    pfd[0].fd = fds[1];
    pfd[0].events = POLLIN;
    pfd[1].fd = fds[0];
    pfd[1].events = POLLIN;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    check(ppoll(pfd, 2, &ts, &mask) == 1 && (pfd[1].revents & POLLIN), "ppoll ready");

    check(read(fds[0], &c, 1) == 1 && c == 'x', "read");
    ts.tv_nsec = 20 * 1000 * 1000;
    check(ppoll(&pfd[1], 1, &ts, NULL) == 0 && pfd[1].revents == 0, "ppoll times out");
    ts.tv_nsec = -1;
    errno = 0;
    check(ppoll(&pfd[1], 1, &ts, NULL) == -1 && errno == EINVAL, "ppoll bad timeout");

    printf("success\n");
    return 0;
}
