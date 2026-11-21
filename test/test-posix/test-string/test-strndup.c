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
/* setrlimit and RLIMIT_AS are XSI */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * strdup and strndup: copies that free() accepts, strndup's
 * termination and its limit, and ENOMEM when there is no memory.
 */

static int ret;

#define fail(...)            \
    do {                     \
        printf(__VA_ARGS__); \
        printf("\n");        \
        ret = 1;             \
    } while (0)

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define SANITIZER_ALLOCATOR 1
#endif
#endif
#ifndef SANITIZER_ALLOCATOR
#define SANITIZER_ALLOCATOR 0
#endif

#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_XSI_MULTI_PROCESS)
#include <sys/resource.h>
#include <sys/wait.h>
#include "../is-qemu.h"

static void *(* volatile p_malloc)(size_t) = malloc;

/* With every byte of memory taken, strdup and strndup fail with ENOMEM */
static int
out_of_memory(void)
{
    static const char src[] = "a string to copy when no memory is left";
    struct rlimit     rl;
    size_t            size;
    char             *p;

    rl.rlim_cur = rl.rlim_max = 256 * 1024 * 1024;
    if (setrlimit(RLIMIT_AS, &rl) != 0) {
        printf("setrlimit: %s\n", strerror(errno));
        return 2;
    }
    /*
     * Leak everything malloc will hand out, through a pointer so that
     * the compiler can't assume each call succeeds
     */
    for (size = 1024 * 1024; size > 0; size /= 2)
        while (p_malloc(size) != NULL)
            ;
    errno = 0;
    p = strdup(src);
    if (p != NULL || errno != ENOMEM) {
        printf("strdup gave %p, errno %d\n", (void *)p, errno);
        return 1;
    }
    errno = 0;
    p = strndup(src, 10);
    if (p != NULL || errno != ENOMEM) {
        printf("strndup gave %p, errno %d\n", (void *)p, errno);
        return 1;
    }
    return 0;
}
#endif

int
main(void)
{
    static const char src[] = "hello, world";
    char              arr[5] = { 'a', 'b', 'c', 'd', 'e' };
    char             *p;
    size_t            n;

    p = strdup(src);
    if (p == NULL || p == src || strcmp(p, src) != 0)
        fail("strdup");
    free(p);
    p = strdup("");
    if (p == NULL || *p != '\0')
        fail("strdup of an empty string");
    free(p);

    /* Up to size bytes, always terminated */
    for (n = 0; n <= sizeof(src) + 2; n++) {
        size_t want = n < strlen(src) ? n : strlen(src);

        p = strndup(src, n);
        if (p == NULL || strlen(p) != want || strncmp(p, src, want) != 0)
            fail("strndup(src, %zu) is \"%s\"", n, p ? p : "(null)");
        free(p);
    }
    /* The array needn't be terminated */
    p = strndup(arr, sizeof(arr));
    if (p == NULL || strcmp(p, "abcde") != 0)
        fail("strndup of an unterminated array");
    free(p);
    /* A null before size bytes ends the copy */
    p = strndup("ab\0cd", 5);
    if (p == NULL || strcmp(p, "ab") != 0)
        fail("strndup past a null");
    free(p);

#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_XSI_MULTI_PROCESS)
    if (SANITIZER_ALLOCATOR) {
        printf("the sanitizer's allocator ignores RLIMIT_AS; ENOMEM not checked\n");
    } else if (is_qemu()) {
        printf("qemu doesn't apply RLIMIT_AS; ENOMEM not checked\n");
    } else {
        pid_t pid;
        int   status;

        fflush(stdout);
        pid = fork();
        if (pid == 0)
            _exit(out_of_memory());
        if (pid < 0 || waitpid(pid, &status, 0) != pid || !WIFEXITED(status)
            || WEXITSTATUS(status) != 0)
            fail("ENOMEM check failed");
    }
#endif

    if (ret == 0)
        printf("success\n");
    return ret;
}
