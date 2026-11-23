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

#define _POSIX_C_SOURCE 202405L
/* gethostid is an XSI interface */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_XSI_SINGLE_PROCESS

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/*
 * gethostid returns the same 32-bit identifier each time, in every
 * process, and the one in /etc/hostid when that file has one, except
 * on FreeBSD, where the file holds the host's UUID as text and the
 * identifier is the kern.hostid sysctl. The identifier itself is never
 * printed.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    int32_t file_id = 0;
    int     have_file = 0, fd, status;
    long    id;
    pid_t   pid;

#ifndef __FreeBSD__
    fd = open("/etc/hostid", O_RDONLY);
    if (fd >= 0) {
        have_file = read(fd, &file_id, sizeof(file_id)) == (ssize_t)sizeof(file_id);
        close(fd);
    }
#else
    (void)fd;
#endif
#ifdef __GLIBC__
    /* Without /etc/hostid, glibc looks the host name up, perhaps in DNS */
    if (!have_file) {
        printf("no /etc/hostid, skipping for glibc\n");
        return 77;
    }
#endif

    id = gethostid();
    check(gethostid() == id, "stable");
#if LONG_MAX > INT32_MAX
    check(id >= INT32_MIN && id <= (long)UINT32_MAX, "32 bits");
#endif
    if (have_file)
        check((int32_t)id == file_id, "the identifier in /etc/hostid");

    /* The same host, the same identifier */
#if TEST_POSIX_MULTI_PROCESS
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(gethostid() == id ? 0 : 1);
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the same in another process");
#else
    (void)pid;
    (void)status;
#endif
    printf("success\n");
    return 0;
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
