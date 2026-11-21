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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "../is-qemu.h"

/* mmap and munmap: what gets mapped where, sharing, and the errors */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-munmap.txt"
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

static size_t page;

/* Map anonymous memory, failing the test if that can't be done */
static char *
anon(size_t len, int flags)
{
    void *p = mmap(NULL, len, PROT_READ | PROT_WRITE, flags | MAP_ANONYMOUS, -1, 0);

    check(p != MAP_FAILED, "anonymous mmap");
    return p;
}

static int
file_fd(int oflag, const char *contents, size_t len)
{
    int fd;

    (void)unlink(file);
    fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0, "create file");
    check(write(fd, contents, len) == (ssize_t)len, "write file");
    check(close(fd) == 0, "close file");
    fd = open(file, oflag);
    check(fd >= 0, "open file");
    return fd;
}

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

static void
anonymous(void)
{
    char *p, *q, *r;
    pid_t pid;
    int   status;

    /* Anonymous memory is zero; a partial page is mapped as a whole page */
    p = mmap(NULL, page + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    check(p != MAP_FAILED, "MAP_ANON");
    for (size_t i = 0; i < 2 * page; i++)
        check(p[i] == 0, "zero filled");
    p[2 * page - 1] = 1;
    check(munmap(p, page + 1) == 0, "munmap partial page");

    /* MAP_SHARED survives fork; MAP_PRIVATE doesn't share the child's writes */
    p = anon(page, MAP_SHARED);
    q = anon(page, MAP_PRIVATE);
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        strcpy(p, "shared");
        strcpy(q, "private");
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
    check(strcmp(p, "shared") == 0, "MAP_SHARED kept across fork");
    check(q[0] == 0, "MAP_PRIVATE not shared");
    check(munmap(q, page) == 0, "munmap");

    /* A hint never replaces a mapping */
    q = mmap(p, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(q != MAP_FAILED && q != p, "hint at a mapped address");
    check(strcmp(p, "shared") == 0, "mapping kept");
    check(munmap(q, page) == 0, "munmap");
    check(munmap(p, page) == 0, "munmap");

    /* MAP_FIXED replaces the pages it covers, and only those */
    p = anon(3 * page, MAP_PRIVATE);
    memset(p, 'a', 3 * page);
    q = mmap(p + page, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1,
             0);
    check(q == p + page, "MAP_FIXED address");
    check(p[0] == 'a' && p[page - 1] == 'a' && p[2 * page] == 'a', "pages around MAP_FIXED");
    check(p[page] == 0 && p[2 * page - 1] == 0, "MAP_FIXED replaced the page");

    /* munmap removes whole pages; later references get SIGSEGV */
    check(munmap(p + page, 1) == 0, "munmap one byte");
    check(touch_signal(p + page + page / 2, 0) == SIGSEGV, "SIGSEGV after munmap");
    check(touch_signal(p, 0) == 0, "page before kept");
    check(touch_signal(p + 2 * page, 0) == 0, "page after kept");
    /* No mappings in the range: no effect */
    check(munmap(p + page, page) == 0, "munmap of nothing");
    check(munmap(p, 3 * page) == 0, "munmap over a hole");
    check(touch_signal(p, 0) == SIGSEGV, "SIGSEGV after munmap over a hole");

    /* A writable mapping of MAP_FIXED over a gap works too */
    r = mmap(p, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    check(r == p, "MAP_FIXED over a gap");
    check(munmap(r, page) == 0, "munmap");
}

static void
errors(void)
{
    uintptr_t top = (uintptr_t)0 - 2 * page;
    char     *p;
    int       fds[2];
    int       fd;

    errno = 0;
    check(mmap(NULL, page, PROT_READ, MAP_ANONYMOUS, -1, 0) == MAP_FAILED && errno == EINVAL,
          "neither MAP_SHARED nor MAP_PRIVATE");
    errno = 0;
    p = mmap(NULL, page, PROT_READ, MAP_SHARED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p == MAP_FAILED && errno == EINVAL, "both MAP_SHARED and MAP_PRIVATE, anonymous");
    /*
     * glibc 2.43 passes both on to Linux, which maps a file shared
     * because the two together are its MAP_SHARED_VALIDATE
     */
#ifndef __GLIBC__
    fd = file_fd(O_RDWR, "x", 1);
    errno = 0;
    p = mmap(NULL, page, PROT_READ, MAP_SHARED | MAP_PRIVATE, fd, 0);
    check(p == MAP_FAILED && errno == EINVAL, "both MAP_SHARED and MAP_PRIVATE, file");
    check(close(fd) == 0, "close");
#endif
    errno = 0;
    check(mmap(NULL, 0, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED
              && errno == EINVAL,
          "len 0");

    /* Address space */
    errno = 0;
    check(mmap(NULL, (size_t)0 - page, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED
              && errno == ENOMEM,
          "no room");
    /* QEMU's user-mode emulation places guest mappings itself */
    if (!is_qemu()) {
        errno = 0;
        check(mmap((void *)top, 2 * page, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0)
                      == MAP_FAILED
                  && errno == ENOMEM,
              "MAP_FIXED outside the address space");
    }
    p = anon(2 * page, MAP_PRIVATE);
    errno = 0;
    check(mmap(p + 1, page, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED
              && errno == EINVAL,
          "MAP_FIXED address not page aligned");

    /* munmap */
    errno = 0;
    check(munmap(p, 0) == -1 && errno == EINVAL, "munmap len 0");
    errno = 0;
    check(munmap(p + 1, page) == -1 && errno == EINVAL, "munmap address not page aligned");
    errno = 0;
    /* QEMU's user-mode emulation places guest mappings itself */
    check((munmap((void *)top, 2 * page) == -1 && errno == EINVAL) || is_qemu(),
          "munmap outside the address space");
    check(munmap(p, 2 * page) == 0, "munmap");

    /* Descriptors */
    errno = 0;
    check(mmap(NULL, page, PROT_READ, MAP_PRIVATE, -1, 0) == MAP_FAILED && errno == EBADF,
          "fd -1 without MAP_ANONYMOUS");
    fd = file_fd(O_RDONLY, "x", 1);
    check(close(fd) == 0, "close");
    errno = 0;
    check(mmap(NULL, page, PROT_READ, MAP_PRIVATE, fd, 0) == MAP_FAILED && errno == EBADF,
          "closed fd");
    check(pipe(fds) == 0, "pipe");
    errno = 0;
    /*
     * QEMU's user-mode emulation maps files itself when guest pages are
     * larger than the host's, and so accepts a pipe
     */
    p = mmap(NULL, page, PROT_READ, MAP_SHARED, fds[0], 0);
    if (p != MAP_FAILED && is_qemu() && page > 4096)
        printf("QEMU mapped a pipe, not checking ENODEV\n");
    else
        check(p == MAP_FAILED && errno == ENODEV, "pipe");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");

    /* Access modes */
    fd = file_fd(O_WRONLY, "x", 1);
    errno = 0;
    check(mmap(NULL, page, PROT_WRITE, MAP_SHARED, fd, 0) == MAP_FAILED && errno == EACCES,
          "not open for reading");
    check(close(fd) == 0, "close");
    fd = file_fd(O_RDONLY, "x", 1);
    errno = 0;
    check(mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) == MAP_FAILED
              && errno == EACCES,
          "PROT_WRITE MAP_SHARED, not open for writing");
    errno = 0;
    check(mmap(NULL, page, PROT_READ, MAP_PRIVATE, fd, 1) == MAP_FAILED && errno == EINVAL,
          "offset not page aligned");
    check(close(fd) == 0, "close");
}

static void
files(void)
{
    struct stat st;
    char       *p;
    int         fd;
    char        buf[16];

    /* MAP_PRIVATE may write a read-only file's pages without changing it */
    fd = file_fd(O_RDONLY, "0123456789", 10);
    p = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    check(p != MAP_FAILED, "MAP_PRIVATE of a read-only file");
    check(memcmp(p, "0123456789", 10) == 0, "file contents");
    /* The rest of the last page is zero */
    for (size_t i = 10; i < page; i++)
        check(p[i] == 0, "partial page zero filled");
    p[0] = 'X';
    check(pread(fd, buf, 10, 0) == 10 && buf[0] == '0', "private change not in the file");
    check(munmap(p, page) == 0, "munmap");
    check(close(fd) == 0, "close");

    /* MAP_SHARED changes the file, even after close; nothing past its end is written */
    fd = file_fd(O_RDWR, "0123456789", 10);
    p = mmap(NULL, 2 * page, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    check(p != MAP_FAILED, "MAP_SHARED");
    check(close(fd) == 0, "close with the file mapped");
    p[1] = 'Y';
    p[100] = 'Z';
    /*
     * Whole pages past the end of the file give SIGBUS. QEMU's user-mode
     * emulation fills guest pages larger than the host's with zeros
     * there instead
     */
    if (is_qemu() && page > 4096)
        printf("QEMU's large pages don't fault past the end, not checking SIGBUS\n");
    else
        check(touch_signal(p + page, 0) == SIGBUS, "SIGBUS past the end");
    check(munmap(p, 2 * page) == 0, "munmap");
    fd = open(file, O_RDONLY);
    check(fd >= 0, "reopen");
    check(fstat(fd, &st) == 0 && st.st_size == 10, "size unchanged");
    check(read(fd, buf, sizeof(buf)) == 10 && memcmp(buf, "0Y23456789", 10) == 0,
          "shared change in the file");

    /*
     * An offset whose page number doesn't fit the 32-bit mmap2 call
     * must not wrap around to the start of the file
     */
    p = mmap(NULL, page, PROT_READ, MAP_SHARED, fd, (off_t)1 << 44);
    if (p != MAP_FAILED) {
        check(touch_signal(p, 0) == SIGBUS, "offset 2^44 maps past the end");
        check(munmap(p, page) == 0, "munmap");
    } else {
        check(errno == EINVAL || errno == EOVERFLOW, "offset 2^44");
    }
    check(close(fd) == 0, "close");
}

int
main(void)
{
    long ps = sysconf(_SC_PAGESIZE);

    check(ps > 0, "_SC_PAGESIZE");
    page = (size_t)ps;
    snprintf(file, sizeof(file), "%s-%ld", TEST_FILE_NAME, (long)getpid());

    anonymous();
    errors();
    files();

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
