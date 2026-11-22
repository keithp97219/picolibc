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

#define _DEFAULT_SOURCE
#include <unistd.h>
#include "../test-flags.h"

#include <stdio.h>

/*
 * Linux's statfs and fstatfs (<sys/vfs.h>, not POSIX): they agree with
 * statvfs, and give the file system type statvfs doesn't
 */

#if defined(__linux__) && TEST_POSIX_MULTI_PROCESS && __has_include(<sys/vfs.h>)
#include <fcntl.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

int
main(void)
{
    struct statfs  fs, ffs;
    struct statvfs vfs;
    int            fd;

    if (statfs(".", &fs) != 0 || statvfs(".", &vfs) != 0) {
        printf("statfs or statvfs failed\n");
        return 1;
    }
    if (fs.f_type == 0 || fs.f_blocks != vfs.f_blocks || fs.f_namelen != (long)vfs.f_namemax) {
        printf("statfs: type %#lx blocks %llu namelen %ld, statvfs blocks %llu namemax %lu\n",
               fs.f_type, (unsigned long long)fs.f_blocks, fs.f_namelen,
               (unsigned long long)vfs.f_blocks, vfs.f_namemax);
        return 1;
    }
    fd = open(".", O_RDONLY);
    if (fd < 0 || fstatfs(fd, &ffs) != 0 || ffs.f_type != fs.f_type) {
        printf("fstatfs\n");
        return 1;
    }
    close(fd);
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no statfs, skipping\n");
    return 77;
}

#endif
