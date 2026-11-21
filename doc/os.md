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

 * int getentropy(void *buffer, size_t length)

   + libc/ssp/stack/protector.c
   + libc/stdlib/arc4random.c

 * int gettimeofday(struct timeval *tv, struct timezone *tz)

   + libc/time/time.c

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

 * ssize_t read(int fd, void *buf, size_t count)

   + libc/search/hash.c
   + libc/search/hash_page.c
   + libc/stdio/fdopen.c
   + libc/stdio/freopen.c

 * struct dirent *readdir(DIR *dirp)

   + libc/posix/readdir_r.c

 * int sigprocmask(int how, sigset_t *set, sigset_t *oldset)

   + libc/search/hash_page.c

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
returns, it then calls `_exit`. The picolibc version of `raise` also
calls `_exit` for uncaught and un-ignored signals.

```c
__noreturn void _exit (int status);
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

`<unistd.h>` includes this header when it exists. It defines only what
the OS provides; anything left undefined is not provided.

 * **Option macros** (`_POSIX_TIMERS`, `_POSIX_THREADS`,
   `_POSIX_SEMAPHORES` and the other `_POSIX_` options) with the
   version of the standard the OS meets, such as `200809L` or
   `202405L`. picolibc adds the options it implements itself:
   `_POSIX_REGEXP` and `_POSIX2_C_BIND`; `_POSIX_ASYNCHRONOUS_IO` when
   picolibc was built with its own POSIX threads;
   `_POSIX_THREAD_SAFE_FUNCTIONS` when it was built with locking and
   the OS has threads; and `_POSIX_SPAWN` when the OS has processes.

 * **Option group macros** `__PICOLIBC_POSIX_<GROUP>` for the OS-level
   parts of the POSIX.1-2024 option groups that no option macro covers,
   with the version met: `C_LIB_EXT`, `DEVICE_IO`, `DEVICE_SPECIFIC`,
   `DEVICE_SPECIFIC_R`, `DYNAMIC_LINKING`, `FD_MGMT`, `FIFO`,
   `FIFO_FD`, `FILE_ATTRIBUTES`, `FILE_ATTRIBUTES_FD`, `FILE_SYSTEM`,
   `FILE_SYSTEM_EXT`, `FILE_SYSTEM_FD`, `MULTI_PROCESS`,
   `MULTI_PROCESS_FD`, `NETWORKING`, `PIPE`, `SIGNALS`,
   `SINGLE_PROCESS`, `SYMBOLIC_LINKS`, `SYMBOLIC_LINKS_FD`,
   `USER_GROUPS`, and `CLOFORK` for the POSIX.1-2024 close-on-fork
   flags.

 * **Realtime profiles** (IEEE 1003.13-2003): `_POSIX_AEP_REALTIME_MINIMAL`
   (PSE51), `_CONTROLLER` (PSE52), `_DEDICATED` (PSE53) or `_MULTI`
   (PSE54). A profile claim implies the smaller profiles and supplies
   what the profile's tables require of the OS and the header left
   undefined: its options, with the profile's value (1 for the
   constants), and its units of functionality, as the option groups
   or option macros that stand for them. It claims nothing a profile
   doesn't require, such as `_POSIX_MEMORY_PROTECTION` for PSE51.

 * **Constants** POSIX requires: `_POSIX_CHOWN_RESTRICTED`,
   `_POSIX_NO_TRUNC` and `_POSIX_VDISABLE`, and `_POSIX_JOB_CONTROL`,
   `_POSIX_SAVED_IDS` and `_POSIX_SHELL` where provided.

`<unistd.h>` then sets `_POSIX_VERSION` to the newest edition whose
required options and groups are all present. A system claiming a
profile without meeting a whole edition implements a subprofile
(POSIX.1-2024 XBD 2.1.5.1): `<unistd.h>` defines `_POSIX_SUBPROFILE`,
sets `_POSIX_VERSION` to `202405L`, the edition it profiles, and leaves
`_POSIX2_VERSION` undefined. These depend on picolibc's share as well as
the OS's, so the OS must not define `_POSIX_VERSION`, `_POSIX2_VERSION`
or `_POSIX_SUBPROFILE`; `<unistd.h>` stops with an error if it does. The header may be included in any compilation unit, so
it uses only macros visible there (Zephyr's, for instance, tests its
Kconfig `CONFIG_` symbols).

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
many through `RTSIG_MAX` and `sysconf(_SC_RTSIG_MAX)`.

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
