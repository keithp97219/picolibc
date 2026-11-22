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
/* System V IPC is XSI */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_XSI_IPC && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

/*
 * semop waits: for semval to change, for the set to go away, or for a
 * signal, keeping semncnt and semzcnt; and SEM_UNDO undoes on exit
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The application declares union semun */
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

/* Every set made here, removed on every way out */
static int ids[8];
static int nids;

static void
cleanup(void)
{
    while (nids > 0)
        semctl(ids[--nids], 0, IPC_RMID);
}

static void
watchdog(int sig)
{
    (void)sig;
    cleanup();
    _exit(1);
}

static int
keep(int id)
{
    if (id >= 0)
        ids[nids++] = id;
    return id;
}

static void
drop(int id)
{
    int i;

    for (i = 0; i < nids; i++)
        if (ids[i] == id)
            ids[i] = ids[--nids];
}
/* Waits until semctl's count reaches want */
static bool
wait_count(int id, int num, int cmd, int want)
{
    struct timespec ts = { 0, 1000 * 1000 };
    int             i;

    for (i = 0; i < 20000; i++) {
        if (semctl(id, num, cmd) == want)
            return true;
        nanosleep(&ts, NULL);
    }
    return false;
}

static int
child_status(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

static void
set_all(int id, unsigned short a, unsigned short b)
{
    unsigned short vals[2] = { a, b };
    union semun    arg;

    arg.array = vals;
    check(semctl(id, 0, SETALL, arg) == 0, "SETALL");
}

static int
value(int id, int num)
{
    return semctl(id, num, GETVAL);
}

static volatile sig_atomic_t caught;

static void
handler(int sig)
{
    caught = sig;
}

/* A child that waits in one semop; what it saw is its status */
static pid_t
waiter(int id, const struct sembuf *ops, size_t nops, bool catch)
{
    pid_t pid;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sembuf    mine[2];
        struct sigaction sa;

        if (catch) {
            /* Without SA_RESTART, the signal interrupts semop */
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = handler;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);
        }
        memcpy(mine, ops, nops * sizeof(*ops));
        if (semop(id, mine, nops) == 0)
            _exit(0);
        if (errno == EIDRM)
            _exit(2);
        if (errno == EINTR)
            _exit(caught == SIGUSR1 ? 3 : 11);
        _exit(12);
    }
    return pid;
}

int
main(void)
{
    struct sembuf ops[2];
    pid_t         pid;
    int           id, fds[2];
    char          c;

    signal(SIGALRM, watchdog);
    alarm(60);
    check(atexit(cleanup) == 0, "atexit");

    id = keep(semget(IPC_PRIVATE, 2, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "semget");

    /* A decrement waits, counted in semncnt, until semval is large enough */
    set_all(id, 0, 0);
    ops[0] = (struct sembuf) { 0, -2, 0 };
    pid = waiter(id, ops, 1, false);
    check(wait_count(id, 0, GETNCNT, 1), "semncnt counts the waiter");
    check(semctl(id, 0, GETZCNT) == 0, "and semzcnt doesn't");
    ops[0] = (struct sembuf) { 0, 1, 0 };
    check(semop(id, ops, 1) == 0, "not enough yet");
    check(semop(id, ops, 1) == 0, "enough now");
    check(child_status(pid) == 0, "the decrement went through");
    check(value(id, 0) == 0 && semctl(id, 0, GETNCNT) == 0, "semval and semncnt");

    /* A wait for zero, counted in semzcnt, until semval is zero */
    set_all(id, 0, 2);
    ops[0] = (struct sembuf) { 1, 0, 0 };
    pid = waiter(id, ops, 1, false);
    check(wait_count(id, 1, GETZCNT, 1), "semzcnt counts the waiter");
    check(semctl(id, 1, GETNCNT) == 0, "and semncnt doesn't");
    ops[0] = (struct sembuf) { 1, -1, 0 };
    check(semop(id, ops, 1) == 0, "not zero yet");
    check(semop(id, ops, 1) == 0, "zero now");
    check(child_status(pid) == 0, "the wait for zero ended");
    check(semctl(id, 1, GETZCNT) == 0, "semzcnt");

    /*
     * A blocked array keeps nothing of what came before the blocking
     * operation, and is retried as a whole
     */
    set_all(id, 0, 0);
    ops[0] = (struct sembuf) { 1, 1, 0 };
    ops[1] = (struct sembuf) { 0, -1, 0 };
    pid = waiter(id, ops, 2, false);
    check(wait_count(id, 0, GETNCNT, 1), "blocked on the second operation");
    check(value(id, 1) == 0, "the first operation was undone while blocked");
    ops[0] = (struct sembuf) { 0, 1, 0 };
    check(semop(id, ops, 1) == 0, "release");
    check(child_status(pid) == 0, "the array went through");
    check(value(id, 0) == 0 && value(id, 1) == 1, "both operations done");

    /* A caught signal ends the wait with EINTR and takes the waiter off semncnt */
    set_all(id, 0, 0);
    ops[0] = (struct sembuf) { 0, -1, 0 };
    pid = waiter(id, ops, 1, true);
    check(wait_count(id, 0, GETNCNT, 1), "waiting");
    check(kill(pid, SIGUSR1) == 0, "kill");
    check(child_status(pid) == 3, "EINTR");
    check(semctl(id, 0, GETNCNT) == 0, "semncnt back down");
    check(value(id, 0) == 0, "nothing done");

    /* So does a caught signal during a wait for zero, with semzcnt */
    set_all(id, 0, 1);
    ops[0] = (struct sembuf) { 1, 0, 0 };
    pid = waiter(id, ops, 1, true);
    check(wait_count(id, 1, GETZCNT, 1), "waiting");
    check(kill(pid, SIGUSR1) == 0, "kill");
    check(child_status(pid) == 3, "EINTR");
    check(semctl(id, 1, GETZCNT) == 0, "semzcnt back down");

    /* SEM_UNDO: the changes are undone when the process exits */
    set_all(id, 5, 5);
    check(pipe(fds) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sembuf undo[2] = {
            { 0, -3, SEM_UNDO },
            { 1, 2,  SEM_UNDO }
        };
        struct sembuf plain = { 1, 1, 0 };

        close(fds[0]);
        if (semop(id, undo, 2) != 0 || semop(id, &plain, 1) != 0)
            _exit(1);
        if (write(fds[1], "x", 1) != 1)
            _exit(2);
        _exit(0);
    }
    close(fds[1]);
    check(read(fds[0], &c, 1) == 1, "the child's operations");
    close(fds[0]);
    check(child_status(pid) == 0, "child");
    check(value(id, 0) == 5, "the decrement was undone");
    check(value(id, 1) == 6, "the increment was undone, and the plain one kept");

    /* Removing the set ends every wait with EIDRM */
    set_all(id, 0, 1);
    ops[0] = (struct sembuf) { 0, -1, 0 };
    pid = waiter(id, ops, 1, false);
    check(wait_count(id, 0, GETNCNT, 1), "waiting to decrement");
    {
        struct sembuf zero = { 1, 0, 0 };
        pid_t         pid2 = waiter(id, &zero, 1, false);

        check(wait_count(id, 1, GETZCNT, 1), "waiting for zero");
        check(semctl(id, 0, IPC_RMID) == 0, "IPC_RMID");
        drop(id);
        check(child_status(pid) == 2, "EIDRM for the decrement");
        check(child_status(pid2) == 2, "EIDRM for the wait for zero");
    }

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
