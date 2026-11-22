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
/* crypt, encrypt and setkey are XSI interfaces */
#define _XOPEN_SOURCE 800
#include <unistd.h>
#include "../test-flags.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * crypt returns a hash starting with the salt, or a null pointer with
 * errno set (ENOSYS when unsupported); encrypt and setkey leave errno
 * alone on success and set ENOSYS when unsupported, and encrypt only
 * ever produces 0 and 1 bytes. _XOPEN_CRYPT says whether the encryption
 * group is there.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    char  key[64], block[64], orig[64];
    char *hash;
    int   i;

    errno = 0;
    hash = crypt("password", "ab");
    if (hash == NULL) {
        check(errno != 0, "crypt sets errno");
#if TEST_XOPEN_CRYPT
        check(errno != ENOSYS, "crypt is supported");
#endif
    } else {
        check(strncmp(hash, "ab", 2) == 0, "the hash starts with the salt");
    }

    for (i = 0; i < 64; i++) {
        key[i] = (char)(i & 1);
        block[i] = orig[i] = (char)((i >> 1) & 1);
    }
    errno = 0;
    setkey(key);
    check(errno == 0 || errno == ENOSYS, "setkey errno");
    if (errno == 0) {
        encrypt(block, 0);
        check(errno == 0 || errno == ENOSYS, "encrypt errno");
        for (i = 0; i < 64; i++)
            check(block[i] == 0 || block[i] == 1, "encrypt gives bits");
        /* Decoding may be unsupported, with ENOSYS */
        encrypt(block, 1);
        if (errno == 0)
            check(memcmp(block, orig, 64) == 0, "decoding restores the block");
    } else {
        errno = 0;
        encrypt(block, 0);
        check(errno == ENOSYS, "encrypt without setkey support");
        check(memcmp(block, orig, 64) == 0 || errno != 0, "block");
    }
#if TEST_XOPEN_CRYPT
    check(errno != ENOSYS, "the encryption group is supported");
#endif
    printf("success\n");
    return 0;
}
