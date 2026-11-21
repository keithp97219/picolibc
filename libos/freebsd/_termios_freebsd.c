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

#include "local-termios.h"

struct _termios_bit {
    tcflag_t     bit;
    unsigned int kbit;
};

#define BIT(name) { name, FREEBSD_##name }

static const struct _termios_bit _iflags[] = {
    BIT(BRKINT), BIT(ICRNL),  BIT(IGNBRK), BIT(IGNCR), BIT(IGNPAR), BIT(INLCR),
    BIT(INPCK),  BIT(ISTRIP), BIT(IXANY),  BIT(IXOFF), BIT(IXON),   BIT(PARMRK),
};

/* FreeBSD has none of the delays but TAB3, nor OFILL and OFDEL */
static const struct _termios_bit _oflags[] = {
    BIT(OPOST), BIT(ONLCR), BIT(OCRNL), BIT(ONOCR), BIT(ONLRET),
};

static const struct _termios_bit _cflags[] = {
    BIT(CSTOPB), BIT(CREAD), BIT(PARENB), BIT(PARODD), BIT(HUPCL), BIT(CLOCAL),
};

static const struct _termios_bit _lflags[] = {
    BIT(ECHO),   BIT(ECHOE), BIT(ECHOK),  BIT(ECHONL), BIT(ICANON),
    BIT(IEXTEN), BIT(ISIG),  BIT(NOFLSH), BIT(TOSTOP),
};

static const unsigned char _cc[NCCS] = {
    [VEOF] = FREEBSD_VEOF,   [VEOL] = FREEBSD_VEOL,     [VERASE] = FREEBSD_VERASE,
    [VINTR] = FREEBSD_VINTR, [VKILL] = FREEBSD_VKILL,   [VMIN] = FREEBSD_VMIN,
    [VQUIT] = FREEBSD_VQUIT, [VSTART] = FREEBSD_VSTART, [VSTOP] = FREEBSD_VSTOP,
    [VSUSP] = FREEBSD_VSUSP, [VTIME] = FREEBSD_VTIME,
};

#define NBITS(a) (sizeof(a) / sizeof(a[0]))

static unsigned int
_bits_to_freebsd(unsigned int kflags, tcflag_t flags, const struct _termios_bit *map, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (flags & map[i].bit)
            kflags |= map[i].kbit;
        else
            kflags &= ~map[i].kbit;
    }
    return kflags;
}

static tcflag_t
_bits_from_freebsd(unsigned int kflags, const struct _termios_bit *map, size_t n)
{
    tcflag_t flags = 0;

    for (size_t i = 0; i < n; i++)
        if (kflags & map[i].kbit)
            flags |= map[i].bit;
    return flags;
}

void
_termios_to_freebsd(struct __freebsd_termios *k, const struct termios *t)
{
    unsigned int csize;

    k->c_iflag = _bits_to_freebsd(k->c_iflag, t->c_iflag, _iflags, NBITS(_iflags));
    k->c_oflag = _bits_to_freebsd(k->c_oflag, t->c_oflag, _oflags, NBITS(_oflags));
    k->c_oflag &= ~FREEBSD_TABDLY;
    if ((t->c_oflag & TABDLY) == TAB3)
        k->c_oflag |= FREEBSD_TAB3;
    k->c_cflag = _bits_to_freebsd(k->c_cflag, t->c_cflag, _cflags, NBITS(_cflags));
    switch (t->c_cflag & CSIZE) {
    case CS5:
        csize = FREEBSD_CS5;
        break;
    case CS6:
        csize = FREEBSD_CS6;
        break;
    case CS7:
        csize = FREEBSD_CS7;
        break;
    default:
        csize = FREEBSD_CS8;
        break;
    }
    k->c_cflag = (k->c_cflag & ~FREEBSD_CSIZE) | csize;
    k->c_lflag = _bits_to_freebsd(k->c_lflag, t->c_lflag, _lflags, NBITS(_lflags));
    for (int i = 0; i < NCCS; i++)
        k->c_cc[_cc[i]] = t->c_cc[i];
    /* Both count the speeds in bits per second */
    k->c_ispeed = t->c_ispeed;
    k->c_ospeed = t->c_ospeed;
}

void
_termios_from_freebsd(struct termios *t, const struct __freebsd_termios *k)
{
    t->c_iflag = _bits_from_freebsd(k->c_iflag, _iflags, NBITS(_iflags));
    t->c_oflag = _bits_from_freebsd(k->c_oflag, _oflags, NBITS(_oflags));
    if ((k->c_oflag & FREEBSD_TABDLY) == FREEBSD_TAB3)
        t->c_oflag |= TAB3;
    t->c_cflag = _bits_from_freebsd(k->c_cflag, _cflags, NBITS(_cflags));
    switch (k->c_cflag & FREEBSD_CSIZE) {
    case FREEBSD_CS5:
        t->c_cflag |= CS5;
        break;
    case FREEBSD_CS6:
        t->c_cflag |= CS6;
        break;
    case FREEBSD_CS7:
        t->c_cflag |= CS7;
        break;
    default:
        t->c_cflag |= CS8;
        break;
    }
    t->c_lflag = _bits_from_freebsd(k->c_lflag, _lflags, NBITS(_lflags));
    for (int i = 0; i < NCCS; i++)
        t->c_cc[i] = k->c_cc[_cc[i]];
    t->c_ispeed = k->c_ispeed;
    t->c_ospeed = k->c_ospeed;
}
