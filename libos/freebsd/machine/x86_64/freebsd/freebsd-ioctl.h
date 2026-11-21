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

#ifndef _FREEBSD_IOCTL_H_
#define _FREEBSD_IOCTL_H_

#define FREEBSD_FIOASYNC           0x8004667dL
#define FREEBSD_FIOBMAP2           0xc0106663L
#define FREEBSD_FIOCLEX            0x20006601
#define FREEBSD_FIODGNAME          0x80106678L
#define FREEBSD_FIODTYPE           0x4004667a
#define FREEBSD_FIOGETLBA          0x40046679
#define FREEBSD_FIOGETOWN          0x4004667b
#define FREEBSD_FIOGSHMLPGCNF      0x40306665
#define FREEBSD_FIONBIO            0x8004667eL
#define FREEBSD_FIONCLEX           0x20006602
#define FREEBSD_FIONREAD           0x4004667f
#define FREEBSD_FIONSPACE          0x40046676
#define FREEBSD_FIONWRITE          0x40046677
#define FREEBSD_FIOSEEKDATA        0xc0086661L
#define FREEBSD_FIOSEEKHOLE        0xc0086662L
#define FREEBSD_FIOSETOWN          0x8004667cL
#define FREEBSD_FIOSSHMLPGCNF      0x80306664L
#define FREEBSD_TIOCCBRK           0x2000747a
#define FREEBSD_TIOCCDTR           0x20007478
#define FREEBSD_TIOCCONS           0x80047462L
#define FREEBSD_TIOCDRAIN          0x2000745e
#define FREEBSD_TIOCEXCL           0x2000740d
#define FREEBSD_TIOCEXT            0x80047460L
#define FREEBSD_TIOCFLUSH          0x80047410L
#define FREEBSD_TIOCGDRAINWAIT     0x40047456
#define FREEBSD_TIOCGETA           0x402c7413
#define FREEBSD_TIOCGETD           0x4004741a
#define FREEBSD_TIOCGPGRP          0x40047477
#define FREEBSD_TIOCGPTN           0x4004740f
#define FREEBSD_TIOCGSID           0x40047463
#define FREEBSD_TIOCGWINSZ         0x40087468
#define FREEBSD_TIOCMBIC           0x8004746bL
#define FREEBSD_TIOCMBIS           0x8004746cL
#define FREEBSD_TIOCMGET           0x4004746a
#define FREEBSD_TIOCMSET           0x8004746dL
#define FREEBSD_TIOCM_CAR          0x00000040
#define FREEBSD_TIOCM_CD           0x00000040
#define FREEBSD_TIOCM_CTS          0x00000020
#define FREEBSD_TIOCM_DCD          0x00000040
#define FREEBSD_TIOCM_DSR          0x00000100
#define FREEBSD_TIOCM_DTR          0x00000002
#define FREEBSD_TIOCM_LE           0x00000001
#define FREEBSD_TIOCM_RI           0x00000080
#define FREEBSD_TIOCM_RNG          0x00000080
#define FREEBSD_TIOCM_RTS          0x00000004
#define FREEBSD_TIOCM_SR           0x00000010
#define FREEBSD_TIOCM_ST           0x00000008
#define FREEBSD_TIOCNOTTY          0x20007471
#define FREEBSD_TIOCNXCL           0x2000740e
#define FREEBSD_TIOCOUTQ           0x40047473
#define FREEBSD_TIOCPKT            0x80047470L
#define FREEBSD_TIOCPKT_DATA       0x00000000
#define FREEBSD_TIOCPKT_DOSTOP     0x00000020
#define FREEBSD_TIOCPKT_FLUSHREAD  0x00000001
#define FREEBSD_TIOCPKT_FLUSHWRITE 0x00000002
#define FREEBSD_TIOCPKT_IOCTL      0x00000040
#define FREEBSD_TIOCPKT_NOSTOP     0x00000010
#define FREEBSD_TIOCPKT_START      0x00000008
#define FREEBSD_TIOCPKT_STOP       0x00000004
#define FREEBSD_TIOCPTMASTER       0x2000741c
#define FREEBSD_TIOCSBRK           0x2000747b
#define FREEBSD_TIOCSCTTY          0x20007461
#define FREEBSD_TIOCSDRAINWAIT     0x80047457L
#define FREEBSD_TIOCSDTR           0x20007479
#define FREEBSD_TIOCSETA           0x802c7414L
#define FREEBSD_TIOCSETAF          0x802c7416L
#define FREEBSD_TIOCSETAW          0x802c7415L
#define FREEBSD_TIOCSETD           0x8004741bL
#define FREEBSD_TIOCSIG            0x2004745f
#define FREEBSD_TIOCSPGRP          0x80047476L
#define FREEBSD_TIOCSTART          0x2000746e
#define FREEBSD_TIOCSTAT           0x20007465
#define FREEBSD_TIOCSTI            0x80017472L
#define FREEBSD_TIOCSTOP           0x2000746f
#define FREEBSD_TIOCSWINSZ         0x80087467L
#define FREEBSD_TIOCTIMESTAMP      0x40107459
#define FREEBSD_TIOCUCNTL          0x80047466L

#endif /* _FREEBSD_IOCTL_H_ */
