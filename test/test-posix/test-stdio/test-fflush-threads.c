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
#if TEST_POSIX_THREADS && TEST_POSIX_FILE_SYSTEM

#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * fflush(NULL) in one thread while others open, write and close
 * streams: every stream is still flushed and closed properly
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NOPENER 3
#define NLOOP   200

static int done;

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

static void *
flusher(void *arg)
{
    FILE *f = arg;
    long  n = 0;

    while (!__atomic_load_n(&done, __ATOMIC_ACQUIRE)) {
        check(fputc('x', f) == 'x', "fputc");
        check(fflush(NULL) == 0, "fflush(NULL)");
        n++;
    }
    return (void *)n;
}

static void *
opener(void *arg)
{
    int  id = (int)(intptr_t)arg;
    char name[32];
    char want[32];
    int  i;

    snprintf(name, sizeof(name), "opener-%d", id);
    for (i = 0; i < NLOOP; i++) {
        FILE *f = fopen(name, "w");

        check(f != NULL, "fopen");
        check(setvbuf(f, NULL, _IOFBF, 256) == 0, "setvbuf");
        check(fprintf(f, "%d %d", id, i) > 0, "fprintf");
        check(fclose(f) == 0, "fclose");
        snprintf(want, sizeof(want), "%d %d", id, i);
        check_file(name, want);
    }
    return NULL;
}

int
main(void)
{
    char      dir[] = "fflush-threads-XXXXXX";
    char      name[32];
    pthread_t flush_thread;
    pthread_t open_thread[NOPENER];
    FILE     *shared;
    void     *flushes;
    long      n;
    int       i;
    int       fd;
    off_t     size;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");

    /* A stream open the whole time, written by the flushing thread */
    shared = fopen("shared", "w");
    check(shared != NULL && setvbuf(shared, NULL, _IOFBF, 4096) == 0, "fopen shared");

    check(pthread_create(&flush_thread, NULL, flusher, shared) == 0, "pthread_create");
    for (i = 0; i < NOPENER; i++)
        check(pthread_create(&open_thread[i], NULL, opener, (void *)(intptr_t)i) == 0,
              "pthread_create");
    for (i = 0; i < NOPENER; i++)
        check(pthread_join(open_thread[i], NULL) == 0, "pthread_join");
    __atomic_store_n(&done, 1, __ATOMIC_RELEASE);
    check(pthread_join(flush_thread, &flushes) == 0, "pthread_join");
    n = (long)flushes;

    /* Every byte written before each fflush(NULL) reached the file */
    fd = open("shared", O_RDONLY);
    check(fd >= 0, "open shared");
    size = lseek(fd, 0, SEEK_END);
    check(close(fd) == 0, "close");
    if (size != n)
        printf("shared holds %ld bytes, expected %ld\n", (long)size, n);
    check(size == n, "shared size");
    check(fclose(shared) == 0, "fclose shared");

    for (i = 0; i < NOPENER; i++) {
        snprintf(name, sizeof(name), "opener-%d", i);
        check(unlink(name) == 0, "cleanup");
    }
    check(unlink("shared") == 0, "cleanup");
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
