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

#include "local-stdio.h"
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>

/* Open popen streams, closed in each new child. Protected by __LIBC_LOCK */
static struct __file_pipe *__popen_list;

static void
__popen_remove(struct __file_pipe *pf)
{
    struct __file_pipe **prev;

    __LIBC_LOCK();
    for (prev = &__popen_list; *prev; prev = &(*prev)->next_pipe) {
        if (*prev == pf) {
            *prev = pf->next_pipe;
            break;
        }
    }
    __LIBC_UNLOCK();
}

static int
__pipe_close(FILE *file)
{
    struct __file_pipe *pf = (struct __file_pipe *)file;
    int                *wstatus = pf->wstatus;
    pid_t               child = pf->child;
    int                 ret;

    __popen_remove(pf);

    ret = __bufio_close(file);

    /* Don't return before the child has terminated */
    while (waitpid(child, wstatus, 0) < 0) {
        if (errno != EINTR)
            return _FDEV_ERR;
    }
    return ret;
}

extern char **environ;

#ifdef __GNUCLIKE_PRAGMA_DIAGNOSTIC
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
#endif

FILE *
popen(const char *command, const char *type)
{
    char               *argv[5];
    char               *shell;
    int                 pipe_fd[2];
    int                 my_fd;
    int                 stdio_flags;
    bool                cloexec = false;
    struct __file_pipe *pf;
    struct __file_pipe *other;
    char               *buf;
    size_t              buf_size;

    switch (type[0]) {
    case 'r':
        my_fd = 0;
        stdio_flags = __SRD;
        break;
    case 'w':
        my_fd = 1;
        stdio_flags = __SWR;
        break;
    default:
        errno = EINVAL;
        return NULL;
    }

    /* 'e' leaves FD_CLOEXEC set on the stream's descriptor */
    if (strchr(type, 'e'))
        cloexec = true;

    /*
     * Both ends start with FD_CLOEXEC so that children started by
     * other threads don't keep them open.
     */
    if (pipe2(pipe_fd, O_CLOEXEC) < 0)
        return NULL;

    buf_size = BUFSIZ;

    /* Allocate file structure and necessary buffers */
    pf = calloc(1, sizeof(struct __file_pipe) + buf_size);

    if (pf == NULL)
        goto bail;

    buf = (char *)(pf + 1);

    pf->bfile = (struct __file_bufio)FDEV_SETUP_POSIX(pipe_fd[my_fd], buf, buf_size, stdio_flags,
                                                      __BFALL | __BPIPE);
    pf->bfile.xfile.cfile.close = __pipe_close;
    pf->wstatus = NULL;

    /*
     * List the stream before forking. fork holds __LIBC_LOCK while it
     * runs, so the child sees a consistent list.
     */
    __LIBC_LOCK();
    pf->next_pipe = __popen_list;
    __popen_list = pf;
    __LIBC_UNLOCK();

    switch (pf->child = fork()) {
    case -1:
        __popen_remove(pf);
        goto bail;
    case 0:
        /* Close the streams from popen calls, including this one's end */
        for (other = __popen_list; other; other = other->next_pipe)
            close(_FDEV_BUFIO_FD(&other->bfile));

        /* Move child end to correct fd, without FD_CLOEXEC */
        my_fd = 1 - my_fd;
        if (pipe_fd[my_fd] != my_fd) {
            dup2(pipe_fd[my_fd], my_fd);
            close(pipe_fd[my_fd]);
        } else {
            fcntl(my_fd, F_SETFD, 0);
        }

        /* Set up argv */
        shell = "/bin/sh";
        argv[0] = shell + 5;
        argv[1] = "-c";
        argv[2] = "--";
        argv[3] = (char *)command;
        argv[4] = NULL;

        /* And we're off */
        execve(shell, argv, environ);

        /* As if the shell had exited with 127 */
        _exit(127);
    default:
        if (!cloexec)
            fcntl(pipe_fd[my_fd], F_SETFD, 0);
        close(pipe_fd[1 - my_fd]);
        bufio_add_file(&pf->bfile.xfile.cfile.file);
        return &pf->bfile.xfile.cfile.file;
    }
bail:
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    free(pf);
    return NULL;
}
