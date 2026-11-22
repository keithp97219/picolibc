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

#define _GNU_SOURCE
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_FD

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
 * fstatat with AT_EMPTY_PATH (Linux and FreeBSD, not POSIX) stats the
 * descriptor itself when the path is empty, or a null pointer, which
 * is the same as fstat
 */

int
main(void)
{
#ifdef AT_EMPTY_PATH
    struct stat by_fd, st;
    int         fd = open(".", O_RDONLY);

    if (fd < 0 || fstat(fd, &by_fd) != 0) {
        printf("open .\n");
        return 1;
    }
    if (fstatat(fd, "", &st, AT_EMPTY_PATH) != 0 || st.st_ino != by_fd.st_ino
        || st.st_dev != by_fd.st_dev) {
        printf("fstatat(fd, \"\", AT_EMPTY_PATH): errno %d\n", errno);
        return 1;
    }
    if (fstatat(fd, NULL, &st, AT_EMPTY_PATH) != 0 || st.st_ino != by_fd.st_ino) {
        printf("fstatat(fd, NULL, AT_EMPTY_PATH): errno %d\n", errno);
        return 1;
    }
    errno = 0;
    if (fstatat(fd, "", &st, 0) == 0 || errno != ENOENT) {
        printf("fstatat(fd, \"\", 0) should fail with ENOENT\n");
        return 1;
    }
    close(fd);
    printf("success\n");
    return 0;
#else
    printf("no AT_EMPTY_PATH, skipping\n");
    return 77;
#endif
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
