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
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include "../is-qemu.h"

/* msgrcv picks messages by type, waits for them, and reports its errors */

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
        if (!f) {
            /* Without Linux's /proc, give the child time to block */
            ts.tv_nsec = 100 * 1000 * 1000;
            nanosleep(&ts, NULL);
            return true;
        }
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
    char mtext[16];
};

static struct msg m;

static void
put(int id, long type, const char *text)
{
    m.mtype = type;
    memset(m.mtext, 0, sizeof(m.mtext));
    strcpy(m.mtext, text);
    check(msgsnd(id, &m, strlen(text) + 1, IPC_NOWAIT) == 0, "msgsnd");
}

/* Receives a message of the given type and checks its text */
static bool
received(int id, long type, long want_type, const char *want)
{
    ssize_t len = msgrcv(id, &m, sizeof(m.mtext), type, IPC_NOWAIT);

    return len == (ssize_t)strlen(want) + 1 && m.mtype == want_type && strcmp(m.mtext, want) == 0;
}

/* A child that blocks receiving type 7; what it saw is its status */
static pid_t
blocked_receiver(int id, int *rfd, bool catch)
{
    int   fds[2];
    pid_t pid;

    check(pipe(fds) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sigaction sa;
        ssize_t          ret;

        if (catch) {
            /* Without SA_RESTART, the signal interrupts msgrcv */
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = handler;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);
        }
        close(fds[0]);
        if (write(fds[1], "x", 1) != 1)
            _exit(10);
        ret = msgrcv(id, &m, sizeof(m.mtext), 7, 0);
        if (ret == 6 && m.mtype == 7 && strcmp(m.mtext, "seven") == 0)
            _exit(0);
        if (ret >= 0)
            _exit(13);
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
    int             id, fd;
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

    /* msgtyp 0 takes the first message, > 0 the first of that type */
    put(id, 2, "second");
    put(id, 1, "first");
    check(received(id, 1, 1, "first"), "msgrcv by type");
    check(received(id, 0, 2, "second"), "msgrcv the first message");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == -1 && errno == ENOMSG, "ENOMSG");

    /* msgtyp < 0 takes the lowest type up to its magnitude, the first of those */
    put(id, 3, "three");
    put(id, 5, "five");
    put(id, 2, "two");
    put(id, 1, "one");
    put(id, 1, "uno");
    check(msgrcv(id, &m, sizeof(m.mtext), 4, IPC_NOWAIT) == -1 && errno == ENOMSG,
          "ENOMSG for a missing type");
    check(received(id, -3, 1, "one"), "-3 gives the first of type 1");
    check(received(id, -3, 1, "uno"), "then the next of type 1");
    check(received(id, -3, 2, "two"), "then type 2");
    check(received(id, -3, 3, "three"), "then type 3");
    check(msgrcv(id, &m, sizeof(m.mtext), -4, IPC_NOWAIT) == -1 && errno == ENOMSG,
          "nothing up to 4 left");
    check(received(id, -5, 5, "five"), "-5 gives type 5");

    /* The return value counts the bytes placed in mtext */
    put(id, 1, "");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == 1, "a one-byte message");

    /* Too big: E2BIG leaves the message; MSG_NOERROR truncates and drops the rest */
    before = time(NULL);
    put(id, 3, "truncated");
    check(msgrcv(id, &m, 4, 0, IPC_NOWAIT) == -1 && errno == E2BIG, "E2BIG");
    if (stat_ok)
        check(msgctl(id, IPC_STAT, &ds) == 0 && ds.msg_qnum == 1 && ds.msg_lrpid == getpid(),
              "E2BIG receives nothing");
    memset(m.mtext, 0, sizeof(m.mtext));
    check(msgrcv(id, &m, 4, 0, IPC_NOWAIT | MSG_NOERROR) == 4 && m.mtype == 3
              && memcmp(m.mtext, "trun", 4) == 0 && m.mtext[4] == '\0',
          "MSG_NOERROR truncates");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == -1 && errno == ENOMSG,
          "the rest is lost");

    /* A receive counts down msg_qnum, and sets msg_lrpid and msg_rtime */
    if (stat_ok) {
        put(id, 1, "x");
        put(id, 1, "y");
        fflush(stdout);
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0)
            _exit(received(id, 0, 1, "x") ? 0 : 1);
        check(child_status(pid) == 0, "the child's msgrcv");
        check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
        check(ds.msg_qnum == 1 && ds.msg_lrpid == pid, "msg_qnum and msg_lrpid");
        check(ds.msg_rtime >= before - 1 && ds.msg_rtime <= time(NULL), "msg_rtime");
        check(received(id, 0, 1, "y"), "msgrcv");
    }

    /* Without IPC_NOWAIT the receiver waits for a message of its type */
    pid = blocked_receiver(id, &fd, false);
    check(wait_blocked(fd, pid), "the receiver blocks");
    close(fd);
    put(id, 6, "six");
    put(id, 7, "seven");
    check(child_status(pid) == 0, "the waiting receiver got type 7");
    check(received(id, 0, 6, "six"), "and left type 6");

    /* A caught signal interrupts the wait with EINTR */
    pid = blocked_receiver(id, &fd, true);
    check(wait_blocked(fd, pid), "the receiver blocks");
    close(fd);
    check(kill(pid, SIGUSR1) == 0, "kill");
    check(child_status(pid) == 3, "EINTR");

    /* Removing the queue wakes the receiver with EIDRM */
    pid = blocked_receiver(id, &fd, false);
    check(wait_blocked(fd, pid), "the receiver blocks");
    close(fd);
    check(msgctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    drop(id);
    check(child_status(pid) == 2, "EIDRM");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == -1 && errno == EINVAL,
          "EINVAL once removed");
    check(msgrcv(-1, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == -1 && errno == EINVAL,
          "EINVAL for the id");

    /* EACCES without read permission */
    if (geteuid() != 0) {
        id = keep(msgget(IPC_PRIVATE, IPC_CREAT | 0200));
        check(id >= 0, "msgget");
        put(id, 1, "secret");
        check(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == -1 && errno == EACCES, "EACCES");
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
