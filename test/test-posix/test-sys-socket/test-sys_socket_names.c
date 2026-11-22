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

/* <sys/socket.h>, <sys/uio.h>, <poll.h> and <sys/select.h> keep to their names */

#define _POSIX_C_SOURCE 202405L
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_NETWORKING

#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/uio.h>

/*
 * In strict POSIX mode these headers may only define their own names
 * (and, for <poll.h> and <sys/select.h>, those of <signal.h> and
 * <time.h>). The byte-order names belong to <endian.h>, and NFDBITS is
 * a BSD extension (the fd_, FD_ and POLL prefixes are reserved).
 */
#if defined(BYTE_ORDER) || defined(LITTLE_ENDIAN) || defined(BIG_ENDIAN) || defined(NFDBITS)
#error "extension macros visible in strict POSIX mode"
#endif

/* Each would conflict with a definition the headers shouldn't make */
static int
htobe16(int a)
{
    return a + 1;
}

static int
be32toh(int a)
{
    return a + 2;
}

static int
le64toh(int a)
{
    return a + 3;
}

#include <stdio.h>

int
main(void)
{
    if (htobe16(0) + be32toh(0) + le64toh(0) != 6) {
        printf("wrong functions called\n");
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
