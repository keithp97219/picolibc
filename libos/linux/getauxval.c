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

#include "local-linux.h"
#include <sys/auxv.h>

#define LINUX_AT_NULL 0

extern char               **environ;

/*
 * The kernel's auxiliary vector follows the environment's terminating
 * null. Find it while environ is still the array the kernel built,
 * which crt0 sets up before constructors run: on the first call, or
 * before main if nothing asks sooner, as setenv may replace environ.
 */
static const unsigned long *_auxv;

static void __attribute__((constructor))
_auxv_find(void)
{
    char **e = environ;

    if (_auxv || !e)
        return;
    while (*e)
        e++;
    _auxv = (const unsigned long *)(e + 1);
}

unsigned long
getauxval(unsigned long type)
{
    const unsigned long *a;

    _auxv_find();
    if (_auxv)
        for (a = _auxv; a[0] != LINUX_AT_NULL; a += 2)
            if (a[0] == type)
                return a[1];
    errno = ENOENT;
    return 0;
}

/* libgcc's aarch64 outline atomics look for this name */
__weak_reference(getauxval, __getauxval);
