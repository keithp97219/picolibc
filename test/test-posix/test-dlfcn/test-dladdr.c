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
/* glibc declares dladdr only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_DYNAMIC_LINKING

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * dlopen, dlsym, dlclose, dlerror and dladdr, on the program itself and
 * on a library that doesn't exist
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
    Dl_info info;
    void   *self;

    self = dlopen(NULL, RTLD_NOW);
    check(self != NULL, "dlopen of the program");
    check(dladdr((void *)&main, &info) != 0, "dladdr of main");
    check(info.dli_fname != NULL && info.dli_fbase != NULL, "dladdr names the object");
    check(dlclose(self) == 0, "dlclose");

    check(dlopen("/picolibc-no-such-library.so", RTLD_NOW) == NULL, "dlopen of nothing");
    check(dlerror() != NULL, "dlerror after a failure");
    check(dlerror() == NULL, "dlerror is cleared once read");

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
