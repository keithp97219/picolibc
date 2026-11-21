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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_MAPPED_FILES > 0 && _POSIX_MEMORY_PROTECTION > 0 \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>

/* mprotect: protections take effect on whole pages, and the errors */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-mprotect.txt"
#endif

static char file[256];

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            if (file[0])                               \
                (void)unlink(file);                    \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Touch *addr in a child process; returns the signal that killed it, or 0 */
static int
touch_signal(volatile char *addr, int write)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        if (write)
            *addr = 1;
        else
            (void)*addr;
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (WIFSIGNALED(status))
        return WTERMSIG(status);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status");
    return 0;
}

int
main(void)
{
    long   ps = sysconf(_SC_PAGESIZE);
    size_t page;
    char  *p;
    int    fd;

    check(ps > 0, "_SC_PAGESIZE");
    page = (size_t)ps;
    snprintf(file, sizeof(file), "%s-%ld", TEST_FILE_NAME, (long)getpid());

    p = mmap(NULL, 3 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap");
    memset(p, 'a', 3 * page);

    /* A partial page changes the whole page, and no other */
    check(mprotect(p, 1, PROT_READ) == 0, "PROT_READ");
    check(touch_signal(p + page - 1, 1) == SIGSEGV, "write to PROT_READ");
    check(touch_signal(p, 0) == 0, "read PROT_READ");
    check(touch_signal(p + page, 1) == 0, "next page still writable");

    check(mprotect(p, page, PROT_NONE) == 0, "PROT_NONE");
    check(touch_signal(p, 0) == SIGSEGV, "read PROT_NONE");
    check(touch_signal(p, 1) == SIGSEGV, "write PROT_NONE");

    check(mprotect(p, page, PROT_WRITE) == 0, "PROT_WRITE");
    check(touch_signal(p, 1) == 0, "write PROT_WRITE");

    check(mprotect(p, page, PROT_READ | PROT_WRITE) == 0, "PROT_READ|PROT_WRITE");
    p[0] = 'b';
    check(p[0] == 'b' && p[1] == 'a' && p[page - 1] == 'a', "contents kept");

    /* Errors */
    errno = 0;
    check(mprotect(p + 1, page, PROT_READ) == -1 && errno == EINVAL, "address not page aligned");
    check(munmap(p + page, page) == 0, "munmap middle page");
    errno = 0;
    check(mprotect(p, 3 * page, PROT_READ) == -1 && errno == ENOMEM, "range with a hole");
    errno = 0;
    check(mprotect(p + page, page, PROT_READ) == -1 && errno == ENOMEM, "unmapped page");
    check(munmap(p, 3 * page) == 0, "munmap");

    /* The file's access mode limits PROT_WRITE on MAP_SHARED, even after close */
    (void)unlink(file);
    fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0, "create");
    check(write(fd, "data", 4) == 4, "write");
    check(close(fd) == 0, "close");
    fd = open(file, O_RDONLY);
    check(fd >= 0, "open read-only");
    p = mmap(NULL, page, PROT_READ, MAP_SHARED, fd, 0);
    check(p != MAP_FAILED, "mmap MAP_SHARED");
    check(close(fd) == 0, "close");
    errno = 0;
    check(mprotect(p, page, PROT_READ | PROT_WRITE) == -1 && errno == EACCES,
          "PROT_WRITE on a read-only MAP_SHARED file");
    check(touch_signal(p, 1) == SIGSEGV, "still read-only");
    check(munmap(p, page) == 0, "munmap");

    /* MAP_PRIVATE copies may be written */
    fd = open(file, O_RDONLY);
    check(fd >= 0, "open read-only");
    p = mmap(NULL, page, PROT_READ, MAP_PRIVATE, fd, 0);
    check(p != MAP_FAILED, "mmap MAP_PRIVATE");
    check(close(fd) == 0, "close");
    check(mprotect(p, page, PROT_READ | PROT_WRITE) == 0, "PROT_WRITE on MAP_PRIVATE");
    p[0] = 'D';
    check(p[0] == 'D', "private write");
    check(munmap(p, page) == 0, "munmap");

    (void)unlink(file);
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
