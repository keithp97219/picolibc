# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

You are running on the user's arm64 machine, in a checkout of the
`claude/arm64-testing` branch of `keithp97219/picolibc`. This branch carries
no picolibc source; it's a message channel. Your job is to test one picolibc
commit on real arm64 hardware and push the results back to this branch.

## Rules

- Commit as Keith Packard <keithp@keithp.com>. No `Co-Authored-By` or other AI
  attribution in commits.
- Push **only** to the `claude/arm64-testing` branch of `keithp97219/picolibc`,
  and never force-push. Never push any other branch or repository.
- Don't modify the picolibc source you test. Report problems; don't fix them.
  The one exception is an audit round (below), whose changes leave this
  machine only as patch files on this branch.

## Why real hardware

Earlier runs were all emulated: qemu-user on an x86 host, and a QEMU TCG
virtual machine. Emulated Arm on x86 gets x86's stronger memory ordering, so
missing barriers in futex, atomic or lock code can't show up. Real arm64 can
expose them. The main subject is POSIX threads and synchronization in
`libos/linux`.

## What to test

`TEST-COMMIT` names the commit and the branch it's on:

```sh
read sha branch < TEST-COMMIT
git fetch origin "$branch"
git worktree add --detach ../picolibc-test "$sha"
cd ../picolibc-test
```

## Host check

```sh
uname -m                                  # aarch64
grep -m1 'CPU part' /proc/cpuinfo
/lib/ld-linux-armhf.so.3 --version        # works only if the CPU and kernel run AArch32
```

Packages (Debian names): `gcc`, `meson`, `ninja-build`, `python3`. For armhf:
`gcc-arm-linux-gnueabihf`, `libc6-dev-armhf-cross`, and `libc6:armhf` (after
`dpkg --add-architecture armhf`). If AArch32 doesn't work, skip armhf and say
so in the results. If packages are missing, ask the user; don't install them
yourself.

## Builds

In `../picolibc-test`, each build goes in `builds/build-<target>`, where
`<target>` is the part of `scripts/do-<target>-configure` between `do-` and
`-configure`:

```sh
for t in aarch64-linux-gnu arm-linux-gnueabihf; do
    mkdir -p builds/build-$t
    (cd builds/build-$t &&
     ../../scripts/do-$t-configure -Dwerror=true > configure.log 2>&1 &&
     ninja > ninja.log 2>&1 &&
     meson test -t 4 --print-errorlogs > test.log 2>&1)
    grep -E '^(Ok|Fail|Expected Fail|Skipped|Timeout):' builds/build-$t/test.log
done
```

The cross files use `exe_wrapper = 'env'`, so on arm64 the tests run natively.

## Stress the thread tests

Ordering bugs are intermittent, so repeat the thread and synchronization tests
in each build directory:

```sh
tests=$(meson test --list | grep -iE 'pthread|sem|rwlock|barrier|spin|cancel|atfork|timer|thread|mqueue|cpuclock|time64|aio|lio_|mtx|cnd|msgrcv|shmat|spawn' | sed 's/^.*picolibc://')
meson test -t 4 --no-rebuild --repeat 200 $tests > stress.log 2>&1
grep -E '^(Ok|Fail|Timeout):' stress.log
```

A test that passes 199 times and fails once is a finding, not noise.

## Expected results from emulation

This round tests `claude/posix-pse`, which adds a lot on top of the threads
work: asynchronous I/O (worker threads), robust mutexes, posix_spawn, System V
IPC, sockets and networking, ISO C threads, and more. Under qemu-user on x86,
for the commit in `TEST-COMMIT`:

- aarch64-linux-gnu: all pass (1100 tests), 6 expected failures, 39 skipped.
- arm-linux-gnueabihf: all pass (1095 tests), 6 expected failures, 43 skipped.

On real hardware some of those skips should run instead, and must pass:

- test-pthread_mutex_consistent (robust mutexes): qemu-user has no
  set_robust_list, so it skips there ("robust mutexes not supported here").
  On a real kernel it must run and pass, on both targets.
- test-msgrcv on armhf: qemu misplaces the IPC_STAT fields for 32-bit ARM, so
  it skips there ("IPC_STAT fields misplaced"). On real AArch32 it must pass;
  this is the main check that 32-bit ARM's System V IPC structures and the
  IPC_64 command flag are right. test-semop and test-shmat check the same
  layout for semaphores and shared memory and pass under qemu.
- test-telldir on armhf skips under qemu (64-bit host directory offsets); a
  real 32-bit task gets 32-bit offsets, so it must pass.
- test-mqueue's notification checks skip only under qemu-user.
- The glibc `-native` forms of test-pthread_mutex_consistent, test-telldir
  (armhf), test-msgrcv (armhf) and four posix_spawn tests are skipped by the
  build off x86, so they don't run here either.

Report every test that skips on real hardware with its message, so the
emulation-only skips can be told apart.

Known: test-setrlimit fails if the session's hard RLIMIT_CORE is 0 (as under
`sudo -i`); the fix isn't in this commit. Run in a session with an unlimited
hard core limit (e.g. `chroot --userspec`), as for the previous round.

Anything else that fails is new.

## Reporting

Back in this checkout, write `results/<YYYY-MM-DD>-<short sha>/` containing:

- `SUMMARY.md`: the commit tested, `uname -a`, CPU part, `gcc --version`; per
  target the Ok/Fail/Expected Fail/Skipped/Timeout lines and the stress
  counts; for each failure or timeout, the test name, how often it failed and
  its error log;
- `<target>/test.log.gz`, `<target>/stress.log.gz`, and
  `<target>/meson-testlog.txt.gz` (from `builds/build-<target>/meson-logs/testlog.txt`).

Then commit and push:

```sh
git add results
git commit -m "Results for <short sha> on <CPU part>"
git pull --rebase origin claude/arm64-testing
git push origin HEAD:claude/arm64-testing
```

When `TEST-COMMIT` changes on this branch, that's a request for another round.

## Audit rounds

`AUDIT-REQUEST` names the branch to audit and the units to run:

```sh
read base units < AUDIT-REQUEST
git fetch origin "$base"
```

`AUDIT-PLAN.md` describes the units, the brief to give each unit's agent,
and what each one delivers: `results/audit/<unit>/report.md` and the
unit's commits as `git format-patch` files beside it. The pilot's reports
are in `results/audit/pilot/`. Run each named unit in its own worktree with
its own agent, up to the number the plan allows at once, then commit the
results and push this branch as for a test round:

```sh
git add results/audit
git commit -m "Audit results for <units>"
git pull --rebase origin claude/arm64-testing
git push origin HEAD:claude/arm64-testing
```

When `AUDIT-REQUEST` changes on this branch, that's a request for another
audit round. A unit already in `results/audit/` isn't run again unless the
request says so.

