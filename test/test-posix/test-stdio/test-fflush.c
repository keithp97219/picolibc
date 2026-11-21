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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE)     \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FD_MGMT) \
    && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
 * fflush and fclose: output written, input streams setting the file
 * offset (XSH 2.5.1), fflush(NULL), every stream flushed by exit and by
 * returning from main, errors, and the timestamps
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static FILE *at_exit_stream;

static void
write_at_exit(void)
{
    fputs(" atexit", at_exit_stream);
}

static void
check_file(const char *name, const char *contents)
{
    char    buf[64];
    int     fd = open(name, O_RDONLY);
    ssize_t len;

    check(fd >= 0, "open");
    len = read(fd, buf, sizeof(buf) - 1);
    check(len >= 0 && close(fd) == 0, "read");
    buf[len] = '\0';
    if (strcmp(buf, contents) != 0)
        printf("%s holds \"%s\", expected \"%s\"\n", name, buf, contents);
    check(strcmp(buf, contents) == 0, "contents");
}

/* Children leave buffered output in streams and end in different ways */
static void
run_child(int how, const char *name)
{
    pid_t child;
    int   status;

    child = fork();
    check(child >= 0, "fork");
    if (child == 0) {
        FILE *a = fopen(name, "w");
        int   fd = open("fd-out", O_WRONLY | O_CREAT | O_TRUNC, 0600);
        FILE *b = fdopen(fd, "w");

        if (!a || !b || setvbuf(a, NULL, _IOFBF, 256) || setvbuf(b, NULL, _IOFBF, 256))
            _exit(2);
        fputs("fopen", a);
        fputs("fdopen", b);
        at_exit_stream = a;
        atexit(write_at_exit);
        switch (how) {
        case 0:
            exit(0);
        case 1:
            fflush(NULL);
            _exit(0);
        default:
            _exit(0);
        }
    }
    check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
}

static void
touch_old(const char *name)
{
    struct timespec times[2] = {
        { 1000, 0 },
        { 1000, 0 }
    };

    check(utimensat(AT_FDCWD, name, times, 0) == 0, "utimensat");
}

static struct stat
stat_of(const char *name)
{
    struct stat st;

    check(stat(name, &st) == 0, "stat");
    return st;
}

int
main(void)
{
    char        dir[] = "fflush-XXXXXX";
    FILE       *f;
    int         fd;
    int         p[2];
    struct stat st;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");

    /* exit flushes every stream, after the atexit functions */
    run_child(0, "exit-out");
    check_file("exit-out", "fopen atexit");
    check_file("fd-out", "fdopen");

    /* fflush(NULL) flushes every output stream */
    run_child(1, "null-out");
    check_file("null-out", "fopen");
    check_file("fd-out", "fdopen");

    /* _exit doesn't flush */
    run_child(2, "none-out");
    check_file("none-out", "");
    check_file("fd-out", "");

    /* fflush on an input stream sets the descriptor's offset */
    f = fopen("exit-out", "r");
    check(f != NULL, "fopen");
    check(fgetc(f) == 'f' && fgetc(f) == 'o' && fflush(f) == 0, "read and flush");
    check(lseek(fileno(f), 0, SEEK_CUR) == 2, "offset after fflush");
    check(fgetc(f) == 'p', "reading goes on after fflush");
    check(ungetc('P', f) == 'P' && fflush(f) == 0, "ungetc then fflush");
    check(lseek(fileno(f), 0, SEEK_CUR) == 2 && fgetc(f) == 'p',
          "fflush drops the pushed-back byte");
    /* fclose does the same */
    fd = dup(fileno(f));
    check(fd >= 0 && fgetc(f) == 'e' && fclose(f) == 0, "fclose");
    check(lseek(fd, 0, SEEK_CUR) == 4 && close(fd) == 0, "offset after fclose");

    /* fflush and fclose write buffered data and mark the timestamps */
    f = fopen("times", "w");
    check(f != NULL && setvbuf(f, NULL, _IOFBF, 256) == 0, "fopen times");
    touch_old("times");
    check(fputs("data", f) >= 0, "buffered write");
    st = stat_of("times");
    check(st.st_size == 0 && st.st_mtime == 1000, "nothing written yet");
    check(fflush(f) == 0, "fflush");
    st = stat_of("times");
    check(st.st_size == 4 && st.st_mtime != 1000, "fflush writes and marks st_mtime");
    touch_old("times");
    check(fputs("more", f) >= 0 && fclose(f) == 0, "fclose");
    st = stat_of("times");
    check(st.st_size == 8 && st.st_mtime != 1000, "fclose writes and marks st_mtime");

    /* fopen 'w' on an existing file marks st_mtime and st_ctime */
    touch_old("times");
    f = fopen("times", "w");
    check(f != NULL && fclose(f) == 0, "fopen w");
    st = stat_of("times");
    check(st.st_size == 0 && st.st_mtime != 1000, "fopen w marks st_mtime");
    /* A new file marks its directory */
    touch_old(".");
    f = fopen("new", "a");
    check(f != NULL && fclose(f) == 0, "fopen a");
    st = stat_of(".");
    check(st.st_mtime != 1000, "creating a file marks the directory");

    /* EBADF: the descriptor was closed under the stream */
    fd = open("new", O_WRONLY);
    f = fdopen(fd, "w");
    check(f != NULL && fputs("x", f) >= 0 && close(fd) == 0, "close under the stream");
    errno = 0;
    check(fflush(f) == EOF && errno == EBADF && ferror(f), "fflush EBADF");
    errno = 0;
    check(fclose(f) == EOF && errno == EBADF, "fclose EBADF");

    /* EPIPE: nobody reads the pipe */
    check(signal(SIGPIPE, SIG_IGN) != SIG_ERR && pipe(p) == 0 && close(p[0]) == 0, "pipe");
    f = fdopen(p[1], "w");
    check(f != NULL && fputs("x", f) >= 0, "fdopen pipe");
    errno = 0;
    check(fflush(f) == EOF && errno == EPIPE && ferror(f), "fflush EPIPE");
    (void)fclose(f);

    check(unlink("exit-out") == 0 && unlink("null-out") == 0 && unlink("none-out") == 0
              && unlink("fd-out") == 0 && unlink("times") == 0 && unlink("new") == 0,
          "cleanup");
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
