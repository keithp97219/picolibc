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
 * picolibc's getentropy without an entropy source: __splitmix64 seeded
 * from the stack address. The result differs between calls, which is
 * enough for the stack canary, but it is predictable and not suitable
 * for cryptography. Applications without <sys/_posix.h> reach it as
 * __baremetal_getentropy; it's also the weak getentropy an OS
 * overrides, and what an OS without an entropy source can wrap. It
 * takes no lock: arc4random calls it holding the libc lock, and racing
 * threads at worst see repeated bits from a generator that is already
 * predictable.
 */
#define __PICOLIBC_BAREMETAL_IMPL
#define _DEFAULT_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint64_t __baremetal_entropy_state;

int
__baremetal_getentropy(void *buffer, size_t length)
{
    uint8_t *b = buffer;

    if (length > GETENTROPY_MAX) {
        errno = EINVAL;
        return -1;
    }

    if (__baremetal_entropy_state == 0)
        __baremetal_entropy_state = (uint64_t)(uintptr_t)&b;
    while (length) {
        uint64_t bits = __splitmix64(&__baremetal_entropy_state);
        size_t   this_time = sizeof(bits);

        if (this_time > length)
            this_time = length;
        memcpy(b, &bits, this_time);
        length -= this_time;
        b += this_time;
    }
    return 0;
}

#if defined(__weak_reference) && !defined(__FALLBACK_HELPERS_ONLY)
__weak_reference(__baremetal_getentropy, getentropy);
#elif !defined(__FALLBACK_HELPERS_ONLY)
int
getentropy(void *buffer, size_t length)
{
    return __baremetal_getentropy(buffer, length);
}
#endif
