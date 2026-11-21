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
/* glibc declares reallocarray only with _DEFAULT_SOURCE */
#ifdef TEST_GLIBC
#define _DEFAULT_SOURCE
#endif
#include <stdlib.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * The allocation functions' POSIX error rules: ENOMEM when memory
 * can't be had (including calloc and reallocarray overflow), EINVAL
 * for bad alignments and for realloc(ptr, 0), the old object kept when
 * realloc fails, and free leaving errno alone. The results are kept in
 * volatile objects so that the compiler can't drop the calls.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

/* Too big for any address space; volatile, so the compiler doesn't object */
static volatile size_t huge = SIZE_MAX - 4096;
static volatile size_t half = SIZE_MAX / 2;
/* Not a power of 2, through a volatile object for the same reason */
static volatile size_t odd_align = 24;

int
main(void)
{
    void * volatile p;
    void * volatile q;
    void * volatile mem;
    char *c;
    int   ret;
    int   i;

    /* malloc, calloc */
    errno = 0;
    p = malloc(huge);
    check(p == NULL && errno == ENOMEM, "malloc of a huge size");
    errno = 0;
    p = calloc(half, 4);
    check(p == NULL && errno == ENOMEM, "calloc overflow");
    errno = 0;
    p = calloc(1, huge);
    check(p == NULL && errno == ENOMEM, "calloc of a huge size");
    p = calloc(16, 4);
    check(p != NULL, "calloc");
    c = p;
    for (i = 0; i < 64; i++)
        check(c[i] == 0, "calloc clears");

    /* free leaves errno alone */
    errno = 1234;
    free(p);
    free(NULL);
    p = malloc(100000);
    check(p != NULL, "malloc");
    errno = 1234;
    free(p);
    check(errno == 1234, "free changed errno");

    /* realloc keeps the contents, and the old object when it fails */
    p = realloc(NULL, 32);
    check(p != NULL, "realloc of NULL allocates");
    memset(p, 'a', 32);
    q = realloc(p, 4096);
    check(q != NULL, "realloc grows");
    c = q;
    for (i = 0; i < 32; i++)
        check(c[i] == 'a', "realloc keeps the contents");
    errno = 0;
    p = realloc(q, huge);
    check(p == NULL && errno == ENOMEM, "realloc of a huge size");
    for (i = 0; i < 32; i++)
        check(c[i] == 'a', "a failed realloc keeps the object");

    /* reallocarray: overflow is ENOMEM, and the old object stays */
    errno = 0;
    p = reallocarray(q, half, 3);
    check(p == NULL && errno == ENOMEM, "reallocarray overflow");
    for (i = 0; i < 32; i++)
        check(c[i] == 'a', "a failed reallocarray keeps the object");
    p = reallocarray(q, 16, 8);
    check(p != NULL, "reallocarray");
    c = p;
    for (i = 0; i < 32; i++)
        check(c[i] == 'a', "reallocarray keeps the contents");

#ifndef __GLIBC__
    /*
     * realloc(ptr, 0) either frees ptr and returns NULL with errno
     * EINVAL, or returns an object. glibc frees and leaves errno.
     */
    errno = 0;
    q = realloc(p, 0);
    check(q != NULL || errno == EINVAL, "realloc(ptr, 0)");
    free(q);
    p = malloc(8);
    check(p != NULL, "malloc");
    errno = 0;
    q = reallocarray(p, 0, 8);
    check(q != NULL || errno == EINVAL, "reallocarray(ptr, 0, n)");
    free(q);
#else
    free(p);
#endif

    /* aligned_alloc */
    p = aligned_alloc(64, 128);
    check(p != NULL && ((uintptr_t)p & 63) == 0, "aligned_alloc");
    free(p);
    errno = 0;
    p = aligned_alloc(odd_align, 48);
    check(p == NULL && errno == EINVAL, "aligned_alloc with an alignment that isn't a power of 2");
    errno = 0;
    p = aligned_alloc(16, huge);
    check(p == NULL && errno == ENOMEM, "aligned_alloc of a huge size");

    /* posix_memalign: EINVAL and ENOMEM returned, memptr unchanged */
    mem = &ret;
    ret = posix_memalign((void **)&mem, sizeof(void *) * 3, 64);
    check(ret == EINVAL && mem == &ret, "posix_memalign, not a power of 2");
    ret = posix_memalign((void **)&mem, sizeof(void *) / 2, 64);
    check(ret == EINVAL && mem == &ret, "posix_memalign, not a multiple of sizeof(void *)");
    ret = posix_memalign((void **)&mem, 0, 64);
    check(ret == EINVAL && mem == &ret, "posix_memalign, zero");
    ret = posix_memalign((void **)&mem, 64, huge);
    check(ret == ENOMEM && (mem == &ret || mem == NULL), "posix_memalign of a huge size");
    for (i = 1; i <= 12; i++) {
        size_t align = sizeof(void *) << i;

        ret = posix_memalign((void **)&mem, align, 100);
        check(ret == 0 && mem != NULL && ((uintptr_t)mem & (align - 1)) == 0,
              "posix_memalign aligns");
        free(mem);
    }

    printf("success\n");
    return 0;
}
