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

#include "local-spawn.h"
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef _PATH_DEFPATH
#define _PATH_DEFPATH "/bin:/usr/bin"
#endif

/* Runs 'file', searching PATH when it has no '/' */
static void
__spawn_exec(const char *file, char * const argv[], char * const envp[], int search)
{
    const char *path, *end;
    size_t      flen;
    int         saved = ENOENT;

    if (!search || strchr(file, '/')) {
        execve(file, argv, envp);
        return;
    }
    /* An empty name names no file */
    if (!*file) {
        errno = ENOENT;
        return;
    }
    path = getenv("PATH");
    if (!path)
        path = _PATH_DEFPATH;
    flen = strlen(file);
    for (;;) {
        size_t dlen;

        end = strchr(path, ':');
        dlen = end ? (size_t)(end - path) : strlen(path);
        if (dlen + flen + 2 > PATH_MAX) {
            saved = ENAMETOOLONG;
        } else {
            char buf[PATH_MAX];

            /* An empty element means the current directory */
            if (dlen) {
                memcpy(buf, path, dlen);
                buf[dlen] = '/';
                memcpy(buf + dlen + 1, file, flen + 1);
            } else {
                memcpy(buf, file, flen + 1);
            }
            execve(buf, argv, envp);
            /* Keep looking past directories that don't hold it */
            if (errno != ENOENT && errno != ENOTDIR)
                saved = errno;
        }
        if (!end)
            break;
        path = end + 1;
    }
    errno = saved;
}

/* Applies the attributes and file actions in the child */
static int
__spawn_child(const posix_spawn_file_actions_t *fa, const posix_spawnattr_t *attr,
              const sigset_t *oldmask, int *errfd)
{
    const struct __posix_spawnattr *a = attr ? *attr : NULL;
    struct sigaction                sa, old;
    int                             sig;

    /*
     * Signals are blocked; return the parent's handlers to the default
     * before unblocking them, so none runs in the child. SETSIGDEF
     * also resets the ignored signals it names.
     */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    for (sig = 1; sig < NSIG; sig++) {
        if (sig == SIGKILL || sig == SIGSTOP)
            continue;
        if (sigaction(sig, NULL, &old) < 0)
            continue;
        if ((a && (a->flags & POSIX_SPAWN_SETSIGDEF) && sigismember(&a->sigdefault, sig))
            || (old.sa_handler != SIG_DFL && old.sa_handler != SIG_IGN))
            sigaction(sig, &sa, NULL);
    }

    /* Keep the error pipe clear of every descriptor the actions name */
    if (fa && *fa) {
        const struct __posix_spawn_file_actions *f = *fa;
        size_t                                   i;
        int                                      max = -1, fd;

        for (i = 0; i < f->count; i++) {
            const struct __spawn_action *act = &f->actions[i];

            if (act->fd > max)
                max = act->fd;
            if (act->kind == __SPAWN_DUP2 && act->newfd > max)
                max = act->newfd;
        }
        if (*errfd <= max) {
            fd = fcntl(*errfd, F_DUPFD_CLOEXEC, max + 1);
            if (fd < 0)
                return errno;
            close(*errfd);
            *errfd = fd;
        }
    }

    if (a) {
        if (a->flags & POSIX_SPAWN_RESETIDS)
            if (setegid(getgid()) < 0 || seteuid(getuid()) < 0)
                return errno;
        if (a->flags & POSIX_SPAWN_SETPGROUP)
            if (setpgid(0, a->pgroup) < 0)
                return errno;
        if (a->flags & POSIX_SPAWN_SETSID)
            if (setsid() < 0)
                return errno;
        if (a->flags & POSIX_SPAWN_SETSCHEDULER) {
            if (sched_setscheduler(0, a->policy, &a->param) < 0)
                return errno;
        } else if (a->flags & POSIX_SPAWN_SETSCHEDPARAM) {
            if (sched_setparam(0, &a->param) < 0)
                return errno;
        }
    }
    if (fa && *fa) {
        const struct __posix_spawn_file_actions *f = *fa;
        size_t                                   i;

        for (i = 0; i < f->count; i++) {
            const struct __spawn_action *act = &f->actions[i];
            int                          fd;

            switch (act->kind) {
            case __SPAWN_OPEN:
                fd = open(act->path, act->oflag, act->mode);
                if (fd < 0)
                    return errno;
                if (fd != act->fd) {
                    if (dup2(fd, act->fd) < 0)
                        return errno;
                    close(fd);
                }
                break;
            case __SPAWN_CLOSE:
                /* Closing a descriptor that isn't open is fine */
                if (close(act->fd) < 0 && errno != EBADF)
                    return errno;
                break;
            case __SPAWN_DUP2:
                if (act->fd == act->newfd) {
                    /* Then the descriptor stays open across exec */
                    int flags = fcntl(act->fd, F_GETFD);

                    if (flags < 0 || fcntl(act->fd, F_SETFD, flags & ~FD_CLOEXEC) < 0)
                        return errno;
                } else if (dup2(act->fd, act->newfd) < 0) {
                    return errno;
                }
                break;
            case __SPAWN_CHDIR:
                if (chdir(act->path) < 0)
                    return errno;
                break;
            case __SPAWN_FCHDIR:
                if (fchdir(act->fd) < 0)
                    return errno;
                break;
            }
        }
    }
    if (sigprocmask(SIG_SETMASK, a && (a->flags & POSIX_SPAWN_SETSIGMASK) ? &a->sigmask : oldmask,
                    NULL)
        < 0)
        return errno;
    return 0;
}

int
__posix_spawn(pid_t * __restrict pid, const char * __restrict path,
              const posix_spawn_file_actions_t *fa, const posix_spawnattr_t * __restrict attr,
              char * const argv[], char * const envp[], int search)
{
    int      fds[2], error = 0, status;
    ssize_t  n;
    pid_t    child;
    sigset_t all, oldmask;

    /* The child reports a failure before or in exec through this pipe */
    if (pipe2(fds, O_CLOEXEC) < 0)
        return errno;
    /* No signal handler may run in the child before its handlers are reset */
    sigfillset(&all);
    sigprocmask(SIG_BLOCK, &all, &oldmask);
    child = fork();
    if (child < 0) {
        error = errno;
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        close(fds[0]);
        close(fds[1]);
        return error;
    }
    if (child == 0) {
        close(fds[0]);
        error = __spawn_child(fa, attr, &oldmask, &fds[1]);
        if (!error) {
            __spawn_exec(path, argv, envp, search);
            error = errno;
        }
        (void)!write(fds[1], &error, sizeof(error));
        _exit(127);
    }
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    close(fds[1]);
    while ((n = read(fds[0], &error, sizeof(error))) < 0 && errno == EINTR)
        ;
    if (n != sizeof(error))
        error = 0;
    close(fds[0]);
    if (error) {
        /* Reap the child that failed */
        while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            ;
        return error;
    }
    if (pid)
        *pid = child;
    return 0;
}
