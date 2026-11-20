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
/* RLIMIT_NPROC, which limits the number of threads on Linux, is an extension */
#define _DEFAULT_SOURCE
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <threads.h>
#include <time.h>

/*
 * thrd_create starts threads that thrd_join, thrd_exit and thrd_detach
 * manage, and reports the thread limit as thrd_error and a lack of
 * memory as thrd_nomem
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static thrd_t main_thread;

static int
child(void *arg)
{
    if (thrd_equal(thrd_current(), main_thread))
        return -1;
    thrd_yield();
    return *(int *)arg * 2;
}

static int
exiter(void *arg)
{
    (void)arg;
    thrd_exit(42);
}

#ifndef __GLIBC__
/* qemu-user ignores RLIMIT_AS; it shows a BogoMIPS of 100.00 */
static bool
is_qemu(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    char  line[256];
    bool  ret = false;

    if (!f)
        return false;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "BogoMIPS", 8) == 0) {
            ret = strstr(line, "100.00") != NULL;
            break;
        }
    }
    fclose(f);
    return ret;
}
#endif

/* The address space in use, in bytes, or 0 if unknown */
static unsigned long
vm_size(void)
{
    FILE         *f = fopen("/proc/self/status", "r");
    char          line[256];
    unsigned long kb = 0;

    if (!f)
        return 0;
    while (fgets(line, sizeof(line), f) != NULL)
        if (strncmp(line, "VmSize:", 7) == 0)
            kb = strtoul(line + 7, NULL, 10);
    fclose(f);
    return kb * 1024;
}

static int
nothing(void *arg)
{
    (void)arg;
    return 0;
}

/*
 * In a child, lower a resource limit so thrd_create fails, and
 * return its result as the exit status (or 100 + a setup failure)
 */
static int
limited_create(int resource)
{
    struct rlimit rl;
    thrd_t        thr;
    pid_t         pid;
    int           status;

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        void *p = malloc(64);

        /* Leave thrd_create's own small allocation room to succeed */
        free(p);
        if (getrlimit(resource, &rl) != 0)
            _exit(100);
        if (resource == RLIMIT_AS) {
            unsigned long size = vm_size();

            if (size == 0)
                _exit(101);
            rl.rlim_cur = size + 32 * 1024;
        } else {
            rl.rlim_cur = 0;
        }
        if (setrlimit(resource, &rl) != 0)
            _exit(102);
        _exit(thrd_create(&thr, nothing, NULL));
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    check(WEXITSTATUS(status) < 100, "child setup");
    return WEXITSTATUS(status);
}

int
main(void)
{
    struct timespec ts = { 0, 1000000 };
    thrd_t          thr;
    int             value = 21, res = 0;

    main_thread = thrd_current();
    check(thrd_equal(main_thread, thrd_current()), "thrd_equal");
    check(thrd_create(&thr, child, &value) == thrd_success, "thrd_create");
    check(!thrd_equal(thr, main_thread), "distinct threads");
    check(thrd_join(thr, &res) == thrd_success && res == 42, "thrd_join result");

    check(thrd_create(&thr, exiter, NULL) == thrd_success, "thrd_create exiter");
    check(thrd_join(thr, &res) == thrd_success && res == 42, "thrd_exit result");

    check(thrd_create(&thr, child, &value) == thrd_success, "thrd_create detached");
    check(thrd_detach(thr) == thrd_success, "thrd_detach");

    check(thrd_sleep(&ts, NULL) == 0, "thrd_sleep");

    /*
     * The limit on the number of threads is thrd_error, not
     * thrd_nomem. Linux counts threads against RLIMIT_NPROC, which
     * doesn't bind a privileged user; picolibc doesn't provide it.
     */
#ifdef RLIMIT_NPROC
    if (getuid() != 0) {
        res = limited_create(RLIMIT_NPROC);
        printf("thrd_create beyond RLIMIT_NPROC: %d\n", res);
        check(res == thrd_error, "thrd_create at the thread limit");
    } else {
        printf("running as root, RLIMIT_NPROC check skipped\n");
    }
#endif

#ifndef __GLIBC__
    /* glibc reports a stack it can't map as EAGAIN, so thrd_error */
    if (!is_qemu()) {
        res = limited_create(RLIMIT_AS);
        printf("thrd_create with no address space left: %d\n", res);
        check(res == thrd_nomem, "thrd_create without memory");
    } else {
        printf("qemu ignores RLIMIT_AS, thrd_nomem check skipped\n");
    }
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
