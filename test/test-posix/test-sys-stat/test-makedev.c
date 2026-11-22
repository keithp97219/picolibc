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

#include <stdio.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>

/*
 * makedev, major and minor (<sys/sysmacros.h>, not POSIX): they take
 * apart what they put together, for 32-bit majors and minors, and
 * agree with the device numbers stat reports (/dev/null is 1,3 on
 * Linux)
 */

int
main(void)
{
    static const unsigned int vals[]
        = { 0, 1, 3, 0xff, 0x100, 0xfff, 0x1000, 0xfffff, 0x123456, 0xffffffffu };
    unsigned int i, j;
    int          ret = 0;

    for (i = 0; i < sizeof(vals) / sizeof(vals[0]); i++)
        for (j = 0; j < sizeof(vals) / sizeof(vals[0]); j++) {
            dev_t d = makedev(vals[i], vals[j]);

            if (major(d) != vals[i] || minor(d) != vals[j]) {
                printf("makedev(%#x, %#x): major %#x minor %#x\n", vals[i], vals[j], major(d),
                       minor(d));
                ret = 1;
            }
        }

#if TEST_POSIX_FILE_SYSTEM
    {
        struct stat st;

        if (stat("/dev/null", &st) == 0) {
            if (!S_ISCHR(st.st_mode) || st.st_rdev == 0) {
                printf("/dev/null: not a character device with a number\n");
                ret = 1;
            }
#ifdef __linux__
            if (major(st.st_rdev) != 1 || minor(st.st_rdev) != 3) {
                printf("/dev/null is %u,%u, not 1,3\n", major(st.st_rdev), minor(st.st_rdev));
                ret = 1;
            }
#endif
        }
    }
#endif

    if (ret == 0)
        printf("success\n");
    return ret;
}
