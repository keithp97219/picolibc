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
/* lockf and F_LOCK are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && _POSIX_SEMAPHORES > 0 && defined(__PICOLIBC_POSIX_PIPE) \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#if _POSIX_MESSAGE_PASSING > 0
#include <mqueue.h>
#endif
#if _POSIX_ASYNCHRONOUS_IO > 0
#include <aio.h>
#endif
#ifndef __STDC_NO_THREADS__
#include <threads.h>
#endif

/* Cancellation points, cancelability while exiting, and pthread_once */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static const struct timespec zero_ts;
static char                  buf[16];
static pthread_mutex_t       lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t        cond = PTHREAD_COND_INITIALIZER;
static sem_t                 started, release;

static void
pause_ms(long ms)
{
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };

    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
        ;
}

static void
unlock(void *arg)
{
    pthread_mutex_unlock(arg);
}

/* Calls to make with a cancellation request pending */

static void
do_accept(void)
{
    (void)accept(-1, NULL, NULL);
}

static void
do_clock_nanosleep(void)
{
    (void)clock_nanosleep(CLOCK_MONOTONIC, 0, &zero_ts, NULL);
}

static void
do_close(void)
{
    (void)close(-1);
}

static void
do_connect(void)
{
    struct sockaddr sa = { .sa_family = AF_UNIX };

    (void)connect(-1, &sa, sizeof(sa));
}

static void
do_creat(void)
{
    (void)creat("/nonexistent-directory/file", 0600);
}

static void
do_fcntl(void)
{
    struct flock fl = { .l_type = F_WRLCK, .l_whence = SEEK_SET };

    (void)fcntl(-1, F_SETLKW, &fl);
}

static void
do_fdatasync(void)
{
    (void)fdatasync(-1);
}

static void
do_fsync(void)
{
    (void)fsync(-1);
}

static void
do_lockf(void)
{
    (void)lockf(-1, F_LOCK, 0);
}

#if _POSIX_MESSAGE_PASSING > 0
static void
do_mq_receive(void)
{
    (void)mq_receive((mqd_t)-1, buf, sizeof(buf), NULL);
}

static void
do_mq_send(void)
{
    (void)mq_send((mqd_t)-1, buf, 1, 0);
}

static void
do_mq_timedreceive(void)
{
    (void)mq_timedreceive((mqd_t)-1, buf, sizeof(buf), NULL, &zero_ts);
}

static void
do_mq_timedsend(void)
{
    (void)mq_timedsend((mqd_t)-1, buf, 1, 0, &zero_ts);
}
#endif

static void
do_msgrcv(void)
{
    (void)msgrcv(-1, buf, sizeof(buf) - sizeof(long), 0, IPC_NOWAIT);
}

static void
do_msgsnd(void)
{
    (void)msgsnd(-1, buf, 1, IPC_NOWAIT);
}

static void
do_msync(void)
{
    (void)msync(NULL, 0, MS_SYNC);
}

static void
do_nanosleep(void)
{
    (void)nanosleep(&zero_ts, NULL);
}

static void
do_open(void)
{
    (void)open("/nonexistent-directory/file", O_RDONLY);
}

static void
do_openat(void)
{
    (void)openat(AT_FDCWD, "/nonexistent-directory/file", O_RDONLY);
}

static void
do_pause(void)
{
    (void)pause();
}

static void
do_poll(void)
{
    (void)poll(NULL, 0, 0);
}

static void
do_pread(void)
{
    /* A bad offset as well: argument checks mustn't skip the cancellation point */
    (void)pread(-1, buf, 1, -1);
}

static void
do_pselect(void)
{
    (void)pselect(0, NULL, NULL, NULL, &zero_ts, NULL);
}

static void
do_cond_timedwait(void)
{
    pthread_mutex_lock(&lock);
    pthread_cleanup_push(unlock, &lock);
    (void)pthread_cond_timedwait(&cond, &lock, &zero_ts);
    pthread_cleanup_pop(1);
}

static void
do_cond_wait(void)
{
    pthread_mutex_lock(&lock);
    pthread_cleanup_push(unlock, &lock);
    (void)pthread_cond_wait(&cond, &lock);
    pthread_cleanup_pop(1);
}

static void
do_pwrite(void)
{
    /* A bad offset as well: argument checks mustn't skip the cancellation point */
    (void)pwrite(-1, buf, 1, -1);
}

static void
do_read(void)
{
    (void)read(-1, buf, 1);
}

static void
do_readv(void)
{
    struct iovec iov = { .iov_base = buf, .iov_len = 1 };

    (void)readv(-1, &iov, 1);
}

static void
do_recv(void)
{
    (void)recv(-1, buf, 1, 0);
}

static void
do_recvfrom(void)
{
    (void)recvfrom(-1, buf, 1, 0, NULL, NULL);
}

static void
do_recvmsg(void)
{
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    (void)recvmsg(-1, &msg, 0);
}

static void
do_select(void)
{
    struct timeval tv = { 0 };

    (void)select(0, NULL, NULL, NULL, &tv);
}

static void
do_send(void)
{
    (void)send(-1, buf, 1, 0);
}

static void
do_sendmsg(void)
{
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    (void)sendmsg(-1, &msg, 0);
}

static void
do_sendto(void)
{
    (void)sendto(-1, buf, 1, 0, NULL, 0);
}

static void
do_sigsuspend(void)
{
    sigset_t set;

    pthread_sigmask(SIG_BLOCK, NULL, &set);
    (void)sigsuspend(&set);
}

/* SIGUSR2, blocked and already pending for the calling thread */
static void
pending_usr2(sigset_t *set)
{
    sigemptyset(set);
    sigaddset(set, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, set, NULL);
    pthread_kill(pthread_self(), SIGUSR2);
}

static void
do_sigtimedwait(void)
{
    sigset_t set;

    pending_usr2(&set);
    (void)sigtimedwait(&set, NULL, &zero_ts);
}

static void
do_sigwait(void)
{
    sigset_t set;
    int      sig;

    pending_usr2(&set);
    (void)sigwait(&set, &sig);
}

static void
do_sigwaitinfo(void)
{
    sigset_t set;

    pending_usr2(&set);
    (void)sigwaitinfo(&set, NULL);
}

static void
do_sleep(void)
{
    (void)sleep(0);
}

static void
do_tcdrain(void)
{
    (void)tcdrain(-1);
}

static void
do_wait(void)
{
    (void)wait(NULL);
}

static void
do_waitid(void)
{
    siginfo_t info;

    (void)waitid(P_ALL, 0, &info, WEXITED | WNOHANG);
}

static void
do_waitpid(void)
{
    (void)waitpid(-1, NULL, WNOHANG);
}

static void
do_write(void)
{
    (void)write(-1, buf, 1);
}

static void
do_writev(void)
{
    struct iovec iov = { .iov_base = buf, .iov_len = 1 };

    (void)writev(-1, &iov, 1);
}

#ifndef __STDC_NO_THREADS__
static cnd_t cnd;
static mtx_t mtx;

static void
mtx_cleanup(void *arg)
{
    mtx_unlock(arg);
}

static void
do_cnd_timedwait(void)
{
    mtx_lock(&mtx);
    pthread_cleanup_push(mtx_cleanup, &mtx);
    (void)cnd_timedwait(&cnd, &mtx, &zero_ts);
    pthread_cleanup_pop(1);
}

static void
do_thrd_sleep(void)
{
    (void)thrd_sleep(&zero_ts, NULL);
}
#endif

/* glibc 2.43 declares these only for _GNU_SOURCE, or not at all */
#ifndef __GLIBC__
static void
do_accept4(void)
{
    (void)accept4(-1, NULL, NULL, 0);
}

static void
do_cond_clockwait(void)
{
    pthread_mutex_lock(&lock);
    pthread_cleanup_push(unlock, &lock);
    (void)pthread_cond_clockwait(&cond, &lock, CLOCK_MONOTONIC, &zero_ts);
    pthread_cleanup_pop(1);
}

static void
do_posix_close(void)
{
    (void)posix_close(-1, 0);
}

static void
do_ppoll(void)
{
    (void)ppoll(NULL, 0, &zero_ts, NULL);
}
#endif

/* Calls which aren't cancellation points */

static void
not_mutex(void)
{
    struct timespec ts;

    pthread_mutex_lock(&lock);
    pthread_mutex_unlock(&lock);
    clock_gettime(CLOCK_REALTIME, &ts);
    pthread_mutex_timedlock(&lock, &ts);
    pthread_mutex_unlock(&lock);
}

static void
not_cond(void)
{
    pthread_cond_signal(&cond);
    pthread_cond_broadcast(&cond);
}

static void
not_sem(void)
{
    sem_t sem;

    sem_init(&sem, 0, 0);
    sem_post(&sem);
    sem_trywait(&sem);
    sem_trywait(&sem);
    sem_destroy(&sem);
}

static void
plain(void)
{
}

static pthread_once_t not_once_control = PTHREAD_ONCE_INIT;

static void
not_once(void)
{
    pthread_once(&not_once_control, plain);
}

static void
not_cancel_calls(void)
{
    int old;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old);
    pthread_kill(pthread_self(), 0);
    sched_yield();
}

static void
not_barrier(void)
{
    pthread_barrier_t barrier;

    pthread_barrier_init(&barrier, NULL, 1);
    pthread_barrier_wait(&barrier);
    pthread_barrier_destroy(&barrier);
}

static void
not_misc(void)
{
    void *p = malloc(10);

    free(p);
    (void)getpid();
    (void)time(NULL);
}

static void
not_if_nametoindex(void)
{
    (void)if_nametoindex("lo");
}

static void
not_if_indextoname(void)
{
    char name[IF_NAMESIZE];

    (void)if_indextoname(1, name);
}

/* glibc 2.43's if_nameindex is a cancellation point, which POSIX doesn't allow */
#ifndef __GLIBC__
static void
not_if_nameindex(void)
{
    struct if_nameindex *ni = if_nameindex();

    if (ni)
        if_freenameindex(ni);
}
#endif

struct call {
    const char *name;
    void        (*func)(void);
    int         cancels;
};

#define CP(f)     { #f, do_##f, 1 }
#define NOT_CP(f) { #f, not_##f, 0 }

static const struct call calls[] = {
    CP(accept),
    CP(clock_nanosleep),
    CP(close),
    CP(connect),
    CP(creat),
    CP(fcntl),
    CP(fdatasync),
    CP(fsync),
    CP(lockf),
#if _POSIX_MESSAGE_PASSING > 0
    CP(mq_receive),
    CP(mq_send),
    CP(mq_timedreceive),
    CP(mq_timedsend),
#endif
    CP(msgrcv),
    CP(msgsnd),
    CP(msync),
    CP(nanosleep),
    CP(open),
    CP(openat),
    CP(pause),
    CP(poll),
    CP(pread),
    CP(pselect),
    CP(cond_timedwait),
    CP(cond_wait),
    CP(pwrite),
    CP(read),
    CP(readv),
    CP(recv),
    CP(recvfrom),
    CP(recvmsg),
    CP(select),
    CP(send),
    CP(sendmsg),
    CP(sendto),
    CP(sigsuspend),
    CP(sigtimedwait),
    CP(sigwait),
    CP(sigwaitinfo),
    CP(sleep),
    CP(tcdrain),
    CP(wait),
    CP(waitid),
    CP(waitpid),
    CP(write),
    CP(writev),
#ifndef __STDC_NO_THREADS__
    CP(cnd_timedwait),
    CP(thrd_sleep),
#endif
#ifndef __GLIBC__
    CP(accept4),
    CP(cond_clockwait),
    CP(posix_close),
    CP(ppoll),
#endif
    NOT_CP(mutex),
    NOT_CP(cond),
    NOT_CP(sem),
    NOT_CP(once),
    NOT_CP(cancel_calls),
    NOT_CP(barrier),
    NOT_CP(misc),
    NOT_CP(if_nametoindex),
    NOT_CP(if_indextoname),
#ifndef __GLIBC__
    NOT_CP(if_nameindex),
#endif
};

/* Make a call with a cancellation request already pending */
static void *
call_pending(void *arg)
{
    const struct call *call = arg;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_cancel(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    call->func();
    return (void *)1;
}

/* Cancelability while cleanup handlers and destructors run */
static volatile int  exit_state, exit_type, exit_reached;
static pthread_key_t key;

static void
record_cancelability(void *arg)
{
    int old;

    (void)arg;
    /* Would act on the pending request if cancellation were enabled */
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
    exit_state = old;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old);
    exit_type = old;
    exit_reached++;
}

static void *
exit_pending(void *arg)
{
    pthread_setspecific(key, (void *)1);
    pthread_cleanup_push(record_cancelability, NULL);
    pthread_cancel(pthread_self());
    pthread_exit(arg);
    pthread_cleanup_pop(0);
    return NULL;
}

static void *
exit_async(void *arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_cleanup_push(record_cancelability, NULL);
    pthread_exit(arg);
    pthread_cleanup_pop(0);
    return NULL;
}

static volatile unsigned long spins;

static void *
cancel_async(void *arg)
{
    (void)arg;
    pthread_cleanup_push(record_cancelability, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    sem_post(&started);
    for (;;)
        spins++;
    pthread_cleanup_pop(0);
    return NULL;
}

/* A request against a thread with cancellation disabled doesn't interrupt it */
static volatile int sleep_ret, sleep_errno;

static void *
disabled_sleep(void *arg)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 200 * 1000 * 1000 };

    (void)arg;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    sem_post(&started);
    errno = 0;
    sleep_ret = nanosleep(&ts, NULL);
    sleep_errno = errno;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    return NULL;
}

/* A deferred request waits for a cancellation point */
static volatile int go, reached;

static void *
deferred(void *arg)
{
    (void)arg;
    sem_post(&started);
    while (!go)
        ;
    reached = 1;
    pthread_testcancel();
    return NULL;
}

/* New threads start with cancellation enabled and deferred */
static void *
initial_cancelability(void *arg)
{
    int state, type;

    (void)arg;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type);
    return (void *)(intptr_t)(state == PTHREAD_CANCEL_ENABLE && type == PTHREAD_CANCEL_DEFERRED);
}

/* An init routine cancelled at a cancellation point leaves once_control untouched */
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static volatile int   once_first, once_second;

static void
once_cancelled(void)
{
    once_first++;
    sem_post(&started);
    for (;;)
        sem_wait(&release);
}

static void
once_runs(void)
{
    once_second++;
}

static void *
run_once(void *arg)
{
    pthread_once(&once_control, arg);
    return NULL;
}

/*
 * aio_suspend is a cancellation point which leaves the library
 * usable. glibc 2.43 returns at once from aio_suspend with only NULL
 * entries, which this uses to wait, so it's only checked for picolibc
 */
#if defined(__PICOLIBC__) && _POSIX_ASYNCHRONOUS_IO > 0
static struct aiocb aio_cb;

static void *
aio_waiter(void *arg)
{
    /* NULL entries are ignored, so this waits until cancelled */
    const struct aiocb *list[1] = { NULL };

    (void)arg;
    sem_post(&started);
    if (aio_suspend(list, 1, NULL) != 0)
        return (void *)(intptr_t)errno;
    return (void *)-2;
}
#endif

int
main(void)
{
    pthread_t thread, waiter;
    void     *result;

    /* Fail rather than hang if a call doesn't act on a request */
    alarm(60);

    check(sem_init(&started, 0, 0) == 0, "sem_init");
    check(sem_init(&release, 0, 0) == 0, "sem_init");
    check(pthread_key_create(&key, record_cancelability) == 0, "pthread_key_create");
#ifndef __STDC_NO_THREADS__
    check(cnd_init(&cnd) == thrd_success && mtx_init(&mtx, mtx_plain) == thrd_success,
          "cnd_init, mtx_init");
#endif

    /* Each cancellation point acts on a pending request; other calls don't */
    for (size_t i = 0; i < sizeof(calls) / sizeof(calls[0]); i++) {
        check(pthread_create(&thread, NULL, call_pending, (void *)&calls[i]) == 0,
              "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        if (result != (calls[i].cancels ? PTHREAD_CANCELED : (void *)1))
            printf("%s %s a cancellation point\n", calls[i].name,
                   calls[i].cancels ? "is not" : "is");
        check(result == (calls[i].cancels ? PTHREAD_CANCELED : (void *)1), calls[i].name);
    }

    /* New threads have cancellation enabled and deferred */
    check(pthread_create(&thread, NULL, initial_cancelability, NULL) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && result == (void *)1,
          "initial cancelability of a new thread");

    /* pthread_exit disables cancellation before running cleanup handlers and destructors */
    exit_reached = 0;
    check(pthread_create(&thread, NULL, exit_pending, (void *)22) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == (void *)22, "pthread_exit value kept");
    check(exit_reached == 2, "cleanup handler and destructor finished");
    check(exit_state == PTHREAD_CANCEL_DISABLE, "cancellation disabled by pthread_exit");

    /* ... and makes it deferred */
    exit_reached = 0;
    check(pthread_create(&thread, NULL, exit_async, (void *)23) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && result == (void *)23, "pthread_join");
    check(exit_reached == 1, "cleanup handler finished");
    check(exit_type == PTHREAD_CANCEL_DEFERRED, "cancellation deferred by pthread_exit");

    /* So does acting on a cancellation request */
    exit_reached = 0;
    check(pthread_create(&thread, NULL, cancel_async, NULL) == 0, "pthread_create");
    check(sem_wait(&started) == 0, "sem_wait");
    check(pthread_cancel(thread) == 0, "pthread_cancel");
    check(pthread_join(thread, &result) == 0 && result == PTHREAD_CANCELED, "pthread_join");
    check(exit_reached == 1, "cleanup handler finished");
    check(exit_state == PTHREAD_CANCEL_DISABLE, "cancellation disabled when acted on");
    check(exit_type == PTHREAD_CANCEL_DEFERRED, "cancellation deferred when acted on");

    /* With cancellation disabled, a request doesn't interrupt a sleep */
    check(pthread_create(&thread, NULL, disabled_sleep, NULL) == 0, "pthread_create");
    check(sem_wait(&started) == 0, "sem_wait");
    pause_ms(50);
    check(pthread_cancel(thread) == 0, "pthread_cancel");
    check(pthread_join(thread, &result) == 0 && result == PTHREAD_CANCELED, "pthread_join");
    if (sleep_ret != 0)
        printf("nanosleep returned %d errno %d\n", sleep_ret, sleep_errno);
    check(sleep_ret == 0, "nanosleep with cancellation disabled");

    /* A deferred request is held until a cancellation point */
    check(pthread_create(&thread, NULL, deferred, NULL) == 0, "pthread_create");
    check(sem_wait(&started) == 0, "sem_wait");
    check(pthread_cancel(thread) == 0, "pthread_cancel");
    pause_ms(20);
    go = 1;
    check(pthread_join(thread, &result) == 0 && result == PTHREAD_CANCELED, "pthread_join");
    check(reached, "deferred request acted on before a cancellation point");

    /* A cancelled init routine: once_control is as if pthread_once was never called */
    check(pthread_create(&thread, NULL, run_once, (void *)once_cancelled) == 0, "pthread_create");
    check(sem_wait(&started) == 0, "sem_wait");
    check(pthread_create(&waiter, NULL, run_once, (void *)once_runs) == 0, "pthread_create");
    pause_ms(20); /* let the waiter block in pthread_once */
    check(pthread_cancel(thread) == 0, "pthread_cancel");
    check(pthread_join(thread, &result) == 0 && result == PTHREAD_CANCELED, "pthread_join");
    check(pthread_join(waiter, NULL) == 0, "waiting pthread_once returned");
    check(once_first == 1 && once_second == 1, "init routine ran again");
    check(pthread_once(&once_control, once_runs) == 0 && once_second == 1, "pthread_once");

#if defined(__PICOLIBC__) && _POSIX_ASYNCHRONOUS_IO > 0
    {
        const struct aiocb *list[1] = { &aio_cb };
        struct timespec     timeout = { .tv_sec = 5, .tv_nsec = 0 };
        int                 fds[2];
        pid_t               pid;
        int                 status = 0;

        check(pthread_create(&thread, NULL, aio_waiter, NULL) == 0, "pthread_create");
        check(sem_wait(&started) == 0, "sem_wait");
        pause_ms(20); /* let the thread block in aio_suspend */
        check(pthread_cancel(thread) == 0, "pthread_cancel");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        if (result != PTHREAD_CANCELED)
            printf("aio_suspend returned %d\n", (int)(intptr_t)result);
        check(result == PTHREAD_CANCELED, "aio_suspend is a cancellation point");

        /*
         * Asynchronous I/O still works. Check in a child, which a lock
         * left held would block with every signal blocked
         */
        fflush(stdout);
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0) {
            if (pipe(fds) != 0 || write(fds[1], "x", 1) != 1)
                _exit(2);
            memset(&aio_cb, 0, sizeof(aio_cb));
            aio_cb.aio_fildes = fds[0];
            aio_cb.aio_buf = buf;
            aio_cb.aio_nbytes = 1;
            aio_cb.aio_sigevent.sigev_notify = SIGEV_NONE;
            if (aio_read(&aio_cb) != 0)
                _exit(3);
            if (aio_suspend(list, 1, &timeout) != 0 || aio_error(&aio_cb) == EINPROGRESS)
                _exit(4);
            (void)aio_return(&aio_cb);
            _exit(0);
        }
        for (int i = 0; i < 1000 && waitpid(pid, &status, WNOHANG) == 0; i++)
            pause_ms(10);
        if (waitpid(pid, &status, WNOHANG) == 0) {
            kill(pid, SIGKILL);
            check(waitpid(pid, &status, 0) == pid, "waitpid");
            printf("asynchronous I/O hung after a cancelled aio_suspend\n");
        }
        check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "aio_suspend after a cancelled one");
    }
#endif

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
