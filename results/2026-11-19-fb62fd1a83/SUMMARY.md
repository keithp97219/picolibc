# Results for fb62fd1a83 on Neoverse N1 (CPU part 0xd0c)

## Commit tested

`fb62fd1a83a891fe8768e08e60ade6eb389ff272` from `claude/linux-threads`,
"libos/linux: Implement POSIX threads for arm and aarch64"

## Host

- `uname -a`: `Linux dev-dsk-keithpac-2a-8cab6371.us-west-2.amazon.com 5.10.273-289.1133.amzn2int.aarch64 #1 SMP Wed Nov 11 00:32:49 UTC 2026 aarch64 aarch64 aarch64 GNU/Linux`
- CPU part: `0xd0c` (Arm Neoverse N1, AWS Graviton2), 8 CPUs
- Host OS: Amazon Linux 2. Builds and tests ran in a Debian sid (forky)
  chroot on the same kernel, upgraded to current sid just before the run.
- `aarch64-linux-gnu-gcc (Debian 16.2.0-4) 16.2.0`
- `arm-linux-gnueabihf-gcc (Debian 16.2.0-3) 16.2.0`
- `gcc (Debian 16.2.0-4) 16.2.0`
- glibc 2.43 (`ld.so (Debian GLIBC 2.43-7)`), meson 1.12.1, ninja 1.13.2
- AArch32 works: `/lib/ld-linux-armhf.so.3` runs and armhf binaries execute
  natively, so the armhf tests ran on real hardware too.

Configure: `scripts/do-<target>-configure -Dwerror=true`, then `ninja`, then
`meson test -t 4 --print-errorlogs`. Stress: `meson test -t 4 --no-rebuild --repeat 200`
over the 74 tests that `meson test --list` matches for
`pthread|sem|rwlock|barrier|spin|cancel|atfork|timer|aio|thread`.

## aarch64-linux-gnu

Full suite:

    Ok:                728
    Expected Fail:     6
    Fail:              0
    Skipped:           36

Stress (74 tests × 200):

    Ok:                14800
    Fail:              0

## arm-linux-gnueabihf

Full suite:

    Ok:                721
    Expected Fail:     6
    Fail:              0
    Skipped:           38

Stress (74 tests × 200):

    Ok:                14800
    Fail:              0

## Failures and timeouts

None in the final runs. There were no timeouts in any run, and no thread,
synchronization, futex or atomic test failed in any run.

The expected failures on both targets are `test-except`,
`test-hosted-exit-fail` and `test-abort-failure`, plus the `-native` form of
each.

### Environment note: test-setrlimit (not a picolibc bug)

On the first full run, `test-setrlimit` and `test-setrlimit-native` failed
on both targets (SIGABRT, `assertion "ret == 0" failed:
test/test-posix/test-setrlimit.c, line 105`). That first run went through
the chroot's `sudo -u keithpac -i`, which leaves the session with
`RLIMIT_CORE` hard = 0. The test's first `setrlimit(RLIMIT_CORE, {cur=1,
max=0})` then gets EINVAL from the kernel. glibc fails the same way, which is
why the `-native` test fails too.

I reran everything in a session entered with `chroot --userspec` (hard core
limit unlimited). Both tests then pass on both targets, and all the results
above come from that rerun. The logs of the first run are in
`<target>/test-run1-sudo.log.gz`.

The test assumes the hard limit for each resource is at least 2. It might be
worth skipping a resource when `rlim_max` is below 2, so the test survives
hosts that disable core dumps with a hard limit of 0.

## Files

- `<target>/test.log.gz`: full-suite `meson test` output (final run)
- `<target>/meson-testlog.txt.gz`: `meson-logs/testlog.txt` from that run
- `<target>/stress.log.gz`: the 200-repeat stress run
- `<target>/test-run1-sudo.log.gz`: first full run, with the setrlimit failures
