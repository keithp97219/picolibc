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
/* IOV_MAX, ATEXIT_MAX's partner NZERO and NL_LANGMAX are XSI */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * sysconf's limits agree with <limits.h>: a runtime invariant value
 * the header defines is what sysconf reports, a runtime increasable
 * one is at most what it reports, and none is below the POSIX
 * minimum. Where the system lets a test reach a limit, the limit is
 * where sysconf says.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The minimum values, which are fixed */
_Static_assert(_POSIX_AIO_LISTIO_MAX == 2, "_POSIX_AIO_LISTIO_MAX");
_Static_assert(_POSIX_AIO_MAX == 1, "_POSIX_AIO_MAX");
_Static_assert(_POSIX_ARG_MAX == 4096, "_POSIX_ARG_MAX");
_Static_assert(_POSIX_CHILD_MAX == 25, "_POSIX_CHILD_MAX");
_Static_assert(_POSIX_DELAYTIMER_MAX == 32, "_POSIX_DELAYTIMER_MAX");
_Static_assert(_POSIX_HOST_NAME_MAX == 255, "_POSIX_HOST_NAME_MAX");
_Static_assert(_POSIX_LINK_MAX == 8, "_POSIX_LINK_MAX");
_Static_assert(_POSIX_LOGIN_NAME_MAX == 9, "_POSIX_LOGIN_NAME_MAX");
_Static_assert(_POSIX_MAX_CANON == 255, "_POSIX_MAX_CANON");
_Static_assert(_POSIX_MAX_INPUT == 255, "_POSIX_MAX_INPUT");
_Static_assert(_POSIX_NAME_MAX == 14, "_POSIX_NAME_MAX");
_Static_assert(_POSIX_NGROUPS_MAX == 8, "_POSIX_NGROUPS_MAX");
_Static_assert(_POSIX_OPEN_MAX == 20, "_POSIX_OPEN_MAX");
_Static_assert(_POSIX_PATH_MAX == 256, "_POSIX_PATH_MAX");
_Static_assert(_POSIX_PIPE_BUF == 512, "_POSIX_PIPE_BUF");
_Static_assert(_POSIX_RE_DUP_MAX == 255, "_POSIX_RE_DUP_MAX");
_Static_assert(_POSIX_RTSIG_MAX == 8, "_POSIX_RTSIG_MAX");
_Static_assert(_POSIX_SEM_NSEMS_MAX == 256, "_POSIX_SEM_NSEMS_MAX");
_Static_assert(_POSIX_SEM_VALUE_MAX == 32767, "_POSIX_SEM_VALUE_MAX");
_Static_assert(_POSIX_SIGQUEUE_MAX == 32, "_POSIX_SIGQUEUE_MAX");
_Static_assert(_POSIX_SSIZE_MAX == 32767, "_POSIX_SSIZE_MAX");
_Static_assert(_POSIX_STREAM_MAX == 8, "_POSIX_STREAM_MAX");
_Static_assert(_POSIX_SYMLINK_MAX == 255, "_POSIX_SYMLINK_MAX");
_Static_assert(_POSIX_SYMLOOP_MAX == 8, "_POSIX_SYMLOOP_MAX");
_Static_assert(_POSIX_THREAD_DESTRUCTOR_ITERATIONS == 4, "_POSIX_THREAD_DESTRUCTOR_ITERATIONS");
_Static_assert(_POSIX_THREAD_KEYS_MAX == 128, "_POSIX_THREAD_KEYS_MAX");
_Static_assert(_POSIX_THREAD_THREADS_MAX == 64, "_POSIX_THREAD_THREADS_MAX");
_Static_assert(_POSIX_TIMER_MAX == 32, "_POSIX_TIMER_MAX");
_Static_assert(_POSIX_TTY_NAME_MAX == 9, "_POSIX_TTY_NAME_MAX");
_Static_assert(_POSIX_TZNAME_MAX == 6, "_POSIX_TZNAME_MAX");
_Static_assert(_POSIX2_BC_BASE_MAX == 99, "_POSIX2_BC_BASE_MAX");
_Static_assert(_POSIX2_BC_DIM_MAX == 2048, "_POSIX2_BC_DIM_MAX");
_Static_assert(_POSIX2_BC_SCALE_MAX == 99, "_POSIX2_BC_SCALE_MAX");
_Static_assert(_POSIX2_BC_STRING_MAX == 1000, "_POSIX2_BC_STRING_MAX");
_Static_assert(_POSIX2_CHARCLASS_NAME_MAX == 14, "_POSIX2_CHARCLASS_NAME_MAX");
_Static_assert(_POSIX2_COLL_WEIGHTS_MAX == 2, "_POSIX2_COLL_WEIGHTS_MAX");
_Static_assert(_POSIX2_EXPR_NEST_MAX == 32, "_POSIX2_EXPR_NEST_MAX");
_Static_assert(_POSIX2_LINE_MAX == 2048, "_POSIX2_LINE_MAX");
_Static_assert(_POSIX2_RE_DUP_MAX == 255, "_POSIX2_RE_DUP_MAX");
/* glibc lacks _XOPEN_NAME_MAX and _XOPEN_PATH_MAX, and _XOPEN_IOV_MAX in strict mode */
#ifndef __GLIBC__
_Static_assert(_XOPEN_IOV_MAX == 16, "_XOPEN_IOV_MAX");
_Static_assert(_XOPEN_NAME_MAX == 255, "_XOPEN_NAME_MAX");
_Static_assert(_XOPEN_PATH_MAX == 1024, "_XOPEN_PATH_MAX");
#define XOPEN_NAME_MAX _XOPEN_NAME_MAX
#define XOPEN_PATH_MAX _XOPEN_PATH_MAX
#define XOPEN_IOV_MAX  _XOPEN_IOV_MAX
#else
#define XOPEN_NAME_MAX 255
#define XOPEN_PATH_MAX 1024
#define XOPEN_IOV_MAX  16
#endif
/* The maximum value */
_Static_assert(_POSIX_CLOCKRES_MIN == 20000000, "_POSIX_CLOCKRES_MIN");

/* The values <limits.h> must define, at least their minimums */
_Static_assert(BC_BASE_MAX >= _POSIX2_BC_BASE_MAX, "BC_BASE_MAX");
_Static_assert(BC_DIM_MAX >= _POSIX2_BC_DIM_MAX, "BC_DIM_MAX");
_Static_assert(BC_SCALE_MAX >= _POSIX2_BC_SCALE_MAX, "BC_SCALE_MAX");
_Static_assert(BC_STRING_MAX >= _POSIX2_BC_STRING_MAX, "BC_STRING_MAX");
_Static_assert(CHARCLASS_NAME_MAX >= _POSIX2_CHARCLASS_NAME_MAX, "CHARCLASS_NAME_MAX");
_Static_assert(COLL_WEIGHTS_MAX >= _POSIX2_COLL_WEIGHTS_MAX, "COLL_WEIGHTS_MAX");
_Static_assert(EXPR_NEST_MAX >= _POSIX2_EXPR_NEST_MAX, "EXPR_NEST_MAX");
_Static_assert(LINE_MAX >= _POSIX2_LINE_MAX, "LINE_MAX");
_Static_assert(NGROUPS_MAX >= _POSIX_NGROUPS_MAX, "NGROUPS_MAX");
_Static_assert(RE_DUP_MAX >= _POSIX_RE_DUP_MAX, "RE_DUP_MAX");
_Static_assert(NL_ARGMAX >= 9, "NL_ARGMAX");
_Static_assert(NL_LANGMAX >= 14, "NL_LANGMAX");
_Static_assert(NL_MSGMAX >= 32767, "NL_MSGMAX");
_Static_assert(NL_SETMAX >= 255, "NL_SETMAX");
_Static_assert(NL_TEXTMAX >= _POSIX2_LINE_MAX, "NL_TEXTMAX");
_Static_assert(LONG_BIT == sizeof(long) * CHAR_BIT, "LONG_BIT");
_Static_assert(WORD_BIT == sizeof(int) * CHAR_BIT, "WORD_BIT");
#if !defined(LONG_BIT) || !defined(WORD_BIT) || LONG_BIT < 32 || WORD_BIT < 32
#error "LONG_BIT and WORD_BIT usable in #if"
#endif
#if defined(NAME_MAX) && NAME_MAX < XOPEN_NAME_MAX
#error "NAME_MAX below _XOPEN_NAME_MAX"
#endif
#if defined(PATH_MAX) && PATH_MAX < XOPEN_PATH_MAX
#error "PATH_MAX below _XOPEN_PATH_MAX"
#endif
#if defined(IOV_MAX) && IOV_MAX < XOPEN_IOV_MAX
#error "IOV_MAX below _XOPEN_IOV_MAX"
#endif

/* A runtime invariant value <limits.h> defines is what sysconf reports */
#define INVARIANT(sc, value)                                                     \
    do {                                                                         \
        long v_ = sysconf(sc);                                                   \
        if (v_ != (long)(value)) {                                               \
            printf("%s: sysconf %ld, <limits.h> %ld\n", #sc, v_, (long)(value)); \
            exit(1);                                                             \
        }                                                                        \
    } while (0)

/* A runtime increasable value is the least sysconf reports */
#define INCREASABLE(sc, value)                                                         \
    do {                                                                               \
        long v_ = sysconf(sc);                                                         \
        if (v_ != -1 && v_ < (long)(value)) {                                          \
            printf("%s: sysconf %ld, below <limits.h> %ld\n", #sc, v_, (long)(value)); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* Without a <limits.h> value, sysconf reports at least the minimum, or -1 */
#define MINIMUM(sc, minimum)                                                              \
    do {                                                                                  \
        long v_ = sysconf(sc);                                                            \
        if (v_ != -1 && v_ < (long)(minimum)) {                                           \
            printf("%s: sysconf %ld, below the minimum %ld\n", #sc, v_, (long)(minimum)); \
            exit(1);                                                                      \
        }                                                                                 \
    } while (0)

#ifdef __PICOLIBC_POSIX_FILE_SYSTEM
/* The limit named by the first word of a /proc/self/limits line, or -2 */
static long
proc_limit(const char *name)
{
    FILE *f = fopen("/proc/self/limits", "r");
    char  line[256];
    long  value = -2;

    if (!f)
        return -2;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, name, strlen(name)) == 0) {
            const char *p = line + strlen(name);

            while (*p == ' ')
                p++;
            value = strncmp(p, "unlimited", 9) == 0 ? -1 : strtol(p, NULL, 10);
            break;
        }
    }
    fclose(f);
    return value;
}
#endif

/* The CPUs in a Linux CPU list file, or -1 */
static long
cpu_list(const char *path)
{
    FILE *f = fopen(path, "r");
    long  count = 0, first, last;
    int   c;

    if (!f)
        return -1;
    for (;;) {
        if (fscanf(f, "%ld", &first) != 1)
            break;
        last = first;
        c = getc(f);
        if (c == '-') {
            if (fscanf(f, "%ld", &last) != 1)
                break;
            c = getc(f);
        }
        count += last - first + 1;
        if (c != ',')
            break;
    }
    fclose(f);
    return count > 0 ? count : -1;
}

#ifdef __PICOLIBC_POSIX_SYMBOLIC_LINKS
#include <fcntl.h>

/*
 * A chain of n symbolic links, link<n> -> link<n-1> ... link1 -> file,
 * resolves; returns the errno of opening link<n>, or 0
 */
static int
symlink_chain(const char *dir, long n)
{
    char from[PATH_MAX], to[PATH_MAX];
    long i;
    int  fd, err = 0;

    snprintf(to, sizeof(to), "%s/file", dir);
    fd = open(to, O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create the file");
    for (i = 1; i <= n; i++) {
        snprintf(from, sizeof(from), "%s/link%ld", dir, i);
        if (i == 1)
            snprintf(to, sizeof(to), "file");
        else
            snprintf(to, sizeof(to), "link%ld", i - 1);
        check(symlink(to, from) == 0, "symlink");
    }
    fd = open(from, O_RDONLY);
    if (fd < 0)
        err = errno;
    else
        close(fd);
    for (i = 1; i <= n; i++) {
        snprintf(from, sizeof(from), "%s/link%ld", dir, i);
        (void)unlink(from);
    }
    snprintf(to, sizeof(to), "%s/file", dir);
    (void)unlink(to);
    return err;
}
#endif

#if defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING >= 0
#include <fcntl.h>
#include <mqueue.h>
#endif

#if defined(_POSIX_SEMAPHORES) && _POSIX_SEMAPHORES >= 0
#include <semaphore.h>
#endif

#include <sys/uio.h>
#include <time.h>

static void
nothing(void)
{
}

int
main(void)
{
    long v;

    /* Runtime invariant values */
#ifdef AIO_LISTIO_MAX
    INVARIANT(_SC_AIO_LISTIO_MAX, AIO_LISTIO_MAX);
#else
    MINIMUM(_SC_AIO_LISTIO_MAX, _POSIX_AIO_LISTIO_MAX);
#endif
#ifdef AIO_MAX
    INVARIANT(_SC_AIO_MAX, AIO_MAX);
#else
    MINIMUM(_SC_AIO_MAX, _POSIX_AIO_MAX);
#endif
#ifdef AIO_PRIO_DELTA_MAX
    INVARIANT(_SC_AIO_PRIO_DELTA_MAX, AIO_PRIO_DELTA_MAX);
#else
    MINIMUM(_SC_AIO_PRIO_DELTA_MAX, 0);
#endif
#ifdef ATEXIT_MAX
    INVARIANT(_SC_ATEXIT_MAX, ATEXIT_MAX);
#else
    MINIMUM(_SC_ATEXIT_MAX, 32);
#endif
#ifdef DELAYTIMER_MAX
    INVARIANT(_SC_DELAYTIMER_MAX, DELAYTIMER_MAX);
#else
    MINIMUM(_SC_DELAYTIMER_MAX, _POSIX_DELAYTIMER_MAX);
#endif
#ifdef HOST_NAME_MAX
    INVARIANT(_SC_HOST_NAME_MAX, HOST_NAME_MAX);
#else
    MINIMUM(_SC_HOST_NAME_MAX, _POSIX_HOST_NAME_MAX);
#endif
#ifdef IOV_MAX
    INVARIANT(_SC_IOV_MAX, IOV_MAX);
#else
    MINIMUM(_SC_IOV_MAX, XOPEN_IOV_MAX);
#endif
#ifdef LOGIN_NAME_MAX
    INVARIANT(_SC_LOGIN_NAME_MAX, LOGIN_NAME_MAX);
#else
    MINIMUM(_SC_LOGIN_NAME_MAX, _POSIX_LOGIN_NAME_MAX);
#endif
#if defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING >= 0
#ifdef MQ_OPEN_MAX
    INVARIANT(_SC_MQ_OPEN_MAX, MQ_OPEN_MAX);
#else
    MINIMUM(_SC_MQ_OPEN_MAX, _POSIX_MQ_OPEN_MAX);
#endif
#ifdef MQ_PRIO_MAX
    INVARIANT(_SC_MQ_PRIO_MAX, MQ_PRIO_MAX);
#else
    MINIMUM(_SC_MQ_PRIO_MAX, _POSIX_MQ_PRIO_MAX);
#endif
#endif
#ifdef PAGESIZE
    INVARIANT(_SC_PAGESIZE, PAGESIZE);
#endif
#ifdef PAGE_SIZE
    INVARIANT(_SC_PAGE_SIZE, PAGE_SIZE);
#endif
    v = sysconf(_SC_PAGESIZE);
    check(v >= 1 && (v & (v - 1)) == 0, "_SC_PAGESIZE is a power of two");
    check(sysconf(_SC_PAGE_SIZE) == v, "_SC_PAGE_SIZE is _SC_PAGESIZE");
#if defined(_POSIX_THREADS) && _POSIX_THREADS > 0
#ifdef PTHREAD_DESTRUCTOR_ITERATIONS
    INVARIANT(_SC_THREAD_DESTRUCTOR_ITERATIONS, PTHREAD_DESTRUCTOR_ITERATIONS);
#else
    MINIMUM(_SC_THREAD_DESTRUCTOR_ITERATIONS, _POSIX_THREAD_DESTRUCTOR_ITERATIONS);
#endif
#ifdef PTHREAD_KEYS_MAX
    INVARIANT(_SC_THREAD_KEYS_MAX, PTHREAD_KEYS_MAX);
#else
    MINIMUM(_SC_THREAD_KEYS_MAX, _POSIX_THREAD_KEYS_MAX);
#endif
#ifdef PTHREAD_STACK_MIN
    INVARIANT(_SC_THREAD_STACK_MIN, PTHREAD_STACK_MIN);
#else
    MINIMUM(_SC_THREAD_STACK_MIN, 0);
#endif
#ifdef PTHREAD_THREADS_MAX
    INVARIANT(_SC_THREAD_THREADS_MAX, PTHREAD_THREADS_MAX);
#else
    MINIMUM(_SC_THREAD_THREADS_MAX, _POSIX_THREAD_THREADS_MAX);
#endif
#endif
#ifdef RTSIG_MAX
    INVARIANT(_SC_RTSIG_MAX, RTSIG_MAX);
#else
    MINIMUM(_SC_RTSIG_MAX, _POSIX_RTSIG_MAX);
#endif
#if defined(_POSIX_SEMAPHORES) && _POSIX_SEMAPHORES >= 0
#ifdef SEM_NSEMS_MAX
    INVARIANT(_SC_SEM_NSEMS_MAX, SEM_NSEMS_MAX);
#else
    MINIMUM(_SC_SEM_NSEMS_MAX, _POSIX_SEM_NSEMS_MAX);
#endif
#ifdef SEM_VALUE_MAX
    INVARIANT(_SC_SEM_VALUE_MAX, SEM_VALUE_MAX);
#else
    MINIMUM(_SC_SEM_VALUE_MAX, _POSIX_SEM_VALUE_MAX);
#endif
#endif
#ifdef SIGQUEUE_MAX
    INVARIANT(_SC_SIGQUEUE_MAX, SIGQUEUE_MAX);
#else
    MINIMUM(_SC_SIGQUEUE_MAX, _POSIX_SIGQUEUE_MAX);
#endif
#ifdef STREAM_MAX
    INVARIANT(_SC_STREAM_MAX, STREAM_MAX);
    _Static_assert(STREAM_MAX == FOPEN_MAX, "STREAM_MAX is FOPEN_MAX");
#else
    MINIMUM(_SC_STREAM_MAX, _POSIX_STREAM_MAX);
#endif
    /* ISO C promises FOPEN_MAX streams */
    INCREASABLE(_SC_STREAM_MAX, FOPEN_MAX);
#ifdef SYMLOOP_MAX
    INVARIANT(_SC_SYMLOOP_MAX, SYMLOOP_MAX);
#else
    MINIMUM(_SC_SYMLOOP_MAX, _POSIX_SYMLOOP_MAX);
#endif
#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
#ifdef TIMER_MAX
    INVARIANT(_SC_TIMER_MAX, TIMER_MAX);
#else
    MINIMUM(_SC_TIMER_MAX, _POSIX_TIMER_MAX);
#endif
#endif
#ifdef TTY_NAME_MAX
    INVARIANT(_SC_TTY_NAME_MAX, TTY_NAME_MAX);
#else
    MINIMUM(_SC_TTY_NAME_MAX, _POSIX_TTY_NAME_MAX);
#endif
#ifdef TZNAME_MAX
    INVARIANT(_SC_TZNAME_MAX, TZNAME_MAX);
#else
    MINIMUM(_SC_TZNAME_MAX, _POSIX_TZNAME_MAX);
#endif

    /*
     * ARG_MAX, CHILD_MAX and OPEN_MAX follow setrlimit on Linux, so
     * picolibc's fixed <limits.h> values can't match them; sysconf is
     * at least never more restrictive here
     */
#ifdef ARG_MAX
    INCREASABLE(_SC_ARG_MAX, ARG_MAX);
#endif
    MINIMUM(_SC_ARG_MAX, _POSIX_ARG_MAX);
#ifdef CHILD_MAX
    INCREASABLE(_SC_CHILD_MAX, CHILD_MAX);
#endif
    MINIMUM(_SC_CHILD_MAX, _POSIX_CHILD_MAX);
#ifdef OPEN_MAX
    INCREASABLE(_SC_OPEN_MAX, OPEN_MAX);
#endif
    MINIMUM(_SC_OPEN_MAX, _POSIX_OPEN_MAX);

    /* Runtime increasable values */
    INCREASABLE(_SC_BC_BASE_MAX, BC_BASE_MAX);
    INCREASABLE(_SC_BC_DIM_MAX, BC_DIM_MAX);
    INCREASABLE(_SC_BC_SCALE_MAX, BC_SCALE_MAX);
    INCREASABLE(_SC_BC_STRING_MAX, BC_STRING_MAX);
    INCREASABLE(_SC_COLL_WEIGHTS_MAX, COLL_WEIGHTS_MAX);
    INCREASABLE(_SC_EXPR_NEST_MAX, EXPR_NEST_MAX);
    INCREASABLE(_SC_LINE_MAX, LINE_MAX);
    INCREASABLE(_SC_NGROUPS_MAX, NGROUPS_MAX);
    INCREASABLE(_SC_RE_DUP_MAX, RE_DUP_MAX);

    /* Clock ticks, and the new names */
    check(sysconf(_SC_CLK_TCK) > 0, "_SC_CLK_TCK");
    {
        long conf = sysconf(_SC_NPROCESSORS_CONF), onln = sysconf(_SC_NPROCESSORS_ONLN);
        long sys_onln = cpu_list("/sys/devices/system/cpu/online");

        check(onln >= 1 && conf >= onln, "1 <= _SC_NPROCESSORS_ONLN <= _SC_NPROCESSORS_CONF");
        /* Only where the system provides files, so sysconf can read /sys */
#ifdef __PICOLIBC_POSIX_FILE_SYSTEM
        if (sys_onln > 0)
            check(onln == sys_onln, "_SC_NPROCESSORS_ONLN counts the online CPUs");
#else
        (void)sys_onln;
#endif
    }
#ifdef _SC_NSIG
    v = sysconf(_SC_NSIG);
    check(v > SIGRTMAX && v <= NSIG_MAX, "SIGRTMAX < _SC_NSIG <= NSIG_MAX");
#endif
    /* SIGRTMIN through SIGRTMAX holds at least RTSIG_MAX signals; glibc's is one short */
#ifndef __GLIBC__
    check(SIGRTMAX - SIGRTMIN + 1 >= sysconf(_SC_RTSIG_MAX), "realtime signals");
#endif

    /* Kernel limits that /proc/self/limits shows on Linux, where there are files */
#ifdef __PICOLIBC_POSIX_FILE_SYSTEM
    v = proc_limit("Max open files");
    if (v != -2)
        check(sysconf(_SC_OPEN_MAX) == v, "_SC_OPEN_MAX is RLIMIT_NOFILE");
    v = proc_limit("Max processes");
    if (v != -2)
        check(sysconf(_SC_CHILD_MAX) == v, "_SC_CHILD_MAX is RLIMIT_NPROC");
    v = proc_limit("Max pending signals");
    if (v != -2)
        check(sysconf(_SC_SIGQUEUE_MAX) == v, "_SC_SIGQUEUE_MAX is RLIMIT_SIGPENDING");
#endif

    /* The supplementary groups fit _SC_NGROUPS_MAX */
#ifdef __PICOLIBC_POSIX_USER_GROUPS
    v = sysconf(_SC_NGROUPS_MAX);
    check(v == -1 || getgroups(0, NULL) <= v, "getgroups within _SC_NGROUPS_MAX");
#endif

    /* Each name takes the reported number of handlers */
    v = sysconf(_SC_ATEXIT_MAX);
    if (v > 0 && v <= 1024) {
        long i;

        for (i = 0; i < v; i++)
            check(atexit(nothing) == 0, "atexit up to _SC_ATEXIT_MAX");
        for (i = 0; i < v; i++)
            check(at_quick_exit(nothing) == 0, "at_quick_exit up to _SC_ATEXIT_MAX");
    }

    /* A time zone name of TZNAME_MAX bytes works, and a longer one doesn't */
    v = sysconf(_SC_TZNAME_MAX);
    if (v > 0 && v < 60) {
        char tz[80], name[64];

        memset(name, 'A', (size_t)v);
        name[v] = '\0';
        snprintf(tz, sizeof(tz), "%s5", name);
        check(setenv("TZ", tz, 1) == 0, "setenv TZ");
        tzset();
        check(strcmp(tzname[0], name) == 0, "a _SC_TZNAME_MAX name");
        /* glibc has no limit, but reports one just the same */
#ifndef __GLIBC__
        memset(name, 'A', (size_t)v + 1);
        name[v + 1] = '\0';
        snprintf(tz, sizeof(tz), "%s5", name);
        check(setenv("TZ", tz, 1) == 0, "setenv TZ");
        tzset();
        check(strcmp(tzname[0], name) != 0, "a name longer than _SC_TZNAME_MAX");
#endif
    }

    /* writev takes _SC_IOV_MAX vectors, and no more */
#if defined(__PICOLIBC_POSIX_PIPE) && defined(__PICOLIBC_POSIX_DEVICE_IO)
    v = sysconf(_SC_IOV_MAX);
    if (v > 0 && v <= 4096) {
        static struct iovec iov[4097];
        static char         bytes[4097];
        int                 fds[2];
        long                i;

        for (i = 0; i <= v; i++) {
            iov[i].iov_base = &bytes[i];
            iov[i].iov_len = 1;
        }
        check(pipe(fds) == 0, "pipe");
        check(writev(fds[1], iov, (int)v) == v, "writev of _SC_IOV_MAX vectors");
        errno = 0;
        check(writev(fds[1], iov, (int)v + 1) == -1 && errno == EINVAL,
              "writev of more than _SC_IOV_MAX vectors");
        check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    }
#endif

    /* SYMLOOP_MAX links are followed, one more is a loop */
#ifdef __PICOLIBC_POSIX_SYMBOLIC_LINKS
    v = sysconf(_SC_SYMLOOP_MAX);
    if (v > 0 && v < 1000) {
        char dir[] = "test-sysconf-limits-XXXXXX";

        check(mkdtemp(dir) != NULL, "mkdtemp");
        check(symlink_chain(dir, v) == 0, "a chain of _SC_SYMLOOP_MAX links");
        check(symlink_chain(dir, v + 1) == ELOOP, "one link more than _SC_SYMLOOP_MAX");
        check(rmdir(dir) == 0, "rmdir");
    }
#endif

    /* No more than _SC_MQ_OPEN_MAX message queue descriptors */
#if defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING >= 0
    if (sysconf(_SC_MESSAGE_PASSING) > 0) {
        char  name[64];
        mqd_t mq[65];
        long  n, i;
        int   err = 0;

        v = sysconf(_SC_MQ_OPEN_MAX);
        n = (v == -1 || v > 63) ? 64 : v + 1;
        snprintf(name, sizeof(name), "/test-sysconf-limits-%ld", (long)getpid());
        for (i = 0; i < n; i++) {
            mq[i] = mq_open(name, O_RDWR | O_CREAT, 0600, NULL);
            if (mq[i] == (mqd_t)-1) {
                err = errno;
                break;
            }
        }
        if (i == 0 && (err == ENOSYS || err == EACCES)) {
            printf("no message queues here (%d), skipped\n", err);
        } else {
            if (v == -1 || v > 63)
                check(i == n, "more message queue descriptors than ever reported");
            else
                check(i == v && err == EMFILE, "_SC_MQ_OPEN_MAX descriptors and no more");
            while (i-- > 0)
                check(mq_close(mq[i]) == 0, "mq_close");
            check(mq_unlink(name) == 0, "mq_unlink");
        }
    }
#endif

    /* Semaphore values up to SEM_VALUE_MAX */
#if defined(_POSIX_SEMAPHORES) && _POSIX_SEMAPHORES >= 0
    v = sysconf(_SC_SEM_VALUE_MAX);
    if (v > 0 && (unsigned long)v < (unsigned long)~0U) {
        sem_t sem;

        check(sem_init(&sem, 0, (unsigned)v) == 0, "sem_init at _SC_SEM_VALUE_MAX");
        check(sem_destroy(&sem) == 0, "sem_destroy");
        errno = 0;
        check(sem_init(&sem, 0, (unsigned)v + 1) == -1 && errno == EINVAL,
              "sem_init past _SC_SEM_VALUE_MAX");
    }
#endif

    printf("success\n");
    return 0;
}
