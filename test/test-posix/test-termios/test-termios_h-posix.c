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
#include <termios.h>
#include <unistd.h>

/*
 * In strict POSIX mode, without XSI, <termios.h> declares no names
 * outside its reserved prefixes (c_, B[0-9], TC, ws_, and I, O and V
 * for macros) beyond those POSIX itself defines there
 */

/* The XSI delay masks and values */
#if defined(NLDLY) || defined(NL0) || defined(NL1) || defined(CRDLY) || defined(CR0)     \
    || defined(CR1) || defined(CR2) || defined(CR3) || defined(TABDLY) || defined(TAB0)  \
    || defined(TAB1) || defined(TAB2) || defined(TAB3) || defined(BSDLY) || defined(BS0) \
    || defined(BS1) || defined(FFDLY) || defined(FF0) || defined(FF1)
#error XSI delay names visible without _XOPEN_SOURCE
#endif

/* BSD extensions; these conflict with any declaration of the same name */
static int            cfsetspeed;
static int            cfmakeraw;

/* POSIX names that are there without XSI */
static const tcflag_t names[] = { BRKINT, ICRNL, OPOST, CSIZE, CS8, CLOCAL, ECHO, ICANON, TOSTOP };

#include <stdio.h>

int
main(void)
{
    (void)cfsetspeed;
    (void)cfmakeraw;
    (void)names;
    printf("success\n");
    return 0;
}
