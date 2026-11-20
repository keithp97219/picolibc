# `<spawn.h>` audit against POSIX.1-2024

Worktree: `/agentic/picolibc-project/worktrees/audit-spawn`, branch `audit/spawn`, starting at `14ebe3b96b`.
Spec: POSIX.1-2024 (Issue 8) pages fetched from pubs.opengroup.org: `basedefs/spawn.h`,
`functions/posix_spawn` (covers posix_spawnp), `posix_spawn_file_actions_addclose` (covers addopen),
`posix_spawn_file_actions_adddup2`, `posix_spawn_file_actions_addchdir` (covers addfchdir),
`posix_spawn_file_actions_destroy` (covers init), `posix_spawnattr_destroy` (covers init), and the
`posix_spawnattr_get*` pages (each covers its `set*`).

Implementation: `libc/posix/spawn/` (child logic in `__posix_spawn.c`), header `libc/include/spawn.h`.

Test file paths below are relative to `test/test-posix/test-spawn/`, shortened by dropping the
`test-` prefix (`posix_spawn.c` = `test-posix_spawn.c`). Line numbers are for the final branch tip.

"Verified" means I ran code; "inferred" means I only read the code.

## Requirement table

Status: TESTED (in the test suite), UNTESTED, NOT TESTABLE (with reason).

### `<spawn.h>` header

| # | Requirement | Spec | Status | Reference |
|---|---|---|---|---|
| H1 | Defines `posix_spawnattr_t`, `posix_spawn_file_actions_t` | spawn.h DESCRIPTION | TESTED | every test; posix_spawn.c:88-98 |
| H2 | Defines `mode_t`, `pid_t` | spawn.h | UNTESTED | Suite tests include `<unistd.h>` first. Verified by hand: a file including only `<spawn.h>` compiles with -std=c99/c11/c18 -Werror |
| H3 | Defines `sigset_t` | spawn.h | UNTESTED | same manual compile check |
| H4 | `struct sched_param` declared as an incomplete tag | spawn.h | UNTESTED | same manual compile check |
| H5 | The seven `POSIX_SPAWN_*` flags, usable OR-ed together | spawn.h, posix_spawnattr_getflags | TESTED | posix_spawn.c:100-107,142-144 (non-zero, pairwise disjoint) |
| H6 | Prototypes for all 26 functions, with the spec's qualifiers | spawn.h | TESTED | posix_spawn.c:88-98 assigns 4 functions to pointers of the exact spec types; all 26 are called by the suite. Manual check also covered addchdir, addfchdir, posix_spawnp, getschedparam |
| H7 | May make `<sched.h>`/`<signal.h>` symbols visible | spawn.h | n/a | permissive |

### posix_spawn / posix_spawnp: DESCRIPTION

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | Creates a child running the new process image with argv | TESTED | posix_spawn.c:155,164-165 |
| S2 | `environ` in the new image comes from envp | TESTED | posix_spawn.c:164-165 (custom envp), :156-158 |
| S3 | System docs say whether ARG_MAX counts list overhead | NOT TESTABLE | documentation requirement |
| S4 | posix_spawn: relative path resolved in the child's cwd after all file actions | TESTED | addchdir.c:130-131 |
| S5 | posix_spawnp: a file name with a slash is used as the pathname, unsearched | TESTED | posix_spawnp.c:138-140 |
| S6 | posix_spawnp: PATH search, relative entries from the child's cwd after file actions | TESTED | posix_spawnp.c:117-118,135-136; addchdir.c:133-135 |
| S7 | PATH unset: implementation-defined | NOT TESTABLE | implementation-defined; picolibc searches `/bin:/usr/bin` (inferred) |
| S8 | An exec ENOEXEC makes posix_spawnp fail, no `sh` fallback | TESTED | posix_spawnp.c:145; posix_spawn.c:175. **Deviation 2, fixed** |
| S9 | NULL file_actions: parent's descriptors stay open except FD_CLOEXEC ones | TESTED | addclose.c:104; adddup2.c:113 |
| S10 | FD_CLOFORK descriptors are not inherited | NOT TESTABLE | picolibc has no FD_CLOFORK/O_CLOFORK (grep of libc/include, libos/linux finds none). If added, the child is made by fork() so it would follow fork (inferred) |
| S11 | Child does not inherit file locks | UNTESTED | follows from fork() (inferred) |
| S12 | NULL file_actions: child's cwd equals the parent's | TESTED | posix_spawn.c:175-177 (relative program names resolved in the parent's cwd) |
| S13 | Signal mask, signal defaults, ids from attrp applied; then file actions in the order added | TESTED (order) | addclose.c:112-120 (close/dup2 order both ways). Sigmask is set after the file actions (`__posix_spawn.c`), with every signal blocked meanwhile, so the difference is not observable (inferred) |
| S14 | Tracked working directory; relative action paths use it | TESTED | addchdir.c:116,128-131 |
| S15 | FD_CLOEXEC descriptors closed after the actions | TESTED | adddup2.c:113 |
| S16 | SETPGROUP with non-zero pgroup joins that group | TESTED | setpgroup.c:129-132 |
| S17 | SETPGROUP with pgroup 0: new group, pgid = pid | TESTED | setpgroup.c:123-126 |
| S18 | SETSID: session leader, group leader, no controlling terminal | TESTED | setpgroup.c:139-141 (sid and pgid); "no controlling terminal" follows from setsid() (inferred) |
| S19 | Neither SETPGROUP nor SETSID: parent's process group | TESTED | setpgroup.c:118-119 |
| S20 | SETSCHEDPARAM alone: parent's policy, attr's params | TESTED (partly) | setschedpolicy.c:122-123 runs with SCHED_OTHER; parameter effect can't be observed without real-time privilege |
| S21 | SETSCHEDULER: attr's policy and params | TESTED (partly) | setschedpolicy.c:111-112, SCHED_OTHER only; SCHED_FIFO/RR need CAP_SYS_NICE |
| S22 | No RESETIDS: child inherits effective uid/gid | UNTESTED | only observable when euid != ruid |
| S23 | RESETIDS: euid/egid reset to real ids | NOT TESTABLE | needs a set-id process or root |
| S24 | Set-id bits on the image take effect | NOT TESTABLE | needs a set-id file owned by another user / root |
| S25 | SETSIGMASK: child's initial mask is spawn-sigmask | TESTED | setsigmask.c:100-105 |
| S26 | Without SETSIGMASK the parent's mask is inherited (fork+exec rule) | TESTED | posix_spawn.c:182-186 |
| S27 | SETSIGDEF: listed signals take the default action | TESTED | setsigdefault.c:108-115 (single signal and full set) |
| S28 | Caught signals become default in the child | TESTED | posix_spawn.c:189-192 |
| S29 | Ignored signals (except SIGCHLD) stay ignored unless in sigdefault | TESTED | setsigdefault.c:98-99, then 111 |
| S30 | SIGCHLD ignored in parent: unspecified | n/a | unspecified |
| S31 | NULL attrp uses defaults | TESTED | most `run(..., NULL)` calls, e.g. posix_spawn.c:155 |
| S32 | Other attributes as if fork()+exec | NOT TESTABLE | open-ended; follows from fork()+execve() (inferred) |
| S33 | Whether fork handlers run is implementation-defined | n/a | picolibc calls fork(), so they run (inferred) |

### posix_spawn / posix_spawnp: RETURN VALUE and ERRORS

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | Success: child pid stored through non-null pid, returns 0 | TESTED | posix_spawn.c:164-165 |
| R2 | Null pid accepted | TESTED | posix_spawn.c:168-169 |
| R3 | Failure: no child is created; error number returned | TESTED | posix_spawn.c:159-161 (no child left to wait for) |
| E1 | May fail EINVAL for invalid file_actions/attrp | NOT TESTABLE | passing an invalid object is undefined; picolibc does not check (allowed: "may") |
| E2 | exec errors returned as exec would | TESTED | ENOENT posix_spawn.c:159, posix_spawnp.c:120,140; EACCES posix_spawn.c:177 (skipped as root); ENOEXEC posix_spawn.c:175, posix_spawnp.c:145; empty name ENOENT posix_spawnp.c:122. **Deviations 1 and 2, fixed** |
| E3 | fork errors returned | UNTESTED | needs exhausting RLIMIT_NPROC; not attempted on a shared machine |
| E4 | setpgid errors returned (SETPGROUP) | TESTED | setpgroup.c:150-155 (EINVAL for pgroup -1) |
| E5 | setsid errors returned (SETSID) | NOT TESTABLE | a newly forked child is never a group leader, so setsid() can't fail |
| E6 | sched_setparam errors returned (SETSCHEDPARAM) | TESTED | setschedpolicy.c:124-127 (EINVAL, priority out of range) |
| E7 | sched_setscheduler errors returned (SETSCHEDULER) | TESTED | setschedpolicy.c:114-118 (EINVAL, policy -1) |
| E8a | open action errors | TESTED | addopen.c:161-167 (ENOENT) |
| E8b | dup2 action errors | TESTED | adddup2.c:127-131 (EBADF) |
| E8c | chdir action errors | TESTED | addchdir.c:141-144 (ENOENT) |
| E8d | fchdir action errors | TESTED | addfchdir.c:112-122 (EBADF, ENOTDIR) |
| E8e | close action errors, except an in-range descriptor that is already closed | TESTED | addclose.c:123-127 (ignored), 130-140 (out of range gives EBADF). **Deviation 4, fixed** |
| E9 | Errors after a successful return: child exits 127 | NOT TESTABLE | picolibc reports every failure before returning (CLOEXEC error pipe), so this never happens |

### posix_spawn_file_actions_addclose / addopen

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | addclose adds a close action | TESTED | addclose.c:105-107 |
| A2 | Closing an in-range descriptor that is already closed is ignored | TESTED | addclose.c:123-127 |
| A3 | addopen opens path and moves the result to fildes | TESTED | addopen.c:148-156, 185-193 |
| A4 | addopen: an open fildes is closed before the file is opened | TESTED | addopen.c:97-124 (every descriptor in use). **Deviation 5, fixed** |
| A5 | Relative addopen path uses the directory from prior actions | TESTED | addchdir.c:111-116 |
| A6 | addopen copies path | TESTED | addopen.c:150-156 |
| A7 | Return 0 on success | TESTED | every add call |
| A8 | addopen EBADF: fildes negative or >= {OPEN_MAX} | TESTED | addopen.c:169 (negative), 170-173 (>= OPEN_MAX). **Deviation 3, fixed** |
| A9 | addclose EBADF: fildes negative | TESTED | addclose.c:108 |
| A10 | ENOMEM when out of memory | NOT TESTABLE | no portable way to make malloc fail; code returns ENOMEM from `__spawn_add_action.c` (inferred) |
| A11 | May fail EINVAL for an invalid object | NOT TESTABLE | undefined to pass one; picolibc returns EINVAL for a destroyed object (inferred) |
| A12 | Unperformable operations are not errors at add time; reported by posix_spawn | TESTED | addopen.c:162-167, adddup2.c:130-131, addfchdir.c:115-122 |

### posix_spawn_file_actions_adddup2

| # | Requirement | Status | Reference |
|---|---|---|---|
| D1 | Adds a dup2 action | TESTED | adddup2.c:104-107 |
| D2 | fildes == newfildes: inherited with FD_CLOEXEC clear | TESTED | adddup2.c:110-117 |
| D3 | EBADF: either fd negative or >= {OPEN_MAX} | TESTED | adddup2.c:118-123. **Deviation 3, fixed** |
| D4 | ENOMEM | NOT TESTABLE | as A10 |
| D5 | May fail EINVAL | NOT TESTABLE | as A11 |
| D6 | Unperformable dup2 reported at spawn | TESTED | adddup2.c:127-131 |

### posix_spawn_file_actions_addchdir / addfchdir

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | addchdir adds a chdir action | TESTED | addchdir.c:105-116 |
| C2 | Relative path follows prior actions | TESTED | addchdir.c:128-131 |
| C3 | addchdir copies path | TESTED | addchdir.c:107-116 |
| C4 | addfchdir adds an fchdir action | TESTED | addfchdir.c:105-107,126 |
| C5 | addfchdir EBADF: negative | TESTED | addfchdir.c:108 |
| C6 | ENOMEM | NOT TESTABLE | as A10 |
| C7 | May fail EINVAL | NOT TESTABLE | as A11 |
| C8 | Unperformable chdir/fchdir reported at spawn | TESTED | addchdir.c:144; addfchdir.c:116,122 |

### posix_spawn_file_actions_init / destroy

| # | Requirement | Status | Reference |
|---|---|---|---|
| I1 | init: no actions | TESTED | every file-actions test |
| I2 | destroy; object can be re-initialized | TESTED | destroy in every file-actions test; addclose.c:109-117 re-inits after destroy |
| I3 | init ENOMEM | NOT TESTABLE | as A10 |
| I4 | destroy may fail EINVAL | NOT TESTABLE | as A11 |

### posix_spawnattr_init / destroy and get/set pairs

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | init gives defaults: flags none | TESTED | setflags.c:93 |
| P2 | default pgroup 0 | TESTED | setpgroup.c:122 |
| P3 | default sigdefault empty | TESTED | setsigdefault.c:101-105 |
| P4 | destroy | TESTED | every attr test |
| P5 | Later attr changes don't affect already spawned processes | NOT TESTABLE | the child has already exec'd; its copy came from fork (inferred) |
| P6 | Implementation documents its attributes | NOT TESTABLE | documentation |
| P7 | init ENOMEM | NOT TESTABLE | as A10 |
| P8 | destroy may fail EINVAL | NOT TESTABLE | as A11 |
| G1 | getflags/setflags round trip | TESTED | setflags.c:95-98 |
| G2 | setflags may fail EINVAL for invalid flags | TESTED | setflags.c:100 (picolibc only; glibc also rejects) |
| G3 | getpgroup/setpgroup round trip | TESTED | setpgroup.c:129-130 |
| G4 | getschedparam/setschedparam round trip | TESTED | setschedpolicy.c:106-110 |
| G5 | getschedpolicy/setschedpolicy round trip | TESTED | setschedpolicy.c:104-105 |
| G6 | getsigdefault/setsigdefault round trip | TESTED | setsigdefault.c:108-109 |
| G7 | getsigmask/setsigmask round trip | TESTED | setsigmask.c:100-101 |
| G8 | get/set may fail EINVAL for an invalid attr | NOT TESTABLE | as A11 |
| G9 | set* may fail EINVAL for an invalid value | n/a | only setflags checks; bad policy/pgroup is reported by posix_spawn instead (setschedpolicy.c:118, setpgroup.c:155) |

### Totals

103 rows: **TESTED 71** (S13, S18, S20, S21 and H6 only partly), **UNTESTED 6** (H2, H3, H4,
S11, S22, E3; H2-H4 were checked by hand outside the suite), **NOT TESTABLE 22**, **n/a 4**
(H7, S30, S33, G9).

## Deviations found and fixed

All five were seen failing by running the new tests before the fix, and passing after. For
deviation 3 (addopen half) I also reverted the fix alone and saw the new check fail.

1. **posix_spawnp with an empty file name returned EACCES, not ENOENT.**
   `__posix_spawn.c` `__spawn_exec` searched PATH with `""`, building `"/bin/"` etc., and
   execve on a directory gives EACCES. posix_spawn says exec errors are returned "as described by
   ... exec"; exec's ERRORS: "[ENOENT] ... path or file is an empty string."
   Fix: return ENOENT for an empty name before searching. Test: posix_spawnp.c:122.
   Commit `df83928023 spawn: Fail posix_spawnp with ENOENT for an empty file name`.

2. **posix_spawnp kept searching PATH after ENOEXEC** and ran a later match (and could report a
   different error). Spec: "if at least one of the exec family of functions would fail with
   [ENOEXEC] because the process image contents are not executable, this shall cause
   posix_spawnp() to fail rather than attempting a fallback". Fix: stop the search at ENOEXEC
   (glibc does the same). Test: posix_spawnp.c:142-145 (PATH=a:b, a/prog not executable,
   b/prog a valid script; expects ENOEXEC).
   Commit `57b0f52054 spawn: End the posix_spawnp search at a file that fails with ENOEXEC`.

3. **addopen and adddup2 accepted descriptors >= {OPEN_MAX}.** Old code
   (`posix_spawn_file_actions_addopen.c:46`, `posix_spawn_file_actions_adddup2.c:46`) only checked
   for negative values. Spec: "[EBADF] The value specified by fildes is negative or greater than
   or equal to {OPEN_MAX}" (and fildes or newfildes for adddup2). Fix: new
   `__spawn_fd_valid()` in `local-spawn.h` compares against `sysconf(_SC_OPEN_MAX)` (the
   RLIMIT_NOFILE soft limit on Linux). Tests: addopen.c:170-173, adddup2.c:119-123.
   Commit `8f0c995769 spawn: Reject addopen and adddup2 descriptors at or above OPEN_MAX`.

4. **A close action on a descriptor >= {OPEN_MAX} was silently ignored.** The child ignored every
   EBADF from close. Issue 8 removed the OPEN_MAX check from addclose and says spawn fails for
   close errors "other than attempting a close() on a file descriptor that is in range but
   already closed". Fix: ignore EBADF only for in-range descriptors. The error-pipe relocation
   also had to skip out-of-range descriptors, since it moved the pipe above the largest named
   descriptor with F_DUPFD_CLOEXEC, which failed with EINVAL first (I saw this: with only the
   close change the test still failed). Test: addclose.c:130-140 (accepts EBADF from either
   addclose or posix_spawn, as glibc reports it at add time).
   Commit `bd785761f9 spawn: Fail a close action on a descriptor at or above OPEN_MAX`.

5. **An open action opened the file before closing fildes.** The child did `open()` then
   `dup2()` onto fildes. Spec: "If fildes was already an open file descriptor, it shall be
   closed before the new file is opened." With every descriptor in use the open failed with
   EMFILE. Fix: close fildes first (glibc does the same). Test: addopen.c:91-131 lowers
   RLIMIT_NOFILE to 32 and fills the table. On glibc the check passes either way (glibc has no
   error pipe, so its child has one more free slot); it only discriminates on picolibc.
   Commit `89bde92386 spawn: Close an open action's descriptor before opening the file`.

## Recent review fixes, checked against the spec (not redone)

- Signals blocked across fork, handlers reset before unblocking: matches S28 (caught signals
  default) — verified by posix_spawn.c:189-192; no handler runs in the child (inferred).
- SETSIGDEF with full sets skips SIGKILL/SIGSTOP: verified by setsigdefault.c:112-115.
- Error pipe kept clear of action descriptors: verified by addopen.c:176-193 (existing test).
- EINTR on the parent's read is retried (inferred from code; not tested).

## Observations not fixed

- `<limits.h>` (`sys/syslimits.h:45`) defines `OPEN_MAX` as 64, but on Linux the limit is
  RLIMIT_NOFILE (1024 here). POSIX allows OPEN_MAX in `<limits.h>` only when it is invariant, so
  it should probably be omitted there. Out of scope for this header; spawn now uses
  `sysconf(_SC_OPEN_MAX)`.
- `spawn.h` declares `posix_spawn`/`posix_spawnp` without `restrict` on file_actions, argv and envp
  (the "XXX" comment from FreeBSD). Top-level parameter qualifiers don't change function type
  compatibility, so not a conformance problem; left alone.
- glibc (2.43 here) only defines `POSIX_SPAWN_SETSID` under `_GNU_SOURCE`, so the -native tests
  guard SETSID with `#if defined(__PICOLIBC__) || defined(POSIX_SPAWN_SETSID)`.
- Resource-limit edge case: the child's error pipe is moved above the highest descriptor named
  by the actions. If the actions name the descriptor at RLIMIT_NOFILE-1, there is nowhere to move
  it and posix_spawn fails although the actions are valid. glibc (clone/vfork, no pipe) doesn't
  have this. Not fixed: would need a different error channel.
- addchdir/addfchdir tests remain native-skipped: glibc 2.43 only has the `_np` names.
- The process-group test previously used `sleep 1`; it now keeps children alive on a pipe read,
  which is deterministic.

## Left undone

- E3 (fork errors): would need RLIMIT_NPROC exhaustion, not safe on a shared machine.
- S22-S24 (RESETIDS, set-id images): need root or set-id binaries.
- ENOMEM paths: no malloc-failure hook used.
- No `-native` coverage for addchdir/addfchdir (glibc lacks the Issue 8 names).

## Verification runs

- `builds/build-x86_64-linux-gnu` (configured with `-Dwerror=true`):
  `meson test -t 4 --suite test-spawn`: 34 OK, 0 fail (picolibc, `_32`, and `-native`).
  Full `meson test -t 4`: Ok 1647, Expected Fail 9, Fail 0, Skipped 46.
- `builds/build-arm` (`scripts/do-arm-configure`): `ninja` completed (209308 steps), exit 0.
- `clang-format --dry-run` clean on all touched C files.

## Commits on audit/spawn

```
df83928023 spawn: Fail posix_spawnp with ENOENT for an empty file name
57b0f52054 spawn: End the posix_spawnp search at a file that fails with ENOEXEC
8f0c995769 spawn: Reject addopen and adddup2 descriptors at or above OPEN_MAX
bd785761f9 spawn: Fail a close action on a descriptor at or above OPEN_MAX
89bde92386 spawn: Close an open action's descriptor before opening the file
217eea9739 test-posix: Check exec errors, argv, envp and signals in posix_spawn
46cc42696d test-posix: Check empty names, slashes and ENOEXEC in posix_spawnp
6e69be66b4 test-posix: Check file action errors, ordering and limits in posix_spawn
d16e3cdd6a test-posix: Check process groups, sessions and scheduling in posix_spawn
```

No new test files, so no meson.build/CMakeLists.txt changes were needed.
