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

#ifndef _FREEBSD_IPC_H_
#define _FREEBSD_IPC_H_

#define FREEBSD_GETALL      6
#define FREEBSD_GETNCNT     3
#define FREEBSD_GETPID      4
#define FREEBSD_GETVAL      5
#define FREEBSD_GETZCNT     7
#define FREEBSD_IPC_CREAT   512
#define FREEBSD_IPC_EXCL    1024
#define FREEBSD_IPC_INFO    3
#define FREEBSD_IPC_M       4096
#define FREEBSD_IPC_NOWAIT  2048
#define FREEBSD_IPC_PRIVATE 0
#define FREEBSD_IPC_R       256
#define FREEBSD_IPC_RMID    0
#define FREEBSD_IPC_SET     1
#define FREEBSD_IPC_STAT    2
#define FREEBSD_IPC_W       128
#define FREEBSD_MSG_NOERROR 4096
#define FREEBSD_SEM_A       128
#define FREEBSD_SEM_INFO    11
#define FREEBSD_SEM_R       256
#define FREEBSD_SEM_STAT    10
#define FREEBSD_SEM_UNDO    4096
#define FREEBSD_SETALL      9
#define FREEBSD_SETVAL      8
#define FREEBSD_SHMLBA      4096
#define FREEBSD_SHM_INFO    14
#define FREEBSD_SHM_LOCK    11
#define FREEBSD_SHM_R       256
#define FREEBSD_SHM_RDONLY  4096
#define FREEBSD_SHM_REMAP   16384
#define FREEBSD_SHM_RND     8192
#define FREEBSD_SHM_STAT    13
#define FREEBSD_SHM_UNLOCK  12
#define FREEBSD_SHM_W       128

#endif /* _FREEBSD_IPC_H_ */
