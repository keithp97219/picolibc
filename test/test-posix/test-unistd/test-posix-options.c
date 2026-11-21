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

/*
 * For each POSIX option and option group the system claims at the
 * POSIX.1-2024 level, take the address of every interface in it, so
 * that a claim the C library and OS can't back fails to link. Claims
 * of earlier versions aren't checked.
 */

#define _POSIX_C_SOURCE 202405L
#define _XOPEN_SOURCE   800
#include <unistd.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glob.h>
#include <grp.h>
#include <libgen.h>
#include <monetary.h>
#include <mqueue.h>
#include <poll.h>
#if _POSIX_THREADS > 0
#include <pthread.h>
#endif
#include <pwd.h>
#include <regex.h>
#include <sched.h>
#if _POSIX_SEMAPHORES > 0
#include <semaphore.h>
#endif
#include <setjmp.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <utime.h>
#include <wordexp.h>

/* Only some systems have these */
#if _POSIX_ASYNCHRONOUS_IO >= 202405L
#include <aio.h>
#endif
#if defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING >= 202405L
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif

/* XSI headers */
#include <fmtmsg.h>
#include <ftw.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <syslog.h>
#if defined(__PICOLIBC_XSI_IPC)
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#endif
#if defined(__PICOLIBC_XSI_USER_GROUPS)
#include <utmpx.h>
#endif

/* POSIX.1-2024 needs close-on-fork; a subprofile of it need not */
#if _POSIX_VERSION >= 202405L && !defined(_POSIX_SUBPROFILE)
#if !defined(O_CLOFORK) || !defined(FD_CLOFORK)
#error "_POSIX_VERSION claims POSIX.1-2024 without close-on-fork"
#endif
#endif

#define FUNC(f) ((void (*)(void))(f))

static void (* const volatile funcs[])(void) = {
#if _POSIX_ASYNCHRONOUS_IO >= 202405L
    FUNC(aio_cancel),
    FUNC(aio_error),
    FUNC(aio_fsync),
    FUNC(aio_read),
    FUNC(aio_return),
    FUNC(aio_suspend),
    FUNC(aio_write),
    FUNC(lio_listio),
#endif
#if _POSIX_BARRIERS >= 202405L
    FUNC(pthread_barrier_destroy),
    FUNC(pthread_barrier_init),
    FUNC(pthread_barrier_wait),
    FUNC(pthread_barrierattr_destroy),
    FUNC(pthread_barrierattr_init),
#endif
#if _POSIX_CLOCK_SELECTION >= 202405L
    FUNC(clock_nanosleep),
    FUNC(pthread_condattr_getclock),
    FUNC(pthread_condattr_setclock),
#endif
#if _POSIX_CPUTIME >= 202405L
    FUNC(clock_getcpuclockid),
#endif
#if _POSIX_FSYNC >= 202405L
    FUNC(fsync),
#endif
#if _POSIX_MAPPED_FILES >= 202405L
    FUNC(mmap),
    FUNC(msync),
    FUNC(munmap),
#endif
#if _POSIX_MEMLOCK >= 202405L
    FUNC(mlockall),
    FUNC(munlockall),
#endif
#if _POSIX_MEMLOCK_RANGE >= 202405L
    FUNC(mlock),
    FUNC(munlock),
#endif
#if _POSIX_MEMORY_PROTECTION >= 202405L
    FUNC(mprotect),
#endif
#if _POSIX_MESSAGE_PASSING >= 202405L
    FUNC(mq_close),
    FUNC(mq_getattr),
    FUNC(mq_notify),
    FUNC(mq_open),
    FUNC(mq_receive),
    FUNC(mq_send),
    FUNC(mq_setattr),
    FUNC(mq_unlink),
#endif
#if _POSIX_PRIORITY_SCHEDULING >= 202405L
    FUNC(sched_get_priority_max),
    FUNC(sched_get_priority_min),
    FUNC(sched_getparam),
    FUNC(sched_getscheduler),
    FUNC(sched_rr_get_interval),
    FUNC(sched_setparam),
    FUNC(sched_setscheduler),
#endif
#if _POSIX_READER_WRITER_LOCKS >= 202405L
    FUNC(pthread_rwlock_destroy),
    FUNC(pthread_rwlock_init),
    FUNC(pthread_rwlock_rdlock),
    FUNC(pthread_rwlock_tryrdlock),
    FUNC(pthread_rwlock_trywrlock),
    FUNC(pthread_rwlock_unlock),
    FUNC(pthread_rwlock_wrlock),
    FUNC(pthread_rwlockattr_destroy),
    FUNC(pthread_rwlockattr_getpshared),
    FUNC(pthread_rwlockattr_init),
    FUNC(pthread_rwlockattr_setpshared),
#endif
#if _POSIX_REALTIME_SIGNALS >= 202405L
    FUNC(sigqueue),
    FUNC(sigtimedwait),
    FUNC(sigwaitinfo),
#endif
#if _POSIX_SEMAPHORES >= 202405L
    FUNC(sem_close),
    FUNC(sem_destroy),
    FUNC(sem_getvalue),
    FUNC(sem_init),
    FUNC(sem_open),
    FUNC(sem_post),
    FUNC(sem_trywait),
    FUNC(sem_unlink),
    FUNC(sem_wait),
#endif
#if _POSIX_SHARED_MEMORY_OBJECTS >= 202405L
    FUNC(mmap),
    FUNC(munmap),
    FUNC(shm_open),
    FUNC(shm_unlink),
#endif
#if _POSIX_SPIN_LOCKS >= 202405L
    FUNC(pthread_spin_destroy),
    FUNC(pthread_spin_init),
    FUNC(pthread_spin_lock),
    FUNC(pthread_spin_trylock),
    FUNC(pthread_spin_unlock),
#endif
#if _POSIX_SYNCHRONIZED_IO >= 202405L
    FUNC(fdatasync),
    FUNC(fsync),
    FUNC(msync),
#endif
#if _POSIX_THREADS >= 202405L
    FUNC(pthread_atfork),
    FUNC(pthread_attr_destroy),
    FUNC(pthread_attr_getdetachstate),
    FUNC(pthread_attr_getschedparam),
    FUNC(pthread_attr_init),
    FUNC(pthread_attr_setdetachstate),
    FUNC(pthread_attr_setschedparam),
    FUNC(pthread_barrier_destroy),
    FUNC(pthread_barrier_init),
    FUNC(pthread_barrier_wait),
    FUNC(pthread_barrierattr_destroy),
    FUNC(pthread_barrierattr_getpshared),
    FUNC(pthread_barrierattr_init),
    FUNC(pthread_barrierattr_setpshared),
    FUNC(pthread_cancel),
    FUNC(pthread_cond_broadcast),
    FUNC(pthread_cond_destroy),
    FUNC(pthread_cond_init),
    FUNC(pthread_cond_signal),
    FUNC(pthread_cond_timedwait),
    FUNC(pthread_cond_wait),
    FUNC(pthread_condattr_destroy),
    FUNC(pthread_condattr_init),
    FUNC(pthread_create),
    FUNC(pthread_detach),
    FUNC(pthread_equal),
    FUNC(pthread_exit),
    FUNC(pthread_getspecific),
    FUNC(pthread_join),
    FUNC(pthread_key_create),
    FUNC(pthread_key_delete),
    FUNC(pthread_kill),
    FUNC(pthread_mutex_destroy),
    FUNC(pthread_mutex_init),
    FUNC(pthread_mutex_lock),
    FUNC(pthread_mutex_trylock),
    FUNC(pthread_mutex_unlock),
    FUNC(pthread_mutexattr_destroy),
    FUNC(pthread_mutexattr_init),
    FUNC(pthread_once),
    FUNC(pthread_self),
    FUNC(pthread_setcancelstate),
    FUNC(pthread_setcanceltype),
    FUNC(pthread_setspecific),
    FUNC(pthread_sigmask),
    FUNC(pthread_testcancel),
    FUNC(sched_yield),
#endif
#if _POSIX_THREAD_ATTR_STACKSIZE >= 202405L
    FUNC(pthread_attr_getstacksize),
    FUNC(pthread_attr_setstacksize),
#endif
#if _POSIX_THREAD_CPUTIME >= 202405L
    FUNC(pthread_getcpuclockid),
#endif
#if _POSIX_THREAD_PRIORITY_SCHEDULING >= 202405L
    FUNC(pthread_attr_getinheritsched),
    FUNC(pthread_attr_getschedpolicy),
    FUNC(pthread_attr_getscope),
    FUNC(pthread_attr_setinheritsched),
    FUNC(pthread_attr_setschedpolicy),
    FUNC(pthread_attr_setscope),
    FUNC(pthread_getschedparam),
    FUNC(pthread_setschedparam),
    FUNC(pthread_setschedprio),
#endif
#if _POSIX_THREAD_PRIO_INHERIT >= 202405L
    FUNC(pthread_mutexattr_getprotocol),
    FUNC(pthread_mutexattr_setprotocol),
#endif
#if _POSIX_THREAD_PRIO_PROTECT >= 202405L
    FUNC(pthread_mutex_getprioceiling),
    FUNC(pthread_mutex_setprioceiling),
    FUNC(pthread_mutexattr_getprioceiling),
    FUNC(pthread_mutexattr_getprotocol),
    FUNC(pthread_mutexattr_setprioceiling),
    FUNC(pthread_mutexattr_setprotocol),
#endif
#if _POSIX_TIMEOUTS >= 202405L
/* These are [MSG TMO]: message passing with timeouts */
#if _POSIX_MESSAGE_PASSING >= 202405L
    FUNC(mq_timedreceive),
    FUNC(mq_timedsend),
#endif
    FUNC(pthread_mutex_timedlock),
    FUNC(pthread_rwlock_timedrdlock),
    FUNC(pthread_rwlock_timedwrlock),
    FUNC(sem_timedwait),
#endif
#if _POSIX_TIMERS >= 202405L
    FUNC(clock_getres),
    FUNC(clock_gettime),
    FUNC(clock_settime),
    FUNC(nanosleep),
    FUNC(timer_create),
    FUNC(timer_delete),
    FUNC(timer_getoverrun),
    FUNC(timer_gettime),
    FUNC(timer_settime),
#endif
#if defined(__PICOLIBC_POSIX_C_LIB_EXT) && __PICOLIBC_POSIX_C_LIB_EXT >= 202405L
    FUNC(fnmatch),
    FUNC(getentropy),
    FUNC(getopt),
    FUNC(getsubopt),
    FUNC(memmem),
    FUNC(reallocarray),
    FUNC(stpcpy),
    FUNC(stpncpy),
    FUNC(strcasecmp),
    FUNC(strdup),
    FUNC(strfmon),
    FUNC(strlcat),
    FUNC(strlcpy),
    FUNC(strncasecmp),
    FUNC(strndup),
    FUNC(strnlen),
#endif
#if defined(__PICOLIBC_POSIX_DEVICE_IO) && __PICOLIBC_POSIX_DEVICE_IO >= 202405L
    FUNC(clearerr),
    FUNC(close),
    FUNC(fclose),
    FUNC(fdopen),
    FUNC(feof),
    FUNC(ferror),
    FUNC(fflush),
    FUNC(fgetc),
    FUNC(fgets),
    FUNC(fileno),
    FUNC(fopen),
    FUNC(fprintf),
    FUNC(fputc),
    FUNC(fputs),
    FUNC(fread),
    FUNC(freopen),
    FUNC(fscanf),
    FUNC(fwrite),
    FUNC(getc),
    FUNC(getchar),
    FUNC(open),
    FUNC(perror),
    FUNC(poll),
    FUNC(posix_close),
    FUNC(ppoll),
    FUNC(pread),
    FUNC(printf),
    FUNC(pselect),
    FUNC(putc),
    FUNC(putchar),
    FUNC(puts),
    FUNC(pwrite),
    FUNC(read),
    FUNC(scanf),
    FUNC(select),
    FUNC(setbuf),
    FUNC(setvbuf),
    FUNC(ungetc),
    FUNC(vfprintf),
    FUNC(vfscanf),
    FUNC(vprintf),
    FUNC(vscanf),
    FUNC(write),
#endif
#if defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC) && __PICOLIBC_POSIX_DEVICE_SPECIFIC >= 202405L
    FUNC(cfgetispeed),
    FUNC(cfgetospeed),
    FUNC(cfsetispeed),
    FUNC(cfsetospeed),
    FUNC(ctermid),
    FUNC(isatty),
    FUNC(tcdrain),
    FUNC(tcflow),
    FUNC(tcflush),
    FUNC(tcgetattr),
    FUNC(tcgetwinsize),
    FUNC(tcsendbreak),
    FUNC(tcsetattr),
    FUNC(tcsetwinsize),
    FUNC(ttyname),
#endif
#if defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC_R) && __PICOLIBC_POSIX_DEVICE_SPECIFIC_R >= 202405L
    FUNC(ttyname_r),
#endif
#if defined(__PICOLIBC_POSIX_DYNAMIC_LINKING) && __PICOLIBC_POSIX_DYNAMIC_LINKING >= 202405L
    FUNC(dladdr),
    FUNC(dlclose),
    FUNC(dlerror),
    FUNC(dlopen),
    FUNC(dlsym),
#endif
#if defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT >= 202405L
    FUNC(dup),
    FUNC(dup2),
    FUNC(dup3),
    FUNC(fcntl),
    FUNC(fgetpos),
    FUNC(fseek),
    FUNC(fseeko),
    FUNC(fsetpos),
    FUNC(ftell),
    FUNC(ftello),
    FUNC(ftruncate),
    FUNC(lseek),
    FUNC(rewind),
#endif
#if defined(__PICOLIBC_POSIX_FIFO) && __PICOLIBC_POSIX_FIFO >= 202405L
    FUNC(mkfifo),
#endif
#if defined(__PICOLIBC_POSIX_FIFO_FD) && __PICOLIBC_POSIX_FIFO_FD >= 202405L
    FUNC(mkfifoat),
    FUNC(mknodat),
#endif
#if defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES) && __PICOLIBC_POSIX_FILE_ATTRIBUTES >= 202405L
    FUNC(chmod),
    FUNC(chown),
    FUNC(fchmod),
    FUNC(fchown),
    FUNC(umask),
#endif
#if defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES_FD) && __PICOLIBC_POSIX_FILE_ATTRIBUTES_FD >= 202405L
    FUNC(fchmodat),
    FUNC(fchownat),
#endif
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM >= 202405L
    FUNC(access),
    FUNC(chdir),
    FUNC(closedir),
    FUNC(creat),
    FUNC(fchdir),
    FUNC(fpathconf),
    FUNC(fstat),
    FUNC(fstatvfs),
    FUNC(futimens),
    FUNC(getcwd),
    FUNC(link),
    FUNC(mkdir),
    FUNC(mkostemp),
    FUNC(mkstemp),
    FUNC(opendir),
    FUNC(pathconf),
    FUNC(posix_getdents),
    FUNC(readdir),
    FUNC(remove),
    FUNC(rename),
    FUNC(rewinddir),
    FUNC(rmdir),
    FUNC(stat),
    FUNC(statvfs),
    FUNC(tmpfile),
    FUNC(tmpnam),
    FUNC(truncate),
    FUNC(unlink),
#endif
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT >= 202405L
    FUNC(alphasort),
    FUNC(dirfd),
    FUNC(getdelim),
    FUNC(getline),
    FUNC(mkdtemp),
    FUNC(scandir),
#endif
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && __PICOLIBC_POSIX_FILE_SYSTEM_FD >= 202405L
    FUNC(faccessat),
    FUNC(fdopendir),
    FUNC(fstatat),
    FUNC(linkat),
    FUNC(mkdirat),
    FUNC(openat),
    FUNC(renameat),
    FUNC(unlinkat),
    FUNC(utimensat),
#endif
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS >= 202405L
    FUNC(_Exit),
    FUNC(_Fork),
    FUNC(_exit),
    FUNC(at_quick_exit),
    FUNC(atexit),
    FUNC(clock),
    FUNC(execl),
    FUNC(execle),
    FUNC(execlp),
    FUNC(execv),
    FUNC(execve),
    FUNC(execvp),
    FUNC(exit),
    FUNC(fork),
    FUNC(getpgid),
    FUNC(getpgrp),
    FUNC(getpid),
    FUNC(getppid),
    FUNC(getrlimit),
    FUNC(getsid),
    FUNC(quick_exit),
    FUNC(setrlimit),
    FUNC(setsid),
    FUNC(sleep),
    FUNC(times),
    FUNC(wait),
    FUNC(waitid),
    FUNC(waitpid),
#endif
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS_FD) && __PICOLIBC_POSIX_MULTI_PROCESS_FD >= 202405L
    FUNC(fexecve),
#endif
#if defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING >= 202405L
    FUNC(accept),
    FUNC(accept4),
    FUNC(bind),
    FUNC(connect),
    FUNC(endhostent),
    FUNC(endnetent),
    FUNC(endprotoent),
    FUNC(endservent),
    FUNC(freeaddrinfo),
    FUNC(gai_strerror),
    FUNC(getaddrinfo),
    FUNC(gethostent),
    FUNC(gethostname),
    FUNC(getnameinfo),
    FUNC(getnetbyaddr),
    FUNC(getnetbyname),
    FUNC(getnetent),
    FUNC(getpeername),
    FUNC(getprotobyname),
    FUNC(getprotobynumber),
    FUNC(getprotoent),
    FUNC(getservbyname),
    FUNC(getservbyport),
    FUNC(getservent),
    FUNC(getsockname),
    FUNC(getsockopt),
    FUNC(if_freenameindex),
    FUNC(if_indextoname),
    FUNC(if_nameindex),
    FUNC(if_nametoindex),
    FUNC(inet_addr),
    FUNC(inet_ntoa),
    FUNC(inet_ntop),
    FUNC(inet_pton),
    FUNC(listen),
    FUNC(recv),
    FUNC(recvfrom),
    FUNC(recvmsg),
    FUNC(send),
    FUNC(sendmsg),
    FUNC(sendto),
    FUNC(sethostent),
    FUNC(setnetent),
    FUNC(setprotoent),
    FUNC(setservent),
    FUNC(setsockopt),
    FUNC(shutdown),
    FUNC(sockatmark),
    FUNC(socket),
    FUNC(socketpair),
#endif
#if defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE >= 202405L
    FUNC(pipe),
    FUNC(pipe2),
#endif
#if defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS >= 202405L
    FUNC(abort),
    FUNC(alarm),
    FUNC(kill),
    FUNC(pause),
    FUNC(raise),
    FUNC(sigaction),
    FUNC(sigaddset),
    FUNC(sigdelset),
    FUNC(sigemptyset),
    FUNC(sigfillset),
    FUNC(sigismember),
    FUNC(signal),
    FUNC(sigpending),
    FUNC(sigprocmask),
    FUNC(sigsuspend),
    FUNC(sigwait),
#endif
#if defined(__PICOLIBC_POSIX_SINGLE_PROCESS) && __PICOLIBC_POSIX_SINGLE_PROCESS >= 202405L
    FUNC(confstr),
    FUNC(getenv),
    FUNC(secure_getenv),
    FUNC(setenv),
    FUNC(sysconf),
    FUNC(uname),
    FUNC(unsetenv),
#endif
#if defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS) && __PICOLIBC_POSIX_SYMBOLIC_LINKS >= 202405L
    FUNC(lchown),
    FUNC(lstat),
    FUNC(readlink),
    FUNC(realpath),
    FUNC(symlink),
#endif
#if defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS_FD) && __PICOLIBC_POSIX_SYMBOLIC_LINKS_FD >= 202405L
    FUNC(readlinkat),
    FUNC(symlinkat),
#endif
#if defined(__PICOLIBC_POSIX_USER_GROUPS) && __PICOLIBC_POSIX_USER_GROUPS >= 202405L
    FUNC(getegid),
    FUNC(geteuid),
    FUNC(getgid),
    FUNC(getgroups),
    FUNC(getlogin),
    FUNC(getuid),
    FUNC(setegid),
    FUNC(seteuid),
    FUNC(setgid),
    FUNC(setuid),
#endif
#if defined(__PICOLIBC_XSI_DEVICE_IO) && __PICOLIBC_XSI_DEVICE_IO >= 202405L
    FUNC(fmtmsg),
    FUNC(readv),
    FUNC(writev),
#endif
#if defined(__PICOLIBC_XSI_DEVICE_SPECIFIC) && __PICOLIBC_XSI_DEVICE_SPECIFIC >= 202405L
    FUNC(grantpt),
    FUNC(posix_openpt),
    FUNC(ptsname),
    FUNC(unlockpt),
#endif
#if defined(__PICOLIBC_XSI_GENERAL_TERMINAL_R) && __PICOLIBC_XSI_GENERAL_TERMINAL_R >= 202405L
    FUNC(ptsname_r),
#endif
#if defined(__PICOLIBC_XSI_FILE_SYSTEM) && __PICOLIBC_XSI_FILE_SYSTEM >= 202405L
    FUNC(dirname),
    FUNC(lockf),
    FUNC(mknod),
    FUNC(nftw),
    FUNC(seekdir),
    FUNC(sync),
    FUNC(telldir),
    FUNC(utimes),
#endif
#if defined(__PICOLIBC_XSI_IPC) && __PICOLIBC_XSI_IPC >= 202405L
    FUNC(ftok),
    FUNC(msgctl),
    FUNC(msgget),
    FUNC(msgrcv),
    FUNC(msgsnd),
    FUNC(semctl),
    FUNC(semget),
    FUNC(semop),
    FUNC(shmat),
    FUNC(shmctl),
    FUNC(shmdt),
    FUNC(shmget),
#endif
#if defined(__PICOLIBC_XSI_MULTI_PROCESS) && __PICOLIBC_XSI_MULTI_PROCESS >= 202405L
    FUNC(getpriority),
    FUNC(getrusage),
    FUNC(nice),
    FUNC(setpriority),
#endif
#if defined(__PICOLIBC_XSI_SIGNALS) && __PICOLIBC_XSI_SIGNALS >= 202405L
    FUNC(killpg),
    FUNC(sigaltstack),
#endif
#if defined(__PICOLIBC_XSI_SINGLE_PROCESS) && __PICOLIBC_XSI_SINGLE_PROCESS >= 202405L
    FUNC(gethostid),
    FUNC(putenv),
#endif
#if defined(__PICOLIBC_XSI_SYSTEM_LOGGING) && __PICOLIBC_XSI_SYSTEM_LOGGING >= 202405L
    FUNC(closelog),
    FUNC(openlog),
    FUNC(setlogmask),
    FUNC(syslog),
#endif
#if defined(__PICOLIBC_XSI_USER_GROUPS) && __PICOLIBC_XSI_USER_GROUPS >= 202405L
    FUNC(endutxent),
    FUNC(getresgid),
    FUNC(getresuid),
    FUNC(getutxent),
    FUNC(getutxid),
    FUNC(getutxline),
    FUNC(pututxline),
    FUNC(setregid),
    FUNC(setresgid),
    FUNC(setresuid),
    FUNC(setreuid),
    FUNC(setutxent),
#endif
    NULL,
};

static const clockid_t clocks[] = {
#if _POSIX_CPUTIME >= 202405L
    CLOCK_PROCESS_CPUTIME_ID,
#endif
#if _POSIX_MONOTONIC_CLOCK >= 202405L
    CLOCK_MONOTONIC,
#endif
#if _POSIX_THREAD_CPUTIME >= 202405L
    CLOCK_THREAD_CPUTIME_ID,
#endif
    CLOCK_REALTIME, /* not checked; keeps the array non-empty */
};

int
main(void)
{
    size_t nfunc = sizeof(funcs) / sizeof(funcs[0]) - 1;
    size_t nclock = sizeof(clocks) / sizeof(clocks[0]) - 1;
    size_t i;

    for (i = 0; i < nfunc; i++) {
        if (funcs[i] == NULL) {
            printf("interface %zu missing\n", i);
            exit(1);
        }
    }
    for (i = 0; i < nclock; i++) {
        struct timespec res;
        if (clock_getres(clocks[i], &res) != 0) {
            printf("clock %d unsupported\n", (int)clocks[i]);
            exit(1);
        }
    }
    printf("%zu interfaces and %zu clocks for the claimed options\n", nfunc, nclock);
    exit(0);
}
