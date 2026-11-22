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
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_PIPE && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FD_MGMT

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <wchar.h>

/*
 * freopen: flush, close, then open as fopen does; the stream is closed
 * even when the open fails; indicators and orientation cleared
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

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

int
main(void)
{
    char  dir[] = "freopen-XXXXXX";
    FILE *f;
    int   fd;
    pid_t child;
    int   status;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");

    /* The old stream is flushed before the new file is opened (and truncated) */
    f = fopen("a", "w");
    check(f != NULL && setvbuf(f, NULL, _IOFBF, 256) == 0, "fopen");
    check(fputs("oldold", f) >= 0, "buffered write");
    check(freopen("a", "w", f) == f, "freopen the same file");
    check(fputs("new", f) >= 0 && fclose(f) == 0, "write new");
    check_file("a", "new");

    /* Indicators and orientation are cleared; the mode is the new one */
    f = fopen("a", "r");
    check(f != NULL && fgetc(f) == 'n', "fopen");
    check(fputc('x', f) == EOF && ferror(f), "set the error indicator");
    check(fseek(f, 0, SEEK_END) == 0 && fgetc(f) == EOF && feof(f), "set end-of-file");
    check(freopen("b", "w+", f) == f, "freopen b");
    check(!ferror(f) && !feof(f), "indicators cleared");
    check(fputs("bee", f) >= 0 && fseek(f, 0, SEEK_SET) == 0 && fgetc(f) == 'b', "update mode");
    check(freopen("b", "r", f) == f && fwide(f, 1) > 0, "make the new stream wide");
    check(freopen("a", "r", f) == f, "freopen a");
    check(fwide(f, 0) == 0, "orientation cleared");
    check(fgetc(f) == 'n', "byte input after freopen");

    /* 'e' and 'x' as for fopen */
    check(freopen("b", "re", f) == f && (fcntl(fileno(f), F_GETFD) & FD_CLOEXEC), "e");
    errno = 0;
    check(freopen("b", "wx", f) == NULL && errno == EEXIST, "x");

    /* A failed open still closes the original stream */
    f = fopen("a", "r");
    check(f != NULL, "fopen");
    fd = fileno(f);
    errno = 0;
    check(freopen("missing/x", "r", f) == NULL && errno == ENOENT, "freopen fails");
    errno = 0;
    check(fcntl(fd, F_GETFD) == -1 && errno == EBADF, "the old descriptor is closed");

    /* A null pathname: whether the descriptor is kept is implementation-defined */
    f = fopen("a", "r");
    check(f != NULL && fgetc(f) == 'n' && fgetc(f) == 'e', "fopen");
    fd = fileno(f);
    check(freopen(NULL, "r", f) == f, "freopen NULL");
#ifdef __PICOLIBC__
    /* picolibc keeps the descriptor and its offset */
    check(fileno(f) == fd && fgetc(f) == 'w', "reading goes on at the stream's position");
#endif
    check(fclose(f) == 0, "fclose");

    /* EBADF: a null pathname with an invalid descriptor */
    fd = open("a", O_RDONLY);
    f = fdopen(fd, "r");
    check(f != NULL && close(fd) == 0, "close under the stream");
    errno = 0;
    check(freopen(NULL, "r", f) == NULL && errno == EBADF, "freopen NULL EBADF");

    /*
     * The new descriptor comes from open after the old one is closed,
     * so freopen of stdout gets descriptor 1 back
     */
    child = fork();
    check(child >= 0, "fork");
    if (child == 0) {
        if (freopen("out", "w", stdout) != stdout || fileno(stdout) != 1)
            _exit(2);
        printf("to stdout");
        fflush(stdout);
        if (write(1, " and fd 1", 9) != 9)
            _exit(3);
        _exit(0);
    }
    check(waitpid(child, &status, 0) == child && WIFEXITED(status), "child");
    if (WEXITSTATUS(status) != 0)
        printf("child exit %d\n", WEXITSTATUS(status));
    check(WEXITSTATUS(status) == 0, "freopen stdout keeps descriptor 1");
    check_file("out", "to stdout and fd 1");

    check(unlink("a") == 0 && unlink("b") == 0 && unlink("out") == 0, "cleanup");
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
