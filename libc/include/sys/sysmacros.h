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

#ifndef _SYS_SYSMACROS_H_
#define _SYS_SYSMACROS_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

/*
 * Device numbers: picolibc's dev_t packs a 32-bit major and a 32-bit
 * minor the way glibc does (which, below major 4096 and minor 2^20,
 * is also the Linux kernel's 32-bit encoding). The OS layer converts
 * the kernel's device numbers to and from this form.
 */
#define makedev(__major, __minor)                                            \
    ((__dev_t)(((((unsigned long long)(__major)) & 0x00000fffULL) << 8)      \
               | ((((unsigned long long)(__major)) & 0xfffff000ULL) << 32)   \
               | (((unsigned long long)(__minor)) & 0x000000ffULL)           \
               | ((((unsigned long long)(__minor)) & 0xffffff00ULL) << 12)))
#define major(__dev)                                                         \
    ((unsigned int)(((((unsigned long long)(__dev)) >> 32) & 0xfffff000U)    \
                    | ((((unsigned long long)(__dev)) >> 8) & 0x00000fffU)))
#define minor(__dev)                                                      \
    ((unsigned int)(((((unsigned long long)(__dev)) >> 12) & 0xffffff00U) \
                    | (((unsigned long long)(__dev)) & 0x000000ffU)))

#endif /* _SYS_SYSMACROS_H_ */
