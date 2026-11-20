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
