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

/* What <sys/ipc.h> provides */

#define _POSIX_C_SOURCE 202405L
/* System V IPC is XSI */
#define _XOPEN_SOURCE 800
#include <sys/ipc.h>

#define same_type(a, T)       _Generic((a), T: 1, default: 0)
#define member_is(type, m, T) same_type(((type *)0)->m, T)

/* The types, without any other header */
_Static_assert((uid_t)-1 > 0 || (uid_t)-1 < 0, "uid_t");
_Static_assert((gid_t)-1 > 0 || (gid_t)-1 < 0, "gid_t");
_Static_assert((mode_t)-1 > 0 || (mode_t)-1 < 0, "mode_t");
_Static_assert((key_t)-1 > 0 || (key_t)-1 < 0, "key_t");

/* struct ipc_perm */
_Static_assert(member_is(struct ipc_perm, uid, uid_t) && member_is(struct ipc_perm, gid, gid_t)
                   && member_is(struct ipc_perm, cuid, uid_t)
                   && member_is(struct ipc_perm, cgid, gid_t)
                   && member_is(struct ipc_perm, mode, mode_t),
               "struct ipc_perm members");

/* The mode bits are distinct from each other and from the permission bits */
_Static_assert((IPC_CREAT & IPC_EXCL) == 0 && (IPC_CREAT & IPC_NOWAIT) == 0
                   && (IPC_EXCL & IPC_NOWAIT) == 0,
               "mode bits are distinct");
_Static_assert(((IPC_CREAT | IPC_EXCL | IPC_NOWAIT) & 0777) == 0,
               "mode bits are outside the permission bits");
_Static_assert(IPC_CREAT != 0 && IPC_EXCL != 0 && IPC_NOWAIT != 0, "mode bits are set");

/* The key and the commands */
_Static_assert(same_type(IPC_PRIVATE, key_t), "IPC_PRIVATE is a key_t");
_Static_assert(IPC_RMID != IPC_SET && IPC_RMID != IPC_STAT && IPC_SET != IPC_STAT,
               "commands are distinct");
#if IPC_CREAT == 0 || IPC_EXCL == 0 || IPC_NOWAIT == 0 || IPC_RMID == IPC_SET
#error the constants can be used in #if
#endif

/* The prototype */
_Static_assert(same_type(&ftok, key_t (*)(const char *, int)), "ftok");

#include <stdio.h>

int
main(void)
{
    printf("success\n");
    return 0;
}
