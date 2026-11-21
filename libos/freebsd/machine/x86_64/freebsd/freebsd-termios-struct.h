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

#ifndef _FREEBSD_TERMIOS_STRUCT_H_
#define _FREEBSD_TERMIOS_STRUCT_H_

struct __freebsd_termios {
    __uint32_t c_iflag;
    __uint32_t c_oflag;
    __uint32_t c_cflag;
    __uint32_t c_lflag;
    __uint8_t  c_cc[20];
    __uint32_t c_ispeed;
    __uint32_t c_ospeed;
};

_Static_assert(sizeof(struct __freebsd_termios) == 44, "struct termios size");
_Static_assert(__builtin_offsetof(struct __freebsd_termios, c_iflag) == 0,
               "struct termios.c_iflag");
_Static_assert(__builtin_offsetof(struct __freebsd_termios, c_oflag) == 4,
               "struct termios.c_oflag");
_Static_assert(__builtin_offsetof(struct __freebsd_termios, c_cflag) == 8,
               "struct termios.c_cflag");
_Static_assert(__builtin_offsetof(struct __freebsd_termios, c_lflag) == 12,
               "struct termios.c_lflag");
_Static_assert(__builtin_offsetof(struct __freebsd_termios, c_cc) == 16, "struct termios.c_cc");
_Static_assert(__builtin_offsetof(struct __freebsd_termios, c_ispeed) == 36,
               "struct termios.c_ispeed");
_Static_assert(__builtin_offsetof(struct __freebsd_termios, c_ospeed) == 40,
               "struct termios.c_ospeed");

#endif /* _FREEBSD_TERMIOS_STRUCT_H_ */
