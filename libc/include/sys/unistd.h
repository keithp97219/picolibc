/*
Copyright (c) 1982, 1986, 1993
The Regents of the University of California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the University nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
 */
#ifndef _SYS_UNISTD_H
#define _SYS_UNISTD_H

#include <sys/cdefs.h>
#define __need_size_t
#define __need_ptrdiff_t
#define __need_NULL
#include <stddef.h>
#include <sys/_types.h>

_BEGIN_STD_C

#ifndef _USECONDS_T_DECLARED
typedef __useconds_t useconds_t; /* microseconds (unsigned) */
#define _USECONDS_T_DECLARED
#endif

#ifndef _INTPTR_T_DECLARED
typedef __intptr_t intptr_t;
#define _INTPTR_T_DECLARED
#endif

#ifndef _SSIZE_T_DECLARED
typedef __ssize_t ssize_t;
#define _SSIZE_T_DECLARED
#endif

#ifndef _OFF_T_DECLARED
typedef __off_t off_t;
#define _OFF_T_DECLARED
#endif

#ifndef _UID_T_DECLARED
typedef __uid_t uid_t; /* user id */
#define _UID_T_DECLARED
#endif

#ifndef _GID_T_DECLARED
typedef __gid_t gid_t; /* group id */
#define _GID_T_DECLARED
#endif

#ifndef _PID_T_DECLARED
typedef __pid_t pid_t; /* process id */
#define _PID_T_DECLARED
#endif

__noreturn void _exit(int __status) __picolibc_export;

int             access(const char *__path, int __amode) __picolibc_export;
unsigned        alarm(unsigned __secs) __picolibc_export;
#if __BSD_VISIBLE || (__XSI_VISIBLE >= 4 && __POSIX_VISIBLE < 200112)
int brk(void *__addr) __picolibc_export;
#endif
int chdir(const char *__path) __picolibc_export;
int chown(const char *__path, uid_t __owner, gid_t __group) __picolibc_export;
#if __BSD_VISIBLE || (__XSI_VISIBLE >= 4 && __POSIX_VISIBLE < 200112)
int chroot(const char *__path) __picolibc_export;
#endif
int close(int __fildes) __picolibc_export;
#if __POSIX_VISIBLE >= 202405
int posix_close(int __fildes, int __flag) __picolibc_export;
#endif
#if __POSIX_VISIBLE >= 199209
size_t confstr(int __name, char *__buf, size_t __len) __picolibc_export;
#endif
#if __XSI_VISIBLE
char *crypt(const char *__key, const char *__salt) __picolibc_export;
#endif
#if __XSI_VISIBLE && __XSI_VISIBLE < 700
char *ctermid(char *__s) __picolibc_export;
#endif
#if __XSI_VISIBLE && __XSI_VISIBLE < 600
char *cuserid(char *__s) __picolibc_export;
#endif
#if __BSD_VISIBLE || (__XSI_VISIBLE && __XSI_VISIBLE < 500)
int daemon(int nochdir, int noclose) __picolibc_export;
#endif
int dup(int __fildes) __picolibc_export;
int dup2(int __fildes, int __fildes2) __picolibc_export;
#if __GNU_VISIBLE || __POSIX_VISIBLE >= 202405
int dup3(int __fildes, int __fildes2, int flags) __picolibc_export;
#endif
#if __GNU_VISIBLE
int eaccess(const char *__path, int __mode) __picolibc_export;
#endif
#if __XSI_VISIBLE
void encrypt(char *, int) __picolibc_export;
#endif
#if __BSD_VISIBLE || (__XSI_VISIBLE && __XSI_VISIBLE < 500)
void endusershell(void) __picolibc_export;
#endif
#if __GNU_VISIBLE
int euidaccess(const char *__path, int __mode) __picolibc_export;
#endif
int execl(const char *__path, const char *, ...) __picolibc_export;
int execle(const char *__path, const char *, ...) __picolibc_export;
int execlp(const char *__file, const char *, ...) __picolibc_export;
#if __MISC_VISIBLE
int execlpe(const char *__file, const char *, ...) __picolibc_export;
#endif
int execv(const char *__path, char * const __argv[]) __picolibc_export;
int execve(const char *__path, char * const __argv[], char * const __envp[]) __picolibc_export;
int execvp(const char *__file, char * const __argv[]) __picolibc_export;
#if __GNU_VISIBLE
int execvpe(const char *__file, char * const __argv[], char * const __envp[]) __picolibc_export;
#endif
#if __ATFILE_VISIBLE
int faccessat(int __dirfd, const char *__path, int __mode, int __flags) __picolibc_export;
#endif
#if __BSD_VISIBLE || __XSI_VISIBLE >= 4 || __POSIX_VISIBLE >= 200809
int fchdir(int __fildes) __picolibc_export;
#endif
#if __BSD_VISIBLE || __XSI_VISIBLE >= 4 || __POSIX_VISIBLE >= 200809
int fchown(int __fildes, uid_t __owner, gid_t __group) __picolibc_export;
#endif
#if __ATFILE_VISIBLE
int fchownat(int __dirfd, const char *__path, uid_t __owner, gid_t __group, int __flags)
    __picolibc_export;
#endif
#if __POSIX_VISIBLE >= 200809
int fexecve(int __fd, char * const __argv[], char * const __envp[]) __picolibc_export;
#endif
pid_t fork(void) __picolibc_export;
#if __POSIX_VISIBLE >= 202405
pid_t _Fork(void) __picolibc_export;
#endif
long fpathconf(int __fd, int __name) __picolibc_export;
long __fallback_fpathconf(int __td, int __name) __picolibc_export;
int  fsync(int __fd) __picolibc_export;
#if __POSIX_VISIBLE >= 199309
int fdatasync(int __fd) __picolibc_export;
#endif
#if __GNU_VISIBLE
char *get_current_dir_name(void) __picolibc_export;
#endif
char *getcwd(char *__buf, size_t __size) __picolibc_export;
#if __BSD_VISIBLE || (__XSI_VISIBLE && __XSI_VISIBLE < 500)
int getdomainname(char *__name, size_t __len) __picolibc_export;
#endif
#if __BSD_VISIBLE || __POSIX_VISIBLE >= 202405
int getentropy(void *, size_t) __picolibc_export;
#endif
gid_t getegid(void) __picolibc_export;
uid_t geteuid(void) __picolibc_export;
gid_t getgid(void) __picolibc_export;
int   getgroups(int __gidsetsize, gid_t __grouplist[]) __picolibc_export;
#if __BSD_VISIBLE || __XSI_VISIBLE >= 4
long gethostid(void) __picolibc_export;
#endif
char *getlogin(void) __picolibc_export;
#if __POSIX_VISIBLE >= 199506
int getlogin_r(char *__name, size_t __namesize) __picolibc_export;
#endif
#if __BSD_VISIBLE || (__XSI_VISIBLE && __POSIX_VISIBLE < 200112)
char *getpass(const char *__prompt) __picolibc_export;
int   getpagesize(void) __picolibc_export;
#endif
#if __BSD_VISIBLE
int getpeereid(int, uid_t *, gid_t *) __picolibc_export;
#endif
#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE >= 4
pid_t getpgid(pid_t) __picolibc_export;
#endif
pid_t getpgrp(void) __picolibc_export;
pid_t getpid(void) __picolibc_export;
pid_t getppid(void) __picolibc_export;
#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE >= 4
pid_t getsid(pid_t) __picolibc_export;
#endif
uid_t getuid(void) __picolibc_export;
#if __BSD_VISIBLE || (__XSI_VISIBLE && __XSI_VISIBLE < 500)
char *getusershell(void) __picolibc_export;
#endif
#if __BSD_VISIBLE || (__XSI_VISIBLE >= 4 && __POSIX_VISIBLE < 200809)
char *getwd(char *__buf) __picolibc_export;
#endif
#if __BSD_VISIBLE
int iruserok(unsigned long raddr, int superuser, const char *ruser, const char *luser)
    __picolibc_export;
#endif
int isatty(int __fildes) __picolibc_export;
#if __BSD_VISIBLE
int issetugid(void) __picolibc_export;
#endif
#if __BSD_VISIBLE || __XSI_VISIBLE >= 4 || __POSIX_VISIBLE >= 200809
int lchown(const char *__path, uid_t __owner, gid_t __group) __picolibc_export;
#endif
int link(const char *__path1, const char *__path2) __picolibc_export;
#if __ATFILE_VISIBLE
int linkat(int __dirfd1, const char *__path1, int __dirfd2, const char *__path2, int __flags)
    __picolibc_export;
#endif
#if __MISC_VISIBLE || __XSI_VISIBLE
int nice(int __nice_value) __picolibc_export;
#endif
off_t lseek(int __fildes, off_t __offset, int __whence) __picolibc_export;
#if __MISC_VISIBLE || __XSI_VISIBLE >= 4
#define F_ULOCK 0
#define F_LOCK  1
#define F_TLOCK 2
#define F_TEST  3
int lockf(int __fd, int __cmd, off_t __len) __picolibc_export;
#endif
long pathconf(const char *__path, int __name) __picolibc_export;
long __fallback_pathconf(const char *__path, int __name) __picolibc_export;
int  pause(void) __picolibc_export;
#if __POSIX_VISIBLE >= 199506
int pthread_atfork(void (*)(void), void (*)(void), void (*)(void)) __picolibc_export;
#endif
int pipe(int __fildes[2]) __picolibc_export;
#if __GNU_VISIBLE || __POSIX_VISIBLE >= 202405
int pipe2(int __fildes[2], int flags) __picolibc_export;
#endif
#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE >= 500
ssize_t pread(int __fd, void *__buf, size_t __nbytes, off_t __offset) __picolibc_export;
ssize_t pwrite(int __fd, const void *__buf, size_t __nbytes, off_t __offset) __picolibc_export;
#endif
ssize_t read(int __fd, void *__buf, size_t __nbyte) __picolibc_export;
#if __BSD_VISIBLE
int rresvport(int *__alport) __picolibc_export;
int revoke(char *__path) __picolibc_export;
#endif
int rmdir(const char *__path) __picolibc_export;
#if __BSD_VISIBLE
int ruserok(const char *rhost, int superuser, const char *ruser, const char *luser)
    __picolibc_export;
#endif
#if __BSD_VISIBLE || (__XSI_VISIBLE >= 4 && __POSIX_VISIBLE < 200112)
void *sbrk(ptrdiff_t __incr) __picolibc_export;
void *__fallback_sbrk(ptrdiff_t __incr) __picolibc_export;
#endif
#if __BSD_VISIBLE || __POSIX_VISIBLE >= 200112
int setegid(gid_t __gid) __picolibc_export;
int seteuid(uid_t __uid) __picolibc_export;
#endif
int setgid(gid_t __gid) __picolibc_export;
#if __BSD_VISIBLE
int setgroups(int ngroups, const gid_t *grouplist) __picolibc_export;
#endif
#if __BSD_VISIBLE || (__XSI_VISIBLE && __XSI_VISIBLE < 500)
int sethostname(const char *, size_t) __picolibc_export;
#endif
int setpgid(pid_t __pid, pid_t __pgid) __picolibc_export;
/* setpgrp was removed in Issue 8 */
#if __SVID_VISIBLE || (__XSI_VISIBLE >= 500 && __XSI_VISIBLE < 800)
int setpgrp(void) __picolibc_export;
#endif
#if __BSD_VISIBLE || __XSI_VISIBLE >= 4
int setregid(gid_t __rgid, gid_t __egid) __picolibc_export;
int setreuid(uid_t __ruid, uid_t __euid) __picolibc_export;
#endif
#if __GNU_VISIBLE || __XSI_VISIBLE >= 800
int getresgid(gid_t * __restrict __rgid, gid_t * __restrict __egid, gid_t * __restrict __sgid)
    __picolibc_export;
int getresuid(uid_t * __restrict __ruid, uid_t * __restrict __euid, uid_t * __restrict __suid)
    __picolibc_export;
int setresgid(gid_t __rgid, gid_t __egid, gid_t __sgid) __picolibc_export;
int setresuid(uid_t __ruid, uid_t __euid, uid_t __suid) __picolibc_export;
#endif
pid_t setsid(void) __picolibc_export;
int   setuid(uid_t __uid) __picolibc_export;
#if __BSD_VISIBLE || (__XSI_VISIBLE && __XSI_VISIBLE < 500)
void setusershell(void) __picolibc_export;
#endif
unsigned sleep(unsigned int __seconds) __picolibc_export;
#if __XSI_VISIBLE
void swab(const void * __restrict, void * __restrict, ssize_t) __picolibc_export;
#endif
long  sysconf(int __name) __picolibc_export;
long  __fallback_sysconf(int __name) __picolibc_export;
pid_t tcgetpgrp(int __fildes) __picolibc_export;
int   tcsetpgrp(int __fildes, pid_t __pgrp_id) __picolibc_export;
char *ttyname(int __fildes) __picolibc_export;
int   ttyname_r(int, char *, size_t) __picolibc_export;
int   unlink(const char *__path) __picolibc_export;
#if __XSI_VISIBLE >= 500 && __POSIX_VISIBLE < 200809 || __BSD_VISIBLE
int usleep(useconds_t __useconds) __picolibc_export;
#endif
#if __BSD_VISIBLE
int vhangup(void) __picolibc_export;
#endif
ssize_t write(int __fd, const void *__buf, size_t __nbyte) __picolibc_export;

#if __GNU_VISIBLE || __POSIX_VISIBLE >= 202405
extern __picolibc_export char **environ;
#endif

extern __picolibc_export char *optarg; /* getopt(3) external variables */
extern __picolibc_export int   optind;
extern __picolibc_export int   opterr;
extern __picolibc_export int   optopt;
int                            getopt(int, char                            *const[], const char *) __picolibc_export;
extern __picolibc_export int   optreset; /* getopt(3) external variable */

#if __BSD_VISIBLE || (__XSI_VISIBLE >= 4 && __POSIX_VISIBLE < 200809)
pid_t vfork(void) __picolibc_export;
#endif

#if __POSIX_VISIBLE >= 200112 || __XSI_VISIBLE >= 500
int ftruncate(int __fd, off_t __length) __picolibc_export;
#endif
#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE >= 500
int truncate(const char *, off_t __length) __picolibc_export;
#endif

#if __BSD_VISIBLE || __POSIX_VISIBLE < 200112
int getdtablesize(void) __picolibc_export;
#endif
/* ualarm was removed in Issue 7 */
#if __BSD_VISIBLE || (__XSI_VISIBLE >= 500 && __POSIX_VISIBLE < 200809)
useconds_t ualarm(useconds_t __useconds, useconds_t __interval) __picolibc_export;
#endif

#if __BSD_VISIBLE || __POSIX_VISIBLE >= 200112 || __XSI_VISIBLE >= 500
#if !(defined(_WINSOCK_H) || defined(_WINSOCKAPI_) || defined(__USE_W32_SOCKETS))
/* winsock[2].h defines as __stdcall, and with int as 2nd arg */
int gethostname(char *__name, size_t __len) __picolibc_export;
#endif
#endif

#if __MISC_VISIBLE
int setdtablesize(int) __picolibc_export;
#endif

#if __BSD_VISIBLE || __XSI_VISIBLE >= 500
void sync(void) __picolibc_export;
#endif

#if __BSD_VISIBLE || __POSIX_VISIBLE >= 200112 || __XSI_VISIBLE >= 4
ssize_t readlink(const char * __restrict __path, char * __restrict __buf, size_t __buflen)
    __picolibc_export;
int symlink(const char *__name1, const char *__name2) __picolibc_export;
#endif
#if __ATFILE_VISIBLE
ssize_t readlinkat(int    __dirfd1, const char    *__restrict __path, char    *__restrict __buf,
                   size_t __buflen) __picolibc_export;
int     symlinkat(const char *, int, const char *) __picolibc_export;
int     unlinkat(int, const char *, int) __picolibc_export;
#endif

#define F_OK     0
#define R_OK     4
#define W_OK     2
#define X_OK     1

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#if __GNU_VISIBLE || __POSIX_VISIBLE >= 202405
#define SEEK_DATA 3
#define SEEK_HOLE 4
#endif

#include <sys/features.h>

#define STDIN_FILENO  0 /* standard input file descriptor */
#define STDOUT_FILENO 1 /* standard output file descriptor */
#define STDERR_FILENO 2 /* standard error file descriptor */

#ifndef _POSIX2_RE_DUP_MAX
/* The maximum number of repeated occurrences of a regular expression
 *    permitted when using the interval notation `\{M,N\}'.  */
#define _POSIX2_RE_DUP_MAX 255
#endif /* _POSIX2_RE_DUP_MAX  */

/*
 * Flag values for posix_close per IEEE Std 1003.1, 2024 Edition
 */
#if __POSIX_VISIBLE >= 202405
#define POSIX_CLOSE_RESTART 1
#endif

/*
 *  sysconf values per IEEE Std 1003.1, 2008 Edition
 */

#define _SC_ARG_MAX                    0
#define _SC_CHILD_MAX                  1
#define _SC_CLK_TCK                    2
#define _SC_NGROUPS_MAX                3
#define _SC_OPEN_MAX                   4
#define _SC_JOB_CONTROL                5
#define _SC_SAVED_IDS                  6
#define _SC_VERSION                    7
#define _SC_PAGESIZE                   8
#define _SC_PAGE_SIZE                  _SC_PAGESIZE
#define _SC_MQ_OPEN_MAX                9
#define _SC_MQ_PRIO_MAX                10
#define _SC_RTSIG_MAX                  11
#define _SC_SEM_NSEMS_MAX              12
#define _SC_SEM_VALUE_MAX              13
#define _SC_SIGQUEUE_MAX               14
#define _SC_TIMER_MAX                  15
#define _SC_TZNAME_MAX                 16
#define _SC_ASYNCHRONOUS_IO            17
#define _SC_FSYNC                      18
#define _SC_MAPPED_FILES               19
#define _SC_MEMLOCK                    20
#define _SC_MEMLOCK_RANGE              21
#define _SC_MEMORY_PROTECTION          22
#define _SC_MESSAGE_PASSING            23
#define _SC_PRIORITIZED_IO             24
#define _SC_REALTIME_SIGNALS           25
#define _SC_SEMAPHORES                 26
#define _SC_SHARED_MEMORY_OBJECTS      27
#define _SC_SYNCHRONIZED_IO            28
#define _SC_TIMERS                     29
#define _SC_AIO_LISTIO_MAX             30
#define _SC_AIO_MAX                    31
#define _SC_AIO_PRIO_DELTA_MAX         32
#define _SC_DELAYTIMER_MAX             33
#define _SC_THREAD_KEYS_MAX            34
#define _SC_THREAD_STACK_MIN           35
#define _SC_THREAD_THREADS_MAX         36
#define _SC_TTY_NAME_MAX               37
#define _SC_THREADS                    38
#define _SC_THREAD_ATTR_STACKADDR      39
#define _SC_THREAD_ATTR_STACKSIZE      40
#define _SC_THREAD_PRIORITY_SCHEDULING 41
#define _SC_THREAD_PRIO_INHERIT        42
/* _SC_THREAD_PRIO_PROTECT was _SC_THREAD_PRIO_CEILING in early drafts */
#define _SC_THREAD_PRIO_PROTECT          43
#define _SC_THREAD_PRIO_CEILING          _SC_THREAD_PRIO_PROTECT
#define _SC_THREAD_PROCESS_SHARED        44
#define _SC_THREAD_SAFE_FUNCTIONS        45
#define _SC_GETGR_R_SIZE_MAX             46
#define _SC_GETPW_R_SIZE_MAX             47
#define _SC_LOGIN_NAME_MAX               48
#define _SC_THREAD_DESTRUCTOR_ITERATIONS 49
#define _SC_ADVISORY_INFO                50
#define _SC_ATEXIT_MAX                   51
#define _SC_BARRIERS                     52
#define _SC_BC_BASE_MAX                  53
#define _SC_BC_DIM_MAX                   54
#define _SC_BC_SCALE_MAX                 55
#define _SC_BC_STRING_MAX                56
#define _SC_CHARCLASS_NAME_MAX           57
#define _SC_CLOCK_SELECTION              58
#define _SC_COLL_WEIGHTS_MAX             59
#define _SC_CPUTIME                      60
#define _SC_EXPR_NEST_MAX                61
#define _SC_HOST_NAME_MAX                62
#define _SC_IOV_MAX                      63
#define _SC_IPV6                         64
#define _SC_LINE_MAX                     65
#define _SC_MONOTONIC_CLOCK              66
#define _SC_RAW_SOCKETS                  67
#define _SC_READER_WRITER_LOCKS          68
#define _SC_REGEXP                       69
#define _SC_RE_DUP_MAX                   70
#define _SC_SHELL                        71
#define _SC_SPAWN                        72
#define _SC_SPIN_LOCKS                   73
#define _SC_SPORADIC_SERVER              74
#define _SC_SS_REPL_MAX                  75
#define _SC_SYMLOOP_MAX                  76
#define _SC_THREAD_CPUTIME               77
#define _SC_THREAD_SPORADIC_SERVER       78
#define _SC_TIMEOUTS                     79
#define _SC_TRACE                        80
#define _SC_TRACE_EVENT_FILTER           81
#define _SC_TRACE_EVENT_NAME_MAX         82
#define _SC_TRACE_INHERIT                83
#define _SC_TRACE_LOG                    84
#define _SC_TRACE_NAME_MAX               85
#define _SC_TRACE_SYS_MAX                86
#define _SC_TRACE_USER_EVENT_MAX         87
#define _SC_TYPED_MEMORY_OBJECTS         88
#define _SC_V7_ILP32_OFF32               89
#define _SC_V6_ILP32_OFF32               _SC_V7_ILP32_OFF32
#define _SC_XBS5_ILP32_OFF32             _SC_V7_ILP32_OFF32
#define _SC_V8_ILP32_OFF32               _SC_V7_ILP32_OFF32
#define _SC_V7_ILP32_OFFBIG              90
#define _SC_V6_ILP32_OFFBIG              _SC_V7_ILP32_OFFBIG
#define _SC_XBS5_ILP32_OFFBIG            _SC_V7_ILP32_OFFBIG
#define _SC_V8_ILP32_OFFBIG              _SC_V7_ILP32_OFFBIG
#define _SC_V7_LP64_OFF64                91
#define _SC_V6_LP64_OFF64                _SC_V7_LP64_OFF64
#define _SC_XBS5_LP64_OFF64              _SC_V7_LP64_OFF64
#define _SC_V8_LP64_OFF64                _SC_V7_LP64_OFF64
#define _SC_V7_LPBIG_OFFBIG              92
#define _SC_V6_LPBIG_OFFBIG              _SC_V7_LPBIG_OFFBIG
#define _SC_XBS5_LPBIG_OFFBIG            _SC_V7_LPBIG_OFFBIG
#define _SC_V8_LPBIG_OFFBIG              _SC_V7_LPBIG_OFFBIG
#define _SC_XOPEN_CRYPT                  93
#define _SC_XOPEN_ENH_I18N               94
#define _SC_XOPEN_LEGACY                 95
#define _SC_XOPEN_REALTIME               96
#define _SC_STREAM_MAX                   97
#define _SC_PRIORITY_SCHEDULING          98
#define _SC_XOPEN_REALTIME_THREADS       99
#define _SC_XOPEN_SHM                    100
#define _SC_XOPEN_STREAMS                101
#define _SC_XOPEN_UNIX                   102
#define _SC_XOPEN_VERSION                103
#define _SC_2_CHAR_TERM                  104
#define _SC_2_C_BIND                     105
#define _SC_2_C_DEV                      106
#define _SC_2_FORT_DEV                   107
#define _SC_2_FORT_RUN                   108
#define _SC_2_LOCALEDEF                  109
#define _SC_2_PBS                        110
#define _SC_2_PBS_ACCOUNTING             111
#define _SC_2_PBS_CHECKPOINT             112
#define _SC_2_PBS_LOCATE                 113
#define _SC_2_PBS_MESSAGE                114
#define _SC_2_PBS_TRACK                  115
#define _SC_2_SW_DEV                     116
#define _SC_2_UPE                        117
#define _SC_2_VERSION                    118
#define _SC_THREAD_ROBUST_PRIO_INHERIT   119
#define _SC_THREAD_ROBUST_PRIO_PROTECT   120
#define _SC_XOPEN_UUCP                   121
#define _SC_LEVEL1_ICACHE_SIZE           122
#define _SC_LEVEL1_ICACHE_ASSOC          123
#define _SC_LEVEL1_ICACHE_LINESIZE       124
#define _SC_LEVEL1_DCACHE_SIZE           125
#define _SC_LEVEL1_DCACHE_ASSOC          126
#define _SC_LEVEL1_DCACHE_LINESIZE       127
#define _SC_LEVEL2_CACHE_SIZE            128
#define _SC_LEVEL2_CACHE_ASSOC           129
#define _SC_LEVEL2_CACHE_LINESIZE        130
#define _SC_LEVEL3_CACHE_SIZE            131
#define _SC_LEVEL3_CACHE_ASSOC           132
#define _SC_LEVEL3_CACHE_LINESIZE        133
#define _SC_LEVEL4_CACHE_SIZE            134
#define _SC_LEVEL4_CACHE_ASSOC           135
#define _SC_LEVEL4_CACHE_LINESIZE        136
#define _SC_POSIX_26_VERSION             137

/*
 *  pathconf values per IEEE Std 1003.1, 2008 Edition
 */

#define _PC_FILESIZEBITS         0
#define _PC_LINK_MAX             1
#define _PC_MAX_CANON            2
#define _PC_MAX_INPUT            3
#define _PC_NAME_MAX             4
#define _PC_PATH_MAX             5
#define _PC_PIPE_BUF             6
#define _PC_2_SYMLINKS           7
#define _PC_ALLOC_SIZE_MIN       8
#define _PC_REC_INCR_XFER_SIZE   9
#define _PC_REC_MAX_XFER_SIZE    10
#define _PC_REC_MIN_XFER_SIZE    11
#define _PC_REC_XFER_ALIGN       12
#define _PC_SYMLINK_MAX          13
#define _PC_TEXTDOMAIN_MAX       14
#define _PC_CHOWN_RESTRICTED     15
#define _PC_NO_TRUNC             16
#define _PC_VDISABLE             17
#define _PC_ASYNC_IO             18
#define _PC_FALLOC               19
#define _PC_PRIO_IO              20
#define _PC_SYNC_IO              21
#define _PC_TIMESTAMP_RESOLUTION 22

/*
 *  confstr values per IEEE Std 1003.1, 2004 Edition
 */

#define _CS_PATH                           0
#define _CS_POSIX_V7_ILP32_OFF32_CFLAGS    1
#define _CS_POSIX_V6_ILP32_OFF32_CFLAGS    _CS_POSIX_V7_ILP32_OFF32_CFLAGS
#define _CS_XBS5_ILP32_OFF32_CFLAGS        _CS_POSIX_V7_ILP32_OFF32_CFLAGS
#define _CS_POSIX_V7_ILP32_OFF32_LDFLAGS   2
#define _CS_POSIX_V6_ILP32_OFF32_LDFLAGS   _CS_POSIX_V7_ILP32_OFF32_LDFLAGS
#define _CS_XBS5_ILP32_OFF32_LDFLAGS       _CS_POSIX_V7_ILP32_OFF32_LDFLAGS
#define _CS_POSIX_V7_ILP32_OFF32_LIBS      3
#define _CS_POSIX_V6_ILP32_OFF32_LIBS      _CS_POSIX_V7_ILP32_OFF32_LIBS
#define _CS_XBS5_ILP32_OFF32_LIBS          _CS_POSIX_V7_ILP32_OFF32_LIBS
#define _CS_XBS5_ILP32_OFF32_LINTFLAGS     4
#define _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS   5
#define _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS   _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS
#define _CS_XBS5_ILP32_OFFBIG_CFLAGS       _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS
#define _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS  6
#define _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS  _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS
#define _CS_XBS5_ILP32_OFFBIG_LDFLAGS      _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS
#define _CS_POSIX_V7_ILP32_OFFBIG_LIBS     7
#define _CS_POSIX_V6_ILP32_OFFBIG_LIBS     _CS_POSIX_V7_ILP32_OFFBIG_LIBS
#define _CS_XBS5_ILP32_OFFBIG_LIBS         _CS_POSIX_V7_ILP32_OFFBIG_LIBS
#define _CS_XBS5_ILP32_OFFBIG_LINTFLAGS    8
#define _CS_POSIX_V7_LP64_OFF64_CFLAGS     9
#define _CS_POSIX_V6_LP64_OFF64_CFLAGS     _CS_POSIX_V7_LP64_OFF64_CFLAGS
#define _CS_XBS5_LP64_OFF64_CFLAGS         _CS_POSIX_V7_LP64_OFF64_CFLAGS
#define _CS_POSIX_V7_LP64_OFF64_LDFLAGS    10
#define _CS_POSIX_V6_LP64_OFF64_LDFLAGS    _CS_POSIX_V7_LP64_OFF64_LDFLAGS
#define _CS_XBS5_LP64_OFF64_LDFLAGS        _CS_POSIX_V7_LP64_OFF64_LDFLAGS
#define _CS_POSIX_V7_LP64_OFF64_LIBS       11
#define _CS_POSIX_V6_LP64_OFF64_LIBS       _CS_POSIX_V7_LP64_OFF64_LIBS
#define _CS_XBS5_LP64_OFF64_LIBS           _CS_POSIX_V7_LP64_OFF64_LIBS
#define _CS_XBS5_LP64_OFF64_LINTFLAGS      12
#define _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS   13
#define _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS   _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS
#define _CS_XBS5_LPBIG_OFFBIG_CFLAGS       _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS
#define _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS  14
#define _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS  _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS
#define _CS_XBS5_LPBIG_OFFBIG_LDFLAGS      _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS
#define _CS_POSIX_V7_LPBIG_OFFBIG_LIBS     15
#define _CS_POSIX_V6_LPBIG_OFFBIG_LIBS     _CS_POSIX_V7_LPBIG_OFFBIG_LIBS
#define _CS_XBS5_LPBIG_OFFBIG_LIBS         _CS_POSIX_V7_LPBIG_OFFBIG_LIBS
#define _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS    16
#define _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS 17
#define _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS
#define _CS_XBS5_WIDTH_RESTRICTED_ENVS     _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS
#define _CS_POSIX_V7_THREADS_CFLAGS        18
#define _CS_POSIX_V7_THREADS_LDFLAGS       19
#define _CS_V7_ENV                         20
#define _CS_V6_ENV                         _CS_V7_ENV
#define _CS_LFS_CFLAGS                     21
#define _CS_LFS_LDFLAGS                    22
#define _CS_LFS_LIBS                       23
#define _CS_LFS_LINTFLAGS                  24

/*
 * The POSIX options and option groups the system provides, and the
 * _POSIX_VERSION they add up to.
 *
 * The OS describes itself in <sys/_posix.h>: the POSIX option macros
 * for the options it supports, and __PICOLIBC_POSIX_<GROUP> for the
 * OS-level parts of each POSIX.1-2024 option group no option macro
 * covers, with the version of the standard they meet. Anything it
 * leaves undefined isn't provided. picolibc installs that header only
 * for its own OS layers.
 */

#if defined(__has_include)
#if __has_include(<sys/_posix.h>)
#include <sys/_posix.h>
#endif
#endif

/*
 * Realtime profiles (IEEE 1003.13). The OS may claim one by defining
 * _POSIX_AEP_REALTIME_<PROFILE>, as shorthand for the profiles below
 * it and the OS options and option groups the profile requires; each
 * of those it leaves undefined takes the profile macro's value.
 * picolibc's own share is checked at the end of this file. The trace
 * options the profiles also name were removed by POSIX.1-2024.
 */
#if defined(_POSIX_AEP_REALTIME_MULTI) && !defined(_POSIX_AEP_REALTIME_DEDICATED)
#define _POSIX_AEP_REALTIME_DEDICATED _POSIX_AEP_REALTIME_MULTI
#endif
#if defined(_POSIX_AEP_REALTIME_DEDICATED) && !defined(_POSIX_AEP_REALTIME_CONTROLLER)
#define _POSIX_AEP_REALTIME_CONTROLLER _POSIX_AEP_REALTIME_DEDICATED
#endif
#if defined(_POSIX_AEP_REALTIME_CONTROLLER) && !defined(_POSIX_AEP_REALTIME_MINIMAL)
#define _POSIX_AEP_REALTIME_MINIMAL _POSIX_AEP_REALTIME_CONTROLLER
#endif

/* PSE51, the minimal realtime system profile */
#ifdef _POSIX_AEP_REALTIME_MINIMAL
#ifndef _POSIX_BARRIERS
#define _POSIX_BARRIERS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_CLOCK_SELECTION
#define _POSIX_CLOCK_SELECTION _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_FSYNC
#define _POSIX_FSYNC _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_MAPPED_FILES
#define _POSIX_MAPPED_FILES _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_MEMLOCK
#define _POSIX_MEMLOCK _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_MEMLOCK_RANGE
#define _POSIX_MEMLOCK_RANGE _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_MEMORY_PROTECTION
#define _POSIX_MEMORY_PROTECTION _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_MONOTONIC_CLOCK
#define _POSIX_MONOTONIC_CLOCK _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_READER_WRITER_LOCKS
#define _POSIX_READER_WRITER_LOCKS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_REALTIME_SIGNALS
#define _POSIX_REALTIME_SIGNALS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_SEMAPHORES
#define _POSIX_SEMAPHORES _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_SHARED_MEMORY_OBJECTS
#define _POSIX_SHARED_MEMORY_OBJECTS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_SPIN_LOCKS
#define _POSIX_SPIN_LOCKS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREAD_ATTR_STACKADDR
#define _POSIX_THREAD_ATTR_STACKADDR _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_ATTR_STACKSIZE _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREAD_CPUTIME
#define _POSIX_THREAD_CPUTIME _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREAD_PRIO_INHERIT
#define _POSIX_THREAD_PRIO_INHERIT _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREAD_PRIO_PROTECT
#define _POSIX_THREAD_PRIO_PROTECT _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREAD_PRIORITY_SCHEDULING
#define _POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREAD_SPORADIC_SERVER
#define _POSIX_THREAD_SPORADIC_SERVER _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_THREADS
#define _POSIX_THREADS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_TIMEOUTS
#define _POSIX_TIMEOUTS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef _POSIX_TIMERS
#define _POSIX_TIMERS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef __PICOLIBC_POSIX_DEVICE_IO
#define __PICOLIBC_POSIX_DEVICE_IO _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef __PICOLIBC_POSIX_SIGNALS
#define __PICOLIBC_POSIX_SIGNALS _POSIX_AEP_REALTIME_MINIMAL
#endif
#ifndef __PICOLIBC_POSIX_SINGLE_PROCESS
#define __PICOLIBC_POSIX_SINGLE_PROCESS _POSIX_AEP_REALTIME_MINIMAL
#endif
#endif /* _POSIX_AEP_REALTIME_MINIMAL */

/* PSE52, the realtime controller system profile */
#ifdef _POSIX_AEP_REALTIME_CONTROLLER
#ifndef _POSIX_MESSAGE_PASSING
#define _POSIX_MESSAGE_PASSING _POSIX_AEP_REALTIME_CONTROLLER
#endif
#ifndef __PICOLIBC_POSIX_FD_MGMT
#define __PICOLIBC_POSIX_FD_MGMT _POSIX_AEP_REALTIME_CONTROLLER
#endif
#ifndef __PICOLIBC_POSIX_FILE_SYSTEM
#define __PICOLIBC_POSIX_FILE_SYSTEM _POSIX_AEP_REALTIME_CONTROLLER
#endif
#endif /* _POSIX_AEP_REALTIME_CONTROLLER */

/* PSE53, the dedicated realtime system profile */
#ifdef _POSIX_AEP_REALTIME_DEDICATED
#ifndef _POSIX_CPUTIME
#define _POSIX_CPUTIME _POSIX_AEP_REALTIME_DEDICATED
#endif
#ifndef _POSIX_PRIORITIZED_IO
#define _POSIX_PRIORITIZED_IO _POSIX_AEP_REALTIME_DEDICATED
#endif
#ifndef _POSIX_PRIORITY_SCHEDULING
#define _POSIX_PRIORITY_SCHEDULING _POSIX_AEP_REALTIME_DEDICATED
#endif
#ifndef _POSIX_RAW_SOCKETS
#define _POSIX_RAW_SOCKETS _POSIX_AEP_REALTIME_DEDICATED
#endif
#ifndef _POSIX_SPORADIC_SERVER
#define _POSIX_SPORADIC_SERVER _POSIX_AEP_REALTIME_DEDICATED
#endif
#ifndef __PICOLIBC_POSIX_MULTI_PROCESS
#define __PICOLIBC_POSIX_MULTI_PROCESS _POSIX_AEP_REALTIME_DEDICATED
#endif
#ifndef __PICOLIBC_POSIX_NETWORKING
#define __PICOLIBC_POSIX_NETWORKING _POSIX_AEP_REALTIME_DEDICATED
#endif
#ifndef __PICOLIBC_POSIX_PIPE
#define __PICOLIBC_POSIX_PIPE _POSIX_AEP_REALTIME_DEDICATED
#endif
#endif /* _POSIX_AEP_REALTIME_DEDICATED */

/* PSE54, the multi-purpose realtime system profile */
#ifdef _POSIX_AEP_REALTIME_MULTI
#ifndef _POSIX_ADVISORY_INFO
#define _POSIX_ADVISORY_INFO _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef _POSIX_JOB_CONTROL
#define _POSIX_JOB_CONTROL _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef _POSIX_SAVED_IDS
#define _POSIX_SAVED_IDS _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef _POSIX_SHELL
#define _POSIX_SHELL _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef __PICOLIBC_POSIX_DEVICE_SPECIFIC
#define __PICOLIBC_POSIX_DEVICE_SPECIFIC _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef __PICOLIBC_POSIX_DEVICE_SPECIFIC_R
#define __PICOLIBC_POSIX_DEVICE_SPECIFIC_R _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef __PICOLIBC_POSIX_FIFO
#define __PICOLIBC_POSIX_FIFO _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef __PICOLIBC_POSIX_FILE_ATTRIBUTES
#define __PICOLIBC_POSIX_FILE_ATTRIBUTES _POSIX_AEP_REALTIME_MULTI
#endif
#ifndef __PICOLIBC_POSIX_USER_GROUPS
#define __PICOLIBC_POSIX_USER_GROUPS _POSIX_AEP_REALTIME_MULTI
#endif
#endif /* _POSIX_AEP_REALTIME_MULTI */

/* Options picolibc provides itself */
#ifndef _POSIX_REGEXP
#define _POSIX_REGEXP 1
#endif
#ifndef _POSIX2_C_BIND
#define _POSIX2_C_BIND 202405L
#endif

/* Options picolibc builds on what the system provides */
#if defined(_POSIX_THREADS) && _POSIX_THREADS > 0 && !defined(__SINGLE_THREAD) \
    && !defined(_POSIX_ASYNCHRONOUS_IO)
/* Asynchronous I/O runs each request in a thread */
#define _POSIX_ASYNCHRONOUS_IO 202405L
#endif
#if defined(_POSIX_THREADS) && _POSIX_THREADS > 0 && !defined(__SINGLE_THREAD) \
    && !defined(_POSIX_THREAD_SAFE_FUNCTIONS)
#define _POSIX_THREAD_SAFE_FUNCTIONS 202405L
#endif
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && !defined(_POSIX_SPAWN)
/* posix_spawn uses fork and exec */
#define _POSIX_SPAWN 202405L
#endif

/*
 * Programming environments, from the type sizes; off_t is always 64
 * bits. Targets with 16-bit int support none of them.
 */
#if __SIZEOF_INT__ >= 4
#if __SIZEOF_LONG__ == 4 && __SIZEOF_POINTER__ == 4
#define _POSIX_V8_ILP32_OFFBIG 1
#define _POSIX_V7_ILP32_OFFBIG 1
#endif
#if __SIZEOF_INT__ == 4 && __SIZEOF_LONG__ == 8 && __SIZEOF_POINTER__ == 8
#define _POSIX_V8_LP64_OFF64 1
#define _POSIX_V7_LP64_OFF64 1
#endif
#if __SIZEOF_LONG__ >= 8 && __SIZEOF_POINTER__ >= 8
#define _POSIX_V8_LPBIG_OFFBIG 1
#define _POSIX_V7_LPBIG_OFFBIG 1
#endif
#endif

/*
 * _POSIX_VERSION is the newest edition whose required options and
 * option groups are all present. POSIX.1-2024 also needs the
 * close-on-fork flags (O_CLOFORK, FD_CLOFORK, SOCK_CLOFORK), which an
 * OS that has them reports with __PICOLIBC_POSIX_CLOFORK. Platforms
 * that set _POSIX_VERSION themselves keep their value.
 */
#ifndef _POSIX_VERSION

/* clang-format off */

/* POSIX.1-2024 */
#if    defined(_POSIX_ASYNCHRONOUS_IO)              && _POSIX_ASYNCHRONOUS_IO              >= 202405L \
    && defined(_POSIX_BARRIERS)                     && _POSIX_BARRIERS                     >= 202405L \
    && defined(_POSIX_CLOCK_SELECTION)              && _POSIX_CLOCK_SELECTION              >= 202405L \
    && defined(_POSIX_MAPPED_FILES)                 && _POSIX_MAPPED_FILES                 >= 202405L \
    && defined(_POSIX_MEMORY_PROTECTION)            && _POSIX_MEMORY_PROTECTION            >= 202405L \
    && defined(_POSIX_MONOTONIC_CLOCK)              && _POSIX_MONOTONIC_CLOCK              >= 202405L \
    && defined(_POSIX_READER_WRITER_LOCKS)          && _POSIX_READER_WRITER_LOCKS          >= 202405L \
    && defined(_POSIX_REALTIME_SIGNALS)             && _POSIX_REALTIME_SIGNALS             >= 202405L \
    && defined(_POSIX_SEMAPHORES)                   && _POSIX_SEMAPHORES                   >= 202405L \
    && defined(_POSIX_SPIN_LOCKS)                   && _POSIX_SPIN_LOCKS                   >= 202405L \
    && defined(_POSIX_THREAD_SAFE_FUNCTIONS)        && _POSIX_THREAD_SAFE_FUNCTIONS        >= 202405L \
    && defined(_POSIX_THREADS)                      && _POSIX_THREADS                      >= 202405L \
    && defined(_POSIX_TIMEOUTS)                     && _POSIX_TIMEOUTS                     >= 202405L \
    && defined(_POSIX_TIMERS)                       && _POSIX_TIMERS                       >= 202405L \
    && defined(_POSIX2_C_BIND)                      && _POSIX2_C_BIND                      >= 202405L \
    && defined(_POSIX_JOB_CONTROL)                  && _POSIX_JOB_CONTROL                  >  0       \
    && defined(_POSIX_REGEXP)                       && _POSIX_REGEXP                       >  0       \
    && defined(_POSIX_SAVED_IDS)                    && _POSIX_SAVED_IDS                    >  0       \
    && defined(_POSIX_SHELL)                        && _POSIX_SHELL                        >  0       \
    && defined(_POSIX_CHOWN_RESTRICTED)             && _POSIX_CHOWN_RESTRICTED             != -1      \
    && defined(_POSIX_NO_TRUNC)                     && _POSIX_NO_TRUNC                     != -1      \
    && defined(_POSIX_VDISABLE)                     && _POSIX_VDISABLE                     != -1      \
    && defined(__PICOLIBC_POSIX_C_LIB_EXT)          && __PICOLIBC_POSIX_C_LIB_EXT          >= 202405L \
    && defined(__PICOLIBC_POSIX_DEVICE_IO)          && __PICOLIBC_POSIX_DEVICE_IO          >= 202405L \
    && defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC)    && __PICOLIBC_POSIX_DEVICE_SPECIFIC    >= 202405L \
    && defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC_R)  && __PICOLIBC_POSIX_DEVICE_SPECIFIC_R  >= 202405L \
    && defined(__PICOLIBC_POSIX_DYNAMIC_LINKING)    && __PICOLIBC_POSIX_DYNAMIC_LINKING    >= 202405L \
    && defined(__PICOLIBC_POSIX_FD_MGMT)            && __PICOLIBC_POSIX_FD_MGMT            >= 202405L \
    && defined(__PICOLIBC_POSIX_FIFO)               && __PICOLIBC_POSIX_FIFO               >= 202405L \
    && defined(__PICOLIBC_POSIX_FIFO_FD)            && __PICOLIBC_POSIX_FIFO_FD            >= 202405L \
    && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES)    && __PICOLIBC_POSIX_FILE_ATTRIBUTES    >= 202405L \
    && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES_FD) && __PICOLIBC_POSIX_FILE_ATTRIBUTES_FD >= 202405L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM)        && __PICOLIBC_POSIX_FILE_SYSTEM        >= 202405L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT)    && __PICOLIBC_POSIX_FILE_SYSTEM_EXT    >= 202405L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD)     && __PICOLIBC_POSIX_FILE_SYSTEM_FD     >= 202405L \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)      && __PICOLIBC_POSIX_MULTI_PROCESS      >= 202405L \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS_FD)   && __PICOLIBC_POSIX_MULTI_PROCESS_FD   >= 202405L \
    && defined(__PICOLIBC_POSIX_NETWORKING)         && __PICOLIBC_POSIX_NETWORKING         >= 202405L \
    && defined(__PICOLIBC_POSIX_PIPE)               && __PICOLIBC_POSIX_PIPE               >= 202405L \
    && defined(__PICOLIBC_POSIX_SIGNALS)            && __PICOLIBC_POSIX_SIGNALS            >= 202405L \
    && defined(__PICOLIBC_POSIX_SINGLE_PROCESS)     && __PICOLIBC_POSIX_SINGLE_PROCESS     >= 202405L \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS)     && __PICOLIBC_POSIX_SYMBOLIC_LINKS     >= 202405L \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS_FD)  && __PICOLIBC_POSIX_SYMBOLIC_LINKS_FD  >= 202405L \
    && defined(__PICOLIBC_POSIX_USER_GROUPS)        && __PICOLIBC_POSIX_USER_GROUPS        >= 202405L \
    && defined(__PICOLIBC_POSIX_CLOFORK)            && __PICOLIBC_POSIX_CLOFORK            >= 202405L
#define _POSIX_VERSION 202405L

/* POSIX.1-2008/2017: the same, without _POSIX_MONOTONIC_CLOCK */
#elif  defined(_POSIX_ASYNCHRONOUS_IO)              && _POSIX_ASYNCHRONOUS_IO              >= 200809L \
    && defined(_POSIX_BARRIERS)                     && _POSIX_BARRIERS                     >= 200809L \
    && defined(_POSIX_CLOCK_SELECTION)              && _POSIX_CLOCK_SELECTION              >= 200809L \
    && defined(_POSIX_MAPPED_FILES)                 && _POSIX_MAPPED_FILES                 >= 200809L \
    && defined(_POSIX_MEMORY_PROTECTION)            && _POSIX_MEMORY_PROTECTION            >= 200809L \
    && defined(_POSIX_READER_WRITER_LOCKS)          && _POSIX_READER_WRITER_LOCKS          >= 200809L \
    && defined(_POSIX_REALTIME_SIGNALS)             && _POSIX_REALTIME_SIGNALS             >= 200809L \
    && defined(_POSIX_SEMAPHORES)                   && _POSIX_SEMAPHORES                   >= 200809L \
    && defined(_POSIX_SPIN_LOCKS)                   && _POSIX_SPIN_LOCKS                   >= 200809L \
    && defined(_POSIX_THREAD_SAFE_FUNCTIONS)        && _POSIX_THREAD_SAFE_FUNCTIONS        >= 200809L \
    && defined(_POSIX_THREADS)                      && _POSIX_THREADS                      >= 200809L \
    && defined(_POSIX_TIMEOUTS)                     && _POSIX_TIMEOUTS                     >= 200809L \
    && defined(_POSIX_TIMERS)                       && _POSIX_TIMERS                       >= 200809L \
    && defined(_POSIX2_C_BIND)                      && _POSIX2_C_BIND                      >= 200809L \
    && defined(_POSIX_JOB_CONTROL)                  && _POSIX_JOB_CONTROL                  >  0       \
    && defined(_POSIX_REGEXP)                       && _POSIX_REGEXP                       >  0       \
    && defined(_POSIX_SAVED_IDS)                    && _POSIX_SAVED_IDS                    >  0       \
    && defined(_POSIX_SHELL)                        && _POSIX_SHELL                        >  0       \
    && defined(_POSIX_CHOWN_RESTRICTED)             && _POSIX_CHOWN_RESTRICTED             != -1      \
    && defined(_POSIX_NO_TRUNC)                     && _POSIX_NO_TRUNC                     != -1      \
    && defined(_POSIX_VDISABLE)                     && _POSIX_VDISABLE                     != -1      \
    && defined(__PICOLIBC_POSIX_C_LIB_EXT)          && __PICOLIBC_POSIX_C_LIB_EXT          >= 200809L \
    && defined(__PICOLIBC_POSIX_DEVICE_IO)          && __PICOLIBC_POSIX_DEVICE_IO          >= 200809L \
    && defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC)    && __PICOLIBC_POSIX_DEVICE_SPECIFIC    >= 200809L \
    && defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC_R)  && __PICOLIBC_POSIX_DEVICE_SPECIFIC_R  >= 200809L \
    && defined(__PICOLIBC_POSIX_DYNAMIC_LINKING)    && __PICOLIBC_POSIX_DYNAMIC_LINKING    >= 200809L \
    && defined(__PICOLIBC_POSIX_FD_MGMT)            && __PICOLIBC_POSIX_FD_MGMT            >= 200809L \
    && defined(__PICOLIBC_POSIX_FIFO)               && __PICOLIBC_POSIX_FIFO               >= 200809L \
    && defined(__PICOLIBC_POSIX_FIFO_FD)            && __PICOLIBC_POSIX_FIFO_FD            >= 200809L \
    && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES)    && __PICOLIBC_POSIX_FILE_ATTRIBUTES    >= 200809L \
    && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES_FD) && __PICOLIBC_POSIX_FILE_ATTRIBUTES_FD >= 200809L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM)        && __PICOLIBC_POSIX_FILE_SYSTEM        >= 200809L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT)    && __PICOLIBC_POSIX_FILE_SYSTEM_EXT    >= 200809L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD)     && __PICOLIBC_POSIX_FILE_SYSTEM_FD     >= 200809L \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)      && __PICOLIBC_POSIX_MULTI_PROCESS      >= 200809L \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS_FD)   && __PICOLIBC_POSIX_MULTI_PROCESS_FD   >= 200809L \
    && defined(__PICOLIBC_POSIX_NETWORKING)         && __PICOLIBC_POSIX_NETWORKING         >= 200809L \
    && defined(__PICOLIBC_POSIX_PIPE)               && __PICOLIBC_POSIX_PIPE               >= 200809L \
    && defined(__PICOLIBC_POSIX_SIGNALS)            && __PICOLIBC_POSIX_SIGNALS            >= 200809L \
    && defined(__PICOLIBC_POSIX_SINGLE_PROCESS)     && __PICOLIBC_POSIX_SINGLE_PROCESS     >= 200809L \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS)     && __PICOLIBC_POSIX_SYMBOLIC_LINKS     >= 200809L \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS_FD)  && __PICOLIBC_POSIX_SYMBOLIC_LINKS_FD  >= 200809L \
    && defined(__PICOLIBC_POSIX_USER_GROUPS)        && __PICOLIBC_POSIX_USER_GROUPS        >= 200809L
#define _POSIX_VERSION 200809L

/* POSIX.1-2001/2004 */
#elif  defined(_POSIX2_C_BIND)                      && _POSIX2_C_BIND                      >= 200112L \
    && defined(_POSIX_JOB_CONTROL)                  && _POSIX_JOB_CONTROL                  >  0       \
    && defined(_POSIX_REGEXP)                       && _POSIX_REGEXP                       >  0       \
    && defined(_POSIX_SAVED_IDS)                    && _POSIX_SAVED_IDS                    >  0       \
    && defined(_POSIX_SHELL)                        && _POSIX_SHELL                        >  0       \
    && defined(_POSIX_CHOWN_RESTRICTED)             && _POSIX_CHOWN_RESTRICTED             != -1      \
    && defined(_POSIX_NO_TRUNC)                     && _POSIX_NO_TRUNC                     != -1      \
    && defined(_POSIX_VDISABLE)                     && _POSIX_VDISABLE                     != -1      \
    && defined(__PICOLIBC_POSIX_C_LIB_EXT)          && __PICOLIBC_POSIX_C_LIB_EXT          >= 200112L \
    && defined(__PICOLIBC_POSIX_DEVICE_IO)          && __PICOLIBC_POSIX_DEVICE_IO          >= 200112L \
    && defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC)    && __PICOLIBC_POSIX_DEVICE_SPECIFIC    >= 200112L \
    && defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC_R)  && __PICOLIBC_POSIX_DEVICE_SPECIFIC_R  >= 200112L \
    && defined(__PICOLIBC_POSIX_DYNAMIC_LINKING)    && __PICOLIBC_POSIX_DYNAMIC_LINKING    >= 200112L \
    && defined(__PICOLIBC_POSIX_FD_MGMT)            && __PICOLIBC_POSIX_FD_MGMT            >= 200112L \
    && defined(__PICOLIBC_POSIX_FIFO)               && __PICOLIBC_POSIX_FIFO               >= 200112L \
    && defined(__PICOLIBC_POSIX_FIFO_FD)            && __PICOLIBC_POSIX_FIFO_FD            >= 200112L \
    && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES)    && __PICOLIBC_POSIX_FILE_ATTRIBUTES    >= 200112L \
    && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES_FD) && __PICOLIBC_POSIX_FILE_ATTRIBUTES_FD >= 200112L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM)        && __PICOLIBC_POSIX_FILE_SYSTEM        >= 200112L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT)    && __PICOLIBC_POSIX_FILE_SYSTEM_EXT    >= 200112L \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD)     && __PICOLIBC_POSIX_FILE_SYSTEM_FD     >= 200112L \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)      && __PICOLIBC_POSIX_MULTI_PROCESS      >= 200112L \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS_FD)   && __PICOLIBC_POSIX_MULTI_PROCESS_FD   >= 200112L \
    && defined(__PICOLIBC_POSIX_NETWORKING)         && __PICOLIBC_POSIX_NETWORKING         >= 200112L \
    && defined(__PICOLIBC_POSIX_PIPE)               && __PICOLIBC_POSIX_PIPE               >= 200112L \
    && defined(__PICOLIBC_POSIX_SIGNALS)            && __PICOLIBC_POSIX_SIGNALS            >= 200112L \
    && defined(__PICOLIBC_POSIX_SINGLE_PROCESS)     && __PICOLIBC_POSIX_SINGLE_PROCESS     >= 200112L \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS)     && __PICOLIBC_POSIX_SYMBOLIC_LINKS     >= 200112L \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS_FD)  && __PICOLIBC_POSIX_SYMBOLIC_LINKS_FD  >= 200112L \
    && defined(__PICOLIBC_POSIX_USER_GROUPS)        && __PICOLIBC_POSIX_USER_GROUPS        >= 200112L
#define _POSIX_VERSION 200112L
#endif

/* clang-format on */

#endif /* !_POSIX_VERSION */

#if defined(_POSIX_VERSION) && !defined(_POSIX2_VERSION)
#define _POSIX2_VERSION _POSIX_VERSION
#endif

/*
 * picolibc's share of a claimed realtime profile. Its code is
 * complete for all of them; what remains is the build configuration
 * picolibc was built with.
 */
#ifdef _POSIX_AEP_REALTIME_MINIMAL
#ifndef _POSIX_THREAD_SAFE_FUNCTIONS
#error "Realtime profiles need thread-safe functions, absent with single-thread"
#endif
#ifndef _POSIX_ASYNCHRONOUS_IO
#error "Realtime profiles need asynchronous I/O, present only with POSIX threads"
#endif
#ifndef _POSIX_VERSION
#ifndef _POSIX_SUBPROFILE
#define _POSIX_SUBPROFILE 1
#endif
#endif
#endif /* _POSIX_AEP_REALTIME_MINIMAL */

_END_STD_C

#if __SSP_FORTIFY_LEVEL > 0
#include <ssp/unistd.h>
#endif

#endif /* _SYS_UNISTD_H */
