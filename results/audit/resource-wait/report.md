# `<sys/resource.h>`, `<sys/wait.h>`, `<sys/utsname.h>`: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-resource-wait`, branch `audit/resource-wait`, based on d14119a43a (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43, and clang 21 for the clang check) as an unprivileged user: no capabilities, RLIMIT_NICE 0, RLIMIT_RTPRIO 0, soft RLIMIT_CORE 0 with an unlimited hard limit, `kernel.core_pattern` = `core`.

**Interfaces.** getrlimit, setrlimit, getrusage, getpriority, setpriority; wait, waitpid, waitid; uname; and the three headers' types, macros and prototypes. All nine functions are in Issue 8.
- Issue 8 moved getrlimit and setrlimit (except RLIMIT_CPU) from XSI to the base.
- getpriority, setpriority, getrusage, RLIMIT_CPU, PRIO_*, RUSAGE_*, struct rusage, WCONTINUED and WIFCONTINUED stay XSI.
- WCOREDUMP is new in Issue 8.
- wait3 isn't POSIX; it is checked only for its null-pointer bug.
- NZERO (`<limits.h>`, XSI) is included because getpriority and setpriority are defined in terms of it.

**Implementation:**
- `libos/linux/`: getrlimit.c, setrlimit.c, local-resource.h, the generated `machine/*/linux/linux-resource.h` RLIMIT_ mapping, getrusage.c, getpriority.c, setpriority.c, nice.c, waitpid.c, waitid.c, wait3.c, uname.c, `_siginfo_from_linux.c`.
- `libc/posix/sys-wait/wait.c`.
- Headers: `libc/include/sys/resource.h`, `sys/wait.h`, `sys/_wait.h`, `sys/utsname.h`, `sys/syslimits.h`.

**Tests.** `test/test-posix/test-sys-resource/`, `test-sys-wait/` and `test-sys-utsname/`. File names below drop the `test-` prefix (`waitpid.c` = `test-waitpid.c`), and line numbers are for the branch tip.
- Every change to a limit, priority or process group is made in a forked child.
- Every child a test waits for is created and reaped by that test. The only exceptions are orphaned grandchildren, which exit on their own after reporting.
- No signal goes to a process the tests didn't create.

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799:
- basedefs/sys_resource.h, sys_wait.h, sys_utsname.h, and limits.h (NZERO).
- functions/getrlimit, getrusage, getpriority, wait, waitid, uname.
- XSH 2.2.2, 2.4.3 and 2.12.

Kernel behaviour was checked against v5.10 `kernel/exit.c`, `kernel/sys.c`, `fs/coredump.c` and `mm/mmap.c`, and confirmed with glibc probes on this host.

**Status key.** TESTED (file:line), UNTESTED, NOT TESTABLE (reason).
- Every TESTED row was verified by running it on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and, unless noted, for the `-native` glibc build.
- "(new)" marks checks this unit added; "(existing)" marks checks that were already there.

## Requirement table

### `<sys/resource.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| RH1 | PRIO_PROCESS, PRIO_PGRP, PRIO_USER, distinct (XSI) | TESTED (new) | sys_resource_h.c:118-123 (a duplicate `case` label doesn't compile) |
| RH2 | rlim_t is an unsigned type | TESTED (new) | sys_resource_h.c:50 |
| RH3 | RLIM_INFINITY, RLIM_SAVED_MAX, RLIM_SAVED_CUR, usable in `#if` | TESTED (new) | sys_resource_h.c:45-60. The `#if` part is picolibc only: glibc's 64-bit RLIM_INFINITY is a cast |
| RH4 | RLIM_INFINITY is a value of rlim_t | TESTED (new) | sys_resource_h.c:99-104 |
| RH5 | RLIM_SAVED_* may equal RLIM_INFINITY | n/a | all three are INT64_MAX (allowed) |
| RH6 | RUSAGE_SELF, RUSAGE_CHILDREN (XSI) | TESTED (new) | sys_resource_h.c:124-128 |
| RH7 | struct rlimit: rlim_t rlim_cur, rlim_max | TESTED (new) | sys_resource_h.c:85-87 |
| RH8 | struct rusage: timeval ru_utime, ru_stime; timeval defined | TESTED (new) | sys_resource_h.c:88-90 |
| RH9 | The seven RLIMIT_ constants, distinct | TESTED (new) | sys_resource_h.c:108-117; sys_resource_h-names.c:46-50 (strict mode) |
| RH10 | Prototypes | TESTED (new) | sys_resource_h.c:74-82 (`_Generic`) |
| RH11 | id_t (XSI) | TESTED (new) | sys_resource_h.c:71 |
| RH12 | Strict (no XSI): getrlimit/setrlimit visible, the XSI functions hidden | TESTED (new; the hiding is picolibc only) | sys_resource_h-names.c:44-82. glibc declares all five functions whatever the feature macros |
| RH13 | May expose `<sys/time.h>` | n/a | a permission, not a requirement |

### getrlimit, setrlimit

| # | Requirement | Status | Reference |
|---|---|---|---|
| RL1 | Reports the soft and hard limits; soft <= hard | TESTED | getrlimit.c:64-73 (existing); setrlimit.c:103 |
| RL2 | The soft limit can be set to any value <= the hard limit | TESTED (new) | setrlimit.c:99-106, in a child for each resource |
| RL3 | The hard limit can be lowered to >= the soft limit | TESTED (new) | setrlimit.c:131-136 |
| RL4 | Raising a hard limit needs privilege (EPERM, nothing changed) | TESTED (new, unprivileged) | setrlimit.c:138-145 |
| RL5 | Both limits in one call | TESTED (new) | setrlimit.c:131-136 |
| RL6 | RLIM_INFINITY is larger than any other value | TESTED (new, picolibc) | setrlimit.c:116-130. **Deviation 9, fixed** |
| RL7 | RLIM_INFINITY from getrlimit means nothing is enforced | UNTESTED | the absence of enforcement can't be shown by a finite test |
| RL8 | Setting RLIM_INFINITY inhibits enforcement | TESTED (accepted only) | setrlimit.c:105, 128 |
| RL9 | RLIMIT_CORE 0 prevents a core file | TESTED (new) | wcoredump.c:161 (cores go to the current directory here) |
| RL10 | A core image stops at RLIMIT_CORE | UNTESTED | kernel behaviour |
| RL11 | Exceeding RLIMIT_CPU sends SIGXCPU (XSI) | TESTED (new) | setrlimit-enforce.c:189-198, 290 |
| RL12 | Exceeding RLIMIT_DATA makes malloc fail with ENOMEM | TESTED (new; not under qemu) | setrlimit-enforce.c:203-209, 296 |
| RL13 | RLIMIT_FSIZE: SIGXFSZ, or EFBIG when blocked or ignored, for write and truncate | TESTED (new) | setrlimit-enforce.c:137-186, 283-287 |
| RL14 | RLIMIT_NOFILE: no descriptor at or above the limit; EMFILE | TESTED (new) | setrlimit-enforce.c:108-118 |
| RL15 | RLIMIT_STACK: SIGSEGV; a handler with no alternate stack isn't run | TESTED (new; not under qemu) | setrlimit-enforce.c:226-254, 300 |
| RL16 | RLIMIT_AS: malloc and mmap fail with ENOMEM | TESTED (new; not under qemu) | setrlimit-enforce.c:212-222, 298. The stack-growth part is UNTESTED |
| RL17 | Unrepresentable limits read back as RLIM_SAVED_* | NOT TESTABLE | every Linux limit fits in a 64-bit rlim_t |
| RL18 | Setting RLIM_SAVED_* uses the saved limit | NOT TESTABLE | same reason as RL17 |
| RL19 | exec keeps the limits | TESTED (new) | setrlimit-enforce.c:257-265, 302-307 (re-execs /proc/self/exe) |
| RL20 | 0 on success; -1 and errno on failure | TESTED | throughout |
| RL21 | EINVAL for an invalid resource | TESTED (new) | getrlimit.c:76-79; setrlimit.c:147-149 |
| RL22 | EINVAL when the new soft limit is above the new hard limit | TESTED | setrlimit.c:108-114 (the existing check, now in a child) |
| RL23 | EINVAL (may fail) when current usage is already above the new limit | UNTESTED | optional; Linux never fails this way |

### getrusage

| # | Requirement | Status | Reference |
|---|---|---|---|
| RU1 | RUSAGE_SELF reports user and system time | TESTED (existing) | getrusage.c:152-160 |
| RU2 | RUSAGE_CHILDREN counts waited-for children and, recursively, their waited-for children | TESTED (new) | getrusage.c:116-129. A WNOWAIT look doesn't count; a grandchild does |
| RU3 | Children that are never waited for aren't counted | TESTED (new, SIGCHLD SIG_IGN) | getrusage.c:131-141 |
| RU4 | 0 on success; -1 and errno on failure | TESTED | getrusage.c:156, 162 |
| RU5 | EINVAL for an invalid who | TESTED (existing) | getrusage.c:162 |

### getpriority, setpriority

| # | Requirement | Status | Reference |
|---|---|---|---|
| PR1 | Nice value of a process, process group or user | TESTED | getpriority.c:66-80 |
| PR2 | setpriority sets value + NZERO | TESTED (existing, now in a child) | setpriority.c:77-78 |
| PR3 | who 0 means the current process, group or user | TESTED | getpriority.c:66-78; setpriority.c:77, 104 |
| PR4 | The value applies to every thread of the process, by 0 or by pid | TESTED (new, picolibc) | setpriority-thread.c:93-101. **Deviation 8, fixed** |
| PR5 | For several processes, getpriority reports the lowest value and setpriority sets them all | TESTED (new) | setpriority.c:91-121 |
| PR6 | The default nice value is NZERO | UNTESTED | depends on how the test was started |
| PR7 | Values above the range are clamped to the highest | TESTED (new) | setpriority.c:123-127 |
| PR8 | Values below the range are clamped to the lowest | NOT TESTABLE | needs privilege |
| PR9 | Only a privileged process can lower its nice value | TESTED (new) | setpriority.c:80-88: EACCES or EPERM, value unchanged. If Linux's RLIMIT_NICE allows it, the test prints that |
| PR10 | SCHED_FIFO/SCHED_RR processes are unaffected | NOT TESTABLE | no real-time privilege |
| PR11 | Result is in -NZERO to NZERO-1 | TESTED | getpriority.c:67; setpriority.c:139 |
| PR12 | NZERO is in `<limits.h>` (XSI) | TESTED (new) | setpriority.c:57-59. **Deviation 1, fixed** |
| PR13 | ESRCH | TESTED | getpriority.c:73, 75; setpriority.c:157 |
| PR14 | EINVAL for an invalid which | TESTED (existing) | getpriority.c:82; setpriority.c:155 |
| PR15 | EINVAL for an invalid who | UNTESTED | Linux gives ESRCH |
| PR16 | EPERM (may fail) | UNTESTED | needs another user's process |
| PR17 | EACCES (may fail) | TESTED (new) | as PR9 |

### `<sys/wait.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| WH1 | WNOHANG, WUNTRACED, WCONTINUED (XSI) | TESTED (new) | sys_wait_h.c:60-62, 68-69, 77-80 |
| WH2 | Status macros, including WCOREDUMP (new in Issue 8) | TESTED (new) | sys_wait_h.c:47-58, 132-135. **Deviation 2, fixed**. glibc 2.43 defines WCOREDUMP only with `_DEFAULT_SOURCE` |
| WH3 | WEXITED, WNOWAIT, WSTOPPED, and that they combine | TESTED (new) | sys_wait_h.c:64-79 |
| WH4 | idtype_t with P_ALL, P_PID, P_PGID | TESTED (new) | sys_wait_h.c:82-83, 111 |
| WH5 | id_t and pid_t | TESTED (new) | sys_wait_h.c:85-87 |
| WH6 | siginfo_t and union sigval | TESTED (new) | sys_wait_h.c:120-126 |
| WH7 | Prototypes | TESTED (new) | sys_wait_h.c:116-118 |
| WH8 | No `<sys/resource.h>` names; no wait3 in strict mode | TESTED (new) | sys_wait_h.c:91-105. **Deviation 3, fixed** |

### wait, waitpid, XSH 2.12

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | Blocks until any child terminates | TESTED (existing) | wait.c:146-167 |
| W2 | Returns at once when status is available | TESTED (existing) | wait.c:171-174 |
| W3 | Interrupted by a signal handler: EINTR | TESTED (new) | wait.c:96; waitpid.c:197 |
| W4 | Status is consumed | TESTED | wait.c:176; waitpid.c:156 (new) |
| W5 | waitpid(-1, s, 0) behaves like wait | TESTED (inferred) | wait.c is a call to waitpid(-1, status, 0) |
| W6 | pid > 0 selects that child | TESTED (existing) | waitpid.c:227-236 |
| W7 | pid 0 selects children in the caller's group | TESTED (new) | waitpid.c:100-108 |
| W8 | pid < -1 selects children in group -pid | TESTED (new) | waitpid.c:110-112 |
| W9 | WCONTINUED (XSI) | TESTED | waitpid.c:224; 150-155 (new) |
| W10 | WNOHANG returns 0 when nothing is available | TESTED (existing) | waitpid.c:216 |
| W11 | WUNTRACED reports stops; without it they aren't reported | TESTED | waitpid.c:220-222; 146-147 (new) |
| W12 | Returns the pid; stat_loc may be null | TESTED (existing) | wait.c:174; waitpid.c:236 |
| W13 | Status is 0 iff the child exited with 0 | TESTED (new, for exit 0 and 0x1ff) | waitpid.c:120-134. Not checked: termination by the last thread exiting. `_exit(256)` giving 0 is a kernel limit |
| W14 | WEXITSTATUS gives the low 8 bits | TESTED (new) | waitpid.c:130-134 |
| W15 | WIFSIGNALED, WTERMSIG | TESTED | wait.c:162; waitpid.c:164-166, 233 |
| W16 | WCOREDUMP iff a core image was attempted | TESTED (new) | wcoredump.c:94-108, 157-165; waitpid.c:167-170. **Deviation 2, fixed** |
| W17 | WIFSTOPPED, WSTOPSIG | TESTED | waitpid.c:220-222; 159-162 (new) |
| W18 | Exactly one WIF macro holds | TESTED (new) | waitpid.c:61-65, 120-171 |
| W19 | Discarding a queued SIGCHLD | n/a | Linux doesn't queue SIGCHLD |
| W20 | With SIGCHLD blocked, a wait clears a pending SIGCHLD unless another child has status | TESTED (new, picolibc) | waitpid-sigchld.c:95-120. **Deviation 7, fixed** |
| W21 | Orphans get a new parent | TESTED (new) | wait.c:103-133 |
| W22 | ECHILD | TESTED | wait.c:141, 176; waitpid.c:105-108, 239-241 |
| W23 | EINVAL for bad options | TESTED (existing) | waitpid.c:243 |
| W24 | A cancellation point, even with bad options | TESTED (new) | waitpid-cancel.c:63-117. **Deviation 6, fixed** |
| W25 | A new status replaces an unreported one | TESTED (new) | waitpid.c:149-156 |
| W26 | SIGCHLD SIG_IGN: no status; ECHILD | TESTED | getrusage.c:136-140; signal unit (SA_NOCLDWAIT) |
| W27 | Several threads waiting at once | UNTESTED | |

### waitid

| # | Requirement | Status | Reference |
|---|---|---|---|
| WI1 | Blocks, or returns at once when status is available | TESTED (existing) | waitid.c:119-126 |
| WI2 | Consumes the status unless WNOWAIT | TESTED | waitid.c:128-133; 142-150 (new) |
| WI3 | Fills in si_signo, si_code, si_pid, si_uid, si_status | TESTED | waitid.c:121-126 (si_uid is new) |
| WI4 | si_status holds the full exit value | NOT TESTABLE | Linux gives 8 bits |
| WI5 | P_PID, P_PGID, P_ALL | TESTED | waitid.c:205-217 (P_PGID is new) |
| WI6 | The option flags behave as described | TESTED | waitid.c:119-159 |
| WI7 | One of WEXITED, WSTOPPED or WCONTINUED is required | TESTED (existing) | waitid.c:165 |
| WI8 | WNOHANG: returns 0 with si_signo and si_pid 0 | TESTED | waitid.c:139 (si_signo is new) |
| WI9 | ECHILD | TESTED | waitid.c:162, 187-189, 211-213 |
| WI10 | EINTR | TESTED (new) | waitid.c:191-203 |
| WI11 | EINVAL for bad options, idtype or id | TESTED (new; the idtype and P_PGID 0 checks are picolibc only) | waitid.c:173-185. **Deviation 4, fixed** |
| WI12 | A cancellation point, even with bad options | TESTED (new) | waitpid-cancel.c |

### `<sys/utsname.h>`, uname

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | struct utsname with five char arrays | TESTED | uname.c:65-90 |
| U2 | Each field is null-terminated | TESTED (new) | uname.c:61, 71-75 |
| U3 | uname is declared | TESTED | uname.c:72 |
| U4 | Non-negative return; the fields are filled | TESTED | uname.c:72-76 |
| U5 | nodename is the node's name | TESTED (new, Linux) | uname.c:84-89: it equals gethostname. Nothing is printed |
| U6 | Two calls agree | TESTED (new) | uname.c:78-82 |
| U7 | No errors defined | n/a | |

### Counts

112 requirements: 96 TESTED, 7 UNTESTED, 5 NOT TESTABLE, 4 n/a. Before this unit about 36 had a test; the other 60 TESTED rows are new. Nine deviations were fixed and five are left (below).

## Deviations found and fixed

Each fix was verified on aarch64-linux-gnu. With only the library change reversed (by `git show <fix> -- libc libos | git apply -R`, or by hand where later commits touch the same lines), the build was rebuilt and the test run; the new check failed, and it passes with the fix. The failing check is quoted for each.

1. **NZERO was missing** (`sys/syslimits.h`).
   - It is now 20, under `__XSI_VISIBLE`.
   - Failing before (compile): `test-getpriority.c:67: 'NZERO' undeclared`.
2. **WCOREDUMP was missing, and waitpid and wait3 dropped the kernel's 0x80 bit** when they mapped the signal number.
   - Failing before (header): `#error "WCOREDUMP missing"`.
   - Failing before, with only the libos part reversed: `WCOREDUMP matches CLD_DUMPED`. A SIGQUIT child really dumped core here.
3. **`<sys/wait.h>` included `<sys/resource.h>`.**
   - `<sys/wait.h>` reserves only `P_` and `W[A-Z]`, and may expose `<signal.h>`.
   - wait3 now uses an incomplete `struct rusage`.
   - Failing before (compile): `<sys/resource.h> constants visible`, `redefinition of 'struct rlimit'`, `'getrlimit' redeclared`.
4. **waitid passed idtype to Linux unchanged, and dropped unknown options.**
   - Linux takes idtype 3 as P_PIDFD (EBADF for 9999), and P_PGID 0 as the caller's group. Picolibc silently dropped option bits it didn't know.
   - Now: a switch onto new LINUX_P_ constants (`linux-wait.h` and `make-wait.c`), EINVAL for anything else, and the checks run after `_cancel_enter()`.
   - Failing before: `unknown option bit`. With each part reversed alone: `unknown idtype` and `process group 0`.
   - glibc passes all three cases to Linux, so those checks are guarded with `__GLIBC__`.
5. **wait3 wrote through a null status, and wrote one with WNOHANG.**
   - Failing before: `WNOHANG leaves status alone`, and with only the null check reversed the test dies with SIGSEGV.
6. **waitpid returned EINVAL for bad options before the cancellation point.**
   - Failing before: `waitpid with bad options didn't act on the pending cancellation`.
7. **waitpid left a blocked SIGCHLD pending.** POSIX: "if SIGCHLD is blocked, if wait() or waitpid() return because the status of a child process is available, any pending SIGCHLD signal shall be cleared unless the status of another child process is available."
   - After a successful wait, if SIGCHLD is blocked and pending, waitpid takes it with a zero-timeout rt_sigtimedwait.
   - It then checks with `waitid(P_ALL, WEXITED|WSTOPPED|WCONTINUED|WNOHANG|WNOWAIT)` whether another child has status. If one does, it puts the original siginfo back with rt_sigqueueinfo, so a SIGCHLD arriving in between isn't lost.
   - Only raw system calls are used, so waitpid stays async-signal-safe.
   - Failing before: `cleared by waitpid`. glibc leaves the signal pending; the check is guarded.
   - test-sigaction and test-_exit assumed the Linux behaviour and now look with WNOWAIT before collecting the child.
8. **setpriority(PRIO_PROCESS) changed one thread only.** Linux keeps a nice value per thread; v5.10 `find_task_by_vpid` resolves who 0 to the calling thread and getpid() to the main thread.
   - For the caller's own process with more than one thread, setpriority now also sets every listed thread by tid, under `_pthread_all_lock`.
   - nice now calls setpriority instead of doing its own walk.
   - Failing before: `the other thread has the new value`. glibc changes one thread; the check is guarded.
9. **setrlimit's own soft-above-hard check didn't treat RLIM_INFINITY (INT64_MAX) as the largest value.**
   - Failing before: `resource 4: check 9 failed`, which is `rlim_cur = RLIM_INFINITY + 1` with `rlim_max = RLIM_INFINITY`.
   - Picolibc only: glibc's RLIM_INFINITY is already the largest rlim_t.

**Also fixed: test-setrlimit itself.** It used to set the test process's own limits down to 1 or 2 (RLIMIT_STACK and RLIMIT_AS included), and failed when a hard limit was 0. It now uses a child for each resource and skips resources whose hard limit is below 2. Verified under `ulimit -Hc 0`: "resource 4: hard limit below 2, skipped", then success, for picolibc and glibc.

## Deviations not fixed

- **setpriority or getpriority on another multi-threaded process** affects or reports only one thread. Fixing it would need a walk of `/proc/<pid>/task`. glibc and musl behave the same.
- **The 8-bit exit status** (W13, WI4) is a kernel limit.
- **RLIM_INFINITY = INT64_MAX.** A kernel limit of 2^63-1 reads back as infinity, and rlim_t has values above RLIM_INFINITY. Changing it to `__UINT64_MAX__` changes the ABI of compiled programs, so it is left for a decision.
- **setpriority's EPERM rule.** Linux `set_one_prio_perm` compares the target's real or effective UID with the caller's effective UID, which is the reverse of POSIX. Untested.
- **No RLIMIT_NPROC, RLIMIT_MEMLOCK, RLIMIT_NICE, ...** (raised by threads-c11). They are allowed by the reserved `RLIMIT_` prefix but would need the generated `linux-resource.h` files extended.

## Other observations

- PRIO_*, RUSAGE_*, struct rusage and RLIMIT_CPU are visible without XSI. XSH 2.2.2 reserves those prefixes for `<sys/resource.h>`, and picolibc is an XSI implementation, so I read this as allowed. The XSI functions are hidden in strict mode.
- **glibc 2.43 gaps** (guarded with `__GLIBC__` in the tests):
  - WCOREDUMP is defined only with `_DEFAULT_SOURCE`.
  - RLIM_INFINITY on 64-bit targets is a cast, so it can't be used in `#if`.
  - The XSI resource functions are declared in strict mode.
  - waitid passes idtype 3 and P_PGID 0 to Linux.
  - waitpid leaves a blocked SIGCHLD pending.
  - setpriority changes one thread.
- uname's arrays are 65 bytes, the kernel's `new_utsname` size, so nothing is truncated.

## Left undone

- W27, RL7, RL10, PR6, PR15, PR16, and the "last thread terminated" part of W13.
- The privileged paths.
- qemu-user wasn't run on this host.
  - test-setrlimit and test-setrlimit-enforce relax or skip RLIMIT_DATA, RLIMIT_AS and RLIMIT_STACK under `is_qemu()`.
  - The SIGCHLD code relies on qemu's emulation of rt_sigprocmask, rt_sigpending, rt_sigtimedwait and rt_sigqueueinfo, so test-waitpid-sigchld should be watched on the x86_64 host.
  - qemu writes its own core files. test-wcoredump removes every file in its temporary directory.

## Test runs (verified by running)

- **aarch64-linux-gnu** (`-Dwerror=true`, full `meson test -t 4`): 1588 tests; 1539 Ok, 0 Fail, 6 Expected Fail, 43 Skipped.
- **arm-linux-gnueabihf**: 1589 tests; 1538 Ok, 0 Fail, 6 Expected Fail, 45 Skipped. An earlier armhf run caught test-sigaction and test-_exit depending on the old SIGCHLD behaviour; that is fixed in 6971663188.
- All 63 runs of this unit's tests (picolibc and `-native`) pass on both targets; none skip.
- The native configuration (`do-native-configure -Dwerror=true`, `ninja` only) builds cleanly.
- **CMake** (`do-cmake-thumbv7m-configure`, `ninja test`): 599 tests, 100% passed (262 ran, 337 skipped). The header tests run there; the behaviour tests skip. test-getrlimit isn't in the CMake list, because CMake's semihost library lacks the fake getrlimit and the test doesn't link.
- **clang-check** (clang 21, run here) on all 27 changed C files: 46 OK, 1 NOENTRY (`libos/linux/utils/make-wait.c`, a host utility outside the build).
- `clang-format --dry-run -Werror` is clean.
- The fix proofs are listed with each deviation above.
- No kills by the host security software and no other incidents.

## Commits (audit/resource-wait, on top of d14119a43a)

- 6686d54a41 limits: Define NZERO
- 007b3c6817 sys/wait.h: Define WCOREDUMP and keep the core flag in statuses
- a0906ebbbe sys/wait.h: Don't include <sys/resource.h>
- 4f657623af libos/linux: Let wait3 take a null status, and leave it alone with WNOHANG
- c29034a959 libos/linux: Check the waitid idtype and options
- 2ca168b806 libos/linux: Reject bad waitpid options inside the cancellation point
- 6971663188 test-posix: Look for SIGCHLD before collecting the child
- 027b446ee8 libos/linux: Clear a pending SIGCHLD in waitpid
- 9615e2ff2b libos/linux: Apply setpriority to every thread
- a388438ea0 libos/linux: Treat RLIM_INFINITY as the largest limit in setrlimit
- d241073cd3 test-posix: Check <sys/wait.h> and the groups, statuses and signals of the wait functions
- 76c5e51f24 test-posix: Check <sys/resource.h>, and setrlimit in children
- 19f3bbe5aa test-posix: Check setpriority across threads and groups, and children's getrusage
- 40336b96cd test-posix: Check that uname's fields are terminated and consistent
- 7fa8c912aa test-posix: Check getrlimit's EINVAL for an invalid resource
