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

#include "local-ipc.h"
#include <stdlib.h>
#include <freebsd/freebsd-errno.h>

/*
 * FreeBSD's shmget doesn't check the permissions of a segment it
 * finds (shmget_existing in sys/kern/sysv_shm.c), where msgget and
 * semget do. This finds existing segments by a lookup without
 * IPC_CREAT, then checks the permissions asked for in the owner bits
 * of the flags, as the kernel's msgget and semget do (ipcperm in
 * sys/kern/sysv_ipc.c): the owner's bits for the owner or creator,
 * the group's for a member of the owning or creating group, the
 * others' for anyone else; root has them all.
 *
 * The segment's permissions come from IPC_STAT, which needs read
 * permission. Without it, a request for read fails; one for write
 * alone can't be checked and succeeds.
 */

static int
_shm_in_group(gid_t gid)
{
    gid_t *groups;
    int    n, i, ret = 0;

    if (getegid() == gid)
        return 1;
    n = getgroups(0, NULL);
    if (n <= 0)
        return 0;
    groups = malloc((size_t)n * sizeof(gid_t));
    if (!groups)
        return 0;
    n = getgroups(n, groups);
    for (i = 0; i < n; i++)
        if (groups[i] == gid) {
            ret = 1;
            break;
        }
    free(groups);
    return ret;
}

/* Returns 0 or the kernel's negative errno */
static long
_shm_access(int id, int flags)
{
    struct __freebsd_shmid_ds k;
    int                       want = flags & (FREEBSD_IPC_R | FREEBSD_IPC_W);
    int                       mode;
    uid_t                     euid;
    long                      ret;

    if (want == 0)
        return 0;
    ret = _syscall_raw(FREEBSD_SYS_shmctl, id, FREEBSD_IPC_STAT, &k);
    if (ret < 0) {
        if (ret == _FREEBSD_ERR(FREEBSD_EACCES) && !(want & FREEBSD_IPC_R))
            return 0;
        return ret;
    }
    euid = geteuid();
    if (euid == 0)
        return 0;
    mode = k.shm_perm.mode;
    if (euid == k.shm_perm.uid || euid == k.shm_perm.cuid)
        ;
    else if (_shm_in_group(k.shm_perm.gid) || _shm_in_group(k.shm_perm.cgid))
        mode <<= 3;
    else
        mode <<= 6;
    if ((mode & want) != want)
        return _FREEBSD_ERR(FREEBSD_EACCES);
    return 0;
}

int
shmget(key_t key, size_t size, int flags)
{
    long kkey = _key_to_freebsd(key);
    long ret;

    /* These always make a new segment */
    if (key == IPC_PRIVATE || (flags & (IPC_CREAT | IPC_EXCL)) == (IPC_CREAT | IPC_EXCL))
        return (int)_syscall(FREEBSD_SYS_shmget, kkey, size, flags);

    for (;;) {
        ret = _syscall_raw(FREEBSD_SYS_shmget, kkey, size, flags & ~(IPC_CREAT | IPC_EXCL));
        if (ret >= 0) {
            long err = _shm_access((int)ret, flags);

            if (err < 0)
                ret = err;
            break;
        }
        if (ret != _FREEBSD_ERR(FREEBSD_ENOENT) || !(flags & IPC_CREAT))
            break;
        ret = _syscall_raw(FREEBSD_SYS_shmget, kkey, size, flags | IPC_EXCL);
        /* Made by another process since the lookup: look again */
        if (ret != _FREEBSD_ERR(FREEBSD_EEXIST))
            break;
    }
    if (ret < 0) {
        errno = _syscall_errno(ret);
        return -1;
    }
    return (int)ret;
}
