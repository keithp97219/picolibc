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
/* RLIMIT_CPU and SIGXCPU/SIGXFSZ are XSI */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_SIGNALS) \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "../is-qemu.h"

/*
 * The limits are enforced: RLIMIT_NOFILE with EMFILE, RLIMIT_FSIZE
 * with SIGXFSZ or EFBIG, RLIMIT_CPU with SIGXCPU, RLIMIT_DATA and
 * RLIMIT_AS with ENOMEM, RLIMIT_STACK with SIGSEGV, and exec keeps
 * them. Each limit is set in a child.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define MiB (1024 * 1024)

static void
set_soft(int resource, rlim_t cur)
{
    struct rlimit rl;

    check(getrlimit(resource, &rl) == 0, "getrlimit");
    if (rl.rlim_max != RLIM_INFINITY && rl.rlim_max < cur)
        cur = rl.rlim_max;
    rl.rlim_cur = cur;
    check(setrlimit(resource, &rl) == 0, "setrlimit");
}

/* Run fn in a child and return its wait status */
static int
in_child(void (*fn)(void))
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        /* No core files from the signals the limits send */
        set_soft(RLIMIT_CORE, 0);
        fn();
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    return status;
}

#define exited_ok(status) (WIFEXITED(status) && WEXITSTATUS(status) == 0)

static void
nofile(void)
{
    int fd;

    set_soft(RLIMIT_NOFILE, 8);
    while ((fd = dup(0)) >= 0)
        check(fd < 8, "descriptor below RLIMIT_NOFILE");
    check(errno == EMFILE, "dup EMFILE");
    check(fcntl(7, F_GETFD) != -1, "every descriptor below the limit was used");
    errno = 0;
    check(open(".", O_RDONLY) == -1 && errno == EMFILE, "open EMFILE");
}

static char template[] = "setrlimit-enforce-XXXXXX";
static char path[sizeof(template)];

static int
temp_file(void)
{
    int fd;

    memcpy(path, template, sizeof(template));
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp");
    check(unlink(path) == 0, "unlink");
    return fd;
}

static void
fsize_ignored(void)
{
    char buf[200];
    int  fd = temp_file();

    memset(buf, 'x', sizeof(buf));
    signal(SIGXFSZ, SIG_IGN);
    set_soft(RLIMIT_FSIZE, 100);
    /* Only what fits is written, then nothing */
    check(write(fd, buf, sizeof(buf)) == 100, "partial write up to the limit");
    errno = 0;
    check(write(fd, buf, 1) == -1 && errno == EFBIG, "write EFBIG");
    errno = 0;
    check(ftruncate(fd, 200) == -1 && errno == EFBIG, "ftruncate EFBIG");
    check(ftruncate(fd, 50) == 0, "ftruncate within the limit");
}

static void
fsize_blocked(void)
{
    sigset_t set;
    char     buf[200];
    int      fd = temp_file(), sig;

    memset(buf, 'x', sizeof(buf));
    sigemptyset(&set);
    sigaddset(&set, SIGXFSZ);
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "block SIGXFSZ");
    set_soft(RLIMIT_FSIZE, 100);
    check(write(fd, buf, 100) == 100, "write up to the limit");
    errno = 0;
    check(write(fd, buf, 1) == -1 && errno == EFBIG, "blocked: write EFBIG");
    /* The signal was still generated for the thread */
    check(sigwait(&set, &sig) == 0 && sig == SIGXFSZ, "SIGXFSZ pending");
}

static void
fsize_default(void)
{
    char buf[200];
    int  fd = temp_file();

    memset(buf, 'x', sizeof(buf));
    set_soft(RLIMIT_FSIZE, 100);
    (void)write(fd, buf, 100);
    (void)write(fd, buf, 1);
    _exit(2);
}

static volatile unsigned long spin;

static void
cpu(void)
{

    /* A safety net in case SIGXCPU never comes */
    alarm(120);
    set_soft(RLIMIT_CPU, 1);
    for (;;)
        spin++;
}

/* Stored, so the compiler can't assume the allocation succeeds and drop it */
static void * volatile allocated;

static void
data(void)
{
    set_soft(RLIMIT_DATA, 32 * MiB);
    errno = 0;
    allocated = malloc(256 * MiB);
    check(allocated == NULL && errno == ENOMEM, "malloc past RLIMIT_DATA");
}

static void
as(void)
{
    set_soft(RLIMIT_AS, 512 * MiB);
    errno = 0;
    allocated = malloc(1024 * MiB);
    check(allocated == NULL && errno == ENOMEM, "malloc past RLIMIT_AS");
#ifdef MAP_ANONYMOUS
    errno = 0;
    allocated = mmap(NULL, 1024 * MiB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(allocated == MAP_FAILED && errno == ENOMEM, "mmap past RLIMIT_AS");
#endif
}

static int
recurse(int depth)
{
    volatile char frame[4096];

    frame[0] = (char)depth;
    frame[sizeof(frame) - 1] = (char)depth;
    if (depth > 1000000)
        return 0;
    return recurse(depth + 1) + frame[0] + frame[sizeof(frame) - 1];
}

static void
on_segv(int sig)
{
    (void)sig;
    _exit(3);
}

static void
stack(void)
{
    /* Without an alternate stack, the handler can't run and SIGSEGV kills */
    signal(SIGSEGV, on_segv);
    set_soft(RLIMIT_STACK, 256 * 1024);
    (void)recurse(0);
    _exit(2);
}

static char exe[1024];

static void
exec_keeps(void)
{
    char *args[] = { exe, "check-nofile", NULL };

    set_soft(RLIMIT_NOFILE, 20);
    execv(exe, args);
    _exit(4);
}

int
main(int argc, char **argv)
{
    int     qemu = is_qemu();
    int     status;
    ssize_t len;

    if (argc > 1 && strcmp(argv[1], "check-nofile") == 0) {
        struct rlimit rl;

        return getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_cur == 20 ? 0 : 5;
    }

    status = in_child(nofile);
    check(exited_ok(status), "RLIMIT_NOFILE");

    status = in_child(fsize_ignored);
    check(exited_ok(status), "RLIMIT_FSIZE, SIGXFSZ ignored");
    status = in_child(fsize_blocked);
    check(exited_ok(status), "RLIMIT_FSIZE, SIGXFSZ blocked");
    status = in_child(fsize_default);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGXFSZ, "RLIMIT_FSIZE sends SIGXFSZ");

    status = in_child(cpu);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGXCPU, "RLIMIT_CPU sends SIGXCPU");

    if (qemu) {
        printf("Running under QEMU, which doesn't apply RLIMIT_DATA, RLIMIT_AS or RLIMIT_STACK\n");
    } else {
        status = in_child(data);
        check(exited_ok(status), "RLIMIT_DATA");
        status = in_child(as);
        check(exited_ok(status), "RLIMIT_AS");
        status = in_child(stack);
        check(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV, "RLIMIT_STACK sends SIGSEGV");
    }

    len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len > 0) {
        exe[len] = '\0';
        status = in_child(exec_keeps);
        check(exited_ok(status), "exec keeps the limits");
    } else {
        printf("no /proc/self/exe, exec not checked\n");
    }

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
