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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_XSI_IPC) && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include "../is-qemu.h"

/* msgsnd queues messages, waits for room, and reports its errors */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Every queue made here, removed on every way out */
static int ids[8];
static int nids;

static void
cleanup(void)
{
    while (nids > 0)
        msgctl(ids[--nids], IPC_RMID, NULL);
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

/* A value from /proc/sys/kernel, or -1 */
static long
kernel_limit(const char *name)
{
    char  file[64];
    FILE *f;
    long  v = -1;

    snprintf(file, sizeof(file), "/proc/sys/kernel/%s", name);
    f = fopen(file, "r");
    if (f) {
        if (fscanf(f, "%ld", &v) != 1)
            v = -1;
        fclose(f);
    }
    return v;
}

/*
 * Waits until the child, which has said through the pipe that it is
 * about to block, is asleep in the kernel
 */
static bool
wait_blocked(int fd, pid_t pid)
{
    struct timespec ts = { 0, 1000 * 1000 };
    char            file[64], buf[512], *state;
    int             i;
    FILE           *f;
    size_t          n;

    if (read(fd, buf, 1) != 1)
        return false;
    snprintf(file, sizeof(file), "/proc/%ld/stat", (long)pid);
    for (i = 0; i < 20000; i++) {
        f = fopen(file, "r");
        if (!f)
            return false;
        n = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        buf[n] = '\0';
        state = strrchr(buf, ')');
        if (state && state[1] == ' ' && state[2] == 'S')
            return true;
        nanosleep(&ts, NULL);
    }
    return false;
}

static volatile sig_atomic_t caught;

static void
handler(int sig)
{
    caught = sig;
}

struct msg {
    long mtype;
    char mtext[1024];
};

static struct msg m;

/* Fills the queue, returning how many messages fit */
static int
fill(int id)
{
    int n = 0;

    m.mtype = 9;
    while (msgsnd(id, &m, sizeof(m.mtext), IPC_NOWAIT) == 0)
        n++;
    if (errno != EAGAIN)
        return -1;
    while (msgsnd(id, &m, 1, IPC_NOWAIT) == 0)
        n++;
    return errno == EAGAIN ? n : -1;
}

/* A child that blocks sending to a full queue; what it saw is its status */
static pid_t
blocked_sender(int id, int *rfd, bool catch)
{
    int   fds[2];
    pid_t pid;

    check(pipe(fds) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sigaction sa;
        int              ret;

        if (catch) {
            /* Without SA_RESTART, the signal interrupts msgsnd */
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = handler;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);
        }
        close(fds[0]);
        m.mtype = 5;
        if (write(fds[1], "x", 1) != 1)
            _exit(10);
        ret = msgsnd(id, &m, 1, 0);
        if (ret == 0)
            _exit(0);
        if (errno == EIDRM)
            _exit(2);
        if (errno == EINTR)
            _exit(caught == SIGUSR1 ? 3 : 11);
        _exit(12);
    }
    close(fds[1]);
    *rfd = fds[0];
    return pid;
}

static int
child_status(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

int
main(void)
{
    struct msqid_ds ds;
    int             id, fd, n;
    long            max;
    pid_t           pid;
    time_t          before;
    bool            stat_ok = true;

    signal(SIGALRM, watchdog);
    alarm(60);
    check(atexit(cleanup) == 0, "atexit");

    id = keep(msgget(IPC_PRIVATE, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "msgget");
#ifdef __arm__
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's msqid_ds fields: not checking them\n");
        stat_ok = false;
    }
#endif

    /* A successful send counts the message and records the sender and time */
    before = time(NULL);
    m.mtype = 1;
    memset(m.mtext, 'm', sizeof(m.mtext));
    check(msgsnd(id, &m, 100, 0) == 0, "msgsnd");
    check(msgsnd(id, &m, 0, 0) == 0, "an empty message");
    if (stat_ok) {
        check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
        check(ds.msg_qnum == 2 && ds.msg_lspid == getpid(), "msg_qnum and msg_lspid");
        check(ds.msg_stime >= before - 1 && ds.msg_stime <= time(NULL), "msg_stime");
    }
    check(msgrcv(id, &m, sizeof(m.mtext), 0, 0) == 100, "the message");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, 0) == 0, "the empty message");

    /* EINVAL: a type below 1, a size over the limit, or a bad identifier */
    m.mtype = 0;
    check(msgsnd(id, &m, 1, IPC_NOWAIT) == -1 && errno == EINVAL, "EINVAL for type 0");
    m.mtype = -1;
    check(msgsnd(id, &m, 1, IPC_NOWAIT) == -1 && errno == EINVAL, "EINVAL for type -1");
    m.mtype = 1;
    max = kernel_limit("msgmax");
    if (max > 0 && (size_t)max < sizeof(m.mtext))
        printf("msgmax %ld is small: not checking EINVAL for the size\n", max);
    else if (max > 0) {
        char *big = malloc(sizeof(long) + (size_t)max + 1);

        check(big != NULL, "malloc");
        memset(big, 0, sizeof(long) + (size_t)max + 1);
        *(long *)(void *)big = 1;
        check(msgsnd(id, big, (size_t)max + 1, IPC_NOWAIT) == -1 && errno == EINVAL,
              "EINVAL past msgmax");
        check(msgsnd(id, big, (size_t)max, IPC_NOWAIT) == 0, "msgmax itself fits");
        check(msgrcv(id, big, (size_t)max, 0, 0) == max, "and comes back");
        free(big);
    }
    check(msgsnd(-1, &m, 1, IPC_NOWAIT) == -1 && errno == EINVAL, "EINVAL for the id");
    if (stat_ok)
        check(msgctl(id, IPC_STAT, &ds) == 0 && ds.msg_qnum == 0, "nothing sent");

    /* A full queue: EAGAIN with IPC_NOWAIT, and nothing sent */
    n = fill(id);
    check(n > 0, "fill the queue");
    m.mtype = 5;
    check(msgsnd(id, &m, 1, IPC_NOWAIT) == -1 && errno == EAGAIN, "EAGAIN");
    if (stat_ok)
        check(msgctl(id, IPC_STAT, &ds) == 0 && ds.msg_qnum == (msgqnum_t)n, "nothing sent");

    /* Without IPC_NOWAIT the sender waits until there's room */
    pid = blocked_sender(id, &fd, false);
    check(wait_blocked(fd, pid), "the sender blocks");
    close(fd);
    check(msgrcv(id, &m, sizeof(m.mtext), 9, 0) > 0, "make room");
    check(child_status(pid) == 0, "the waiting sender's message went");
    check(msgrcv(id, &m, 1, 5, IPC_NOWAIT) == 1, "and arrived");
    if (stat_ok)
        check(msgctl(id, IPC_STAT, &ds) == 0 && ds.msg_lspid == pid, "msg_lspid is the child");

    /* A caught signal interrupts the wait with EINTR, sending nothing */
    check(fill(id) >= 0, "refill");
    pid = blocked_sender(id, &fd, true);
    check(wait_blocked(fd, pid), "the sender blocks");
    close(fd);
    check(kill(pid, SIGUSR1) == 0, "kill");
    check(child_status(pid) == 3, "EINTR");
    check(msgrcv(id, &m, 1, 5, IPC_NOWAIT) == -1 && errno == ENOMSG, "nothing sent");

    /* Removing the queue wakes the sender with EIDRM */
    pid = blocked_sender(id, &fd, false);
    check(wait_blocked(fd, pid), "the sender blocks");
    close(fd);
    check(msgctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    drop(id);
    check(child_status(pid) == 2, "EIDRM");
    check(msgsnd(id, &m, 1, IPC_NOWAIT) == -1 && errno == EINVAL, "EINVAL once removed");

    /* EACCES without write permission, which needs IPC_STAT's mode */
    if (geteuid() != 0 && stat_ok) {
        id = keep(msgget(IPC_PRIVATE, IPC_CREAT | 0400));
        check(id >= 0, "msgget");
        check(msgsnd(id, &m, 1, IPC_NOWAIT) == -1 && errno == EACCES, "EACCES");
    } else {
        printf("running as root: not checking EACCES\n");
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
