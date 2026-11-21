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

/*
 * The OS describes its POSIX support in <sys/_posix.h> (doc/os.md).
 * Without one, picolibc stands alone: __PICOLIBC_BAREMETAL is defined,
 * and <unistd.h> and <sys/utsname.h> send sysconf and uname to
 * picolibc's own __baremetal_sysconf and __baremetal_uname, which
 * describe the POSIX subset the library provides by itself. They are
 * separate names so that an OS library linked anyway, or an
 * application defining sysconf, can't contradict what the headers
 * claim. Defining __PICOLIBC_BAREMETAL_IMPL selects this case even when
 * <sys/_posix.h> exists, for building those functions.
 */

#ifndef _SYS__OS_H_
#define _SYS__OS_H_

#if defined(__has_include)
#if __has_include(<sys/_posix.h>) && !defined(__PICOLIBC_BAREMETAL_IMPL)
#include <sys/_posix.h>
#else
#define __PICOLIBC_BAREMETAL 1
#endif
#endif

#endif /* _SYS__OS_H_ */
