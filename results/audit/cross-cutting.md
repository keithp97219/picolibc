# Cross-cutting findings

Problems that span headers, recorded by audit units rather than fixed.
Append yours under a heading naming the unit.

## From the pilot and earlier work (x86_64 host)

- `sys/syslimits.h` fixes `OPEN_MAX` at 64; on Linux the limit is
  RLIMIT_NOFILE (1024 or more). posix_spawn now uses `sysconf(_SC_OPEN_MAX)`.
- `PATH_MAX` is 1024 and `PIPE_BUF` 512 in `<limits.h>`; Linux allows 4096
  for both. glob reports ENAMETOOLONG past 1024; pathconf reports the
  `<limits.h>` values.
- `LOGIN_NAME_MAX` isn't defined (allowed when indeterminate).
- `sysconf(_SC_MQ_OPEN_MAX)` returns 8; Linux has no such limit.
- `MQ_PRIO_MAX` isn't in `<limits.h>` (allowed); sysconf reports 32768.

## From pthread-sync (aarch64 host)

- glibc 2.43 (Debian sid) declares pthread_mutex_clocklock, pthread_cond_clockwait,
  pthread_rwlock_clockrdlock and pthread_rwlock_clockwrlock only for _GNU_SOURCE,
  not for `_POSIX_C_SOURCE 202405L`. Native (-native) header tests for any
  POSIX 2024 addition need an `#ifdef __PICOLIBC__` guard or `_GNU_SOURCE`.
- glibc defines most `<pthread.h>` symbolic constants (PTHREAD_MUTEX_*,
  PTHREAD_PRIO_*, PTHREAD_PROCESS_*) as enumerators, not macros, so `#ifdef`
  checks for them only work against picolibc. Use `_Static_assert` to check them
  portably.
- `meson test --no-rebuild` leaves the ctype-diff custom targets
  (`test/test-ctype/*-out`) unbuilt, and test-wctype-compare, test-encode-compare
  and test-wcase-compare then fail with "No such file or directory". That's a
  harness artefact, not a bug. Run the full suite without `--no-rebuild`.
- Real-time scheduling behaviour (SCHED_FIFO/RR: mutex priority protocols, the
  rwlock writer preference, the scheduling-order clauses) can't be exercised on
  this host: the chroot user has RLIMIT_RTPRIO 0 and no CAP_SYS_NICE. Units
  touching TPS requirements will hit the same limit.

## From the signal unit (arm64 host)

- `_SC_NSIG` (POSIX.1-2024) isn't defined in `<unistd.h>` and sysconf doesn't handle
  it. `<signal.h>` requires SIGRTMAX < sysconf(_SC_NSIG). This belongs to
  unistd-config; it should return 64 (`NSIG_MAX`, now 64 on every target; see the
  signal unit's limits commit).
- `NSIG` is visible whatever the feature-test macros are, because `<signal.h>` tests
  `#ifdef __GNU_VISIBLE` and that macro is always defined (as 0 or 1). The same
  `#ifdef __X_VISIBLE` mistake is in stdio.h:333, 348, 354, 366 and pwd.h:57.
- `mcontext_t`/`ucontext_t` are required in `<signal.h>` by Issue 8 and are missing.
  They need per-architecture layouts that match the kernel's signal frame.

## From net-names (arm64 host)

- **`git stash` is shared by every worktree.** `refs/stash` belongs to the
  repository, not the worktree, so two agents that each `git stash push`
  and `git stash pop` around a revert-and-retest can swap their changes.
  This happened between net-names and signal (net-names' inet_pton fix
  landed in the signal worktree and signal's sigaction fix in net-names);
  the coordinator sorted it out. To prove a fix, reverse the commit's diff
  (`git show <fix> -- libc libos | git apply -R`), test, then reapply, or
  save a patch file. Never use `git stash` with parallel worktrees.
- **Name lookups differ from glibc by design.** picolibc's netdb reads only
  `/etc/hosts`, `/etc/services`, `/etc/protocols` and `/etc/networks` (with
  built-in fallbacks), never DNS, so "this address has no name" or "this
  name doesn't resolve" holds for picolibc but not for a `-native` glibc
  build on a host with `hosts: files dns`. Tests that need a missing name
  should use the reserved `.invalid` TLD, and checks that an address has no
  name should run only under `__PICOLIBC__` after scanning `gethostent()`
  for it. Any unit that touches name lookups (syslog's host name, for
  example) should follow this.
- **glibc doesn't define `NI_NUMERICSCOPE`**, which POSIX requires in
  `<netdb.h>`. Tests that use it need `#ifdef NI_NUMERICSCOPE`.

## From the x86_64 host (after batch 1)

- About a third of the test-posix tests use `_DEFAULT_SOURCE` or
  `_GNU_SOURCE`, and most XSI tests use `_XOPEN_SOURCE 700`, so they can't
  catch POSIX symbols hidden behind extension macros. 120 of the 128 compile
  unchanged against picolibc with strict POSIX.1-2024 plus XSI 800. The
  x86_64 host converts them after batch 2 is merged; units shouldn't
  convert tests outside their own headers.
- Names leaking in strict mode: `<stdio.h>` declares `setbuffer` and
  `setlinebuf` unconditionally, and `<sys/wait.h>` declares `wait3`. These
  are fixed on the x86_64 host.

## From pthread-threads (aarch64 host)

- **Library functions that call cancellation points.** XSH 2.9.5.2: "An
  implementation shall not introduce cancellation points into any other
  functions". Any library function outside the two cancellation-point lists
  that calls close(), read(), write(), open(), waitpid(), nanosleep() and so
  on becomes one in picolibc, because those wrappers act on a pending request
  (and leak whatever the caller held). This unit fixed if_nametoindex,
  if_indextoname and if_nameindex (net-names area) and lio_listio's wait (aio
  area); sem_open and shm_open already use `_cancel_disable`. Other units
  adding code that calls a cancellation point internally should use the raw
  system call or `_cancel_disable`/`_cancel_restore` (libos/linux/local-cancel.h),
  or pthread_setcancelstate outside libos. A library function that *is* a
  cancellation point and holds a lock or allocation across one needs a
  cleanup handler (aio_suspend did not; fixed).
- **glibc 2.43 makes if_nameindex a cancellation point** and returns at once
  from aio_suspend when every list entry is NULL. Native tests guard such checks
  with `__PICOLIBC__`.
- **glibc 2.43 doesn't define PTHREAD_NULL** (new in POSIX.1-2024), and defines
  `_POSIX_THREAD_CPUTIME` as 0 (decide at run time), so `> 0` gates skip CPU-time
  checks in native builds; use `>= 0`.
- **aio on unseekable files.** picolibc's aio worker uses pread, so aio_read on a
  pipe or socket completes at once with ESPIPE; glibc falls back to read and
  blocks. POSIX describes the transfer "as if lseek() were called immediately
  prior", so ESPIPE is defensible, but it differs from glibc. For the aio unit.
- **pthread_getconcurrency/pthread_setconcurrency and
  pthread_attr_getstackaddr/setstackaddr** are no longer in POSIX.1-2024
  (removed in Issue 8 and Issue 7 respectively). picolibc still declares the
  concurrency pair under `__XSI_VISIBLE`, which is harmless since `pthread_` is a
  reserved prefix in <pthread.h>.

## From time (arm64 host)

- **setenv freed memory while holding the libc lock** (libc/stdlib/setenv.c).
  The second setenv that grows the environment array freed the old array
  under `__LIBC_LOCK`, which free also takes. With the tests' non-recursive
  locks (test/lock-valid.c) that asserted, and the assert recursed until
  the stack overflowed (SIGSEGV). Fixed here (commit "stdlib: Free the old
  environment array after releasing the lock in setenv") because
  test-getdate_err hit it; the stdlib unit should know.
- **sysconf limits that come from the kernel.** `sysconf(_SC_DELAYTIMER_MAX)`
  returned the POSIX minimum 32 from libos/fallback while Linux counts
  timer overruns up to INT_MAX; fixed in libos/linux/sysconf.c. The same
  pattern (fallback minimum vs. the kernel's real value) may hold for
  other `_SC_*` names; unistd-config should check them against Linux.
- **glibc 2.43 differs from POSIX.1-2024 in several time interfaces**, so
  tests guard these with `__PICOLIBC__`: strftime ignores the `+` flag and
  gives one digit for `%C` of years below 1000 and doesn't set ERANGE;
  strptime ignores field widths, accepts `%M` 60 and `%S` 61 and doesn't set
  tm_isdst from `%Z`; localtime doesn't apply POSIX TZ rules before 1970
  and misses a DST transition that falls in the next year
  (`EST5EDT,0/0,J365/25`); getdate keeps the current day for a month given
  alone and reports error 3, not 2, for a missing template file;
  `tm_gmtoff`/`tm_zone` are `__tm_gmtoff`/`__tm_zone` without
  `_DEFAULT_SOURCE`, and its strftime `%z`/`%Z` read them rather than TZ.
- **No portable fstat in libc/time.** libos/semihost has no fstat, so
  generic library code can't call it. getdate detects a directory template
  through the EISDIR read error instead; other non-regular files (FIFOs,
  devices) aren't reported as getdate error 4.
- **`struct tm` grew** (tm_gmtoff, tm_zone): a layout change for anyone
  mixing objects built against old and new headers.

## From files (arm64 host)

- **No close-on-fork.** O_CLOFORK, FD_CLOFORK and F_DUPFD_CLOFORK (Issue 8) are
  missing, and Linux has no such descriptor flag, so supporting them means a
  user-space table kept by every call that makes, duplicates or closes descriptors
  (open, openat, dup, dup2, dup3, fcntl, pipe2, socket, accept4, posix_spawn) and
  applied in fork. The same gap shows up for dup3/pipe2 (unistd-files),
  SOCK_CLOFORK/accept4 (net-sockets) and posix_spawn. Needs a decision before any
  unit adds the names.
- **O_SEARCH is O_PATH, and Linux still checks search permission through O_PATH
  directory descriptors.** Every *at function says the check is skipped for an
  O_SEARCH descriptor; that can't hold on Linux (verified with a glibc O_PATH
  program). Units with *at functions (unistd-files, dirs) should mark it NOT
  TESTABLE rather than chase it.
- **`_at_fd_to_linux` now maps -100 (Linux's AT_FDCWD) to an invalid descriptor**
  (commit "libos/linux: Treat Linux's AT_FDCWD value as a bad descriptor"). That
  changes faccessat, fchownat, linkat, unlinkat, renameat, readlinkat, symlinkat
  and the rest: picolibc's AT_FDCWD is -2, so -100 now gives EBADF for relative
  paths, as POSIX requires.
- **O_ACCMODE now includes O_EXEC/O_SEARCH.** Code in other units that does
  `flags & O_ACCMODE` and hands the result to Linux must mask with
  `O_RDONLY | O_WRONLY | O_RDWR` instead (fixed here in `_open_flags_*` and
  mq_open; posix_openpt's check now lets O_SEARCH through).
- **st_dev/st_rdev use `major << 8 | minor`** (libos/linux/_statbuf.c), which
  isn't one-to-one for minors above 255, doesn't match the kernel's encoding that
  mknod's dev argument uses, and doesn't match the BSD `major()`/`minor()` macros.
  Not seen failing on this host (no minor above 255, no root).
- **Linux lets unprivileged users mknod a 0:0 character device** (the overlayfs
  whiteout, since 5.8), as well as regular files and sockets. A test of "mknod of
  a device needs privileges" must use another device number (the files unit uses
  1:3).
- **glibc header gaps** for this area: no O_EXEC, O_SEARCH, O_TTY_INIT or the
  CLOFORK names at all; F_OFD_*, F_GETOWN_EX/F_SETOWN_EX and struct f_owner_ex
  only with `_GNU_SOURCE`; S_ISVTX and mknod only with `_XOPEN_SOURCE`; realpath
  isn't declared under `_POSIX_C_SOURCE 202405L`. glibc's posix_fallocate emulates
  unsupported file systems by writing, so ENOTSUP checks are picolibc-only.
- `<sys/stat.h>` declares lstat and mknod under `defined(__BSD_VISIBLE)`, which is
  always true: one more instance of the `#ifdef __X_VISIBLE` mistake already listed.

## From net-sockets (arm64 host)

- **Host security software kills some test binaries.** This host runs
  endpoint security software. It sent SIGKILL, in the middle of a
  socket call, to test programs named `*.elf` that combined many socket calls
  with signals repeatedly interrupting a blocking call (a POSIX timer, or a
  child sending SIGUSR1); the same binary without the `.elf` suffix ran to the
  end every time. meson shows it as "killed by signal 9 SIGKILL" with no
  output. The net-sockets tests were restructured until it stopped (fork-based
  interrupter, recvfrom split from recv). Other units writing EINTR tests
  around sockets may hit it; nothing on the host should be changed.
- **Every header leaks wint_t and MALLOC_ALIGNMENT in strict mode.**
  `<sys/_types.h>` asks `<stddef.h>` for `__need_wint_t`, and `<sys/config.h>`
  defines `MALLOC_ALIGNMENT`, so even `<sys/uio.h>` with only
  `_POSIX_C_SOURCE 202405L` defines both. (Every header also gets size_t this
  way, which is harmless but means "header X defines size_t" can't be tested
  in isolation.)
- **sysconf(_SC_IOV_MAX) in libos/fallback** returns `_XOPEN_IOV_MAX` (16) while
  `<limits.h>` defines IOV_MAX as 1024 on every target. Fixed for Linux only
  (libos/linux/sysconf.c returns IOV_MAX); unistd-config should decide for the
  fallback (POSIX: when IOV_MAX is defined, it is the actual limit).
- **FD_SETSIZE is 64**, like OPEN_MAX; Linux descriptors routinely go higher, so
  select can't watch them. Changing it changes the size of fd_set.
- **Library-level argument checks in cancellation points** must come after
  `_cancel_enter()`: test-pthread_testcancel probes recvmsg and sendmsg with an
  empty msghdr, and an early EMSGSIZE return skipped the pending cancel. Units
  adding validation to readv/writev/sendmsg-style wrappers should check after
  entering the cancellation point.
- **glibc 2.43 gaps for this area** (native tests guard them): accept4, ppoll,
  SO_DOMAIN and SO_PROTOCOL need `_GNU_SOURCE`; `<poll.h>` doesn't define
  sigset_t; msg_iovlen, msg_controllen and cmsg_len are size_t;
  CMSG_NXTHDR(mhdr, NULL) crashes; select accepts nfds > FD_SETSIZE; and with
  64-bit time on 32-bit targets struct timeval's tv_usec isn't suseconds_t.

## From termios (arm64 host)

- **O_TTY_INIT = 0 is not conforming on Linux.** The files unit asked termios to
  assess this. Linux serial drivers keep termios between opens, so non-standard flags
  (IUCLC, OLCUC, XCASE, the line discipline) survive from the last user, and a zero
  O_TTY_INIT promises a reset that never happens. A fix means a non-zero, picolibc-only
  O_TTY_INIT that open/openat handle in user space by clearing the Linux-only flags on
  terminals other than ptys. Whether the device is open elsewhere doesn't matter, since
  POSIX leaves that case unspecified. It needs a decision on the flag value and the
  list of flags, and it can't be tested without a serial device. Separately, Linux
  never resets a pty subsidiary on reopen (it does so only when the manager is opened),
  so the pty half of that open() rule can't hold either.
- **sysconf limits from libos/fallback that are too small for Linux:**
  `_SC_TTY_NAME_MAX` was 9, shorter than `/dev/pts/N`; fixed here to 32 in
  libos/linux/sysconf.c. This is the same pattern as `_SC_DELAYTIMER_MAX` (time unit);
  unistd-config should check the remaining `_SC_*` values against Linux.
- **The terminal access check (SIGTTOU/EIO) is missing for tcsetwinsize** in the
  kernel (TIOCSWINSZ), and may be missing for other new Issue 8 calls. Linux applies it
  only to the ioctls listed in tty_ioctl and n_tty_ioctl_helper.
- **glibc 2.43 gaps for this area:** no tcgetwinsize/tcsetwinsize; `struct winsize`
  only in `<sys/ioctl.h>`; ptsname_r crashes on a null name (and declares it non-null,
  so pass NULL through a volatile pointer); ttyname_r returns ENODEV, not ERANGE, for a
  buffer one byte short; no O_TTY_INIT.
- **Test-writing lesson for process-group tests:** a grandchild that waits to be
  orphaned must get its parent's PID from before the fork. `getppid()` called in the
  child can already return the new reaper, and then the child waits forever. When
  `poll` on a tty is used as a readiness wait, note that with VMIN>1 and VTIME=0 it
  waits for MIN bytes, not one.
- **The posix_openpt O_ACCMODE note from the files unit is resolved:** posix_openpt now
  rejects O_SEARCH and O_EXEC with EINVAL.

## From dirs (arm64 host)

- **Public structures with plain member names.** `DIR` had members named `fd`,
  `offset`, `count`, `dirent`, `buf` and `align`, so an application macro with
  one of those names, defined before `#include <dirent.h>`, broke the header.
  POSIX reserves only the header's own prefixes (`d_` here). Fixed for DIR
  (`__dd_` names). Other units should check structures that picolibc defines but
  POSIX leaves opaque or only partly specifies (FILE, glob_t extras, the
  pthread types, sem_t, mqd_t and so on) the same way: define a macro with each
  member name before including the header.
- **The Linux configurations skipped three picolibc tests.**
  `test/test-posix/test-dirent/meson.build` skipped the picolibc builds of
  test-readdir, test-dirfd and test-scandir whenever `native-tests` was on. Every
  Linux configuration turns it on, so those tests never ran against picolibc on
  Linux. The skip was meant for the `use-stdlib` native configuration, and that
  is now the condition. No other test directory has the same pattern.
- **32-bit long against 64-bit kernel offsets.** telldir returned -1/EOVERFLOW
  for directory offsets above LONG_MAX on armhf. That always happens under
  qemu-user on x86_64, because the host kernel gives 64-bit ext4 hashes, and on
  real hardware it happens for tmpfs offsets set with lseek. It is now fixed with
  a cookie table. Any other interface that squeezes an off_t into long (ftell,
  fseek) has the same shape and is already covered by the off_t/long rules there.
  With the fix, test-telldir should now pass under qemu on armhf too, instead of
  skipping.
- **glibc 2.43 gaps in this area** (guarded with `__GLIBC__` in the tests):
  - It has no posix_getdents, struct posix_dent, reclen_t, DT_MQ/DT_SEM/DT_SHM
    or FTW_XDEV.
  - It defines DT_BLK…DT_UNKNOWN only with `_DEFAULT_SOURCE`, although POSIX.1-2024
    requires them in `<dirent.h>`.
  - Its FTW_* values are enumerators, so they can't be used in `#if`.
  - Its `<libgen.h>` defines basename as `__xpg_basename`, so after
    `#undef basename` nothing is declared.
  - nftw skips every directory it has already visited, not only the ones that
    would be inside themselves: a link to a sibling directory isn't walked, and a
    link to a parent isn't reported without FTW_DEPTH.
  - glibc's FTW_ACTIONRETVAL is 16, the same value as picolibc's new FTW_XDEV.
    This is harmless, but worth knowing when comparing traces.

## From memory-sched (arm64 host)

- **`<sys/types.h>` includes all of `<stdint.h>`** (libc/include/sys/types.h:270,
  unconditionally, plus `__Long`/`__ULong`). Every header that includes
  `<sys/types.h>` therefore defines INT8_MAX, SIZE_MAX, INTPTR_MAX, UINT64_C and the
  rest in strict mode, names POSIX reserves for `<stdint.h>` (and `<sys/types.h>`
  itself may only add `_t` names). `<sched.h>` was fixed here by declaring pid_t
  and time_t itself; `<pthread.h>`, `<spawn.h>` and the other includers still leak.
  Dropping the include from `<sys/types.h>` needs a check of what relies on it.
- **Plain parameter names in public prototypes.** `<time.h>` declares
  `timer_gettime(timer_t timerid, struct itimerspec *value)`,
  `timer_settime(..., int flags, ... value, ... ovalue)`, `timer_create(...,
  timerid)` and `clock_nanosleep(clockid_t clock_id, int flags, ...)`. An
  application macro named `value`, `flags` or `timerid` defined before the include
  breaks the header (found through `<semaphore.h>`, which includes `<time.h>`).
  Prototypes in installed headers should use reserved (`__`) names or none. The time
  unit's headers, and probably others, should be swept for this.
- **glibc 2.43 gaps in this area** (tests guard them with `__GLIBC__`):
  - `<sys/mman.h>` doesn't define O_RDONLY, O_RDWR, O_CREAT, O_EXCL or O_TRUNC, and
    defines MCL_ONFAULT in strict mode.
  - `<semaphore.h>` doesn't define O_CREAT or O_EXCL.
  - posix_madvise(POSIX_MADV_DONTNEED) returns 0 without looking at the range, so
    it never reports ENOMEM.
  - shm_open("/.") and shm_open("/..") open `/dev/shm` and `/dev`.
  - mmap with MAP_SHARED | MAP_PRIVATE maps a file shared (Linux's
    MAP_SHARED_VALIDATE); anonymous memory gets EINVAL from the kernel.
- **Linux scheduling calls act on one thread, not the process.** sched_setparam,
  sched_setscheduler, sched_getparam and sched_getscheduler take a thread ID on
  Linux, so pid 0 or getpid() changes or reports only the calling (or main) thread,
  where POSIX describes the process. glibc and musl behave the same. Fixing it would
  mean walking /proc/<pid>/task; nothing in picolibc tries. Not testable here anyway
  for anything but SCHED_OTHER priority 0.

## From sysv-ipc (arm64 host)

- **`<sys/types.h>` includes all of `<stdint.h>`** (libc/include/sys/types.h:270), so
  every header that includes it defines INT8_MAX ... UINTMAX_MAX, INTn_C(),
  SIZE_MAX, PTRDIFF_MIN/MAX, SIG_ATOMIC_MIN/MAX, WCHAR_MIN/MAX and WINT_MIN/MAX in
  strict mode, plus `__Long`/`__ULong`. Seen with `-E -dM` on `<sys/ipc.h>`,
  `<sys/msg.h>`, `<sys/sem.h>` and `<sys/shm.h>` under `_POSIX_C_SOURCE 202405L` and
  `_XOPEN_SOURCE 800`. The `_MAX`/`_MIN` macros aren't reserved for those headers. The
  typedefs (int8_t and so on) end in `_t`, so they are allowed. This joins the known
  wint_t/MALLOC_ALIGNMENT leak; one fix in `<sys/types.h>` would cover every header.
- **SHMLBA is a compile-time constant**: 4096 everywhere except 16384 on 32-bit ARM.
  Linux's SHMLBA is PAGE_SIZE on arm64 (and 4 * PAGE_SIZE for compat tasks), so on
  arm64 kernels with 16 KiB or 64 KiB pages, shmat(SHM_RND) rounds to a different value
  from the one the application sees. glibc uses `__getpagesize()` (aarch64) and
  `__getpagesize() << 2` (arm). It can't be shown on this 4 KiB-page host. A fix needs a
  reserved-name page-size function that `<sys/shm.h>` can call, so it is left for a
  decision. Any other header constant that copies the kernel's page size has the same
  problem.
- **Linux SysV IPC differs from POSIX in ways a library can't fix cheaply** (confirmed
  with glibc probes): an unprivileged owner can raise msg_qbytes up to msgmnb (POSIX:
  only a privileged process may raise it), and a shared memory id removed with IPC_RMID
  while attached stays valid for shmat and shmctl until the last detach (POSIX removes
  the identifier at once). Units testing other kernel objects (POSIX message queues,
  shm_open) should expect the same kind of difference.
- **glibc 2.43 gaps for this area** (the tests guard them with `__GLIBC__`): no
  SHM_FAILED and no intptr_t in `<sys/shm.h>` (both new in Issue 8); semid_ds's
  sem_nsems is `unsigned long`, not `unsigned short`; semop truncates nsops to 32 bits on
  64-bit targets, so a count of 2^32 + 1 does one operation instead of failing with
  E2BIG.
- **Kernel timestamps can trail time() by a tick.** The IPC timestamps come from
  `ktime_get_real_seconds()`, which can still show the previous second for up to one
  tick after CLOCK_REALTIME has moved on. Tests that compare a kernel timestamp with a
  `time(NULL)` taken just before should allow one second of slack, or wait two seconds
  when they need two timestamps to differ.

## From threads-c11 (aarch64 host)

- **Linux's nanosleep remaining time can exceed the request.** The kernel
  computes the remaining time from the timer's hard expiry, which includes
  the thread's timer slack (50 us by default, PR_SET_TIMERSLACK). A sleep
  interrupted within the slack of its start reported 30.000008 s left of a
  30 s request. POSIX (nanosleep, thrd_sleep): "the requested interval minus
  the time actually slept". picolibc and glibc both pass the kernel's value
  through. test-thrd_sleep allows 1 ms over; clamping in nanosleep would be
  the time unit's call.
- **glibc 2.43's thrd_sleep doesn't set errno** for EINTR or EINVAL (it
  returns clock_nanosleep's error code as -1/-2). POSIX.1-2024 CX: "If it
  returns a negative value, it shall set errno to indicate the error."
  Native tests guard errno checks with `#ifndef __GLIBC__`.
- **glibc 2.43's thrd_create returns thrd_error when the stack can't be
  mapped** (pthread_create turns ENOMEM into EAGAIN, which glibc's
  thrd_create maps to thrd_error) where C says thrd_nomem.
- **picolibc has no RLIMIT_NPROC**, so the Linux thread limit (which counts
  threads against RLIMIT_NPROC) can't be lowered from a picolibc program;
  tests that need to hit the thread limit can only run against glibc. For the
  resource-wait unit.
- **Library code holding `__LIBC_LOCK` across a user callback** hangs every
  other thread's libc use if the callback blocks or is cancelled. call_once
  did (now fixed on Linux). Other generic code that calls application
  functions with the libc lock held may have the same problem; not surveyed.

## From unistd-process (arm64 host)

- **exit() doesn't flush fopen/fdopen streams on the Linux configurations.** `stdio-exit-flush`
  defaults to false (meson_options.txt), so `_bufio_exit_flush` is never registered and only the
  standard streams get flushed. ISO C (exit): "all open streams with unwritten buffered data are
  flushed". Seen with a fully buffered fdopen stream: after exit() the reader got the atexit
  output and nothing else. For the stdio or stdlib unit: either turn the option on for hosted
  (Linux) builds or document it.
- **Linux keeps per-thread nice values.** nice is fixed here (it now sets every thread), but
  setpriority(PRIO_PROCESS, 0 or getpid(), ...) still changes only the calling thread, and
  getpriority reports only the calling thread's value. POSIX describes the process. For the
  resource-wait unit; libos/linux/nice.c shows how to walk the thread list.
- **Credential changes with threads aren't async-signal-safe.** `_pthread_setxid` takes
  `_pthread_all_lock`, so setuid and friends called from a signal handler that interrupted
  pthread_create or pthread_exit in the same thread deadlock. nice now takes the same lock (but nice
  isn't async-signal-safe). Blocking signals while the lock is held in pthread_create, pthread_exit
  and the setxid path would fix it; pthread-threads area.
- **Plain parameter names in strict-mode `<unistd.h>` prototypes:** `dup3(..., int flags)` and
  `pipe2(int __fildes[2], int flags)`. An application macro named `flags` breaks the header. For
  unistd-files (getlogin_r's `name`/`namesize` were fixed here).
- **Declared but not defined:** setpgrp and ualarm are declared under `_DEFAULT_SOURCE` (and setpgrp
  was declared for XSI up to Issue 7), but nothing defines them, so programs fail to link.
  `libos/linux/sleep.c` is an empty file (licence only) that no build file lists.
- **`environ` in `<unistd.h>`:** declared for `_POSIX_C_SOURCE >= 202405` on purpose (7e8126df52),
  but the 2024 `<unistd.h>` page doesn't list it and XBD says every identifier except environ is in
  some header. Harmless, but a namespace question for the maintainer.
- **glibc 2.43 gaps met here** (tests guard them with `__GLIBC__`): nice overflows for large
  increments and changes only the calling thread; tcsetpgrp passes on Linux's ESRCH (should be
  EPERM) and ENOTTY for an orphaned group (should be EIO); on 32-bit targets setitimer fails with
  EOVERFLOW for UINT_MAX seconds, so alarm(UINT_MAX) silently sets no alarm.
- **Test-writing note:** tests that exec should run their own binary through `/proc/self/exe` with
  a mode argument (test-execl.c, test-setpgid.c, test-execl-thread.c do this), and pass state to the
  new image in argv. To detect that a library call hangs in the child of `_Fork`, poll
  `waitpid(WNOHANG)` with a deadline and SIGKILL only that pid.

## From aio (arm64 host)

- **`<time.h>` defines CLK_TCK in strict mode** (libc/include/time.h:95-96, unconditionally). CLK_TCK was removed from POSIX long ago and isn't reserved for `<time.h>`. It was found through `<aio.h>`, which includes `<time.h>`. For the time unit.
- **sysconf aio limits:** libos/fallback/sysconf.c now reports AIO_LISTIO_MAX (64) and AIO_PRIO_DELTA_MAX (20) from `<limits.h>`, and -1 for _SC_AIO_MAX. Before, it gave the POSIX minimums. unistd-config should know this file changed. The same "fallback minimum versus the real value" pattern as _SC_DELAYTIMER_MAX, _SC_IOV_MAX and _SC_TTY_NAME_MAX.
- **Library code that uses a descriptor later, from another thread,** must duplicate it when the call is made. aio used the caller's descriptor whenever the request's thread ran, so a close() in between gave EBADF or hit a reused number. Any other deferred-work code (notification threads, timers) that keeps a descriptor number has the same shape.
- **Condition variables can't give EINTR.** A "shall fail [EINTR]" wait built on pthread_cond_wait is never interrupted by a signal handler. aio now waits on a semaphore (sem_wait returns EINTR). Other generic code that blocks on a condition variable where POSIX promises EINTR should be checked.
- **glibc 2.43 aio gaps** (the tests guard them with `__GLIBC__`):
  - A request queued before a close() fails with EBADF.
  - aio_suspend ignores a pending cancellation when a listed request has completed.
  - lio_listio doesn't check sig with LIO_NOWAIT.
  - aio_fsync checks aio_reqprio.
  - lio_listio(LIO_WAIT) interrupted by a signal returns EINTR, and then one of its request threads crashes with SIGSEGV.
  - The SIGXFSZ from an aio_write past RLIMIT_FSIZE never arrives.
  - lio_listio's list elements aren't restrict-qualified in its prototype.
  - AIO_LISTIO_MAX isn't defined in `<limits.h>` (use sysconf).
- **Test-writing lesson:** with glibc, a child forked after the parent has used aio waits forever for request threads it doesn't have. Run fork-based aio checks before any other aio call in the test, and give the child its own alarm(), since alarms aren't inherited.

## From regex-wordexp (arm64 host)

- **dash ignores IFS from its environment** (allowed by XCU 2.5.3), so any
  library code that runs `/bin/sh` to expand words on the application's behalf
  (wordexp; popen/system don't) has to pass IFS explicitly. wordexp now does.
- **wordexp depends on `/bin/sh` for every expansion**, not just command
  substitution, so on a system without a shell wordexp always fails with
  WRDE_SYNTAX. If picolibc ever targets a Linux system without `/bin/sh`, that
  is a design question, not a bug to fix in a test.
- **glibc 2.43 gaps for this area** (the tests guard them with `__GLIBC__`):
  regoff_t is `int`, too narrow for ptrdiff_t on 64-bit targets (POSIX:
  "can hold the largest value that can be stored in either a ptrdiff_t type or
  a ssize_t type"); no REG_MINIMAL; nested back-references and inner
  subexpressions report stale matches; wordexp leaves we_wordc alone after
  WRDE_BADCHAR, rejects `${x:-y}` under WRDE_UNDEF, mis-splits `x${S}y`, and
  doesn't glob the result of a parameter expansion.
- **Libraries running a child on the caller's behalf and SIGCHLD = SIG_IGN.**
  With SIGCHLD ignored, the kernel reaps children itself and waitpid fails with
  ECHILD. wordexp then reports WRDE_SYNTAX although the child succeeded (read,
  not run). system() and popen()/pclose() have the same exposure; the stdio and
  unistd-process units may want to check how those handle it.

## From the arm64 coordinator: clang native build of the base (d14119a43a)

`CC=clang scripts/do-native-configure -Dwerror=true` then `ninja -k 0` on aarch64 (Debian clang 21.1.8, glibc 2.43) fails at the audit base, before any batch-6 change:

- `libc/native/stat_host.c:42-43`: `_Static_assert` with no message is a C23 extension (`-Werror,-Wc23-extensions`). The native library doesn't build, so about 645 test links fail after it.
- `test/test-stdbit/test-stdc-leading-ones.c`: shift count >= width of type (`-Wshift-count-overflow`), 7 places.
- `test/test-posix/test-fnmatch/test-fnmatch.c:76`: `!FNM_PATHNAME` etc. on glibc's `(1 << n)` macros gives `-Wtautological-constant-compare` (3 places, `-native` entry). Write these as `FNM_X != 0`, as the plan's clang rule says.
- `test/test-math/test-iseqsig.c`: implicit float to double/long double promotion (`-Wdouble-promotion`), 6 places.
- `test/test-math/test-tgmath.c`: against glibc, `dadd`, `dsub`, `dmul`, `ddiv`, `dfma` and `dsqrt` are undeclared and `__STDC_VERSION_TGMATH_H__` has the wrong value. clang 21 also crashes in the aarch64 backend at test-tgmath.c:294 ("Cannot select: i32 = setcc ... Constant:i128<0>"), which is a clang bug, so the test may need to avoid that expression on aarch64 clang.

Until those are fixed, the arm64 units check their own new or changed files with `/tmp/arm64run/clang-check` (clang `-fsyntax-only -Werror` with meson's gcc flags, for the picolibc and `-native` entries) rather than a full clang build.

## From the arm64 coordinator: clang native build of claude/fix-clang-native (8b8b1780b2) on aarch64

`CC=clang CXX=clang++ scripts/do-native-configure -Dwerror=true`, `ninja -k 0`, Debian clang 21.1.8 with libclang-rt-21-dev, glibc 2.43, real aarch64 hardware:

- **Build:** one failure left. clang 21's aarch64 backend crashes on `test/test-math/test-tgmath.c:303` ("fatal error: error in backend: Cannot select: i32 = setcc ..., Constant:i128<0>, seteq"). f292931a18 skips test-tgmath only when `<tgmath.h>` isn't C23, and picolibc's is, so the picolibc build still compiles the `__int128` comparison. This is a clang bug that x86_64 doesn't hit; the test needs to avoid that comparison (or skip it) for clang on aarch64.
- **CXX:** with `CC=clang` alone, meson picks g++ for C++, and 52b2f08287's `-fno-sanitize-link-runtime` reaches g++ ("unrecognized command-line option"), failing 4 C++ compiles. With `CXX=clang++` they build. Checking the C++ compiler for that flag separately would make mixed setups work.
- **Runtime:** without libclang-rt-21-dev every `-native` test link fails (no `libclang_rt.ubsan_standalone.a` for `-fsanitize=bounds`). CI hosts need that package.
- **Tests** (everything but test-tgmath, `meson test -t 4`): Ok 615, Expected Fail 6, Fail 2, Skipped 38.
  - `test-math-errhandling` (picolibc build) fails with clang only; gcc passes it on the same commit. It reports "Exceptions are not supposed to be supported, but 0/0 raises FE_INVALID" (and FE_DIVBYZERO, FE_UNDERFLOW, FE_OVERFLOW, and FE_INVALID from the sNaN cases), so under clang the build's idea of whether exceptions are supported disagrees with what aarch64 does.
  - `test-mktemp` (picolibc build) fails with both clang and gcc ("contents: (char)c == *s"), so it's a native-configuration problem on aarch64, not a clang one.

## From unistd-files (arm64 host)

- **Descriptor functions built on `AT_EMPTY_PATH` take picolibc's AT_FDCWD (-2) or
  Linux's (-100) as the working directory.** fchown called fchownat(fd, "", ...,
  AT_EMPTY_PATH), so fchown(-2, ...) changed the owner of the current directory
  (fixed here: it rejects negative descriptors and calls the system call itself).
  The same shape is left in two other units' code: libos/linux/fstat.c passes the
  descriptor straight to statx(fd, "", AT_EMPTY_PATH), so fstat(-100, &st)
  succeeds with the working directory's attributes instead of EBADF (files unit),
  and libos/linux/fexecve.c passes it to execveat(fd, "", ..., AT_EMPTY_PATH), so
  fexecve(-100, ...) tries to run the working directory and gives EACCES, not
  EBADF (unistd-process). Inferred by reading; not run. The fix is the same: fail
  with EBADF for any negative descriptor first.
- **read/recv on an unconnected AF_UNIX stream socket give EINVAL.** Linux's
  unix_stream_read_generic returns EINVAL when the socket isn't connected; POSIX
  (read, recv) says "shall fail [ENOTCONN]". TCP sockets give ENOTCONN. picolibc
  passes the kernel's value through in read, recv and recvfrom (glibc too).
  test-read uses an AF_INET socket for its ENOTCONN check. For net-sockets.
- **`<stdio.h>`'s FILE members have plain names** (`unget`, `flags`, `put`, `get`,
  `flush`, `lock`, and `close` in struct __file_close; libc/include/stdio.h:91-117). An application macro named `flags`
  defined before `#include <stdio.h>` breaks the header, as DIR's members did
  (dirs unit). Found while checking `<unistd.h>`'s dup3/pipe2 parameter names; the
  unistd header tests #undef their macros before including `<stdio.h>`. For stdio.
- **`<unistd.h>` and `<limits.h>` constants that unistd-config should look at:**
  - `_POSIX_CHOWN_RESTRICTED`, `_POSIX_NO_TRUNC` and `_POSIX_VDISABLE` aren't
    defined anywhere, although POSIX.1-2024 requires each "with a value other
    than -1". The profile checks in sys/unistd.h (`defined(_POSIX_CHOWN_RESTRICTED)
    && ...`) therefore never succeed. Linux always restricts chown, never
    truncates names, and uses 0 for `_POSIX_VDISABLE` (`LINUX_POSIX_VDISABLE`).
  - `NGROUPS_MAX` isn't in `<limits.h>` (a Runtime Increasable Value, minimum
    `{_POSIX_NGROUPS_MAX}`), and `_POSIX_NGROUPS_MAX` is 0 where POSIX.1-2024 says 8.
    test-chown sizes its group list with getgroups(0, NULL) instead.
  - `optreset` is declared in `<unistd.h>` in strict mode (sys/unistd.h:322), a
    BSD getopt extension that isn't reserved for the header.
  - Changed here, for symlink's ENAMETOOLONG: pathconf/fpathconf(_PC_SYMLINK_MAX)
    on Linux now return 4095 (libos/linux/local-statfs.h) instead of falling
    through to the POSIX minimum 255; Linux accepts link contents up to its
    PATH_MAX of 4096 bytes with the null. `GETENTROPY_MAX` (256) is new in
    sys/syslimits.h.
- **`<unistd.h>` has no O_CLOFORK.** POSIX.1-2024 requires `<unistd.h>` to define
  O_CLOEXEC and O_CLOFORK; O_CLOEXEC is added here, O_CLOFORK waits on the
  close-on-fork decision already recorded by the files unit (dup3 and pipe2 are
  the `<unistd.h>` users).
- **Linux deviations a library can't cheaply fix** (noted, not fixed):
  - pwrite on an O_APPEND descriptor writes at the end of the file, not at the
    offset ("writes into a given position ... regardless of whether O_APPEND is
    set"). Linux 6.9 added RWF_NOAPPEND for pwritev2; this host's 5.10 has no way
    to do it short of clearing O_APPEND around the call, which races with other
    threads. The file offset itself is left alone, as required (checked).
  - symlink("", path2) fails with ENOENT; POSIX says path1 "shall be treated only
    as a string and shall not be validated as a pathname".
  - chown(path, -1, -1) succeeds on a file owned by someone else, since Linux
    changes nothing and checks nothing; POSIX lists EPERM when "The effective user
    ID does not match the owner of the file". glibc behaves the same.
- **glibc 2.43 gaps met here** (the tests guard them with `__GLIBC__`): pread and
  pwrite with a negative offset on a pipe give EINVAL (should be ESPIPE); unlink
  and unlinkat of a directory give EISDIR (should be EPERM); readlink and
  readlinkat with a zero size give EINVAL; getentropy past 256 bytes gives EIO
  (should be EINVAL) and `GETENTROPY_MAX` isn't defined; no posix_close or
  POSIX_CLOSE_RESTART; `<unistd.h>` doesn't define O_CLOEXEC; dup3, pipe2,
  SEEK_DATA and SEEK_HOLE need `_GNU_SOURCE`.
- **File system limits in tests:** ext4 rejects lseek past its s_maxbytes (16 TiB
  with 4 KiB blocks) with EINVAL, so a test can't put the file offset near
  OFF_MAX; and on this file system SEEK_DATA may report a sparse region as data.
  Tests of off_t limits should build their offsets from 2^40 or so plus a length,
  not seek to OFF_MAX.

## From users (arm64 host)

- **picolibc's strsep drops an empty last field.** `strsep` is `__strtok_r(*s, delim, s, 0)`
  (libc/string/strsep.c), and `__strtok_r` returns NULL when the remaining string is empty, so
  `strsep` on `"a:"` gives `"a"` and then NULL, where BSD and glibc give `"a"`, `""`, NULL. The
  old passwd parser used it, so an entry with an empty shell field (`name:x:1:1::/home:`) was
  "malformed" and ended every lookup with EINVAL. pwd and grp no longer use strsep; any other
  field splitter built on it loses trailing empty fields. strsep isn't POSIX (BSD extension);
  for the strings unit. Read in the source, not run on its own.
- **`ptrdiff_t / sizeof` is unsigned.** The old fgetgrent_r computed the room left for the
  member array as `((buf + size) - (char *)aligned) / sizeof(char *)`; when alignment pushed
  `aligned` past the end, the negative difference became a huge size_t and the array's NULL was
  written past the caller's buffer. Any code that divides a pointer difference by `sizeof`
  should check the sign first.
- **The user and group lookups now use getline (malloc).** libc/posix/pwd and grp read each line
  with getline and keep the non-reentrant functions' entry in allocated memory sized to fit.
  Targets without malloc can't use them (they couldn't read /etc/passwd before either).
- **getlogin has no login record to consult.** `<utmpx.h>` is an empty database by design
  (14ebe3b96b), so getlogin and getlogin_r keep answering with the user database name for
  `getuid()`, not "the name the user logged in under". glibc first reads
  `/proc/self/loginuid` (the audit login uid, which survives su and setuid) and then utmp;
  a libos/linux getlogin could do the former. Left for a decision (details in the users report).
- **glibc 2.43 differences met here:** getpwnam_r, getpwuid_r, getgrnam_r and getgrgid_r return
  ERANGE when *any* line before the match (or the whole file, for a missing entry) doesn't fit
  the buffer, so "not found with a small buffer" checks are `#ifndef __GLIBC__`; glibc's
  fgetpwent accepts a 4-field line (`short:x:1:1`) with empty dir and shell, and its
  fgetpwent/fgetgrent set errno at the end of the file.

## From resource-wait (arm64 host)

- **waitpid now clears a blocked, pending SIGCHLD** when it returns a status and no other child has one, as POSIX requires; Linux never does.
  - Tests that check SIGCHLD is still pending *after* collecting the only child are now wrong. test-sigaction and test-_exit were changed to look with `waitid(WNOWAIT)` first.
  - system() (which blocks SIGCHLD while it waits), posix_spawn users and wordexp now get this clearing. That is what POSIX intends, but it matters to the stdlib and stdio units.
- **nice now calls setpriority**, which sets every thread for the caller's own process. The unistd-process walk moved into libos/linux/setpriority.c.
- **NZERO (20) is now in `sys/syslimits.h` under `__XSI_VISIBLE`.** This is for unistd-config and the limits work.
- **gcc removes a malloc whose result is only compared with NULL.** The picolibc test builds don't pass `-fno-builtin`, so a test of ENOMEM from malloc passed or failed depending on the code around it. Store the result in a `void *volatile` object.
- **CMake's semihost library lacks several fakes that meson has** (access, closedir, fstatvfs, ftruncate, getcwd, getrlimit, opendir, readdir, setrlimit, statvfs). Tests that call them unconditionally (test-getrlimit) can't be listed in CMake. Gated tests are fine.
- **RLIM_INFINITY is INT64_MAX** while rlim_t is 64-bit unsigned. Changing it is an ABI decision.
- **glibc 2.43 gaps for this area:**
  - WCOREDUMP is defined only with `_DEFAULT_SOURCE`.
  - RLIM_INFINITY is a cast, so it can't be used in `#if`.
  - getpriority, setpriority and getrusage are declared in strict POSIX mode.
  - waitid passes idtype 3 (P_PIDFD) and P_PGID 0 to Linux.
  - waitpid doesn't clear a blocked SIGCHLD.
  - setpriority changes one thread.

## From the arm64 coordinator: clang native build of posix-spec-audit f6ce89fbd8 on aarch64

`CC=clang CXX=clang++ scripts/do-native-configure -Dwerror=true`, clang 21.1.8 with libclang-rt-21-dev, on real aarch64: the build is clean (the test-tgmath backend crash is gone). The full `meson test -t 4` gives Ok 1168, Expected Fail 6, **Fail 2**, Skipped 361:

- `test-math-errhandling` (picolibc build, exit 126) fails with clang only, as on 8b8b1780b2: the clang build believes floating-point exceptions aren't supported, but aarch64 raises FE_INVALID, FE_DIVBYZERO, FE_UNDERFLOW and FE_OVERFLOW (and FE_INVALID from the sNaN cases).
- `test-mktemp` (picolibc build, "contents: (char)c == *s") fails with gcc as well, as seen on 8b8b1780b2, so it is an aarch64 native-configuration problem rather than a clang one.

Until those are fixed, units on this host use the full clang native build and treat these two failures as known.
