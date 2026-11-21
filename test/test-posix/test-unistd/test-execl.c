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
/* sigaltstack and nice are XSI */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_FILE_SYSTEM) \
    && defined(__PICOLIBC_POSIX_SIGNALS) && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS)

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../is-qemu.h"

/*
 * Each exec function runs this program again (through /proc/self/exe)
 * in "state" mode, where it checks what the new image inherited, and
 * the errors are checked in the parent.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define HELPER_NAME "picolibc-exec-helper"
#define ARGV0       "helper-argv0"

extern char **environ;

static char   exe[PATH_MAX];
static char   dir[PATH_MAX];

static void
caught(int sig)
{
    (void)sig;
}

/* Returns 0 when the state matches, or the number of the first check that failed */
static int
helper(int argc, char **argv)
{
    int              pid, ppid, pgrp, sid, fd, cloexec_fd, dir_fd, prio;
    char             cwd[PATH_MAX];
    struct sigaction sa;
    sigset_t         set;
    stack_t          ss;
    unsigned         left;
    const char      *locale;

    if (argc != 5 || strcmp(argv[0], ARGV0) != 0 || argv[5] != NULL)
        return 10;
    if (sscanf(argv[2], "%d:%d:%d:%d:%d:%d:%d:%d", &pid, &ppid, &pgrp, &sid, &fd, &cloexec_fd,
               &dir_fd, &prio)
        != 8)
        return 10;
    /* The environment comes from environ, or from envp */
    if (strcmp(argv[4], "inherit") == 0) {
        const char *v = getenv("PICOLIBC_EXEC");
        if (!v || strcmp(v, "inherit") != 0)
            return 11;
    } else if (!environ || !environ[0] || strcmp(environ[0], "PICOLIBC_EXEC=envp") != 0
               || (environ[1] != NULL
                   && (strncmp(environ[1], "UBSAN_OPTIONS=", 14) != 0 || environ[2] != NULL))) {
        /* envp may carry the sanitizer runtime's options along */
        return 11;
    }
    if (getpid() != pid)
        return 12;
    if (getppid() != ppid)
        return 13;
    if (getpgrp() != pgrp || getsid(0) != sid)
        return 14;
    /* Open descriptors stay open, with their file offset */
    if (fcntl(fd, F_GETFD) != 0 || lseek(fd, 0, SEEK_CUR) != 3)
        return 15;
    errno = 0;
    if (fcntl(cloexec_fd, F_GETFD) != -1 || errno != EBADF)
        return 16;
    /* Directory streams are closed */
    errno = 0;
    if (fcntl(dir_fd, F_GETFD) != -1 || errno != EBADF)
        return 17;
    if (getcwd(cwd, sizeof(cwd)) == NULL || strcmp(cwd, argv[3]) != 0)
        return 18;
    if (umask(0) != 027)
        return 19;
    /* Ignored signals stay ignored, caught ones go back to the default, and flags are cleared */
    if (sigaction(SIGINT, NULL, &sa) != 0 || sa.sa_handler != SIG_IGN
        || (sa.sa_flags & SA_ONSTACK) != 0)
        return 20;
    if (sigaction(SIGTERM, NULL, &sa) != 0 || sa.sa_handler != SIG_DFL)
        return 21;
    if (sigaction(SIGUSR1, NULL, &sa) != 0 || sa.sa_handler != SIG_DFL
        || (sa.sa_flags & SA_ONSTACK) != 0)
        return 22;
    /* The signal mask and pending signals are kept */
    if (sigprocmask(SIG_BLOCK, NULL, &set) != 0 || sigismember(&set, SIGUSR1) != 1
        || sigismember(&set, SIGUSR2) != 1)
        return 23;
    if (sigpending(&set) != 0 || sigismember(&set, SIGUSR2) != 1)
        return 24;
    /* The alternate signal stack is not */
    if (sigaltstack(NULL, &ss) != 0 || !(ss.ss_flags & SS_DISABLE))
        return 25;
    /* So is the time left until the alarm */
    left = alarm(0);
    if (left < 1 || left > 100)
        return 26;
    /* The locale starts as "C" */
    locale = setlocale(LC_ALL, NULL);
    if (!locale || strcmp(locale, "C") != 0)
        return 27;
    errno = 0;
    if (nice(0) != prio || errno != 0)
        return 28;
    return 0;
}

enum how { EXECL, EXECLE, EXECLP, EXECV, EXECVE, EXECVP, FEXECVE, FEXECVE_O_EXEC, NHOW };

static const char * const names[NHOW] = {
    "execl", "execle", "execlp", "execv", "execve", "execvp", "fexecve", "fexecve O_EXEC",
};

/* Sets up state for the new image to check, then runs it */
static void
run_child(enum how how)
{
    char             info[128];
    /*
     * The sanitizer runtime in the -native tests reads its options from
     * the environment, so hand them on even with an explicit envp
     */
    static char      ubsan[256];
    const char      *ubsan_options = getenv("UBSAN_OPTIONS");
    char            *envp[] = { "PICOLIBC_EXEC=envp", NULL, NULL };
    const char      *envmode = "inherit";
    char             path[PATH_MAX + 32];
    int              fd, cloexec_fd, efd = -1, prio;
    DIR             *d;
    struct sigaction sa;
    sigset_t         set;
    static char      altstack[65536];
    stack_t          ss;

    if (ubsan_options
        && (size_t)snprintf(ubsan, sizeof(ubsan), "UBSAN_OPTIONS=%s", ubsan_options)
            < sizeof(ubsan))
        envp[1] = ubsan;

    if (chdir(dir) != 0)
        _exit(2);
    snprintf(path, sizeof(path), "%s/file", dir);
    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0 || write(fd, "abc", 3) != 3)
        _exit(3);
    cloexec_fd = open(path, O_RDONLY | O_CLOEXEC);
    d = opendir(".");
    if (cloexec_fd < 0 || !d)
        _exit(4);
    umask(027);

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_ONSTACK;
    if (sigaction(SIGINT, &sa, NULL) != 0)
        _exit(5);
    sa.sa_handler = caught;
    if (sigaction(SIGTERM, &sa, NULL) != 0 || sigaction(SIGUSR1, &sa, NULL) != 0)
        _exit(6);
    ss.ss_sp = altstack;
    ss.ss_size = sizeof(altstack);
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) != 0)
        _exit(7);
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    if (sigprocmask(SIG_BLOCK, &set, NULL) != 0 || raise(SIGUSR2) != 0)
        _exit(8);
    alarm(100);
    /* A higher nice value is allowed without privileges */
    errno = 0;
    prio = nice(1);
    if (prio == -1 && errno != 0)
        _exit(9);

    snprintf(info, sizeof(info), "%d:%d:%d:%d:%d:%d:%d:%d", (int)getpid(), (int)getppid(),
             (int)getpgrp(), (int)getsid(0), fd, cloexec_fd, dirfd(d), prio);
    if (how == EXECLE || how == EXECVE || how == FEXECVE || how == FEXECVE_O_EXEC)
        envmode = "envp";
    char * const argv[] = { ARGV0, "state", info, dir, (char *)envmode, NULL };

    switch (how) {
    case EXECL:
        execl(exe, ARGV0, "state", info, dir, envmode, (char *)NULL);
        break;
    case EXECLE:
        execle(exe, ARGV0, "state", info, dir, envmode, (char *)NULL, envp);
        break;
    case EXECLP:
        execlp(HELPER_NAME, ARGV0, "state", info, dir, envmode, (char *)NULL);
        break;
    case EXECV:
        execv(exe, argv);
        break;
    case EXECVE:
        execve(exe, argv, envp);
        break;
    case EXECVP:
        execvp(HELPER_NAME, argv);
        break;
    case FEXECVE:
        efd = open(exe, O_RDONLY | O_CLOEXEC);
        /* The file offset doesn't matter */
        if (efd < 0 || lseek(efd, 0, SEEK_END) < 0)
            _exit(9);
        fexecve(efd, argv, envp);
        break;
    case FEXECVE_O_EXEC:
#ifdef O_EXEC
        efd = open(exe, O_EXEC | O_CLOEXEC);
        if (efd < 0)
            _exit(9);
        fexecve(efd, argv, envp);
#else
        _exit(77);
#endif
        break;
    default:
        break;
    }
    _exit(100 + errno);
}

static int
run(enum how how)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        run_child(how);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    return WEXITSTATUS(status);
}

/* Expects exec_err to return -1 with errno set to err */
#define check_err(call, err)                                                         \
    do {                                                                             \
        errno = 0;                                                                   \
        int _r = (call);                                                             \
        if (_r != -1 || errno != (err)) {                                            \
            printf("%s: returned %d errno %d (%s), expected %s\n", #call, _r, errno, \
                   strerror(errno), #err);                                           \
            exit(1);                                                                 \
        }                                                                            \
    } while (0)

static void
make_file(const char *name, const char *contents, mode_t mode)
{
    int    fd = open(name, O_WRONLY | O_CREAT | O_EXCL, mode);
    size_t len = strlen(contents);

    check(fd >= 0, name);
    check(write(fd, contents, len) == (ssize_t)len, "write");
    check(close(fd) == 0, "close");
}

int
main(int argc, char **argv)
{
    char         path[PATH_MAX + 64], copy[PATH_MAX + 64], file[PATH_MAX + 64];
    char         link1[PATH_MAX + 64], link2[PATH_MAX + 64];
    char        *argv_err[] = { ARGV0, "unexpected", NULL };
    char * const envp[] = { NULL };
    char        *longname, *longpath;
    ssize_t      len;
    int          fd, wfd, status;
    pid_t        pid;
    long         arg_max;

    if (argc > 1 && strcmp(argv[1], "state") == 0)
        return helper(argc, argv);
    if (argc > 1 && strcmp(argv[1], "exit") == 0)
        return argc > 2 ? atoi(argv[2]) : 0;
    if (argc > 1)
        return 99;

    len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len <= 0) {
        printf("no /proc/self/exe, skipping\n");
        return 77;
    }
    exe[len] = '\0';
    check(getcwd(dir, sizeof(dir) - 32) != NULL, "getcwd");
    strcat(dir, "/execl-XXXXXX");
    check(mkdtemp(dir) != NULL, "mkdtemp");

    /* A name the p variants find through PATH */
    snprintf(link1, sizeof(link1), "%s/" HELPER_NAME, dir);
    check(symlink(exe, link1) == 0, "symlink");
    check(setenv("PATH", dir, 1) == 0, "setenv PATH");
    check(setenv("PICOLIBC_EXEC", "inherit", 1) == 0, "setenv");

    for (int how = 0; how < NHOW; how++) {
        int ret = run((enum how)how);

        if (ret == 77) {
            printf("%s: not supported, skipped\n", names[how]);
            continue;
        }
        /*
         * QEMU's user-mode emulation runs fexecve through the
         * descriptor's /proc/self/fd path, which close-on-exec has
         * already closed
         */
        if ((how == FEXECVE || how == FEXECVE_O_EXEC) && ret == 100 + ENOENT && is_qemu()) {
            printf("%s: QEMU lost the close-on-exec descriptor, skipped\n", names[how]);
            continue;
        }
        if (ret != 0) {
            if (ret >= 100)
                printf("%s failed: %s\n", names[how], strerror(ret - 100));
            else
                printf("%s: check %d failed\n", names[how], ret);
            exit(1);
        }
    }

    /* Errors that every exec function reports for a path */
    check_err(execv("", argv_err), ENOENT);
    check_err(execvp("", argv_err), ENOENT);
    snprintf(path, sizeof(path), "%s/missing/prog", dir);
    check_err(execv(path, argv_err), ENOENT);
    check_err(execl(path, ARGV0, (char *)NULL), ENOENT);
    check_err(execle(path, ARGV0, (char *)NULL, envp), ENOENT);
    check_err(execve(path, argv_err, envp), ENOENT);

    /* A plain file used as a directory, with or without a trailing slash */
    snprintf(file, sizeof(file), "%s/plain", dir);
    make_file(file, "", 0644);
    snprintf(path, sizeof(path), "%s/plain/prog", dir);
    check_err(execv(path, argv_err), ENOTDIR);
    snprintf(path, sizeof(path), "%s/plain/", dir);
    check_err(execv(path, argv_err), ENOTDIR);
    snprintf(path, sizeof(path), "%s/plain/", dir);
    check_err(execvp(path, argv_err), ENOTDIR);

    /* No execute permission, or not a regular file */
    check_err(execv(file, argv_err), EACCES);
    check_err(execv(dir, argv_err), EACCES);
    if (geteuid() != 0) {
        snprintf(path, sizeof(path), "%s/locked", dir);
        check(mkdir(path, 0700) == 0, "mkdir");
        snprintf(copy, sizeof(copy), "%s/locked/prog", dir);
        make_file(copy, "", 0755);
        check(chmod(path, 0600) == 0, "chmod");
        check_err(execv(copy, argv_err), EACCES);
        check(chmod(path, 0700) == 0 && unlink(copy) == 0 && rmdir(path) == 0, "unlock");
    }

    /* A loop of symbolic links */
    snprintf(link1, sizeof(link1), "%s/loop1", dir);
    snprintf(link2, sizeof(link2), "%s/loop2", dir);
    check(symlink(link2, link1) == 0 && symlink(link1, link2) == 0, "symlink loop");
    check_err(execv(link1, argv_err), ELOOP);
    check_err(execvp(link1, argv_err), ELOOP);

    /* A component longer than NAME_MAX, and a path longer than PATH_MAX */
    longname = malloc(2049);
    longpath = malloc(8193);
    check(longname && longpath, "malloc");
    memset(longname, 'n', 300);
    longname[300] = '\0';
    snprintf(path, sizeof(path), "%s/%s", dir, longname);
    check_err(execv(path, argv_err), ENAMETOOLONG);
    check_err(execvp(longname, argv_err), ENAMETOOLONG);
    /* Even when the name is too long to join to any directory in PATH */
    memset(longname, 'n', 2048);
    longname[2048] = '\0';
    check_err(execvp(longname, argv_err), ENAMETOOLONG);
    for (int i = 0; i < 4096; i++)
        memcpy(longpath + 2 * i, "./", 2);
    strcpy(longpath + 8192, "");
    longpath[8191] = 'x';
    check_err(execv(longpath, argv_err), ENAMETOOLONG);
    free(longname);
    free(longpath);

    /* A file in no format the system knows */
    snprintf(path, sizeof(path), "%s/junk", dir);
    make_file(path, "\177junk\n", 0755);
    check_err(execv(path, argv_err), ENOEXEC);
    check_err(execve(path, argv_err, envp), ENOEXEC);
    fd = open(path, O_RDONLY | O_CLOEXEC);
    check(fd >= 0, "open junk");
    check_err(fexecve(fd, argv_err, envp), ENOEXEC);
    check(close(fd) == 0 && unlink(path) == 0, "remove junk");

    /* fexecve needs a descriptor for a regular file */
    fd = open(dir, O_RDONLY | O_CLOEXEC);
    check(fd >= 0, "open dir");
    check_err(fexecve(fd, argv_err, envp), EACCES);
    check(close(fd) == 0, "close dir");
    fd = open(file, O_RDONLY);
    check(fd >= 0 && close(fd) == 0, "closed descriptor");
    check_err(fexecve(fd, argv_err, envp), EBADF);

    /* A program open for writing may give ETXTBSY; once closed, it runs */
    snprintf(copy, sizeof(copy), "%s/copy", dir);
    fd = open(exe, O_RDONLY);
    wfd = open(copy, O_WRONLY | O_CREAT | O_EXCL, 0755);
    check(fd >= 0 && wfd >= 0, "open copy");
    for (;;) {
        char    buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf));

        check(n >= 0, "read");
        if (n == 0)
            break;
        check(write(wfd, buf, (size_t)n) == n, "write copy");
    }
    check(close(fd) == 0, "close");
    errno = 0;
    check(execl(copy, ARGV0, "unexpected", (char *)NULL) == -1 && errno == ETXTBSY, "ETXTBSY");
    check(close(wfd) == 0, "close copy");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        execl(copy, ARGV0, "exit", "7", (char *)NULL);
        _exit(100 + errno);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 7,
          "the copy runs");

    /* More argument bytes than {ARG_MAX} */
    arg_max = sysconf(_SC_ARG_MAX);
    if (arg_max > 0 && arg_max <= 64L * 1024 * 1024) {
        size_t chunk = 65536, n = (size_t)arg_max / chunk + 2;
        char  *big = malloc(chunk);
        char **bigv = calloc(n + 2, sizeof(char *));

        check(big && bigv, "malloc");
        memset(big, 'a', chunk - 1);
        big[chunk - 1] = '\0';
        bigv[0] = ARGV0;
        for (size_t i = 1; i <= n; i++)
            bigv[i] = big;
        bigv[n + 1] = NULL;
        check_err(execv(exe, bigv), E2BIG);
        free(bigv);
        free(big);
    }

    /* A failed exec leaves the arrays alone */
    check(strcmp(argv_err[0], ARGV0) == 0 && strcmp(argv_err[1], "unexpected") == 0
              && argv_err[2] == NULL,
          "argv unchanged");

    snprintf(path, sizeof(path), "%s/" HELPER_NAME, dir);
    check(unlink(path) == 0 && unlink(copy) == 0 && unlink(file) == 0, "clean up");
    snprintf(path, sizeof(path), "%s/file", dir);
    check(unlink(path) == 0 && unlink(link1) == 0 && unlink(link2) == 0, "clean up links");
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
