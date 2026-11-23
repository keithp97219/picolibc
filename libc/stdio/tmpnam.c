/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2022 Keith Packard
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

#include "local-stdio.h"

#ifdef __GNUCLIKE_PRAGMA_DIAGNOSTIC
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/* No requirement for re-entrancy for this variable */
static char _tmpnam[L_tmpnam];

/*
 * C requires a different string from each call, so rather than six
 * random characters (which repeat within TMP_MAX calls about 2% of
 * the time), step through the 36**6 names in an order set by a
 * per-process start and a stride coprime to 36.
 */
#define TMPNAM_NAMES  (36UL * 36UL * 36UL * 36UL * 36UL * 36UL)
#define TMPNAM_STRIDE 1220703125UL /* 5**13 */

static uint32_t _tmpnam_next;
static bool     _tmpnam_seeded;

char *
tmpnam(char *s)
{
    char    *start;
    uint32_t attempt;

    /* Fail the build if L_tmpnam can't hold the name. */
    _Static_assert(L_tmpnam >= sizeof(P_tmpdir "TXXXXXX"), "L_tmpnam too small for P_tmpdir");

    if (!s)
        s = _tmpnam;

    strcpy(s, P_tmpdir "T");
    start = s + sizeof(P_tmpdir "T") - 1;

    for (attempt = 0; attempt < TMPNAM_NAMES; attempt++) {
        uint32_t bits;
        int      i;
        int      fd;

        __LIBC_LOCK();
        if (!_tmpnam_seeded) {
            uint64_t seed = (uint64_t)(uintptr_t)&bits ^ 0x2545f4914f6cdd1dULL;
            _tmpnam_next = (uint32_t)(__splitmix64(&seed) % TMPNAM_NAMES);
            _tmpnam_seeded = true;
        }
        bits = _tmpnam_next;
        _tmpnam_next = (uint32_t)((_tmpnam_next + TMPNAM_STRIDE) % TMPNAM_NAMES);
        __LIBC_UNLOCK();

        for (i = 0; i < 6; i++) {
            char c = (char)(bits % 36);
            bits /= 36;
            start[i] = c < 10 ? c + '0' : c + 'a' - 10;
        }
        start[6] = '\0';

        /* Skip names in use, the same test mktemp makes */
        fd = open(s, O_RDONLY | O_NONBLOCK | O_NOCTTY);
        if (fd >= 0) {
            close(fd);
            continue;
        }
        if (errno == EACCES)
            continue;
        if (errno != ENOENT)
            return NULL;
        return s;
    }
    errno = EEXIST;
    return NULL;
}
