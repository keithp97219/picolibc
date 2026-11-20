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

#define _GNU_SOURCE /* glibc declares sem_clockwait only for _GNU_SOURCE */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NTHREADS 4
#define NITEMS   10000

/* Unnamed semaphores: values, waits and timeouts, between threads and between processes */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static sem_t items;
static long  consumed;

static void *
consumer(void *arg)
{
    (void)arg;
    for (int i = 0; i < NITEMS; i++) {
        while (sem_wait(&items) != 0)
            if (errno != EINTR)
                return (void *)1;
        __atomic_add_fetch(&consumed, 1, __ATOMIC_RELAXED);
    }
    return NULL;
}

static void
timeout_in(clockid_t clock, struct timespec *ts, long ms)
{
    clock_gettime(clock, ts);
    ts->tv_nsec += ms * 1000 * 1000;
    while (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec++;
    }
}

static void
wait_child(pid_t pid)
{
    int status;

    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status");
}

int
main(void)
{
    pthread_t       threads[NTHREADS];
    struct timespec ts;
    sem_t           sem, *shared;
    int             value;
    pid_t           pid;
    void           *result;

    /* Initial values and errors */
    errno = 0;
    check(sem_init(&sem, 0, (unsigned)SEM_VALUE_MAX + 1) == -1 && errno == EINVAL,
          "initial value too large");
    check(sem_init(&sem, 0, 2) == 0, "sem_init");
    check(sem_getvalue(&sem, &value) == 0 && value == 2, "sem_getvalue");
    check(sem_trywait(&sem) == 0, "sem_trywait 1");
    check(sem_wait(&sem) == 0, "sem_wait");
    check(sem_trywait(&sem) == -1 && errno == EAGAIN, "sem_trywait at zero");
    timeout_in(CLOCK_REALTIME, &ts, 20);
    check(sem_timedwait(&sem, &ts) == -1 && errno == ETIMEDOUT, "sem_timedwait times out");
    timeout_in(CLOCK_MONOTONIC, &ts, 20);
    check(sem_clockwait(&sem, CLOCK_MONOTONIC, &ts) == -1 && errno == ETIMEDOUT,
          "sem_clockwait times out");
    ts.tv_nsec = 1000000000L;
    check(sem_timedwait(&sem, &ts) == -1 && errno == EINVAL, "bad timeout");
    check(sem_post(&sem) == 0, "sem_post");
    timeout_in(CLOCK_REALTIME, &ts, 20);
    check(sem_timedwait(&sem, &ts) == 0, "sem_timedwait available");
    check(sem_destroy(&sem) == 0, "sem_destroy");

    check(sem_init(&sem, 0, SEM_VALUE_MAX) == 0, "sem_init max");
    check(sem_post(&sem) == -1 && errno == EOVERFLOW, "sem_post overflow");
    check(sem_destroy(&sem) == 0, "sem_destroy");

    /* Threads consuming posted items */
    check(sem_init(&items, 0, 0) == 0, "sem_init items");
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&threads[i], NULL, consumer, NULL) == 0, "pthread_create");
    for (int i = 0; i < NTHREADS * NITEMS; i++)
        check(sem_post(&items) == 0, "sem_post items");
    for (int i = 0; i < NTHREADS; i++) {
        check(pthread_join(threads[i], &result) == 0, "pthread_join");
        check(result == NULL, "consumer");
    }
    check(consumed == (long)NTHREADS * NITEMS, "all items consumed");
    check(sem_getvalue(&items, &value) == 0 && value == 0, "items left");
    check(sem_destroy(&items) == 0, "sem_destroy items");

    /* A process-shared semaphore in shared memory */
    shared = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(shared != MAP_FAILED, "mmap");
    check(sem_init(shared, 1, 0) == 0, "sem_init shared");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        for (int i = 0; i < 100; i++)
            if (sem_post(shared) != 0)
                _exit(1);
        _exit(0);
    }
    for (int i = 0; i < 100; i++)
        check(sem_wait(shared) == 0, "sem_wait shared");
    wait_child(pid);
    check(sem_destroy(shared) == 0, "sem_destroy shared");
    check(munmap(shared, sizeof(sem_t)) == 0, "munmap");

    printf("success\n");
    return 0;
}
