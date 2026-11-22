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

#ifndef _STDIO_EXT_H_
#define _STDIO_EXT_H_

#include <stdio.h>

_BEGIN_STD_C

/*
 * Stream introspection functions from Solaris and glibc, used by
 * gnulib (close-stdout, fpurge, freading, ...) and other portable
 * code in place of poking at FILE internals.
 */

#define FSETLOCKING_QUERY    0
#define FSETLOCKING_INTERNAL 1
#define FSETLOCKING_BYCALLER 2

size_t      __fbufsize(FILE *__stream) __picolibc_export;
size_t      __fpending(FILE *__stream) __picolibc_export;
int         __flbf(FILE *__stream) __picolibc_export;
int         __freadable(FILE *__stream) __picolibc_export;
int         __fwritable(FILE *__stream) __picolibc_export;
int         __freading(FILE *__stream) __picolibc_export;
int         __fwriting(FILE *__stream) __picolibc_export;
int         __fsetlocking(FILE *__stream, int __type) __picolibc_export;
void        __fpurge(FILE *__stream) __picolibc_export;

/* musl extensions, also used by gnulib */
size_t      __freadahead(FILE *__stream) __picolibc_export;
const char *__freadptr(FILE *__stream, size_t *__sizep) __picolibc_export;
void        __freadptrinc(FILE *__stream, size_t __inc) __picolibc_export;
/* __fseterr is declared in <stdio.h> */

_END_STD_C

#endif /* _STDIO_EXT_H_ */
