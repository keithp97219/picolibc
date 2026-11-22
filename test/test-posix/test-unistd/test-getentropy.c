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
/* glibc declares getentropy only with _GNU_SOURCE (or _DEFAULT_SOURCE) */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_C_LIB_EXT >= 202405L && TEST_POSIX_SINGLE_PROCESS

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* getentropy fills exactly the length asked, up to GETENTROPY_MAX */

#ifndef GETENTROPY_MAX /* glibc doesn't define it; its limit is 256 */
#define GETENTROPY_MAX 256
#endif

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

static unsigned char buf[GETENTROPY_MAX + 16], buf2[GETENTROPY_MAX + 16];

int
main(void)
{
    size_t i, zero;

    /* Nothing written for a zero length */
    memset(buf, 0xa5, sizeof(buf));
    check(getentropy(buf, 0) == 0, "zero length");
    check(buf[0] == 0xa5, "nothing written");

    /* Exactly length bytes, and different each time */
    memset(buf, 0xa5, sizeof(buf));
    check(getentropy(buf, GETENTROPY_MAX) == 0, "GETENTROPY_MAX bytes");
    for (i = GETENTROPY_MAX; i < sizeof(buf); i++)
        check(buf[i] == 0xa5, "nothing past the length");
    memset(buf2, 0xa5, sizeof(buf2));
    check(getentropy(buf2, GETENTROPY_MAX) == 0, "again");
    check(memcmp(buf, buf2, GETENTROPY_MAX) != 0, "different data");
    for (zero = 0, i = 0; i < GETENTROPY_MAX; i++)
        zero += buf[i] == 0xa5;
    check(zero < GETENTROPY_MAX / 4, "the buffer was written");

    /* EINVAL past GETENTROPY_MAX, writing nothing */
    memset(buf, 0xa5, sizeof(buf));
    errno = 0;
    check(getentropy(buf, GETENTROPY_MAX + 1) == -1, "GETENTROPY_MAX + 1 fails");
#ifndef __GLIBC__ /* glibc reports EIO */
    check(errno == EINVAL, "EINVAL past GETENTROPY_MAX");
#endif
    for (i = 0; i < sizeof(buf); i++)
        check(buf[i] == 0xa5, "nothing written on failure");

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
