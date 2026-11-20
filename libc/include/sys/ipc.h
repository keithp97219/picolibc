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

#ifndef _SYS_IPC_H_
#define _SYS_IPC_H_

#include <sys/cdefs.h>
#include <sys/types.h>

_BEGIN_STD_C

struct ipc_perm {
    uid_t  uid;  /* Owner's user ID */
    gid_t  gid;  /* Owner's group ID */
    uid_t  cuid; /* Creator's user ID */
    gid_t  cgid; /* Creator's group ID */
    mode_t mode; /* Read/write permission */
};

/* Mode bits */
#define IPC_CREAT  01000 /* Create entry if key does not exist */
#define IPC_EXCL   02000 /* Fail if key exists */
#define IPC_NOWAIT 04000 /* Error if request must wait */

/* Keys */
#define IPC_PRIVATE ((key_t)0)

/* Control commands */
#define IPC_RMID 0 /* Remove identifier */
#define IPC_SET  1 /* Set options */
#define IPC_STAT 2 /* Get options */

key_t ftok(const char *, int) __picolibc_export;

_END_STD_C

#endif /* _SYS_IPC_H_ */
