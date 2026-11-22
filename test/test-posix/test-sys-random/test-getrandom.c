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

#define _DEFAULT_SOURCE
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FD_MGMT && (defined(__PICOLIBC__) || defined(TEST_GLIBC))

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>

/*
 * getrandom (not POSIX; Linux, FreeBSD, glibc, musl): fills the
 * buffer, takes GRND_NONBLOCK, rejects unknown flags
 */

int
main(void)
{
    unsigned char a[64], b[64];
    ssize_t       n;

    memset(a, 0, sizeof(a));
    memset(b, 0, sizeof(b));
    n = getrandom(a, sizeof(a), 0);
    if (n != (ssize_t)sizeof(a)) {
        printf("getrandom returned %zd\n", n);
        return 1;
    }
    n = getrandom(b, sizeof(b), GRND_NONBLOCK);
    if (n != (ssize_t)sizeof(b) || memcmp(a, b, sizeof(a)) == 0) {
        printf("GRND_NONBLOCK: %zd, or the same bytes twice\n", n);
        return 1;
    }
    errno = 0;
    if (getrandom(a, sizeof(a), 0x40000000) != -1 || errno != EINVAL) {
        printf("unknown flags should be EINVAL\n");
        return 1;
    }
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
