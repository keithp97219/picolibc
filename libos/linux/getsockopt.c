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

#include "local-socket.h"
#include <sys/time.h>
#include <string.h>

int
getsockopt(int fd, int level, int name, void * __restrict value, socklen_t * __restrict len)
{
#if __SIZEOF_LONG__ != 8
    /* The kernel's 64-bit timeouts have a 64-bit tv_usec */
    if (level == SOL_SOCKET && (name == SO_RCVTIMEO || name == SO_SNDTIMEO)) {
        struct timeval               tv;
        struct __kernel_sock_timeval ktv = { 0, 0 };
        socklen_t                    klen = sizeof(ktv);

        if (_syscall(LINUX_SYS_getsockopt, fd, level, name, &ktv, &klen) < 0)
            return -1;
        tv.tv_sec = ktv.tv_sec;
        tv.tv_usec = (suseconds_t)ktv.tv_usec;
        /* A shorter buffer gets the start of the value */
        if (*len > sizeof(tv))
            *len = sizeof(tv);
        memcpy(value, &tv, *len);
        return 0;
    }
#endif
    return (int)_syscall(LINUX_SYS_getsockopt, fd, level, name, value, len);
}
