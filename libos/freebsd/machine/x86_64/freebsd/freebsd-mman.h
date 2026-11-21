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

#ifndef _FREEBSD_MMAN_H_
#define _FREEBSD_MMAN_H_

#define FREEBSD_MADV_AUTOSYNC         0x00000007
#define FREEBSD_MADV_CORE             0x00000009
#define FREEBSD_MADV_DONTNEED         0x00000004
#define FREEBSD_MADV_FREE             0x00000005
#define FREEBSD_MADV_NOCORE           0x00000008
#define FREEBSD_MADV_NORMAL           0x00000000
#define FREEBSD_MADV_NOSYNC           0x00000006
#define FREEBSD_MADV_PROTECT          0x0000000a
#define FREEBSD_MADV_RANDOM           0x00000001
#define FREEBSD_MADV_SEQUENTIAL       0x00000002
#define FREEBSD_MADV_WILLNEED         0x00000003
#define FREEBSD_MAP_32BIT             0x00080000
#define FREEBSD_MAP_ALIGNED_SUPER     0x01000000
#define FREEBSD_MAP_ALIGNMENT_MASK    -16777216
#define FREEBSD_MAP_ALIGNMENT_SHIFT   0x00000018
#define FREEBSD_MAP_ANON              0x00001000
#define FREEBSD_MAP_ANONYMOUS         0x00001000
#define FREEBSD_MAP_COPY              0x00000002
#define FREEBSD_MAP_EXCL              0x00004000
#define FREEBSD_MAP_FAILED            -1
#define FREEBSD_MAP_FILE              0x00000000
#define FREEBSD_MAP_FIXED             0x00000010
#define FREEBSD_MAP_GUARD             0x00002000
#define FREEBSD_MAP_HASSEMAPHORE      0x00000200
#define FREEBSD_MAP_NOCORE            0x00020000
#define FREEBSD_MAP_NOSYNC            0x00000800
#define FREEBSD_MAP_PREFAULT_READ     0x00040000
#define FREEBSD_MAP_PRIVATE           0x00000002
#define FREEBSD_MAP_RESERVED0020      0x00000020
#define FREEBSD_MAP_RESERVED0040      0x00000040
#define FREEBSD_MAP_RESERVED0080      0x00000080
#define FREEBSD_MAP_RESERVED0100      0x00000100
#define FREEBSD_MAP_SHARED            0x00000001
#define FREEBSD_MAP_STACK             0x00000400
#define FREEBSD_MCL_CURRENT           0x00000001
#define FREEBSD_MCL_FUTURE            0x00000002
#define FREEBSD_MS_ASYNC              0x00000001
#define FREEBSD_MS_INVALIDATE         0x00000002
#define FREEBSD_MS_SYNC               0x00000000
#define FREEBSD_POSIX_MADV_DONTNEED   0x00000004
#define FREEBSD_POSIX_MADV_NORMAL     0x00000000
#define FREEBSD_POSIX_MADV_RANDOM     0x00000001
#define FREEBSD_POSIX_MADV_SEQUENTIAL 0x00000002
#define FREEBSD_POSIX_MADV_WILLNEED   0x00000003
#define FREEBSD_PROT_CHERI0           0x00000008
#define FREEBSD_PROT_CHERI1           0x00000010
#define FREEBSD_PROT_EXEC             0x00000004
#define FREEBSD_PROT_NONE             0x00000000
#define FREEBSD_PROT_READ             0x00000001
#define FREEBSD_PROT_WRITE            0x00000002

#endif /* _FREEBSD_MMAN_H_ */
