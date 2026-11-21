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
#if _POSIX_SEMAPHORES > 0 && _POSIX_THREADS > 0 && _POSIX_SHARED_MEMORY_OBJECTS > 0 \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Named semaphores: creation, permissions, unlinking, names, and cancellation */

static char name[64];
static char name2[64];
static char shm_name[64];

static void
cleanup(void)
{
    if (name[0])
        (void)sem_unlink(name);
    if (name2[0])
        (void)sem_unlink(name2);
    if (shm_name[0])
        (void)shm_unlink(shm_name);
}

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
value(sem_t *sem)
{
    int v;

    check(sem_getvalue(sem, &v) == 0, "sem_getvalue");
    return v;
}

/* None of these is a cancellation point; run them with a request pending */
static void *
not_cancellation_points(void *arg)
{
    sem_t *sem;
    int    fd;

    (void)arg;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_cancel(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    sem = sem_open(name2, O_CREAT | O_EXCL, 0600, 1);
    if (sem == SEM_FAILED)
        return (void *)1;
    if (sem_trywait(sem) != 0 || sem_post(sem) != 0 || sem_close(sem) != 0)
        return (void *)2;
    if (sem_unlink(name2) != 0)
        return (void *)3;
    fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
        return (void *)4;
    if (shm_unlink(shm_name) != 0)
        return (void *)5;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    close(fd);
    return NULL;
}

#define NCHILD 8

int
main(void)
{
    static char long_name[NAME_MAX + 2];
    struct stat st;
    char        path[128];
    sem_t      *sem, *old;
    sem_t       unnamed;
    pthread_t   thread;
    void       *result;
    pid_t       pids[NCHILD];
    mode_t      mask;
    int         status, created;
    size_t      len;

    snprintf(name, sizeof(name), "/picolibc-test-sem_unlink-%ld", (long)getpid());
    snprintf(name2, sizeof(name2), "/picolibc-test-sem_unlink-%ld-2", (long)getpid());
    snprintf(shm_name, sizeof(shm_name), "/picolibc-test-sem_unlink-%ld-shm", (long)getpid());
    cleanup();

    /* The initial value is checked */
    errno = 0;
    check(sem_open(name, O_CREAT, 0600, (unsigned)SEM_VALUE_MAX + 1) == SEM_FAILED
              && errno == EINVAL,
          "value above SEM_VALUE_MAX");

    /* Creation applies the umask; an existing semaphore keeps its mode and value */
    mask = umask(027);
    sem = sem_open(name, O_CREAT | O_EXCL, 0666, 3);
    umask(mask);
    check(sem != SEM_FAILED, "create");
    check(value(sem) == 3, "initial value");
    /* Where the name is visible in the file system, as on Linux, check its mode */
    snprintf(path, sizeof(path), "/dev/shm/sem.%s", name + 1);
    if (stat(path, &st) == 0) {
        check((st.st_mode & 0777) == 0640, "mode less the umask");
        check(st.st_uid == geteuid(), "owner");
    }
    old = sem_open(name, O_CREAT, 0600, 7);
    check(old == sem, "O_CREAT of an existing semaphore");
    check(value(sem) == 3, "value kept");
    check(sem_close(old) == 0, "sem_close");

    /* Permissions, when they apply to this process */
    if (geteuid() != 0) {
        sem_t *denied;

        check(sem_unlink(name2) == -1, "no second semaphore");
        denied = sem_open(name2, O_CREAT | O_EXCL, 0, 0);
        check(denied != SEM_FAILED, "create with mode 0");
        check(sem_close(denied) == 0, "sem_close");
        errno = 0;
        check(sem_open(name2, 0) == SEM_FAILED && errno == EACCES, "no permission");
        check(sem_unlink(name2) == 0, "sem_unlink");
    } else {
        printf("running as root, skipping EACCES\n");
    }

    /* Unlinking leaves open semaphores working, and the name makes a new one */
    check(sem_unlink(name) == 0, "sem_unlink open semaphore");
    errno = 0;
    check(sem_open(name, 0) == SEM_FAILED && errno == ENOENT, "name removed");
    check(sem_post(sem) == 0 && value(sem) == 4, "still usable");
    fflush(stdout);
    pids[0] = fork();
    check(pids[0] >= 0, "fork");
    if (pids[0] == 0)
        _exit(sem_post(sem) == 0 ? 0 : 1);
    check(waitpid(pids[0], &status, 0) == pids[0] && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
    check(value(sem) == 5, "shared with the child after unlink");
    old = sem;
    sem = sem_open(name, O_CREAT | O_EXCL, 0600, 0);
    check(sem != SEM_FAILED && sem != old, "new semaphore with the old name");
    check(value(sem) == 0 && value(old) == 5, "separate semaphores");
    check(sem_close(old) == 0, "sem_close old");
    check(sem_close(sem) == 0, "sem_close new");
    check(sem_unlink(name) == 0, "sem_unlink");

    /* O_CREAT | O_EXCL is atomic: exactly one of several processes creates it */
    fflush(stdout);
    for (int i = 0; i < NCHILD; i++) {
        pids[i] = fork();
        check(pids[i] >= 0, "fork");
        if (pids[i] == 0) {
            if (sem_open(name, O_CREAT | O_EXCL, 0600, 0) != SEM_FAILED)
                _exit(0);
            _exit(errno == EEXIST ? 1 : 2);
        }
    }
    created = 0;
    for (int i = 0; i < NCHILD; i++) {
        check(waitpid(pids[i], &status, 0) == pids[i] && WIFEXITED(status), "child");
        check(WEXITSTATUS(status) != 2, "child error");
        if (WEXITSTATUS(status) == 0)
            created++;
    }
    check(created == 1, "one creator");
    check(sem_unlink(name) == 0, "sem_unlink");
    errno = 0;
    check(sem_unlink(name) == -1 && errno == ENOENT, "sem_unlink missing");

    /* A name sem_open accepted never gives ENAMETOOLONG to sem_unlink */
    memset(long_name, 'n', sizeof(long_name) - 1);
    long_name[0] = '/';
    for (len = sizeof(long_name) - 1; len > 1; len--) {
        long_name[len] = '\0';
        sem = sem_open(long_name, O_CREAT | O_EXCL, 0600, 0);
        if (sem != SEM_FAILED)
            break;
        check(errno == ENAMETOOLONG || errno == EINVAL, "long name");
    }
    check(len > 1, "some long name works");
    check(sem_close(sem) == 0, "sem_close");
    check(sem_unlink(long_name) == 0, "sem_unlink of the longest name");

    /* sem_close of an unnamed semaphore, which picolibc reports */
#ifdef __PICOLIBC__
    check(sem_init(&unnamed, 0, 0) == 0, "sem_init");
    errno = 0;
    check(sem_close(&unnamed) == -1 && errno == EINVAL, "sem_close unnamed");
    check(sem_destroy(&unnamed) == 0, "sem_destroy");
#else
    (void)unnamed;
#endif

    /* The named semaphore and shared memory calls aren't cancellation points */
    check(pthread_create(&thread, NULL, not_cancellation_points, NULL) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == NULL, "not cancelled");

    name[0] = name2[0] = shm_name[0] = '\0';
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
