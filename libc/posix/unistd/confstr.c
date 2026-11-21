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
#define _XOPEN_SOURCE   800
#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

/*
 * The types _CS_POSIX_V8_WIDTH_RESTRICTED_ENVS must fit in a long for
 * the environment to be listed
 */
#define FITS(t) (sizeof(t) <= sizeof(long))
#define WIDTH_RESTRICTED                                                                       \
    (FITS(blksize_t) && FITS(cc_t) && FITS(mode_t) && FITS(nfds_t) && FITS(pid_t)              \
     && FITS(ptrdiff_t) && FITS(size_t) && FITS(speed_t) && FITS(ssize_t) && FITS(suseconds_t) \
     && FITS(tcflag_t) && FITS(wchar_t) && FITS(wint_t))

/* The programming environments picolibc was built for */
static const char envs[] = ""
#ifdef _POSIX_V8_ILP32_OFF32
                           "POSIX_V8_ILP32_OFF32\n"
#endif
#ifdef _POSIX_V8_ILP32_OFFBIG
                           "POSIX_V8_ILP32_OFFBIG\n"
#endif
#ifdef _POSIX_V8_LP64_OFF64
                           "POSIX_V8_LP64_OFF64\n"
#endif
#ifdef _POSIX_V8_LPBIG_OFFBIG
                           "POSIX_V8_LPBIG_OFFBIG\n"
#endif
    ;

/*
 * _CS_PATH and the width-restricted environments have values. The
 * compilation environment names are valid, with empty values, as
 * picolibc doesn't describe its compiler.
 */
size_t
confstr(int name, char *buf, size_t len)
{
    const char *value;
    size_t      size;

    switch (name) {
    case _CS_PATH:
        value = "/bin:/usr/bin";
        size = strlen(value) + 1;
        break;
    case _CS_POSIX_V8_WIDTH_RESTRICTED_ENVS:
        /* Newline-separated, without a newline at the end */
        value = envs;
        size = WIDTH_RESTRICTED && sizeof(envs) > 1 ? sizeof(envs) - 1 : 1;
        break;
    default:
        if (name < 0 || name > _CS_LFS_LINTFLAGS) {
            errno = EINVAL;
            return 0;
        }
        value = "";
        size = 1;
        break;
    }
    if (buf && len) {
        size_t n = size < len ? size : len;
        memcpy(buf, value, n - 1);
        buf[n - 1] = '\0';
    }
    return size;
}
