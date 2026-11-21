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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_SIGNALS) \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>

/*
 * WCOREDUMP reports whether a core image was attempted, which waitid
 * reports as CLD_DUMPED, and RLIMIT_CORE 0 prevents the core file.
 * Core files go to a directory of the test's own, and only when the
 * system writes them to the current directory.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* glibc 2.43 defines WCOREDUMP only with _DEFAULT_SOURCE */
#ifdef WCOREDUMP
#define coredump(status) (WCOREDUMP(status) != 0)
#else
#define coredump(status) (-1)
#endif

#define CORE_MAX (64 * 1024 * 1024)

/* Kill a child with sig, with RLIMIT_CORE set to core, and check its status */
static int
killed(char *dir, int sig, rlim_t core)
{
    siginfo_t info;
    pid_t     pid;
    int       status;

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit rl;

        if (chdir(dir) != 0 || getrlimit(RLIMIT_CORE, &rl) != 0)
            _exit(1);
        if (rl.rlim_max != RLIM_INFINITY && rl.rlim_max < core)
            core = rl.rlim_max;
        rl.rlim_cur = core;
        if (setrlimit(RLIMIT_CORE, &rl) != 0)
            _exit(2);
        signal(sig, SIG_DFL);
        raise(sig);
        _exit(3);
    }
    memset(&info, 0, sizeof(info));
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOWAIT) == 0, "waitid");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFSIGNALED(status) && WTERMSIG(status) == sig, "killed by the signal");
    check(info.si_code == CLD_KILLED || info.si_code == CLD_DUMPED, "si_code");
#ifdef WCOREDUMP
    check(coredump(status) == (info.si_code == CLD_DUMPED), "WCOREDUMP matches CLD_DUMPED");
#endif
    return info.si_code == CLD_DUMPED;
}

/* Count, and remove, the files in dir */
static int
empty(char *dir)
{
    DIR           *d = opendir(dir);
    struct dirent *e;
    char           path[512];
    int            n = 0;

    check(d != NULL, "opendir");
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        check(unlink(path) == 0, "unlink");
        n++;
    }
    closedir(d);
    return n;
}

/* Cores are written to the current directory unless the pattern is a path or a pipe */
static int
local_cores(void)
{
    FILE *f = fopen("/proc/sys/kernel/core_pattern", "r");
    char  pattern[256];
    int   local = 0;

    if (!f)
        return 0;
    if (fgets(pattern, sizeof(pattern), f))
        local = pattern[0] != '|' && strchr(pattern, '/') == NULL;
    fclose(f);
    return local;
}

int
main(void)
{
    char dir[] = "wcoredump-XXXXXX";
    int  dumped;

    check(mkdtemp(dir) != NULL, "mkdtemp");

    /* SIGKILL never dumps core */
    check(killed(dir, SIGKILL, CORE_MAX) == 0, "SIGKILL dumped core");

    if (local_cores()) {
        /* A limit of 0 prevents the core file */
        check(killed(dir, SIGQUIT, 0) == 0 && empty(dir) == 0, "core file with RLIMIT_CORE 0");

        /* SIGQUIT dumps core where the system can */
        dumped = killed(dir, SIGQUIT, CORE_MAX);
        printf("SIGQUIT %s\n", dumped ? "dumped core" : "didn't dump core");
        (void)empty(dir);
    } else {
        printf("cores aren't written to the current directory, SIGQUIT not checked\n");
    }
    check(rmdir(dir) == 0, "rmdir");

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
