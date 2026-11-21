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

#include "local-pathconf.h"
#include "local-statfs.h"
#include <limits.h>
#include <string.h>
#include <freebsd/freebsd-errno.h>
#include <freebsd/freebsd-pathconf.h>

/*
 * The kernel's name for a variable, or -1. _PC_SYMLINK_MAX is left out:
 * FreeBSD counts the terminating null in it (and ZFS has no answer).
 */
static int
_pathconf_name_to_freebsd(int name)
{
    switch (name) {
    case _PC_FILESIZEBITS:
        return FREEBSD__PC_FILESIZEBITS;
    case _PC_LINK_MAX:
        return FREEBSD__PC_LINK_MAX;
    case _PC_MAX_CANON:
        return FREEBSD__PC_MAX_CANON;
    case _PC_MAX_INPUT:
        return FREEBSD__PC_MAX_INPUT;
    case _PC_NAME_MAX:
        return FREEBSD__PC_NAME_MAX;
    case _PC_PATH_MAX:
        return FREEBSD__PC_PATH_MAX;
    case _PC_PIPE_BUF:
        return FREEBSD__PC_PIPE_BUF;
    case _PC_ALLOC_SIZE_MIN:
        return FREEBSD__PC_ALLOC_SIZE_MIN;
    case _PC_REC_INCR_XFER_SIZE:
        return FREEBSD__PC_REC_INCR_XFER_SIZE;
    case _PC_REC_MAX_XFER_SIZE:
        return FREEBSD__PC_REC_MAX_XFER_SIZE;
    case _PC_REC_MIN_XFER_SIZE:
        return FREEBSD__PC_REC_MIN_XFER_SIZE;
    case _PC_REC_XFER_ALIGN:
        return FREEBSD__PC_REC_XFER_ALIGN;
    case _PC_VDISABLE:
        return FREEBSD__PC_VDISABLE;
    case _PC_ASYNC_IO:
        return FREEBSD__PC_ASYNC_IO;
    case _PC_PRIO_IO:
        return FREEBSD__PC_PRIO_IO;
    case _PC_SYNC_IO:
        return FREEBSD__PC_SYNC_IO;
    default:
        return -1;
    }
}

/* File timestamps follow vfs.timestamp_precision */
static long
_timestamp_resolution(void)
{
    static const char name[] = "vfs.timestamp_precision";
    int               precision;
    size_t            len = sizeof(precision);

    if (_syscall_raw(FREEBSD_SYS___sysctlbyname, name, sizeof(name) - 1, &precision, &len, NULL, 0)
        < 0)
        return 1000;
    switch (precision) {
    case 0: /* seconds */
        return 1000000000L;
    case 1: /* the clock tick, at most 1/1000 s by default */
        return 1000000L;
    case 2: /* microseconds */
        return 1000L;
    default: /* nanoseconds */
        return 1L;
    }
}

long
_pathconf(const char *path, int fd, int name)
{
    struct statvfs buf;
    int            kname;
    long           ret;

    kname = _pathconf_name_to_freebsd(name);
    if (kname >= 0) {
        if (path)
            ret = _syscall_raw(FREEBSD_SYS_pathconf, path, kname);
        else
            ret = _syscall_raw(FREEBSD_SYS_fpathconf, fd, kname);
        /* -1 is no limit (or EPERM, which pathconf doesn't report) */
        if (ret >= -1)
            return ret;
        /* Some file systems (ZFS) leave many variables to the library */
        if (ret != _FREEBSD_ERR(FREEBSD_EINVAL)) {
            errno = _syscall_errno(ret);
            return -1;
        }
    }

    /* The library's answers, for a file that exists */
    if ((path ? statvfs(path, &buf) : fstatvfs(fd, &buf)) < 0)
        return -1;
    switch (name) {
    case _PC_CHOWN_RESTRICTED:
        return _POSIX_CHOWN_RESTRICTED;
    case _PC_NO_TRUNC:
        return _POSIX_NO_TRUNC;
    case _PC_2_SYMLINKS:
        return 1;
    case _PC_SYMLINK_MAX:
        /* FreeBSD takes link contents up to its PATH_MAX, 1024 with the null */
        return 1023;
    case _PC_TEXTDOMAIN_MAX:
        /* A domain names a "<domain>.mo" file */
        return (long)buf.f_namemax - 3;
    case _PC_TIMESTAMP_RESOLUTION:
        return _timestamp_resolution();
    case _PC_ALLOC_SIZE_MIN:
        /* Files take whole fragments */
        return (long)buf.f_frsize;
    case _PC_REC_MIN_XFER_SIZE:
    case _PC_REC_XFER_ALIGN:
        return (long)buf.f_bsize;
    case _PC_PRIO_IO:
        /* FreeBSD has no prioritized I/O */
        return -1;
    case _PC_SYNC_IO:
        /* O_SYNC and O_DSYNC work on every file system */
        return 1;
    case _PC_VDISABLE:
    case _PC_MAX_CANON:
    case _PC_MAX_INPUT:
        /* Only terminals have these */
        errno = EINVAL;
        return -1;
    }
    return __fallback_fpathconf(-1, name);
}
