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
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The POSIX option groups (XBD 2.1.5) a test needs.
 *
 * POSIX names these groups but defines no macros for them: a conforming
 * system provides every one. picolibc, which can run on systems that
 * provide only some, describes them with __PICOLIBC_POSIX_<group> and
 * __PICOLIBC_XSI_<group> in <unistd.h>. For any other C library, this
 * header works those macros out from the standard symbols, so tests can
 * gate on the groups alone:
 *
 *  - a system claiming _POSIX_VERSION provides every POSIX group, and
 *    one claiming _XOPEN_UNIX every XSI group as well;
 *  - otherwise, a realtime profile (_POSIX_AEP_REALTIME_*) provides the
 *    groups its subprofile lists.
 *
 * Include this in place of <unistd.h>, after the feature-test macros.
 */

#ifndef _POSIX_GROUPS_H_
#define _POSIX_GROUPS_H_

#include <unistd.h>

#ifndef __PICOLIBC__

#if _POSIX_VERSION >= 200112L
#define __PICOLIBC_POSIX_C_LIB_EXT          _POSIX_VERSION
#define __PICOLIBC_POSIX_DEVICE_IO          _POSIX_VERSION
#define __PICOLIBC_POSIX_DEVICE_SPECIFIC    _POSIX_VERSION
#define __PICOLIBC_POSIX_DEVICE_SPECIFIC_R  _POSIX_VERSION
#define __PICOLIBC_POSIX_DYNAMIC_LINKING    _POSIX_VERSION
#define __PICOLIBC_POSIX_FD_MGMT            _POSIX_VERSION
#define __PICOLIBC_POSIX_FIFO               _POSIX_VERSION
#define __PICOLIBC_POSIX_FIFO_FD            _POSIX_VERSION
#define __PICOLIBC_POSIX_FILE_ATTRIBUTES    _POSIX_VERSION
#define __PICOLIBC_POSIX_FILE_ATTRIBUTES_FD _POSIX_VERSION
#define __PICOLIBC_POSIX_FILE_SYSTEM        _POSIX_VERSION
#define __PICOLIBC_POSIX_FILE_SYSTEM_EXT    _POSIX_VERSION
#define __PICOLIBC_POSIX_FILE_SYSTEM_FD     _POSIX_VERSION
#define __PICOLIBC_POSIX_MULTI_PROCESS      _POSIX_VERSION
#define __PICOLIBC_POSIX_MULTI_PROCESS_FD   _POSIX_VERSION
#define __PICOLIBC_POSIX_NETWORKING         _POSIX_VERSION
#define __PICOLIBC_POSIX_PIPE               _POSIX_VERSION
#define __PICOLIBC_POSIX_SIGNALS            _POSIX_VERSION
#define __PICOLIBC_POSIX_SINGLE_PROCESS     _POSIX_VERSION
#define __PICOLIBC_POSIX_SYMBOLIC_LINKS     _POSIX_VERSION
#define __PICOLIBC_POSIX_SYMBOLIC_LINKS_FD  _POSIX_VERSION
#define __PICOLIBC_POSIX_USER_GROUPS        _POSIX_VERSION
#if _POSIX_VERSION >= 200809L
/* POSIX.1-2008 added these groups of library functions */
#define __PICOLIBC_POSIX_C_LANG_WIDE_CHAR_EXT     _POSIX_VERSION
#define __PICOLIBC_POSIX_MULTI_CONCURRENT_LOCALES _POSIX_VERSION
#endif
#if _POSIX_VERSION >= 202405L
/* Close-on-fork and the <uchar.h> group are new in POSIX.1-2024 */
#define __PICOLIBC_POSIX_CLOFORK      _POSIX_VERSION
#define __PICOLIBC_POSIX_C_LANG_UCHAR _POSIX_VERSION
#endif

#if _XOPEN_UNIX > 0
#define __PICOLIBC_XSI_DEVICE_IO          _POSIX_VERSION
#define __PICOLIBC_XSI_DEVICE_SPECIFIC    _POSIX_VERSION
#define __PICOLIBC_XSI_FILE_SYSTEM        _POSIX_VERSION
#define __PICOLIBC_XSI_GENERAL_TERMINAL_R _POSIX_VERSION
#define __PICOLIBC_XSI_IPC                _POSIX_VERSION
#define __PICOLIBC_XSI_MULTI_PROCESS      _POSIX_VERSION
#define __PICOLIBC_XSI_SIGNALS            _POSIX_VERSION
#define __PICOLIBC_XSI_SINGLE_PROCESS     _POSIX_VERSION
#define __PICOLIBC_XSI_SYSTEM_LOGGING     _POSIX_VERSION
#define __PICOLIBC_XSI_USER_GROUPS        _POSIX_VERSION
#define __PICOLIBC_XSI_MATH               _POSIX_VERSION
#define __PICOLIBC_XSI_WIDE_CHAR          _POSIX_VERSION
#endif

#else /* _POSIX_VERSION */

/*
 * Each realtime profile provides the groups for its units of
 * functionality (IEEE 1003.13-2003 Tables 6-1 to 9-1) and those of
 * the ones below it
 */
#ifdef _POSIX_AEP_REALTIME_MULTI
#define __PICOLIBC_POSIX_DEVICE_SPECIFIC _POSIX_AEP_REALTIME_MULTI
#define __PICOLIBC_POSIX_DYNAMIC_LINKING _POSIX_AEP_REALTIME_MULTI
#define __PICOLIBC_POSIX_FIFO            _POSIX_AEP_REALTIME_MULTI
#define __PICOLIBC_POSIX_FILE_ATTRIBUTES _POSIX_AEP_REALTIME_MULTI
#define __PICOLIBC_POSIX_FILE_SYSTEM_EXT _POSIX_AEP_REALTIME_MULTI
#define __PICOLIBC_POSIX_SYMBOLIC_LINKS  _POSIX_AEP_REALTIME_MULTI
#define __PICOLIBC_POSIX_USER_GROUPS     _POSIX_AEP_REALTIME_MULTI
#ifndef _POSIX_AEP_REALTIME_DEDICATED
#define _POSIX_AEP_REALTIME_DEDICATED _POSIX_AEP_REALTIME_MULTI
#endif
#endif
#ifdef _POSIX_AEP_REALTIME_DEDICATED
#define __PICOLIBC_POSIX_MULTI_PROCESS _POSIX_AEP_REALTIME_DEDICATED
#define __PICOLIBC_POSIX_NETWORKING    _POSIX_AEP_REALTIME_DEDICATED
#define __PICOLIBC_POSIX_PIPE          _POSIX_AEP_REALTIME_DEDICATED
#ifndef _POSIX_AEP_REALTIME_CONTROLLER
#define _POSIX_AEP_REALTIME_CONTROLLER _POSIX_AEP_REALTIME_DEDICATED
#endif
#endif
#ifdef _POSIX_AEP_REALTIME_CONTROLLER
#define __PICOLIBC_POSIX_FD_MGMT     _POSIX_AEP_REALTIME_CONTROLLER
#define __PICOLIBC_POSIX_FILE_SYSTEM _POSIX_AEP_REALTIME_CONTROLLER
#ifndef _POSIX_AEP_REALTIME_MINIMAL
#define _POSIX_AEP_REALTIME_MINIMAL _POSIX_AEP_REALTIME_CONTROLLER
#endif
#endif
#ifdef _POSIX_AEP_REALTIME_MINIMAL
#define __PICOLIBC_POSIX_C_LIB_EXT      _POSIX_AEP_REALTIME_MINIMAL
#define __PICOLIBC_POSIX_DEVICE_IO      _POSIX_AEP_REALTIME_MINIMAL
#define __PICOLIBC_POSIX_SIGNALS        _POSIX_AEP_REALTIME_MINIMAL
#define __PICOLIBC_POSIX_SINGLE_PROCESS _POSIX_AEP_REALTIME_MINIMAL
#endif

#endif /* _POSIX_VERSION */

#endif /* __PICOLIBC__ */

#endif /* _POSIX_GROUPS_H_ */
