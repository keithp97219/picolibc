# Results for 916004fcf6 (posix-pse) on Neoverse N1 (CPU part 0xd0c)

## Commit tested

`916004fcf6f9ef3d2d458426981905db3495ced9` from `claude/posix-pse`,
"unistd.h: Define the programming environment macros". I tested only this
commit; the earlier request for `ef8cf6ebac` is superseded by it.

## Host

- `uname -srvm`: `Linux 5.10.273-289.1133.amzn2int.aarch64 #1 SMP Wed Nov 11 00:32:49 UTC 2026 aarch64`
- CPU part: `0xd0c` (Arm Neoverse N1, AWS Graviton2), 8 CPUs
- Debian sid chroot on the same kernel, entered with `chroot --userspec`
  (hard RLIMIT_CORE unlimited)
- `aarch64-linux-gnu-gcc (Debian 16.2.0-4) 16.2.0`
- `arm-linux-gnueabihf-gcc (Debian 16.2.0-3) 16.2.0`. `arm-linux-gnueabihf-g++`
  16.2.0-3 is now installed, so the armhf C++ tests are back.
- `gcc (Debian 16.2.0-4) 16.2.0`
- glibc 2.43 (`ld.so (Debian GLIBC 2.43-7)`), meson 1.12.1, ninja 1.13.2
- AArch32 runs natively.

The builds were configured fresh with `scripts/do-<target>-configure -Dwerror=true`,
then `ninja`, then `meson test -t 4 --print-errorlogs`. Stress used
`meson test -t 4 --no-rebuild --repeat 200` over the tests matching
`pthread|sem|rwlock|barrier|spin|cancel|atfork|timer|thread|mqueue|cpuclock|time64|aio|lio_|mtx|cnd|msgrcv|shmat|spawn`:
145 on aarch64 and 144 on armhf. The lists differ only by
`test-msgrcv-native`, which the build doesn't define on armhf.

## aarch64-linux-gnu

Full suite:

    Ok:                1106
    Expected Fail:     6
    Fail:              1    (test-getnameinfo-native, glibc + site DNS, see below)
    Skipped:           38

Stress (145 tests × 200):

    Ok:                29000
    Fail:              0

## arm-linux-gnueabihf

Full suite:

    Ok:                1103
    Expected Fail:     6
    Fail:              1    (test-getnameinfo-native, glibc + site DNS, see below)
    Skipped:           40

Stress (144 tests × 200):

    Ok:                28800
    Fail:              0

## Real-hardware checks requested in CLAUDE.md

All of these ran and printed `success` (exit 0), on both targets unless noted:

| Test | aarch64 | armhf |
|---|---|---|
| test-pthread_mutex_consistent (robust mutexes) | pass | pass |
| test-msgrcv (System V IPC_STAT layout / IPC_64) | pass | **pass** |
| test-semop | pass | pass |
| test-shmat | pass | pass |
| test-telldir | pass | **pass** |
| test-mq_notify (mq_notify checks; no "not supported here") | pass | pass |

The glibc `-native` forms of test-msgrcv and test-telldir don't exist on
armhf, as the instructions say. The skip counts match: aarch64 skips 38
against 39 under qemu (test-pthread_mutex_consistent now runs), and armhf
skips 40 against 43 (test-pthread_mutex_consistent, test-msgrcv and
test-telldir now run).

The total test counts are 6 higher than the qemu-user numbers on both
targets (aarch64: 1106+1+6+38 = 1151 against 1100+6+39 = 1145; armhf:
1103+1+6+40 = 1150 against 1095+6+43 = 1144). I can't tell from here which
six tests differ. The `meson-testlog.txt.gz` files have the full lists for
comparison.

## Failures

### test-getnameinfo-native (both targets): host DNS, not picolibc

    NI_NAMEREQD: getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host), NULL, 0, NI_NAMEREQD) == EAI_NONAME

This is the glibc build only. picolibc's `test-getnameinfo` passes on both
targets. The test assumes 10.9.8.7 has no name. The chroot uses
`hosts: files dns`, and this site's DNS servers do return a PTR record for
10.9.8.7 (`getent hosts 10.9.8.7` resolves). So glibc correctly returns a
name instead of EAI_NONAME. The test could use an address that is much less
likely to have a PTR record, such as one from TEST-NET-1 (192.0.2.0/24,
RFC 5737), or skip the NI_NAMEREQD check in its `-native` form. No DNS
resolver can guarantee an address has no name, though.

No timeouts, no unexpected passes, and no stress failures.

## Expected failures

These are the same on both targets: `test-except`, `test-hosted-exit-fail`
and `test-abort-failure`, plus the `-native` form of each.

## Skips on real hardware

Every skipped test and its message is in `<target>/skips.txt`. None of them
are emulation-related. They're the usual target/feature skips (Hexagon,
RISC-V jvt, RX, semihost lseek), glibc `-native` forms of picolibc-only
APIs (Annex K `_s` functions, fdevopen, funopen, posix_getdents,
posix_close, ...), `printf: %n unsupported`, `iconvjp`, and the
locking-error tests that skip without a message. armhf also skips
`test-time-native` ("__TIMESIZE is only 32 bits") and `test-long-long-int`.

## Files

- `<target>/test.log.gz`: full-suite `meson test` output
- `<target>/meson-testlog.txt.gz`: `meson-logs/testlog.txt` from the full run
- `<target>/stress.log.gz`: the 200-repeat stress run
- `<target>/skips.txt`: each skipped test with its output
