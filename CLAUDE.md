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
tests=$(meson test --list | grep -iE 'pthread|sem|rwlock|barrier|spin|cancel|atfork|timer|aio|thread' | sed 's/^.*picolibc://')
meson test -t 4 --no-rebuild --repeat 200 $tests > stress.log 2>&1
grep -E '^(Ok|Fail|Timeout):' stress.log
```

A test that passes 199 times and fails once is a finding, not noise.

## Expected results from emulation

- aarch64-linux-gnu: all pass (about 720 tests), 6 expected failures.
- arm-linux-gnueabihf: all pass (about 720 tests), 6 expected failures.
- libc-test's robust-mutex tests are skipped in their glibc `-native` form for
  cross builds, which includes these cross-file builds.

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
