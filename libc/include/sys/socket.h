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

#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <sys/cdefs.h>
#include <sys/uio.h>
/* The types and values come from the system's network stack */
#if defined(__has_include)
#if !__has_include(<sys/_socket.h>)
#error "<sys/socket.h> needs networking, which this system doesn't provide"
#endif
#endif
#include <sys/_socket.h>

_BEGIN_STD_C

int     accept(int, struct sockaddr     *__restrict, socklen_t     *__restrict) __picolibc_export;
int     accept4(int, struct sockaddr     *__restrict, socklen_t     *__restrict, int) __picolibc_export;
int     bind(int, const struct sockaddr *, socklen_t) __picolibc_export;
int     connect(int, const struct sockaddr *, socklen_t) __picolibc_export;
int     getpeername(int, struct sockaddr     *__restrict, socklen_t     *__restrict) __picolibc_export;
int     getsockname(int, struct sockaddr     *__restrict, socklen_t     *__restrict) __picolibc_export;
int     getsockopt(int, int, int, void     *__restrict, socklen_t     *__restrict) __picolibc_export;
int     listen(int, int) __picolibc_export;
ssize_t recv(int, void *, size_t, int) __picolibc_export;
ssize_t recvfrom(int, void * __restrict, size_t, int, struct sockaddr * __restrict,
                 socklen_t * __restrict) __picolibc_export;
ssize_t recvmsg(int, struct msghdr *, int) __picolibc_export;
ssize_t send(int, const void *, size_t, int) __picolibc_export;
ssize_t sendmsg(int, const struct msghdr *, int) __picolibc_export;
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t)
    __picolibc_export;
int setsockopt(int, int, int, const void *, socklen_t) __picolibc_export;
int shutdown(int, int) __picolibc_export;
int sockatmark(int) __picolibc_export;
int socket(int, int, int) __picolibc_export;
int socketpair(int, int, int, int[2]) __picolibc_export;

_END_STD_C

#endif /* _SYS_SOCKET_H_ */
