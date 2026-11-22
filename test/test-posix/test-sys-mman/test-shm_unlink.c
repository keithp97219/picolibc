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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_MAPPED_FILES && TEST_POSIX_SHARED_MEMORY_OBJECTS && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Shared memory objects: descriptors, flags, permissions, names and unlinking */

static char name[64];
static char name2[64];

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
cleanup(void)
{
    if (name[0])
        (void)shm_unlink(name);
    if (name2[0])
        (void)shm_unlink(name2);
}

#define NCHILD 8

int
main(void)
{
    static char long_name[PATH_MAX + 2];
    struct stat st;
    char       *map;
    char        c;
    int         fd, fd2, low, status, created;
    pid_t       pids[NCHILD];
    mode_t      mask;

    snprintf(name, sizeof(name), "/picolibc-test-shm_unlink-%ld", (long)getpid());
    snprintf(name2, sizeof(name2), "/picolibc-test-shm_unlink-%ld-2", (long)getpid());
    cleanup();

    /* A new object: size zero, mode less the umask, FD_CLOEXEC, lowest descriptor */
    mask = umask(022);
    low = open("/dev/null", O_RDONLY);
    check(low >= 0, "open");
    check(close(low) == 0, "close");
    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0666);
    umask(mask);
    check(fd == low, "lowest descriptor");
    check(fcntl(fd, F_GETFD) & FD_CLOEXEC, "FD_CLOEXEC");
    check((fcntl(fd, F_GETFL) & O_ACCMODE) == O_RDWR, "O_RDWR");
    check(fstat(fd, &st) == 0 && st.st_size == 0, "size zero");
    check((st.st_mode & 0777) == 0644, "mode less the umask");
    check(st.st_uid == geteuid(), "owner");
    check(ftruncate(fd, 4096) == 0, "ftruncate");

    /* Each open has its own open file description */
    fd2 = shm_open(name, O_RDWR, 0);
    check(fd2 >= 0 && fd2 != fd, "second open");
    check(lseek(fd, 2, SEEK_SET) == 2, "lseek");
    check(lseek(fd2, 0, SEEK_CUR) == 0, "separate offsets");
    check(close(fd2) == 0, "close");

    /* O_RDONLY gives a read-only description */
    fd2 = shm_open(name, O_RDONLY, 0);
    check(fd2 >= 0, "O_RDONLY");
    check((fcntl(fd2, F_GETFL) & O_ACCMODE) == O_RDONLY, "access mode O_RDONLY");
    errno = 0;
    check(write(fd2, "x", 1) == -1 && errno == EBADF, "write to O_RDONLY");
    errno = 0;
    check(mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0) == MAP_FAILED
              && errno == EACCES,
          "writable mapping of O_RDONLY");
    check(close(fd2) == 0, "close");

    /* O_CREAT without O_EXCL opens the existing object; O_TRUNC empties it */
    fd2 = shm_open(name, O_RDWR | O_CREAT, 0600);
    check(fd2 >= 0, "O_CREAT existing");
    check(fstat(fd2, &st) == 0 && st.st_size == 4096 && (st.st_mode & 0777) == 0644,
          "existing object unchanged");
    check(close(fd2) == 0, "close");
    fd2 = shm_open(name, O_RDWR | O_TRUNC, 0);
    check(fd2 >= 0, "O_TRUNC");
    check(fstat(fd2, &st) == 0 && st.st_size == 0 && (st.st_mode & 0777) == 0644,
          "truncated, mode kept");
    check(close(fd2) == 0, "close");
    check(ftruncate(fd, 4096) == 0, "ftruncate");

    /* Permissions: only when they apply to this process */
    if (geteuid() != 0) {
        check(fchmod(fd, 0400) == 0, "fchmod");
        errno = 0;
        check(shm_open(name, O_RDWR, 0) == -1 && errno == EACCES,
              "O_RDWR without write permission");
        fd2 = shm_open(name, O_RDONLY, 0);
        check(fd2 >= 0, "O_RDONLY with read permission");
        check(close(fd2) == 0, "close");
        check(fchmod(fd, 0600) == 0, "fchmod");
    } else {
        printf("running as root, skipping EACCES\n");
    }

    /* Unlinking removes the name at once; the object lives while referenced */
    map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    check(map != MAP_FAILED, "mmap");
    /* write on a shared memory object is unspecified; store through the mapping */
    memcpy(map, "data", 4);
    check(close(fd) == 0, "close");
    check(shm_unlink(name) == 0, "shm_unlink");
    errno = 0;
    check(shm_open(name, O_RDWR, 0) == -1 && errno == ENOENT, "name removed");
    check(memcmp(map, "data", 4) == 0, "contents still mapped");
    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "new object with the old name");
    check(fstat(fd, &st) == 0 && st.st_size == 0, "new object is empty");
    check(pread(fd, &c, 1, 0) == 0, "nothing to read");
    check(close(fd) == 0, "close");
    check(munmap(map, 4096) == 0, "munmap");
    check(shm_unlink(name) == 0, "shm_unlink");

    /* O_CREAT | O_EXCL is atomic: exactly one of several processes creates the object */
    fflush(stdout);
    for (int i = 0; i < NCHILD; i++) {
        pids[i] = fork();
        check(pids[i] >= 0, "fork");
        if (pids[i] == 0) {
            int cfd = shm_open(name2, O_RDWR | O_CREAT | O_EXCL, 0600);

            if (cfd >= 0)
                _exit(0);
            _exit(errno == EEXIST ? 1 : 2);
        }
    }
    created = 0;
    for (int i = 0; i < NCHILD; i++) {
        check(waitpid(pids[i], &status, 0) == pids[i] && WIFEXITED(status), "child");
        check(WEXITSTATUS(status) != 2, "child error");
        if (WEXITSTATUS(status) == 0)
            created++;
    }
    check(created == 1, "one creator");
    check(shm_unlink(name2) == 0, "shm_unlink");
    errno = 0;
    check(shm_unlink(name2) == -1 && errno == ENOENT, "shm_unlink missing");
    name2[0] = '\0';

    /* Names */
    memset(long_name, 'n', sizeof(long_name) - 1);
    long_name[0] = '/';
    errno = 0;
    check(shm_open(long_name, O_RDWR | O_CREAT, 0600) == -1
              && (errno == ENAMETOOLONG || errno == EINVAL),
          "long name");
    errno = 0;
    check(shm_unlink(long_name) == -1 && (errno == ENAMETOOLONG || errno == ENOENT),
          "shm_unlink long name");
    /* glibc 2.43 opens its shared memory directory or its parent for these */
#ifndef __GLIBC__
    errno = 0;
    check(shm_open("/.", O_RDONLY, 0) == -1 && errno == EINVAL, "name /.");
    errno = 0;
    check(shm_open("/..", O_RDONLY, 0) == -1 && errno == EINVAL, "name /..");
    errno = 0;
    check(shm_unlink("/..") == -1 && errno == EINVAL, "shm_unlink /..");
#endif
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
