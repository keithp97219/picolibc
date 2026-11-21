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

#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

extern char **environ;

/*
 * execl and execle must be async-signal-safe, so build argv on the
 * stack where possible: malloc can't be used from a signal handler or
 * the child of a multi-threaded fork
 */
int
execl(const char *path, const char *arg, ...)
{
    va_list      ap;
    size_t       argc = 1;
    const char **argv;

    /* count args */
    va_start(ap, arg);
    while (va_arg(ap, const char *) != NULL)
        argc++;
    va_end(ap);

#ifdef __HAVE_BUILTIN_ALLOCA
    argv = __builtin_alloca((argc + 1) * sizeof(char *));
#else
    argv = calloc(argc + 1, sizeof(char *));
    if (!argv) {
        errno = ENOMEM;
        return -1;
    }
#endif

    /* fill in argv */
    va_start(ap, arg);
    argv[0] = arg;
    for (size_t i = 1; i <= argc; i++)
        argv[i] = va_arg(ap, const char *);
    va_end(ap);

#ifdef __HAVE_BUILTIN_ALLOCA
    return execve(path, (char * const *)argv, environ);
#else
    int ret = execve(path, (char * const *)argv, environ);
    free(argv);
    return ret;
#endif
}
