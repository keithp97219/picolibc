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

#define _POSIX_C_SOURCE 202405L
/* The output delay masks and the mapping flags other than OPOST are XSI */
#define _XOPEN_SOURCE 800
#include <termios.h>
#include <unistd.h>

/* The types <termios.h> defines; cc_t, speed_t and tcflag_t are unsigned */
_Static_assert((cc_t)-1 > 0 && (speed_t)-1 > 0 && (tcflag_t)-1 > 0,
               "cc_t, speed_t and tcflag_t are unsigned integer types");
_Static_assert((pid_t)-1 < 0, "pid_t is defined and signed");

#define member_is(type, m, t) _Generic(((type *)0)->m, t: 1, default: 0)

/* struct termios */
_Static_assert(member_is(struct termios, c_iflag, tcflag_t)
                   && member_is(struct termios, c_oflag, tcflag_t)
                   && member_is(struct termios, c_cflag, tcflag_t)
                   && member_is(struct termios, c_lflag, tcflag_t)
                   && member_is(struct termios, c_cc[0], cc_t),
               "struct termios members");
_Static_assert(sizeof(((struct termios *)0)->c_cc) == NCCS * sizeof(cc_t),
               "c_cc has NCCS elements");

/* struct winsize; glibc defines it only in <sys/ioctl.h> */
#ifndef __GLIBC__
_Static_assert(member_is(struct winsize, ws_row, unsigned short)
                   && member_is(struct winsize, ws_col, unsigned short),
               "struct winsize members");
#endif

/* The c_cc subscripts can be used in #if and fit in the array */
#if VEOF < 0 || VEOL < 0 || VERASE < 0 || VINTR < 0 || VKILL < 0 || VMIN < 0 || VQUIT < 0 \
    || VSTART < 0 || VSTOP < 0 || VSUSP < 0 || VTIME < 0
#error c_cc subscripts
#endif
#if VEOF >= NCCS || VEOL >= NCCS || VERASE >= NCCS || VINTR >= NCCS || VKILL >= NCCS     \
    || VMIN >= NCCS || VQUIT >= NCCS || VSTART >= NCCS || VSTOP >= NCCS || VSUSP >= NCCS \
    || VTIME >= NCCS
#error c_cc subscripts within NCCS
#endif

/* Distinct, except that VMIN and VTIME may share with VEOF and VEOL */
static const int subscripts[] = { VEOF, VEOL, VERASE, VINTR, VKILL, VQUIT, VSTART, VSTOP, VSUSP };
static const int min_time[] = { VMIN, VTIME, VERASE, VINTR, VKILL, VQUIT, VSTART, VSTOP, VSUSP };

/* The masks for each field, which are bitwise-distinct */
static const tcflag_t iflags[]
    = { BRKINT, ICRNL, IGNBRK, IGNCR, IGNPAR, INLCR, INPCK, ISTRIP, IXANY, IXOFF, IXON, PARMRK };
static const tcflag_t oflags[] = { OPOST, ONLCR, OCRNL,  ONOCR, ONLRET, OFDEL, OFILL,
                                   NLDLY, CRDLY, TABDLY, BSDLY, VTDLY,  FFDLY };
static const tcflag_t cflags[] = { CSIZE, CSTOPB, CREAD, PARENB, PARODD, HUPCL, CLOCAL };
static const tcflag_t lflags[]
    = { ECHO, ECHOE, ECHOK, ECHONL, ICANON, IEXTEN, ISIG, NOFLSH, TOSTOP };

/* The values within each multi-bit mask */
static const tcflag_t nl[] = { NL0, NL1 };
static const tcflag_t cr[] = { CR0, CR1, CR2, CR3 };
static const tcflag_t tab[] = { TAB0, TAB1, TAB2, TAB3 };
static const tcflag_t bs[] = { BS0, BS1 };
static const tcflag_t vt[] = { VT0, VT1 };
static const tcflag_t ff[] = { FF0, FF1 };
static const tcflag_t cs[] = { CS5, CS6, CS7, CS8 };

static const speed_t  speeds[] = { B0,   B50,   B75,   B110,  B134,  B150,  B200,   B300,
                                   B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400 };

static const int      actions[] = { TCSANOW, TCSADRAIN, TCSAFLUSH };
static const int      queues[] = { TCIFLUSH, TCIOFLUSH, TCOFLUSH };
static const int      flows[] = { TCIOFF, TCION, TCOOFF, TCOON };

#include <stdio.h>
#include <stdlib.h>

/* The functions need system support */
#if !defined(__PICOLIBC__)                                                                 \
    || (defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC) && __PICOLIBC_POSIX_DEVICE_SPECIFIC > 0)
#define CHECK_PROTOTYPES

/* Each function has the specified prototype */
static speed_t (* const p_cfgetispeed)(const struct termios *) = cfgetispeed;
static speed_t (* const p_cfgetospeed)(const struct termios *) = cfgetospeed;
static int     (    *const p_cfsetispeed)(struct termios *, speed_t) = cfsetispeed;
static int     (    *const p_cfsetospeed)(struct termios *, speed_t) = cfsetospeed;
static int     (    *const p_tcdrain)(int) = tcdrain;
static int     (    *const p_tcflow)(int, int) = tcflow;
static int     (    *const p_tcflush)(int, int) = tcflush;
static int     (    *const p_tcgetattr)(int, struct termios *) = tcgetattr;
static pid_t   (  *const p_tcgetsid)(int) = tcgetsid;
#ifdef __PICOLIBC__
/* glibc doesn't have tcgetwinsize and tcsetwinsize */
static int (* const p_tcgetwinsize)(int, struct winsize *) = tcgetwinsize;
static int (* const p_tcsetwinsize)(int, const struct winsize *) = tcsetwinsize;
#endif
static int                (               *const p_tcsendbreak)(int, int) = tcsendbreak;
static int                (               *const p_tcsetattr)(int, int, const struct termios *) = tcsetattr;

static const void * const prototypes[] = {
    &p_cfgetispeed,  &p_cfgetospeed,  &p_cfsetispeed, &p_cfsetospeed, &p_tcdrain,
    &p_tcflow,       &p_tcflush,      &p_tcgetattr,   &p_tcgetsid,
#ifdef __PICOLIBC__
    &p_tcgetwinsize, &p_tcsetwinsize,
#endif
    &p_tcsendbreak,  &p_tcsetattr,
};
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
distinct_int(const int *v, size_t n)
{
    size_t i, j;

    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (v[i] == v[j])
                return 0;
    return 1;
}

static int
distinct_speed(const speed_t *v, size_t n)
{
    size_t i, j;

    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (v[i] == v[j])
                return 0;
    return 1;
}

/* Non-zero masks that share no bits */
static int
bitwise_distinct(const tcflag_t *v, size_t n)
{
    size_t i, j;

    for (i = 0; i < n; i++) {
        if (v[i] == 0)
            return 0;
        for (j = i + 1; j < n; j++)
            if (v[i] & v[j])
                return 0;
    }
    return 1;
}

/* Distinct values that lie within mask */
static int
within(tcflag_t mask, const tcflag_t *v, size_t n)
{
    size_t i, j;

    for (i = 0; i < n; i++) {
        if ((v[i] & ~mask) != 0)
            return 0;
        for (j = i + 1; j < n; j++)
            if (v[i] == v[j])
                return 0;
    }
    return 1;
}

#define N(a) (sizeof(a) / sizeof(a[0]))

int
main(void)
{
#ifdef CHECK_PROTOTYPES
    (void)prototypes;
#endif

    check(distinct_int(subscripts, N(subscripts)), "c_cc subscripts are distinct");
    check(distinct_int(min_time, N(min_time)), "VMIN and VTIME are distinct from the others");
    check(VMIN != VTIME, "VMIN and VTIME");

    check(bitwise_distinct(iflags, N(iflags)), "c_iflag masks");
    check(bitwise_distinct(oflags, N(oflags)), "c_oflag masks");
    check(bitwise_distinct(cflags, N(cflags)), "c_cflag masks");
    check(bitwise_distinct(lflags, N(lflags)), "c_lflag masks");

    check(within(NLDLY, nl, N(nl)) && NL0 == 0, "NL0 and NL1");
    check(within(CRDLY, cr, N(cr)) && CR0 == 0, "CR0 to CR3");
    check(within(TABDLY, tab, N(tab)) && TAB0 == 0, "TAB0 to TAB3");
    check(within(BSDLY, bs, N(bs)) && BS0 == 0, "BS0 and BS1");
    check(within(VTDLY, vt, N(vt)) && VT0 == 0, "VT0 and VT1");
    check(within(FFDLY, ff, N(ff)) && FF0 == 0, "FF0 and FF1");
    check(within(CSIZE, cs, N(cs)), "CS5 to CS8");

    check(distinct_speed(speeds, N(speeds)), "the B speeds are distinct");
    check(distinct_int(actions, N(actions)), "TCSANOW, TCSADRAIN and TCSAFLUSH");
    check(distinct_int(queues, N(queues)), "TCIFLUSH, TCIOFLUSH and TCOFLUSH");
    check(distinct_int(flows, N(flows)), "TCIOFF, TCION, TCOOFF and TCOON");

    printf("success\n");
    return 0;
}
