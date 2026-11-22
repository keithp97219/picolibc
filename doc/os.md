# Picolibc and Operating Systems

Picolibc is designed to be operating-system independent, so it doesn't
embed any operating system support into libc. Some portions of
Picolibc need system support, like I/O and task termination. Picolibc
expects these interfaces to conform to POSIX, both as a way to
simplify integration into POSIX-conforming operating system and as a
way to provide a well defined interface between picolibc and the
target system.

When using picolibc in a non-POSIX environment, applications may
constrain themselves to portions of picolibc which do not rely upon
these functions, provide wrapping functions which expose POSIX apis on
top of their native implementation or replace portions of picolibc
with code designed atop their interfaces.

## stdin/stdout/stderr

Picolibc stdio splits support for simple console input/output and more
complex file operations so that a minimal system can easily support
the former with only a few functions.

To get stdin/stdout/stderr working, the application needs to define
the `stdin`, `stdout` and `stderr` globals, which contain pointers to
FILE objects. The pointers may reside in read-only memory, but the
FILE objects may not. A single FILE object may be used for all three
pointers, and linker aliases may be used to make all three pointers be
stored in the same location. The FILE object contains function
pointers for putc, getc, which might be defined as follows:

```c
static int
sample_putc(char c, FILE *file)
{
	(void) file;		/* Not used in this function
	__uart_putc(c);		/* Defined by underlying system */
	return c;
}

static int
sample_getc(FILE *file)
{
	unsigned char c;
	(void) file;		/* Not used in this function */
	c = __uart_getc();	/* Defined by underlying system */
	return c;
}
```

It also contains a pointer to an optional `flush` function, which, if
provide, will be called to flush pending output to the file, e.g.,
when the `fflush()` function is called:

```c
static int
sample_flush(FILE *file)
{
	/* This function doesn't need to do anything */
	(void) file;		/* Not used in this function */
	return 0;
}
```

These functions are used to initialize a FILE structure:

```c
static FILE __stdio = FDEV_SETUP_STREAM(sample_putc,
					sample_getc,
					NULL,
					_FDEV_SETUP_RW);
```

This defines a FILE which can read and write characters using the putc
and getc functions described above, but not using any flush
function. The final parameter, which specifies the operations
supported, can be one of the following:

| Mode              | Operations | Required Functions |
|-------------------|------------|--------------------|
| _FDEV_SETUP_READ  | Read       | getc               |
| _FDEV_SETUP_WRITE | Write      | putc               |
| _FDEV_SETUP_RW    | Read/Write | putc, getc         |

Finally, the FILE is used to initialize the `stdin`, `stdout` and
`stderr` values, the latter two of which are simply aliases to `stdin`:

```c
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
```

## Operating System Interfaces used by Picolibc

Here's the full list of external operating system functions used by
Picolibc and which source files reference them. 

 * int close(int fd)

   + libc/posix/posix_close.c
   + libc/search/hash.c
   + libc/stdio/fdopen.c
   + libc/stdio/fopen.c
   + libc/stdio/freopen.c
   + libc/stdio/mktemp.c
   + libc/stdio/tmpfile.c

 * void _exit(int status)

   + libc/signal/signal.c
   + libc/stdlib/abort.c
   + libc/stdlib/exit.c
   + libc/stdlib/_Exit.c

 * int fstat(int fd, struct stat *statbuf)

   + libc/search/hash.c

 * char *getcwd(char *buf, size_t size)

   + libc/posix/unistd/getwd.c

 * int getentropy(void *buffer, size_t length)

   + libc/ssp/stack/protector.c
   + libc/stdlib/arc4random.c

 * pid_t getpid(void)

   + libc/signal/raise.c

 * int gettimeofday(struct timeval *tv, struct timezone *tz)

   + libc/time/time.c

 * int kill(pid_t pid, int sig)

   + libc/signal/raise.c

 * off_t lseek(int fd, off_t offset, int whence)

   + libc/search/hash.c
   + libc/search/hash_page.c
   + libc/stdio/fdopen.c
   + libc/stdio/freopen.c
   + libc/xdr/xdr_rec.c

 * uid_t getuid(void)

   + libc/posix/getlogin.c
   + libc/posix/getlogin_r.c

 * int open(char *path, int flags, ...)

   + libc/search/hash.c
   + libc/stdio/fopen.c
   + libc/stdio/freopen.c
   + libc/stdio/mktemp.c

 * int pthread_kill(pthread_t thread, int sig)

   + libc/signal/raise.c

 * ssize_t read(int fd, void *buf, size_t count)

   + libc/search/hash.c
   + libc/search/hash_page.c
   + libc/stdio/fdopen.c
   + libc/stdio/freopen.c

 * struct dirent *readdir(DIR *dirp)

   + libc/posix/readdir_r.c

 * int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact)

   + libc/signal/signal.c

 * int sigprocmask(int how, sigset_t *set, sigset_t *oldset)

   + libc/search/hash_page.c

 * int sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout)

   + libc/signal/sigwait.c
   + libc/signal/sigwaitinfo.c

 * int stat(char *path, struct stat *statbuf)

   + libc/search/hash.c

 * clock_t times(struct tms *buf)

   + libc/time/clock.c

 * int unlink(char *path)

   + libc/search/hash_page.c
   + libc/stdio/remove.c
   + libc/stdio/tmpfile.c

 * ssize_t write(int fd, void *buf, size_t count)

   + libc/search/hash.c
   + libc/search/hash_page.c
   + libc/stdio/fdopen.c
   + libc/stdio/freopen.c
   + libc/stdio/vdprintf.c

### fopen, fdopen and freopen

Support for these requires malloc/free along with a handful of
POSIX-compatible functions:

```c
int     close (int fd);
off_t   lseek (int fd, off_t offset, int whence);
int     open (const char *, int, ...);
ssize_t read (int fd, void *buf, size_t nbyte);
ssize_t write (int fd, const void *buf, size_t nbyte);
```

### dprintf, vdprintf

These functions directly operate on file descriptors, so they use
`write`:

```c
ssize_t write (int fd, const void *buf, size_t nbyte);
```

### mktemp, mkstemp, mkostemp, mkstemps, mkostemps, tmpnam and tmpfile

All of these functions share a common implementation which operates on
file descriptors, so they all need a selection of POSIX functions:

```c
int close (int fd);
int open (const char *, int, ...);
int unlink(char *path)
```

### exit and _Exit

`_Exit` is a trivial wrapper around `_exit`; `exit` is a wrapper around
`_exit` that also calls destructors and callbacks registered with `atexit`
or `on_exit`. To make these work, you'll need to implement the `_exit`
function:

```c
void _exit (int status);
```

### malloc, free (and friends)

Picolibc's `malloc` implementation relies on `sbrk`. Malloc can handle
sbrk returning dis-continuous memory.

```c
void *sbrk(intptr_t incr);
```

### sbrk

Picolibc includes a simple version of sbrk that can return chunks of
memory from a pre-defined contiguous heap. You can either specify
`-Dinternal-heap=<size-in-bytes>` while building picolibc to declare a
fixed-size pre-allocated heap, or have the linker script define
symbols that provide the bounds of the heap:

 * `__heap_start` — points at the start of the heap available for sbrk.
 * `__heap_end` — points at the end of the heap available for sbrk.

The sample linker script provided with picolibc defines these two
symbols to enclose all RAM which is not otherwise used by the
application.

### abort and raise

Posix says that `abort` sends `SIGABRT` to the calling process as if
the process called `raise(SIGABRT)`. It also says `abort` shall not
return. The picolibc implementation of `abort` calls `raise`; if that
returns, it then calls `_exit`. picolibc's `raise` sends the signal to
the calling thread, with `pthread_kill` when picolibc has POSIX
threads and with `kill` to `getpid` otherwise. Without an OS library,
libos/fallback's `raise` delivers the signal itself and calls `_exit`
for uncaught and un-ignored signals.

```c
__noreturn void _exit (int status);
pid_t   getpid(void);
int     kill(pid_t pid, int sig);
int     pthread_kill(pthread_t thread, int sig);
```

### signal, sigwait, sigwaitinfo

`signal` sets the handler with `sigaction`; `sigwait` and
`sigwaitinfo` wait with `sigtimedwait`:

```c
int     sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
int     sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
```

### ndbm

Picolibc provides the BSD ndbm APIs and those operate directly via a
POSIX OS interface, not through stdio. Support for this API set
require all of the functions required by `fopen` along a few
additional ones:

```c
int     close (int fd);
off_t   lseek (int fd, off_t offset, int whence);
int     open (const char *, int, ...);
ssize_t read (int fd, void *buf, size_t nbyte);
int     sigprocmask(int how, sigset_t *set, sigset_t *oldset);
int     stat(char *path, struct stat *statbuf);
int     unlink(char *path);
ssize_t write (int fd, const void *buf, size_t nbyte);
```

### xdr

The Sun XDR APIs are generally abstracted away from OS interfaces
except that when using xdrrec_create. As that interface provides no
seek interface, the underlying implementation assumes that the
provided handle actually contains a POSIX file descriptor and directly
calls lseek for xdr_setpos and xdr_getpos. This provides compatibility
with glibc which does the same thing.

```c
off_t lseek(int fd, off_t offset, int whence);
```

### arc4random, arc4random_buf

To generate secure random numbers, picolibc's arc4random
implementation is seeded from system entropy:

```c
int getentropy(void *buffer, size_t length);
```

### clock

The clock API returns a total of all values returned from the times
API.

```c
clock_t times(struct tms *buf);
```

### posix_close

`posix_close` uses `close`, reporting an interrupted close as
EINPROGRESS, as close is expected to leave the descriptor closed.

```c
int close(int fd);
```

### getlogin, getlogin_r

These look up the real user with picolibc's password database
functions:

```c
uid_t getuid(void);
```

### getwd

`getwd` is `getcwd` with a `PATH_MAX` buffer:

```c
char   *getcwd(char *buf, size_t size);
```

### readdir_r

`readdir_r` copies the entry returned by `readdir`, which the OS
provides along with `opendir` and `closedir`:

```c
struct dirent *readdir(DIR *dirp);
```

## Operating system headers

picolibc's public headers declare the POSIX interfaces; the parts that
depend on the operating system come from a few private headers the OS
supplies on the include path. picolibc's own OS layers (`libos/linux`)
provide them, and another OS, such as Zephyr, provides its own along with
the code that implements them. They are interfaces between picolibc and
the OS: their names and meanings stay stable.

picolibc's ABI doesn't depend on `<sys/_posix.h>`. No type, structure
layout or library code changes with what it defines, so a picolibc
built without the OS's header, or with a different version of it, is
the same library. Only applications see its claims. The one exception
is a weak fallback, such as `sysconf` in `libos/fallback`, which the
operating system replaces with its own. The other headers below do
carry the OS's ABI, so picolibc's own code that uses them is built
only with the OS layer that provides them.

### `<sys/_posix.h>`: POSIX options and option groups

`<unistd.h>` includes this header when it exists. It contains no public
POSIX names, only `__PICOLIBC_OS_` macros, each defined only for what
the OS provides; anything left undefined is not provided. `<unistd.h>`
combines them with picolibc's own share to define the public names,
and stops with an error if the header defines one of those itself.

 * **Options:** `__PICOLIBC_OS_<OPTION>` for each POSIX option the OS
   supports, named as the option without `_POSIX_`
   (`__PICOLIBC_OS_TIMERS`, `__PICOLIBC_OS_THREAD_PRIO_INHERIT`, ...),
   with the version of the standard met, such as `200809L` or
   `202405L`. The OS provides every interface POSIX puts under that
   option. `<unistd.h>` defines `_POSIX_<OPTION>` from it. picolibc
   adds the options it implements itself: `_POSIX_REGEXP` and
   `_POSIX2_C_BIND`; `_POSIX_ASYNCHRONOUS_IO` when picolibc was built
   with its own POSIX threads, unless the OS provides it;
   `_POSIX_THREAD_SAFE_FUNCTIONS` when picolibc was built with locking
   and the OS has threads; and `_POSIX_SPAWN` when the OS has
   processes, unless the OS provides it. These take the version of the
   OS option or group they're built on.

 * **Constants:** `__PICOLIBC_OS_CHOWN_RESTRICTED`,
   `__PICOLIBC_OS_NO_TRUNC` and `__PICOLIBC_OS_VDISABLE`, which POSIX
   requires, and `__PICOLIBC_OS_JOB_CONTROL`,
   `__PICOLIBC_OS_SAVED_IDS` and `__PICOLIBC_OS_SHELL` where provided,
   with the values `<unistd.h>` publishes as `_POSIX_CHOWN_RESTRICTED`
   and the rest.

 * **Limits:** `__PICOLIBC_OS_RTSIG_MAX`, the number of real-time
   signals the OS supports when that's fewer than the 31 picolibc
   numbers, which `<limits.h>` publishes as `RTSIG_MAX`.

 * **Option groups:** `__PICOLIBC_OS_<GROUP>` for the OS's part of each
   POSIX.1-2024 option group, with the version met; `<unistd.h>`
   defines `__PICOLIBC_POSIX_<GROUP>` from it. Groups picolibc
   implements entirely (`C_LANG_JUMP`, `C_LANG_MATH`, `C_LANG_SUPPORT`,
   `C_LANG_SUPPORT_R`, `C_LANG_WIDE_CHAR`, `C_LIB_EXT`, `FILE_LOCKING`,
   `REGEXP`) are always defined, and groups of picolibc code over OS
   groups follow those (`FILE_SYSTEM_GLOB`, `FILE_SYSTEM_R`,
   `SYSTEM_DATABASE`, `SYSTEM_DATABASE_R` and `USER_GROUPS_R` follow
   `FILE_SYSTEM`; `SIGNAL_JUMP` follows `SIGNALS`;
   `WIDE_CHAR_DEVICE_IO` follows `DEVICE_IO`). The OS's part of each
   group is the group's interfaces that picolibc doesn't implement:

   | Group | The OS provides |
   |---|---|
| `CLOFORK` | O_CLOFORK, FD_CLOFORK and SOCK_CLOFORK in open, fcntl, dup3, pipe2, accept4 and socket |
| `DEVICE_IO` | `close`, `poll`, `ppoll`, `pread`, `pselect`, `pwrite`, `read`, `select`, `write` |
| `DEVICE_SPECIFIC` | `cfgetispeed`, `cfgetospeed`, `cfsetispeed`, `cfsetospeed`, `isatty`, `tcdrain`, `tcflow`, `tcflush`, `tcgetattr`, `tcgetwinsize`, `tcsendbreak`, `tcsetattr`, `tcsetwinsize`, `ttyname` |
| `DEVICE_SPECIFIC_R` | `ttyname_r` |
| `DYNAMIC_LINKING` | `dladdr`, `dlclose`, `dlerror`, `dlopen`, `dlsym` |
| `FD_MGMT` | `dup`, `dup2`, `dup3`, `fcntl`, `ftruncate`, `lseek` |
| `FIFO` | `mkfifo` |
| `FIFO_FD` | `mkfifoat`, `mknodat` |
| `FILE_ATTRIBUTES` | `chmod`, `chown`, `fchmod`, `fchown`, `umask` |
| `FILE_ATTRIBUTES_FD` | `fchmodat`, `fchownat` |
| `FILE_SYSTEM` | `access`, `chdir`, `closedir`, `fchdir`, `fpathconf`, `futimens`, `getcwd`, `link`, `mkdir`, `opendir`, `pathconf`, `posix_getdents`, `readdir`, `rename`, `rewinddir`, `rmdir`, `truncate`, `unlink`; stat, lstat and fstat |
| `FILE_SYSTEM_EXT` | the file system functions glob, nftw and the like rely on, from FILE_SYSTEM |
| `FILE_SYSTEM_FD` | `faccessat`, `fdopendir`, `fstatat`, `linkat`, `mkdirat`, `openat`, `renameat`, `unlinkat`, `utimensat` |
| `MULTI_PROCESS` | `_Fork`, `_exit`, `execve`, `fork`, `getpgid`, `getpgrp`, `getpid`, `getppid`, `getrlimit`, `getsid`, `setrlimit`, `setsid`, `times`, `waitid`, `waitpid` |
| `MULTI_PROCESS_FD` | `fexecve` |
| `NETWORKING` | `accept`, `accept4`, `bind`, `connect`, `gethostname`, `getpeername`, `getsockname`, `getsockopt`, `if_freenameindex`, `if_indextoname`, `if_nameindex`, `if_nametoindex`, `listen`, `recv`, `recvfrom`, `recvmsg`, `send`, `sendmsg`, `sendto`, `setsockopt`, `shutdown`, `sockatmark`, `socket`, `socketpair` |
| `PIPE` | `pipe`, `pipe2` |
| `SIGNALS` | `alarm`, `kill`, `pause`, `sigaction`, `sigpending`, `sigprocmask`, `sigsuspend`, `sigtimedwait` |
| `SINGLE_PROCESS` | `sysconf`, `uname` |
| `SYMBOLIC_LINKS` | `lchown`, `readlink`, `symlink` |
| `SYMBOLIC_LINKS_FD` | `readlinkat`, `symlinkat` |
| `USER_GROUPS` | `getegid`, `geteuid`, `getgid`, `getgroups`, `getuid`, `setegid`, `seteuid`, `setgid`, `setuid` |

   `__PICOLIBC_OS_XSI_<GROUP>` likewise describes the OS's part of an
   XSI option group (`DEVICE_IO`, `DEVICE_SPECIFIC`, `FILE_SYSTEM`,
   `GENERAL_TERMINAL_R`, `IPC`, `MULTI_PROCESS`, `SIGNALS`,
   `SINGLE_PROCESS`, `SYSTEM_LOGGING`, `USER_GROUPS`), and
   `<unistd.h>` defines `__PICOLIBC_XSI_<GROUP>` from it.

**Versions.** Each option and group claim carries the newest edition
whose requirements for it the OS meets in full: `202405L` only when
everything POSIX.1-2024 asks of it works, otherwise `200809L` (or
`200112L`) when the earlier edition's does. `<unistd.h>` passes the
values on, so applications and the edition computation see what each
part really provides. What POSIX.1-2024 added to the OS's part, beyond
the POSIX.1-2008 interfaces, is:

| Option or group | Added in POSIX.1-2024 |
|---|---|
| `CLOFORK` | the whole group |
| `DEVICE_IO` | `ppoll` |
| `DEVICE_SPECIFIC` | `tcgetwinsize`, `tcsetwinsize` |
| `DYNAMIC_LINKING` | `dladdr` |
| `FD_MGMT` | `dup3`; open file description locks (`F_OFD_GETLK`, `F_OFD_SETLK`, `F_OFD_SETLKW`) in `fcntl` |
| `FILE_SYSTEM` | `futimens`, `posix_getdents` |
| `MULTI_PROCESS` | `_Fork`, `getrlimit`, `setrlimit` |
| `NETWORKING` | `accept4` |
| `PIPE` | `pipe2` |
| `XSI_FILE_SYSTEM` | `utimes` |
| `XSI_GENERAL_TERMINAL_R` | the whole group (`ptsname_r`) |
| `XSI_USER_GROUPS` | `getresgid`, `getresuid`, `setresgid`, `setresuid` |
| `THREADS` | `pthread_cond_clockwait`, `pthread_mutex_clocklock`, `sched_yield` |
| `READER_WRITER_LOCKS` | `pthread_rwlock_clockrdlock`, `pthread_rwlock_clockwrlock` |
| `SEMAPHORES` | `sem_clockwait` |

The table follows the option group lists of the two editions'
rationale (XRAT E.1) for membership; changes Issue 8 made to existing
interfaces count too, as the `fcntl` locks show.

`<unistd.h>` then sets `_POSIX_VERSION` to the newest edition whose
required options and groups are all present at that version.

A system that claims the single process group (`sysconf`, `uname`,
...) without meeting a whole edition implements a subprofile
(POSIX.1-2024 XBD 2.1.5.1). Its profile is the subset the OS claims,
published or not; the OS documents it, and `sysconf` reports it at run
time. `<unistd.h>` defines `_POSIX_SUBPROFILE`, telling applications to
test each option rather than infer them from the edition, sets
`_POSIX_VERSION` to `202405L`, the edition it profiles, and leaves
`_POSIX2_VERSION` undefined. A system claiming less than the single
process group gets no `_POSIX_VERSION`.

Without `<sys/_posix.h>`, picolibc describes itself (`<sys/_os.h>`):
it claims the single process group, which libc.a provides alone, so
bare-metal applications get `_POSIX_SUBPROFILE` and `_POSIX_VERSION`
`202405L`. `<unistd.h>` and `<sys/utsname.h>` send `sysconf` and
`uname` to `__baremetal_sysconf` and `__baremetal_uname`, so an OS
library linked anyway can't contradict those claims; the same
functions are the weak `sysconf` and `uname` an OS replaces.

The realtime profiles of IEEE 1003.13-2003 follow from the claims:
`<unistd.h>` defines `_POSIX_AEP_REALTIME_MINIMAL` (PSE51),
`_CONTROLLER` (PSE52), `_DEDICATED` (PSE53) and `_MULTI` (PSE54) when
every option and unit of functionality in the profile's tables is
present, as the option or group that stands for it in POSIX.1-2024.
The OS can't claim a profile directly, and `<unistd.h>` stops with an
error if `<sys/_posix.h>` defines `__PICOLIBC_OS_AEP_REALTIME_*`. Most
systems lack sporadic server, which every profile requires, and so
claim none.

`<sys/_posix.h>` may be included in any compilation unit, so it uses
only macros visible there (Zephyr's, for instance, tests its Kconfig
`CONFIG_` symbols).

### `<sys/_pthread.h>`: threads

`<pthread.h>`, `<semaphore.h>` and `<threads.h>` require this header and
stop with an error without it; `<sys/types.h>`, `<signal.h>` and
`<limits.h>` use it when it exists. It defines:

 * **Types:** `pthread_t`, `pthread_attr_t`, `pthread_mutex_t`,
   `pthread_mutexattr_t`, `pthread_cond_t`, `pthread_condattr_t`,
   `pthread_rwlock_t`, `pthread_rwlockattr_t`, `pthread_barrier_t`,
   `pthread_barrierattr_t`, `pthread_spinlock_t`, `pthread_once_t`,
   `pthread_key_t` and `sem_t`.

 * **Static initializers:** `PTHREAD_MUTEX_INITIALIZER`,
   `PTHREAD_COND_INITIALIZER`, `PTHREAD_RWLOCK_INITIALIZER` and
   `PTHREAD_ONCE_INIT`. They may use `<pthread.h>`'s constants and
   `CLOCK_REALTIME`, which are defined wherever they're expanded.

 * **Limits**, which `<limits.h>` and `<threads.h>` publish:
   `__PTHREAD_DESTRUCTOR_ITERATIONS`, `__PTHREAD_KEYS_MAX`,
   `__PTHREAD_STACK_MIN` and `__SEM_VALUE_MAX`.

`<pthread.h>` supplies everything else: the prototypes, the constants
(`PTHREAD_CREATE_*`, `PTHREAD_MUTEX_*`, `PTHREAD_PRIO_*`,
`PTHREAD_PROCESS_*`, `PTHREAD_CANCEL_*`, ...) and
`pthread_cleanup_push` and `pthread_cleanup_pop`, which the OS
implements as `__pthread_cleanup_push` and `__pthread_cleanup_pop`
over `<pthread.h>`'s `struct __pthread_cleanup`.

### `<sys/_signal.h>`: signal stacks

`<signal.h>` includes this header when it exists. It may define
`MINSIGSTKSZ` and `SIGSTKSZ`; otherwise `<signal.h>` uses 2048 and
8192. picolibc numbers the signals itself, including the real-time
signals `SIGRTMIN` (32) to `SIGRTMAX` (62), and the OS layer maps them
to its own. An OS that delivers fewer real-time signals reports how
many with `__PICOLIBC_OS_RTSIG_MAX` in `<sys/_posix.h>`, which
`<limits.h>` publishes as `RTSIG_MAX`, and through
`sysconf(_SC_RTSIG_MAX)`.

### Networking

The network headers require their OS headers and stop with an error
without them:

 * `<sys/_socket.h>`, for `<sys/socket.h>`: `sa_family_t`,
   `socklen_t`, `struct sockaddr`, `struct sockaddr_storage`,
   `struct msghdr`, `struct cmsghdr` with `CMSG_DATA`, `CMSG_LEN`,
   `CMSG_SPACE`, `CMSG_FIRSTHDR` and `CMSG_NXTHDR`, `struct linger`,
   `SCM_RIGHTS`, the `SOCK_`, `SOL_SOCKET`, `SO_`, `MSG_`, `AF_`, `PF_`
   and `SHUT_` values, and `SOMAXCONN`.

 * `<netinet/_in.h>`, for `<netinet/in.h>`: `in_port_t`, `in_addr_t`,
   `struct in_addr`, `struct sockaddr_in`, `struct in6_addr`,
   `struct sockaddr_in6`, `struct ipv6_mreq`, and the `IPPROTO_` and
   `IPV6_` values.

 * `<netinet/_tcp.h>`, for `<netinet/tcp.h>`: `TCP_NODELAY`.

 * `<sys/_un.h>`, for `<sys/un.h>`: `struct sockaddr_un`.

 * `<net/_if.h>`, for `<net/if.h>`: `IF_NAMESIZE`.

`<arpa/inet.h>` declares the address functions (`inet_addr`,
`inet_ntoa`, `inet_ntop`, `inet_pton`) when the OS provides
`<netinet/_in.h>`, and only the byte order functions otherwise.
picolibc's resolver (`getaddrinfo` and friends) is built on these.

## Linking with System Library

To get Picolibc to use a system library, that library needs to be
specified *after* libc on the linker command line. The picolibc.specs
file provides a way to specify a library after libc using the
`--oslib=` parameter:

```console
$ gcc -o program.elf program.o --oslib=myos
```

This will include -lmyos after -lc so that the linker can resolve
functions used by picolibc from libmyos.a. You can, alternatively,
include the functions in object files with the rest of your
application, which avoids the problem with libraries. Note that this
mechanism requires the definition of _exit in the myos library.

## Semihosting support

For RISC-V and ARM processors, Picolibc provides an implementation of
all of the above functions along with a couple more POSIX APIs used by
the Picolibc test suite. This implementation relies on semihosting
support from the execution environment, which is available when
running under qemu or when using openocd and gdb. Link this into your
application using `--oslib=semihost` in your link command line.
Please note that this will replace the default `crt0` with a variant
calling `exit` upon return from `main`. The default is to enter an
infinite loop, and the change ensure a clean return to the execution
environment.

## POSIX console support

As a build-time option, Picolibc can be configured to use POSIX read
and write APIs to support stdin, stdout and stderr. Add
`-Dposix-console=true` to enable this. This is incompatible with
semihosting support above.

## Building picolibc on native POSIX systems

To allow for testing of picolibc and applications using picolibc, you
can actually build picolibc on a full POSIX system. In this
configuration, picolibc provides the non-POSIX libc APIs while the
underlying system C library is used for the POSIX functions described
above. To build in this mode, you'll need to override a few default
picolibc configuration parameters:

```console
$ meson \
        -Dtls-model=global-dynamic \
        -Dmultilib=false \
	-Dpicocrt=false \
	-Dsemihost=false \
	-Duse-stdlib=true \
	-Dposix-console=true \
	-Dthread-local-storage=true \
	-Dthread-local-storage-api=false \
	-Dinternal-heap=8388608 \
	-Derrno-function=auto \
	-Dinitfini-array=false\
	-Dincludedir=lib/picolibc/include \
	-Dlibdir=lib/picolibc/lib \
	-Dspecsdir=none \
```

 * -Dtls-model=global-dynamic makes picolibc use the default TLS model
   for GCC.

 * -Dmultilib=false makes picolibc build only a single library for the
   default GCC configuration.

 * -Dpicocrt=false disables building the C startup code as that is
   provided by the underlying system.

 * -Dsemihost=false disables building semihosting support as OS
   interfaces are provided by the underlying system.

 * -Duse-stdlib=true removes -nostdlib from the compile and link
   command lines so that the native C library will be used.

 * -Dposix-console=true uses POSIX I/O read/write APIs for stdin,
    stdout and stderr.

 * -Dthread-local-storage=true and -Dthread-local-storage-api=false
   set up the library to enable TLS but not attempt to provide the
   picolibc-specific TLS apis as those don't work with the system C
   library.

 * -Dinternal-heap=8388608 allocates a stack chunk of memory for use
   by the picolibc `sbrk` implementation.

 * -Derrno-function=auto detects the native C library errno implementation.

 * -Dinitfini-array=false skips the picolibc constructor bits and lets
    the native C library do that initialization step.

 * -Dincludedir, -Dlibdir and -Dspecsdir set up the installation so it
   doesn't conflict with the native system.

Once built, you can install and use picolibc on the host:

```console
$ cc -I/usr/local/lib/picolibc/include hello-world.c \
	/usr/local/lib/picolibc/lib/libc.a
```
