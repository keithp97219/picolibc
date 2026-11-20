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
