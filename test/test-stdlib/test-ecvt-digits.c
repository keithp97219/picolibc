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
 * ecvt_r, ecvtf_r and ecvtl_r asked for more digits than the
 * conversion engine generates: the engine's digits followed by
 * zeros, ndigit of them, without overrunning the engine's buffer
 * or the caller's.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_NDIGIT 120
#define BUF_LEN    (MAX_NDIGIT + 16)
#define GUARD      '#'

static int errors;

static void
check(const char *name, int ndigit, int ret, int decpt, int sign, const char *buf)
{
    int i;

    if (ret != 0 || decpt != 0 || sign != 0) {
        printf("%s ndigit %d: ret %d decpt %d sign %d\n", name, ndigit, ret, decpt, sign);
        errors++;
        return;
    }
    /* 0.125 exactly, then zeros */
    if (strncmp(buf, "125", 3) != 0)
        goto bad;
    for (i = 3; i < ndigit; i++)
        if (buf[i] != '0')
            goto bad;
    if (buf[ndigit] != '\0')
        goto bad;
    for (i = ndigit + 1; i < BUF_LEN; i++)
        if (buf[i] != GUARD)
            goto bad;
    return;
bad:
    printf("%s ndigit %d: got \"%.*s\"\n", name, ndigit, BUF_LEN, buf);
    errors++;
}

int
main(void)
{
    char buf[BUF_LEN];
    int  ndigit, decpt, sign, ret;

    for (ndigit = 3; ndigit <= MAX_NDIGIT; ndigit++) {
        memset(buf, GUARD, sizeof(buf));
        ret = ecvt_r(0.125, ndigit, &decpt, &sign, buf, (size_t)ndigit + 1);
        check("ecvt_r", ndigit, ret, decpt, sign, buf);

        memset(buf, GUARD, sizeof(buf));
        ret = ecvtf_r(0.125f, ndigit, &decpt, &sign, buf, (size_t)ndigit + 1);
        check("ecvtf_r", ndigit, ret, decpt, sign, buf);

        memset(buf, GUARD, sizeof(buf));
        ret = ecvtl_r(0.125L, ndigit, &decpt, &sign, buf, (size_t)ndigit + 1);
        check("ecvtl_r", ndigit, ret, decpt, sign, buf);
    }
    if (errors)
        return 1;
    printf("success\n");
    return 0;
}
