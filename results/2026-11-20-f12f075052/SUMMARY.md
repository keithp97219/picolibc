# Results for f12f075052 on Neoverse N1 (CPU part 0xd0c)

## Commit tested

`f12f07505246336955493130642d9933f781d0e5` from `claude/linux-threads`,
"libos/linux: Add dup3, fchdir, ftruncate, truncate, futimens and posix_getdents"

## Host

- `uname -a`: `Linux dev-dsk-keithpac-2a-8cab6371.us-west-2.amazon.com 5.10.273-289.1133.amzn2int.aarch64 #1 SMP Wed Nov 11 00:32:49 UTC 2026 aarch64 aarch64 aarch64 GNU/Linux`
- CPU part: `0xd0c` (Arm Neoverse N1, AWS Graviton2), 8 CPUs
- Debian sid chroot on the same kernel (unchanged since the previous round)
- `aarch64-linux-gnu-gcc (Debian 16.2.0-4) 16.2.0`
- `arm-linux-gnueabihf-gcc (Debian 16.2.0-3) 16.2.0`
- `gcc (Debian 16.2.0-4) 16.2.0`
- glibc 2.43 (`ld.so (Debian GLIBC 2.43-7)`), meson 1.12.1, ninja 1.13.2
- AArch32 runs natively.
- The session was entered with `chroot --userspec`, so the hard RLIMIT_CORE
  is unlimited.

The builds were configured fresh with `scripts/do-<target>-configure -Dwerror=true`,
then `ninja`, then `meson test -t 4 --print-errorlogs`. Stress used
`meson test -t 4 --no-rebuild --repeat 200` over the 80 tests that match
`pthread|sem|rwlock|barrier|spin|cancel|atfork|timer|thread|mqueue|cpuclock|time64`.
The list is the same on both targets.

## aarch64-linux-gnu

Full suite (matches the emulation numbers exactly):

    Ok:                736
    Expected Fail:     6
    Fail:              0
    Skipped:           36

Stress (80 tests × 200):

    Ok:                16000
    Fail:              0

## arm-linux-gnueabihf

Full suite:

    Ok:                729
    Expected Fail:     6
    Fail:              0
    Skipped:           38

Stress (80 tests × 200):

    Ok:                16000
    Fail:              0

The Ok count is 6 short of the expected 735 because the chroot has no
`arm-linux-gnueabihf-g++`. The cross file names it as `cpp`, and meson logs
"Compiler for language cpp for the host machine not found", so the C++ tests
aren't defined on this target. Compared with the aarch64 list, these are
missing:

- `test-cdefs-cxx03`, `test-cdefs-cxx03-native`
- `test-cplusplus`, `test-cplusplus-native`
- `test-inttypes-cplusplus`
- `test-qsort-r-cplusplus`

armhf also gains `check-cdef-sys-fcntl.h`, which aarch64 doesn't list.
Installing `g++-arm-linux-gnueabihf` should bring the six C++ tests back. I
didn't install it, per the instructions.

## Failures and timeouts

None, in either target's full run or stress run. No test passed unexpectedly.

The expected failures on both targets are `test-except`,
`test-hosted-exit-fail` and `test-abort-failure`, plus the `-native` form of
each. The skipped tests are the same as last round.

`test-mqueue` ran its mq_notify checks; the "mq_notify not supported here"
message doesn't appear in either log. `test-setrlimit` passed on both
targets.

## Files

- `<target>/test.log.gz`: full-suite `meson test` output
- `<target>/meson-testlog.txt.gz`: `meson-logs/testlog.txt` from the full run
- `<target>/stress.log.gz`: the 200-repeat stress run
