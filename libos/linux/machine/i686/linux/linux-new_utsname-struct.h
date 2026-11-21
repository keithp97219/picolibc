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

#ifndef _LINUX_NEW_UTSNAME_STRUCT_H_
#define _LINUX_NEW_UTSNAME_STRUCT_H_

struct __kernel_new_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

#define SIMPLE_MAP_NEW_UTSNAME(_t, _f)                         \
    do {                                                       \
        {                                                      \
            unsigned __i;                                      \
            for (__i = 0; __i < 65; __i++)                     \
                (_t)->sysname[__i] = (_f)->sysname[__i];       \
        }                                                      \
        {                                                      \
            unsigned __i;                                      \
            for (__i = 0; __i < 65; __i++)                     \
                (_t)->nodename[__i] = (_f)->nodename[__i];     \
        }                                                      \
        {                                                      \
            unsigned __i;                                      \
            for (__i = 0; __i < 65; __i++)                     \
                (_t)->release[__i] = (_f)->release[__i];       \
        }                                                      \
        {                                                      \
            unsigned __i;                                      \
            for (__i = 0; __i < 65; __i++)                     \
                (_t)->version[__i] = (_f)->version[__i];       \
        }                                                      \
        {                                                      \
            unsigned __i;                                      \
            for (__i = 0; __i < 65; __i++)                     \
                (_t)->machine[__i] = (_f)->machine[__i];       \
        }                                                      \
        {                                                      \
            unsigned __i;                                      \
            for (__i = 0; __i < 65; __i++)                     \
                (_t)->domainname[__i] = (_f)->domainname[__i]; \
        }                                                      \
    } while (0)

#endif /* _LINUX_NEW_UTSNAME_STRUCT_H_ */
