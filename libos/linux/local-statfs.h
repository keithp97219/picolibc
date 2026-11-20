/*
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

  * Neither the name of Qualcomm Technologies, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _LOCAL_STATFS_H_
#define _LOCAL_STATFS_H_

#include "local-linux.h"
#include <linux/linux-fsid-struct.h>
#include <linux/linux-termios.h>
#ifdef LINUX_SYS_statfs64
#include <linux/linux-statfs64-struct.h>
#else
#include <linux/linux-statfs-struct.h>
#endif
#include <sys/statvfs.h>
#include <limits.h>
#include <unistd.h>

#ifdef LINUX_SYS_statfs64
#define LINUX_SYS_statfs_size LINUX_SYS_statfs64
#define __kernel_statfs_size  __kernel_statfs64
#else
#define LINUX_SYS_statfs_size LINUX_SYS_statfs
#define __kernel_statfs_size  __kernel_statfs
#endif

int _statvfsbuf(struct statvfs *buf, const struct __kernel_statfs_size *kbuf);

/*
 * pathconf and fpathconf answers for a file on the file system buf
 * describes; limits <limits.h> fixes keep their values there
 */
static inline long
_pathconf_statvfs(const struct statvfs *buf, int name)
{
    switch (name) {
    case _PC_NAME_MAX:
        return (long)buf->f_namemax;
    case _PC_PATH_MAX:
        return PATH_MAX;
    case _PC_PIPE_BUF:
        return PIPE_BUF;
    case _PC_LINK_MAX:
        return LINK_MAX;
    case _PC_2_SYMLINKS:
        return 1;
    case _PC_VDISABLE:
        return LINUX_POSIX_VDISABLE;
    case _PC_TIMESTAMP_RESOLUTION:
        return 1;
    }
    return __fallback_fpathconf(-1, name);
}

#endif /* _LOCAL_STATFS_H_ */
