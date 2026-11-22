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

/*
 * <sys/ioctl.h> (not POSIX): ioctl is variadic, as on every other
 * system, and the prototype's parameter names are reserved ones,
 * so a program's macros named like them (gawk has "param") don't
 * break it. It defines struct winsize for TIOCGWINSZ, as glibc's does.
 */
#define fd  \
    do      \
    not use
#define op  \
    do      \
    not use
#define param \
    do        \
    not use
#define arg \
    do      \
    not use
#define request \
    do          \
    not use
#include <sys/ioctl.h>
#undef fd
#undef op
#undef param
#undef arg
#undef request

#include <errno.h>
#include <stdio.h>

/* Without the system support this test needs, it is skipped: ioctl is the OS layer's */
#if TEST_POSIX_FD_MGMT

int
main(void)
{
    struct winsize ws;

    /* With the pointer the request needs, and without a third argument */
    if (ioctl(-1, TIOCGWINSZ, &ws) != -1) {
        printf("ioctl on a bad descriptor succeeded\n");
        return 1;
    }
    (void)ioctl(-1, TIOCGWINSZ);
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    struct winsize ws = { .ws_row = 24, .ws_col = 80 };

    (void)ws;
    printf("no system support, skipping\n");
    return 77;
}

#endif
