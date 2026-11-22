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

#ifndef _LOCAL_DEV_H_
#define _LOCAL_DEV_H_

#include <sys/types.h>
#include <sys/sysmacros.h>

/*
 * FreeBSD's kernel dev_t packs major and minor differently from
 * picolibc's (glibc's) dev_t; these are the formulas of FreeBSD's
 * <sys/types.h> major(), minor() and makedev().
 */
static inline dev_t
_dev_from_freebsd(__uint64_t kdev)
{
    unsigned int maj = (unsigned int)(((kdev >> 32) & 0xffffff00) | ((kdev >> 8) & 0xff));
    unsigned int min = (unsigned int)(((kdev >> 24) & 0xff00) | (kdev & 0xffff00ff));

    return makedev(maj, min);
}

static inline __uint64_t
_dev_to_freebsd(dev_t dev)
{
    __uint64_t maj = major(dev), min = minor(dev);

    return ((maj & 0xffffff00) << 32) | ((maj & 0xff) << 8) | ((min & 0xff00) << 24)
        | (min & 0xffff00ff);
}

#endif /* _LOCAL_DEV_H_ */
