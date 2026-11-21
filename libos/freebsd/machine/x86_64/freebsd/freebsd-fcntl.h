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

#ifndef _FREEBSD_FCNTL_H_
#define _FREEBSD_FCNTL_H_

#define FREEBSD_AT_EACCESS          0x00000100
#define FREEBSD_AT_EMPTY_PATH       0x00004000
#define FREEBSD_AT_FDCWD            -100
#define FREEBSD_AT_REMOVEDIR        0x00000800
#define FREEBSD_AT_RENAME_NOREPLACE 0x00000001
#define FREEBSD_AT_RESOLVE_BENEATH  0x00002000
#define FREEBSD_AT_SYMLINK_FOLLOW   0x00000400
#define FREEBSD_AT_SYMLINK_NOFOLLOW 0x00000200
#define FREEBSD_FD_CLOEXEC          0x00000001
#define FREEBSD_FD_CLOFORK          0x00000004
#define FREEBSD_FD_NONE             -200
#define FREEBSD_FD_RESOLVE_BENEATH  0x00000002
#define FREEBSD_F_ADD_SEALS         0x00000013
#define FREEBSD_F_CANCEL            0x00000005
#define FREEBSD_F_DUP2FD            0x0000000a
#define FREEBSD_F_DUP2FD_CLOEXEC    0x00000012
#define FREEBSD_F_DUP3FD            0x00000018
#define FREEBSD_F_DUP3FD_SHIFT      0x00000010
#define FREEBSD_F_DUPFD             0x00000000
#define FREEBSD_F_DUPFD_CLOEXEC     0x00000011
#define FREEBSD_F_DUPFD_CLOFORK     0x00000017
#define FREEBSD_F_GETFD             0x00000001
#define FREEBSD_F_GETFL             0x00000003
#define FREEBSD_F_GETLK             0x0000000b
#define FREEBSD_F_GETOWN            0x00000005
#define FREEBSD_F_GET_SEALS         0x00000014
#define FREEBSD_F_ISUNIONSTACK      0x00000015
#define FREEBSD_F_KINFO             0x00000016
#define FREEBSD_F_OGETLK            0x00000007
#define FREEBSD_F_OSETLK            0x00000008
#define FREEBSD_F_OSETLKW           0x00000009
#define FREEBSD_F_RDAHEAD           0x00000010
#define FREEBSD_F_RDLCK             0x00000001
#define FREEBSD_F_READAHEAD         0x0000000f
#define FREEBSD_F_SEAL_GROW         0x00000004
#define FREEBSD_F_SEAL_SEAL         0x00000001
#define FREEBSD_F_SEAL_SHRINK       0x00000002
#define FREEBSD_F_SEAL_WRITE        0x00000008
#define FREEBSD_F_SETFD             0x00000002
#define FREEBSD_F_SETFL             0x00000004
#define FREEBSD_F_SETLK             0x0000000c
#define FREEBSD_F_SETLKW            0x0000000d
#define FREEBSD_F_SETLK_REMOTE      0x0000000e
#define FREEBSD_F_SETOWN            0x00000006
#define FREEBSD_F_UNLCK             0x00000002
#define FREEBSD_F_UNLCKSYS          0x00000004
#define FREEBSD_F_WRLCK             0x00000003
#define FREEBSD_O_ACCMODE           0x00000003
#define FREEBSD_O_APPEND            0x00000008
#define FREEBSD_O_ASYNC             0x00000040
#define FREEBSD_O_CLOEXEC           0x00100000
#define FREEBSD_O_CLOFORK           0x08000000
#define FREEBSD_O_CREAT             0x00000200
#define FREEBSD_O_DIRECT            0x00010000
#define FREEBSD_O_DIRECTORY         0x00020000
#define FREEBSD_O_DSYNC             0x01000000
#define FREEBSD_O_EMPTY_PATH        0x02000000
#define FREEBSD_O_EXCL              0x00000800
#define FREEBSD_O_EXEC              0x00040000
#define FREEBSD_O_EXLOCK            0x00000020
#define FREEBSD_O_FSYNC             0x00000080
#define FREEBSD_O_NAMEDATTR         0x04000000
#define FREEBSD_O_NDELAY            0x00000004
#define FREEBSD_O_NOCTTY            0x00008000
#define FREEBSD_O_NOFOLLOW          0x00000100
#define FREEBSD_O_NONBLOCK          0x00000004
#define FREEBSD_O_PATH              0x00400000
#define FREEBSD_O_RDONLY            0x00000000
#define FREEBSD_O_RDWR              0x00000002
#define FREEBSD_O_RESOLVE_BENEATH   0x00800000
#define FREEBSD_O_SEARCH            0x00040000
#define FREEBSD_O_SHLOCK            0x00000010
#define FREEBSD_O_SYNC              0x00000080
#define FREEBSD_O_TRUNC             0x00000400
#define FREEBSD_O_TTY_INIT          0x00080000
#define FREEBSD_O_VERIFY            0x00200000
#define FREEBSD_O_WRONLY            0x00000001
#define FREEBSD_O_XATTR             0x04000000

#endif /* _FREEBSD_FCNTL_H_ */
