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
#if _POSIX_MAPPED_FILES > 0 && _POSIX_SHARED_MEMORY_OBJECTS > 0 \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

/* Shared memory objects */

static char name[64];

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            if (name[0])                               \
                (void)shm_unlink(name);                \
            exit(1);                                   \
        }                                              \
    } while (0)

#define SIZE 4096

int
main(void)
{
    char *map;
    int   fd, status;
    pid_t pid;

    snprintf(name, sizeof(name), "/picolibc-test-shm-%ld", (long)getpid());
    (void)shm_unlink(name);

    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "shm_open create");
    check(shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600) == -1 && errno == EEXIST,
          "shm_open O_EXCL existing");
    /* write on a shared memory object is unspecified; ftruncate sizes it */
    check(ftruncate(fd, SIZE) == 0, "size object");
    map = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    check(map != MAP_FAILED, "mmap");
    check(close(fd) == 0, "close");

    /* Another process opens the object by name and writes to it */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        char *child;

        fd = shm_open(name, O_RDWR, 0);
        if (fd < 0)
            _exit(1);
        child = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (child == MAP_FAILED)
            _exit(2);
        strcpy(child, "hello from the child");
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status");
    check(strcmp(map, "hello from the child") == 0, "shared contents");
    check(munmap(map, SIZE) == 0, "munmap");

    check(shm_unlink(name) == 0, "shm_unlink");
    check(shm_open(name, O_RDWR, 0) == -1 && errno == ENOENT, "shm_open after unlink");
    check(shm_unlink(name) == -1 && errno == ENOENT, "shm_unlink missing");
    name[0] = '\0';

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
