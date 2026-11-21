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

#ifndef _LOCAL_TERMIOS_H_
#define _LOCAL_TERMIOS_H_

#include "local-freebsd.h"
#include <termios.h>
#include <freebsd/freebsd-fflags.h>
#include <freebsd/freebsd-termios.h>
#include <freebsd/freebsd-termios-struct.h>
#include <freebsd/freebsd-winsize-struct.h>

/*
 * Convert between picolibc's struct termios and the kernel's. Setting
 * changes only what picolibc describes, leaving the kernel's other
 * flags and characters as they were in *ktermios.
 */
void _termios_to_freebsd(struct __freebsd_termios *ktermios, const struct termios *termios);
void _termios_from_freebsd(struct termios *termios, const struct __freebsd_termios *ktermios);

/* "/dev/" and the device name of fd; returns 0 or an errno value */
int  _fdevname(int fd, char *buf, size_t size);

/* Fetch the kernel's settings */
static inline int
_tcgetattr_freebsd(int fd, struct __freebsd_termios *ktermios)
{
    return (int)_syscall(FREEBSD_SYS_ioctl, fd, FREEBSD_TIOCGETA, ktermios);
}

#endif /* _LOCAL_TERMIOS_H_ */
