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

#ifndef _LOCAL_MMAN_H_
#define _LOCAL_MMAN_H_

#include "local-freebsd.h"
#include <sys/mman.h>
#include <stdint.h>
#include <freebsd/freebsd-param.h>

static inline int
_prot_to_freebsd(int prot)
{
    int kprot = 0;

    if (prot & PROT_READ)
        kprot |= FREEBSD_PROT_READ;
    if (prot & PROT_WRITE)
        kprot |= FREEBSD_PROT_WRITE;
    if (prot & PROT_EXEC)
        kprot |= FREEBSD_PROT_EXEC;
    return kprot;
}

/* FreeBSD rounds addresses down to a page; POSIX says EINVAL */
static inline int
_page_misaligned(const void *addr)
{
    if ((uintptr_t)addr & FREEBSD_PAGE_MASK) {
        errno = EINVAL;
        return 1;
    }
    return 0;
}

/* Whether every page in the range is mapped; mincore checks without touching them */
int _range_mapped(void *addr, size_t len);

#endif /* _LOCAL_MMAN_H_ */
