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

#ifndef _FREEBSD_DIRENT_STRUCT_H_
#define _FREEBSD_DIRENT_STRUCT_H_

struct __freebsd_dirent {
    __uint64_t d_fileno;
    __int64_t  d_off;
    __uint16_t d_reclen;
    __uint8_t  d_type;
    __uint8_t  __adjust_19[1];
    __uint16_t d_namlen;
    __uint8_t  __adjust_22[2];
    char       d_name[256];
};

_Static_assert(sizeof(struct __freebsd_dirent) == 280, "struct dirent size");
_Static_assert(__builtin_offsetof(struct __freebsd_dirent, d_fileno) == 0,
               "struct dirent.d_fileno");
_Static_assert(__builtin_offsetof(struct __freebsd_dirent, d_off) == 8, "struct dirent.d_off");
_Static_assert(__builtin_offsetof(struct __freebsd_dirent, d_reclen) == 16,
               "struct dirent.d_reclen");
_Static_assert(__builtin_offsetof(struct __freebsd_dirent, d_type) == 18, "struct dirent.d_type");
_Static_assert(__builtin_offsetof(struct __freebsd_dirent, d_namlen) == 20,
               "struct dirent.d_namlen");
_Static_assert(__builtin_offsetof(struct __freebsd_dirent, d_name) == 24, "struct dirent.d_name");

#endif /* _FREEBSD_DIRENT_STRUCT_H_ */
