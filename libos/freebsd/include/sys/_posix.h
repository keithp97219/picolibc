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

#ifndef _SYS__POSIX_H_
#define _SYS__POSIX_H_

/*
 * What libos/freebsd provides, in the __PICOLIBC_OS_ terms doc/os.md
 * defines; <unistd.h> derives the public POSIX names from these. This
 * grows with the implementation: so far the single process group
 * (sysconf from libos/fallback, uname here).
 */

#define __PICOLIBC_OS_SINGLE_PROCESS 202405L

/* A terminal special character of 0xff is turned off */
#define __PICOLIBC_OS_VDISABLE 0xff

/* Options */
#define __PICOLIBC_OS_MAPPED_FILES          202405L
#define __PICOLIBC_OS_MEMLOCK               202405L
#define __PICOLIBC_OS_MEMLOCK_RANGE         202405L
#define __PICOLIBC_OS_MEMORY_PROTECTION     202405L
#define __PICOLIBC_OS_SHARED_MEMORY_OBJECTS 202405L

/*
 * Option groups. The terminal functions are all there, but FreeBSD's
 * canonical input ends a line at a NUL byte, so DEVICE_SPECIFIC isn't
 * claimed
 */
#define __PICOLIBC_OS_DEVICE_SPECIFIC_R 202405L

/* XSI option groups */
#define __PICOLIBC_OS_XSI_DEVICE_SPECIFIC    202405L
#define __PICOLIBC_OS_XSI_GENERAL_TERMINAL_R 202405L

/* Sockets, with picolibc built for the FreeBSD network interfaces */
#ifdef __PICOLIBC_NETWORK_FREEBSD
#define __PICOLIBC_OS_IPV6        202405L
#define __PICOLIBC_OS_NETWORKING  202405L
#define __PICOLIBC_OS_RAW_SOCKETS 202405L
#endif

#endif /* _SYS__POSIX_H_ */
