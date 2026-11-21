# `<unistd.h>` configuration and remaining interfaces: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-unistd-config`, branch `audit/unistd-config`, based on 6edd04ce22 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43, clang 21) as an unprivileged user. Relevant host facts:
- RLIMIT_STACK 10MiB, both soft and hard.
- RLIMIT_NPROC unlimited, RLIMIT_NOFILE 65535, RLIMIT_SIGPENDING 30446.
- 8 CPUs, online and possible.
- No `/etc/hostid`; Seccomp 0; the build directory is on ext4.

**Interfaces.**
- **Configuration:** sysconf, pathconf, fpathconf and confstr.
  - Also the `<unistd.h>` version, option and programming-environment macros, and every `_SC_`, `_PC_` and `_CS_` name.
  - Also the `<limits.h>` values they report: runtime invariant, pathname variable, runtime increasable, minimum, and the other invariant values.
  - The ISO C numerical limits are left alone, except LONG_BIT and WORD_BIT, which POSIX adds.
- **The remaining `<unistd.h>` functions:** getopt (with optarg, optind, opterr and optopt), gethostname, and four XSI functions: gethostid, swab, crypt and encrypt (obsolescent). setkey (`<stdlib.h>`) is included because it belongs to the same encryption group.
- **Covered by other units:**
  - sync, fsync and fdatasync (unistd-files).
  - The process and file interfaces (unistd-process and unistd-files).
  - isatty and ttyname (termios).
  - ctermid is a `<stdio.h>` function, which picolibc declares in `<unistd.h>` only for XSI before Issue 7.
  - getpagesize isn't in Issue 8; the only check is that it stays hidden.

**Implementation:**
- `libos/linux/`: sysconf.c, pathconf.c, fpathconf.c, local-statfs.h, gethostname.c, and the generated `machine/*/linux/linux-resource.h` with `utils/make-resource.c`.
- `libos/fallback/`: sysconf.c, fpathconf.c, pathconf.c. Also `libos/semihost/common/sysconf.c`.
- `libc/posix/unistd/`: confstr.c, gethostid.c, and the new crypt.c and encrypt.c. Also the new `libc/posix/stdlib/setkey.c`, plus `libc/stdlib/getopt.c` and `libc/string/swab.c`.
- Headers: `libc/include/sys/unistd.h`, `libos/linux/include/sys/_posix.h`, `libc/include/sys/syslimits.h`, `libc/include/limits.h`.

**Tests.** All are in `test/test-posix/test-unistd/`. File names below drop the `test-` prefix, and line numbers are for the branch tip.
- New files:
  - optind.c, the getopt test (the name test-getopt is taken by test/test-getopt.c).
  - sysconf-limits.c and sysconf-arg_max.c.
  - unistd_h-config.c, swab.c and crypt.c.
- Extended: pathconf.c, confstr.c, gethostname.c, gethostid.c and unistd_h-names.c.
- sysconf-arg_max.c execs its own binary through `/proc/self/exe`.
- Scratch files use mkdtemp or the per-variant TEST_FILE_NAME.
- The host name and host id are never printed or compared with constants.

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799:
- basedefs/unistd.h and limits.h.
- functions/sysconf, fpathconf, confstr, getopt, gethostname, gethostid, swab, sync, crypt, encrypt, setkey and ctermid. getpagesize has no page.

The kernel limits come from v5.10:
- `fs/exec.c`: ARG_MAX and `_STK_LIM`.
- `include/linux/namei.h`: MAXSYMLINKS.
- `kernel/time/posix-timers.c` and `kernel/signal.c`: each timer takes a queued signal, counted against RLIMIT_SIGPENDING.
- `include/uapi/linux/limits.h`.

**Status key.** TESTED (file:line), UNTESTED, NOT TESTABLE (reason).
- Every TESTED row was verified by running it on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and, unless the row says otherwise, for the `-native` glibc build.
- "(new)" marks checks this unit added; "(existing)" marks checks that were already there.

## Requirement table

### `<limits.h>`, the configuration values

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | A runtime invariant value the header defines equals sysconf's (AIO_*, ATEXIT_MAX, IOV_MAX, PTHREAD_*, RTSIG_MAX, SEM_VALUE_MAX, TZNAME_MAX, ...) | TESTED (new) | sysconf-limits.c:299-429. **Deviation 5, fixed** |
| L2 | One that varies is omitted (ARG_MAX, CHILD_MAX, OPEN_MAX on Linux) | UNTESTED | **Deviation, not fixed** (ABI; see cross-cutting.md). sysconf-limits.c:440-448 checks only that sysconf is never more restrictive |
| L3 | Pathname variable values that vary by file are omitted (LINK_MAX, NAME_MAX, PATH_MAX, PIPE_BUF) | UNTESTED | **Deviation, not fixed** (ABI; see cross-cutting.md) |
| L4 | The runtime increasable values are defined, each at least its minimum: BC_*, CHARCLASS_NAME_MAX, COLL_WEIGHTS_MAX, EXPR_NEST_MAX, LINE_MAX, NGROUPS_MAX, RE_DUP_MAX | TESTED (new) | sysconf-limits.c:121-130. **Deviations 1 and 2, fixed** (COLL_WEIGHTS_MAX was 0; six were missing) |
| L5 | sysconf reports at least each runtime increasable value | TESTED (new) | sysconf-limits.c:453-461 |
| L6 | The minimum values are exactly POSIX's | TESTED (new) | sysconf-limits.c:66-117. **Deviation 1, fixed**. The `_XOPEN_*` checks run for picolibc only (glibc gap) |
| L7 | `_POSIX_CLOCKRES_MIN` is 20000000 | TESTED (new) | sysconf-limits.c:118. **Deviation 2, fixed** |
| L8 | LONG_BIT and WORD_BIT are defined and usable in `#if` | TESTED (new) | sysconf-limits.c:136-140. **Deviation 2, fixed** |
| L9 | NL_ARGMAX, NL_MSGMAX, NL_SETMAX, NL_TEXTMAX and NL_LANGMAX (XSI) are at least their minimums | TESTED (new) | sysconf-limits.c:131-135. **Deviation 2, fixed** |
| L10 | GETENTROPY_MAX, NSIG_MAX, NZERO | TESTED | by the unistd-files, signal and resource-wait units |
| L11 | XSI: NAME_MAX, PATH_MAX and IOV_MAX are at least `_XOPEN_NAME_MAX`, `_XOPEN_PATH_MAX` and `_XOPEN_IOV_MAX` | TESTED (new) | sysconf-limits.c:141-149 |
| L12 | STREAM_MAX, if defined, is FOPEN_MAX | TESTED (new) | sysconf-limits.c:404-410. It isn't defined; sysconf now reports FOPEN_MAX (**Deviation 5**) |
| L13 | PAGESIZE and PAGE_SIZE agree | TESTED (new) | sysconf-limits.c:351-358. Neither is defined; the two sysconf names agree |
| L14 | Every macro is usable in `#if` | TESTED (new) | the `#if` lines of sysconf-limits.c and unistd_h-config.c; check-wundef in the headers suite |

### `<unistd.h>`, the configuration part

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | `_POSIX_VERSION` is 202405L when POSIX.1-2024 is claimed | TESTED (new) | unistd_h-config.c:52-55. Undefined on Linux by design (see cross-cutting.md) |
| U2 | `_POSIX2_VERSION` likewise, or -1 | TESTED (new) | unistd_h-config.c:56-59 |
| U3 | `_XOPEN_VERSION` is 800, and only defined with XSI | TESTED (new; glibc has 700) | unistd_h-config.c:60-63. Not defined by picolibc |
| U4 | Each option POSIX limits to -1, 0 or 202405L takes one of those values | TESTED (new; picolibc only, glibc uses 200809L) | unistd_h-config.c:69-190 |
| U5 | The options that are always 202405L | TESTED (new; picolibc only) | unistd_h-config.c:192-236 |
| U6 | JOB_CONTROL, REGEXP, SAVED_IDS and SHELL are greater than 0 | TESTED (new) | unistd_h-config.c:240-251 |
| U7 | CHOWN_RESTRICTED, NO_TRUNC and VDISABLE are defined, and not -1 | TESTED (new) | unistd_h-config.c:257-275. Already defined at the base; the backlog note was stale |
| U8 | `_XOPEN_ENH_I18N` and `_XOPEN_SHM` are not -1 | TESTED (new), when defined | unistd_h-config.c:276-281. Undefined, because XSI isn't claimed |
| U9 | A supported `_POSIX_V8_*` environment matches the type sizes | TESTED (new) | unistd_h-config.c:284-299 |
| U10 | Every `_SC_` name is defined, with a distinct value; only `_SC_PAGESIZE` and `_SC_PAGE_SIZE` may match | TESTED (new) | unistd_h-config.c:395 (a duplicate is a duplicate `case`). **Deviations 3 and 12, fixed** |
| U11 | Every `_PC_` name, distinct | TESTED (new) | unistd_h-config.c:516 |
| U12 | Every `_CS_` name, distinct | TESTED (new) | unistd_h-config.c:548. **Deviations 3 and 12, fixed** |
| U13 | optarg, opterr, optind and optopt have POSIX's types | TESTED (new) | unistd_h-config.c:751-754 |
| U14 | Prototypes of getopt, sysconf, pathconf, fpathconf, confstr, gethostname, gethostid, crypt, encrypt and swab | TESTED (new; crypt and encrypt picolibc only) | unistd_h-config.c:755-767. **Deviation 11** (encrypt's prototype) |
| U15 | Strict mode hides crypt, encrypt, gethostid, swab, getpagesize, getpass and optreset | TESTED (new) | unistd_h-names.c:66-85. **Deviation 4, fixed** |
| U16 | The execution-time constants are undefined (they vary by file) | n/a | allowed |

### sysconf

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | Every variable in the table is supported (no EINVAL) | TESTED (new) | unistd_h-config.c:778-793. **Deviations 3, 5 and 13, fixed** |
| S2 | An invalid name gives -1 with EINVAL | TESTED (existing) | sysconf.c:484 |
| S3 | Options report their `<unistd.h>` value, or -1 | TESTED (existing; picolibc only) | sysconf.c:66-470 |
| S4 | Valid names leave errno alone, including "no limit" | TESTED (new) | unistd_h-config.c:778-793 |
| S5 | Never more restrictive than `<limits.h>` | TESTED (new) | sysconf-limits.c:299-461. **Deviation 5, fixed** |
| S6 | Values don't change during the process, except OPEN_MAX | UNTESTED | **Deviation, not fixed**: ARG_MAX, CHILD_MAX, SIGQUEUE_MAX and TIMER_MAX follow setrlimit on Linux |
| S7 | `_SC_OPEN_MAX` is RLIMIT_NOFILE | TESTED (new) | sysconf-limits.c:491 (compared with /proc/self/limits) |
| S8 | `_SC_ARG_MAX` is the real exec limit | TESTED (new) | sysconf-arg_max.c:179-184, with the current stack limit and a 256KiB one. **Deviation 6, fixed**. The 6MiB cap is NOT TESTABLE here (the hard stack limit is 10MiB) |
| S9 | `_SC_CHILD_MAX` is RLIMIT_NPROC | TESTED (new) | sysconf-limits.c:494. **Deviation 6** |
| S10 | `_SC_SIGQUEUE_MAX` is RLIMIT_SIGPENDING | TESTED (new) | sysconf-limits.c:497. **Deviation 6** |
| S11 | `_SC_TIMER_MAX` | TESTED (minimum only) | sysconf-limits.c:418 |
| S12 | `_SC_MQ_OPEN_MAX` is real: N descriptors open, N+1 fail | TESTED (new) | sysconf-limits.c:596-598. **Deviation 6** (it was 8) |
| S13 | A chain of `_SC_SYMLOOP_MAX` links resolves; one more gives ELOOP | TESTED (new; the ELOOP half is skipped for glibc's -1) | sysconf-limits.c:568-569. **Deviation 6** (it was 8; Linux follows 40) |
| S14 | writev takes `_SC_IOV_MAX` vectors; one more gives EINVAL | TESTED (new) | sysconf-limits.c:553-555. **Deviation 5** (the fallback said 16) |
| S15 | A time zone name of `_SC_TZNAME_MAX` bytes is accepted, a longer one isn't | TESTED (new; the longer-name half is picolibc only) | sysconf-limits.c:527-535. **Deviation 5** (it was 3; tzset takes 10) |
| S16 | sem_init takes `_SC_SEM_VALUE_MAX` and rejects one more | TESTED (new) | sysconf-limits.c:612-615. **Deviation 5** |
| S17 | `_SC_ATEXIT_MAX` handlers can be registered with atexit and with at_quick_exit | TESTED (new) | sysconf-limits.c:512-514 |
| S18 | 1 <= ONLN <= CONF, and ONLN matches /sys | TESTED (new) | sysconf-limits.c:469-473. **Deviations 3 and 6** |
| S19 | SIGRTMAX < `_SC_NSIG` <= NSIG_MAX | TESTED (new; picolibc only, as glibc has no `_SC_NSIG`) | sysconf-limits.c:480. **Deviation 3** |
| S20 | SIGRTMIN..SIGRTMAX holds at least `_SC_RTSIG_MAX` signals | TESTED (new; picolibc only) | sysconf-limits.c:484 |
| S21 | PAGESIZE is a power of two and equals PAGE_SIZE | TESTED (new) | sysconf-limits.c:357-358 |
| S22 | `_SC_CLK_TCK` > 0 | TESTED (existing and new) | sysconf.c:479; sysconf-limits.c:464. **Deviation 13, fixed** (fallback) |
| S23 | The supplementary groups fit `_SC_NGROUPS_MAX` | TESTED (new) | sysconf-limits.c:503 |
| S24 | `_SC_HOST_NAME_MAX` bounds gethostname, and is at least 255 | TESTED (new; the minimum is picolibc only) | gethostname.c:80-89 |
| S25 | The thread, login, tty, getpw/getgr, delaytimer and aio limits are actually reached | UNTESTED | only the value and minimum are checked; other units own the behaviour |
| S26 | sysconf is async-signal-safe | UNTESTED | by reading: only raw system calls, no locks |
| S27 | The semihost sysconf answers every name | NOT TESTABLE here | **Deviation 7, fixed** by reading; the CMake thumbv7m build passes the same either way |

### pathconf, fpathconf

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | For a directory, `_PC_NAME_MAX` applies to names within it | TESTED (existing) | pathconf.c:86-87 |
| P2 | `_PC_PIPE_BUF` on a pipe | TESTED (existing) | pathconf.c:95-96 |
| P3 | Limits are at least the POSIX minimums, or -1 | TESTED (existing and new) | pathconf.c:76-120 |
| P4 | Success leaves errno alone | TESTED (new) | pathconf.c:100-122 |
| P5 | CHOWN_RESTRICTED and NO_TRUNC aren't -1 when claimed | TESTED (new) | pathconf.c:101-106. **Deviation 8** |
| P6 | With NO_TRUNC, a name longer than NAME_MAX is an error | TESTED (new) | pathconf.c:152-165 |
| P7 | `_PC_ALLOC_SIZE_MIN` is what a small file actually takes | TESTED (new) | pathconf.c:128-148. **Deviation 8, fixed** (it was 0) |
| P8 | `_PC_REC_XFER_ALIGN` and `_PC_REC_MIN_XFER_SIZE` are positive | TESTED (new) | pathconf.c:143-146 |
| P9 | `_PC_TEXTDOMAIN_MAX` meets the base and XSI minimums | TESTED (new; glibc lacks the name) | pathconf.c:115-120. **Deviation 8, fixed** (it was 11) |
| P10 | FILESIZEBITS >= 32; SYMLINK_MAX; TIMESTAMP_RESOLUTION > 0 | TESTED (new) | pathconf.c:107-114 |
| P11 | `_PC_2_SYMLINKS` where links can be made | TESTED (new) | pathconf.c:186 |
| P12 | Not more restrictive than `<limits.h>` | TESTED by reading | PATH_MAX, PIPE_BUF and LINK_MAX report the header values (see L3) |
| P13 | EINVAL for a bad name, from pathconf and fpathconf | TESTED (existing and new) | pathconf.c:203, 207 |
| P14 | EOVERFLOW for TIMESTAMP_RESOLUTION | NOT TESTABLE | the resolution is 1ns |
| P15 | EACCES | TESTED (new; unprivileged only) | pathconf.c:189-198 |
| P16 | ELOOP | TESTED (new) | pathconf.c:181-185 |
| P17 | ENAMETOOLONG | TESTED (new) | pathconf.c:162-164 |
| P18 | ENOENT, also for "" | TESTED (existing and new) | pathconf.c:212-214 |
| P19 | ENOTDIR, for a path prefix and before a trailing slash | TESTED (new) | pathconf.c:167-176 |
| P20 | EBADF | TESTED (existing) | pathconf.c:210 |
| P21 | EINVAL when a name doesn't apply to the file (may fail) | UNTESTED | Linux answers every name for every file |
| P22 | VDISABLE, MAX_CANON and MAX_INPUT on a terminal | UNTESTED | needs a terminal |
| P23 | ASYNC_IO, SYNC_IO, PRIO_IO and FALLOC for each file | UNTESTED | by reading: PRIO_IO is now -1 (**Deviation 8**) |

### confstr

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Every name POSIX lists is valid | TESTED (new) | confstr.c:126-176; unistd_h-config.c:795-802. **Deviation 3, fixed** |
| C2 | Copies the value, truncated and null-terminated | TESTED (existing and new) | confstr.c:118-120, 172-175 |
| C3 | len 0 with a null buf returns the size | TESTED (existing) | confstr.c:116-117 |
| C4 | An invalid name gives 0 with EINVAL | TESTED (existing) | confstr.c:121-122 |
| C5 | A name with no value gives 0 with errno unchanged | TESTED (new; either outcome is accepted) | confstr.c:169-171 |
| C6 | `_CS_PATH` finds the standard utilities | TESTED (existing, weak) | confstr.c:119 |
| C7 | `_CS_V8_ENV` | TESTED (new) | the value is empty: no variables are needed |
| C8 | `_CS_POSIX_V8_WIDTH_RESTRICTED_ENVS` lists the supported environments whose types fit in a long | TESTED (new) | confstr.c:178-210. **Deviation 9, fixed** (it was empty) |
| C9 | The compiler flag values work with c17 | NOT TESTABLE | there is no c17 utility; the values are empty |

### getopt

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | The system initializes optind to 1 | TESTED (new) | optind.c:125. **Deviation 10** (it was 0) |
| G2 | opterr starts non-zero | TESTED (new) | optind.c:126 |
| G3 | optind moves only when an element is finished, including grouped options | TESTED (new) | optind.c:138-143 |
| G4 | Argument in the next element: optind moves by 2 | TESTED (existing and new) | optind.c:133 |
| G5 | Argument in the same element: optind moves by 1 | TESTED (new) | optind.c:134 |
| G6 | An option-argument may start with '-' | TESTED (new) | optind.c:148 |
| G7 | A null argv[optind] gives -1, optind unchanged | TESTED (new) | optind.c:174-176 |
| G8 | An operand gives -1, optind unchanged; no permutation | TESTED (new) | optind.c:155-160. **Deviation 10** |
| G9 | "-" gives -1, optind unchanged | TESTED (new) | optind.c:163-166. **Deviation 10** |
| G10 | "--" gives -1 after incrementing optind | TESTED (new) | optind.c:135, 169-171 |
| G11 | ':' as an option character gives '?' | TESTED (new) | optind.c:186-189. **Deviation 10** |
| G12 | An unknown option gives '?' | TESTED (existing and new) | optind.c:151, 179-183 |
| G13 | optopt is the character that caused the error | TESTED (new) | optind.c:152, 182, 189, 205, 209. **Deviation 10** |
| G14 | A missing option-argument gives ':' with a leading ':', otherwise '?' | TESTED (new) | optind.c:204, 208, 213 |
| G15 | After a missing option-argument, optind is past argc | TESTED (new; picolibc only, glibc gives argc) | optind.c:197-213. **Deviation 10** |
| G16 | With opterr set, a diagnostic names the program and the option | TESTED (new) | optind.c:243-252 |
| G17 | A leading ':' (also after '+') silences the diagnostics | TESTED (new) | optind.c:254-262. **Deviation 10** |
| G18 | opterr = 0 silences them | TESTED (new) | optind.c:264-271 |
| G19 | A leading '+' has no effect | TESTED (new) | optind.c:215-223 |
| G20 | A write error sets stderr's error indicator, and getopt still succeeds | TESTED (new) | optind.c:281-290 |
| G21 | No diagnostics when there are no errors | TESTED (new) | optind.c:273-279 |
| G22 | Setting optind to 1 rescans | TESTED (new) | optind.c:225-235 |
| G23 | Restarting inside an element, and optind = 0 | n/a | unspecified (the GNU optind = 0 reset still works) |

### gethostname, gethostid

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | Returns uname's nodename | TESTED (existing) | gethostname.c:77 |
| H2 | Null-terminated when it fits, including an exact-size buffer | TESTED (new) | gethostname.c:76, 91-96 |
| H3 | A short buffer gets a truncated name | TESTED (every size; picolibc only, glibc gives ENAMETOOLONG) | gethostname.c:98-113 |
| H4 | Nothing is written past namelen | TESTED (new) | gethostname.c:96, 112 |
| H5 | The name is at most HOST_NAME_MAX bytes | TESTED (new) | gethostname.c:80-89 |
| H6 | Returns 0 | TESTED (existing) | gethostname.c:75 |
| I1 | gethostid returns a 32-bit identifier | TESTED (new; 64-bit long only) | gethostid.c:87-89 |
| I2 | The identifier is stable, and the same in another process | TESTED (existing and new) | gethostid.c:86, 92-99 |
| I3 | `/etc/hostid` is honoured | UNTESTED here | the file doesn't exist on this host; glibc is skipped without it (it would use DNS) |

### swab, crypt, encrypt, setkey

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | Copies nbytes, exchanging adjacent bytes | TESTED (new) | swab.c:72-79 |
| W2 | An odd nbytes exchanges nbytes - 1 bytes and writes nothing past nbytes | TESTED (new) | swab.c:80-82 |
| W3 | A negative nbytes does nothing | TESTED (new) | swab.c:85-95 |
| W4 | The source is unchanged | TESTED (new) | swab.c:75 |
| E1 | crypt returns a hash starting with the salt, or a null pointer with errno set | TESTED (new; not built natively) | crypt.c:69-78. **Deviation 11, fixed** |
| E2 | ENOSYS when unsupported, consistent with `_XOPEN_CRYPT` | TESTED (new) | crypt.c:72-74, 99-103 |
| E3 | encrypt and setkey keep errno on success, or set ENOSYS | TESTED (new) | crypt.c:84-101 |
| E4 | encrypt produces 0 and 1 bytes, and decoding works | UNTESTED | only runs where encryption is supported |

**Counts.** 127 requirements, plus 2 n/a rows. Tested before: 19. Tested after: 114. Untested: 10. Not testable: 3.

## Deviations found and fixed

1. **The `<limits.h>` minimums came from older editions** (b6759b85db).
   - `_POSIX_CHILD_MAX` 6 is now 25, `_POSIX_NGROUPS_MAX` 0 is now 8, `_POSIX_OPEN_MAX` 16 is now 20, `_POSIX_PATH_MAX` 255 is now 256, and `_POSIX_TZNAME_MAX` 3 is now 6.
   - COLL_WEIGHTS_MAX was 0, below its minimum of 2.
   - Before the fix: six `static assertion failed` errors.
2. **Constants the header must define were missing** (fff5bc1bd1): BC_BASE_MAX, BC_DIM_MAX, BC_SCALE_MAX, BC_STRING_MAX, CHARCLASS_NAME_MAX, NGROUPS_MAX, NL_MSGMAX, NL_SETMAX, NL_TEXTMAX, NL_LANGMAX, LONG_BIT, WORD_BIT and `_POSIX_CLOCKRES_MIN`.
   - The NL_* constants take their minimums, since catopen never opens a catalog.
   - Before the fix: `'BC_BASE_MAX' undeclared` and similar errors.
3. **sysconf and confstr names were missing** (68b2b25a7f): `_SC_NSIG`, `_SC_NPROCESSORS_CONF`, `_SC_NPROCESSORS_ONLN`, `_SC_DEVICE_CONTROL`, `_CS_POSIX_V8_*` and `_CS_V8_ENV`.
   - Before the fix: `'_SC_NSIG' undeclared` and similar errors.
4. **`optreset` was declared in strict mode but defined nowhere** (ff36345f56).
   - Before the fix: `static declaration of 'optreset' follows non-static declaration`.
5. **The fallback sysconf contradicted `<limits.h>`** (797734c9ab). Linux falls through to it for every name libos/linux doesn't answer.
   - It returned the POSIX minimums for ARG_MAX, CHILD_MAX, IOV_MAX (16), OPEN_MAX, RTSIG_MAX (8), SEM_VALUE_MAX (32767) and TZNAME_MAX (3), and 8 for STREAM_MAX, against FOPEN_MAX's 32.
   - The four thread limits failed with EINVAL; the code was under `#if 0`, with the wrong names.
   - Before the fix: `_SC_THREAD_DESTRUCTOR_ITERATIONS: sysconf -1, <limits.h> 4`. The other values were seen with a probe at the base.
6. **libos/linux sysconf didn't report the kernel's limits** (0df7839a19).
   - ARG_MAX is max(min(stack/4, 6MiB), 128KiB). It was stack/4, and -1 with an unlimited stack.
   - CHILD_MAX was 6; it is now RLIMIT_NPROC.
   - SIGQUEUE_MAX and TIMER_MAX were 32; they are now RLIMIT_SIGPENDING.
   - MQ_OPEN_MAX was 8 and SEM_NSEMS_MAX 256; neither has a limit of its own, so both are now -1.
   - SYMLOOP_MAX was 8; the kernel follows 40.
   - The CPU counts are new.
   - The resource limits are read with `_syscall_raw`, so errno is untouched and sysconf adds no cancellation point.
   - Before the fix: `256KiB stack: _SC_ARG_MAX 65536`, then `arguments over _SC_ARG_MAX give E2BIG` failed (exec succeeded). sysconf-limits failed at `_SC_NPROCESSORS_ONLN counts the online CPUs`. The other values were seen with a probe at the base.
7. **The semihost sysconf failed every name except `_SC_CLK_TCK` with EINVAL** (699e13ea03). It now defers to `__fallback_sysconf`. Inferred by reading only: the CMake thumbv7m tests pass with and without the change, so that build doesn't seem to link this file.
8. **pathconf on Linux** (c884ef1429).
   - `_PC_ALLOC_SIZE_MIN` was 0; it is now f_frsize.
   - `_PC_REC_MIN_XFER_SIZE` and `_PC_REC_XFER_ALIGN` are now f_bsize.
   - `_PC_TEXTDOMAIN_MAX` was 11; it is now f_namemax - 3 (the XSI minimum is 252).
   - CHOWN_RESTRICTED and NO_TRUNC now return the header values, and PRIO_IO is -1.
   - Before the fix: `XSI _PC_TEXTDOMAIN_MAX: v >= _XOPEN_NAME_MAX - 3`.
9. **`_CS_POSIX_V8_WIDTH_RESTRICTED_ENVS` was empty** (68fb4e7c4c, 13247b3aca). It now lists picolibc's environments when all the listed types fit in a long.
   - Before the fix (both answers made empty): `this environment is width-restricted: found`.
10. **getopt broke several POSIX rules** (47dc139dba).
    - optind started at 0.
    - "-" was parsed as option '\0', because strchr matches optstring's terminator.
    - ':' was accepted as an option character.
    - optopt was '?' for unknown options.
    - A leading ':' didn't silence the diagnostics.
    - argv was permuted instead of stopping at the first operand.
    - A missing option-argument left optind at argc.
    - getopt_long and getopt_long_only still permute.
    - Before the fix: `initial optind`. The other problems were seen with a probe at the base.
11. **crypt, encrypt and setkey were declared but undefined** (a89d78f127). They now fail with ENOSYS, and encrypt's prototype now takes `char[64]`.
    - Before the fix: `undefined reference to 'crypt'`, `'setkey'` and `'encrypt'`.
12. **The V8 `_SC_` and `_CS_` names shared the V7 values** (13247b3aca). POSIX lets only PAGESIZE and PAGE_SIZE share a value.
    - Before the fix: four `duplicate case value` errors.
13. **The fallback sysconf failed `_SC_CLK_TCK` with EINVAL** (8e36b1579f). It now returns CLOCKS_PER_SEC, the rate picolibc's clock() assumes for times().
    - Found by the clang native build. Before the fix (native configuration): `_SC_CLK_TCK: sysconf(_SC_CLK_TCK) > 0` and `sysconf(_SC_CLK_TCK): errno 22`.

**Also fixed in the tests:**
- test-gethostid made glibc resolve the host name, possibly through DNS, when there is no `/etc/hostid`; the native build now skips in that case.
- test-gethostname's overrun guard wasn't next to the buffer, so it couldn't see an overrun.

## Deviations not fixed

- **L2 and L3: `<limits.h>` constants that vary on Linux.** Changing them is an ABI decision; the recommendation, an OS hook header, is in cross-cutting.md.
- **S6: values that follow setrlimit.** The kernel applies the current limit at each exec or fork, and glibc does the same.
- **U1 and U8: `_POSIX_VERSION` and `_XOPEN_*` are undefined on Linux.** This is deliberate.
- **HOST_NAME_MAX stays 255 while Linux names are at most 64 bytes.** That is allowed, since it is never more restrictive.

## Other observations

- These backlog values were already consistent at this base and are now checked by tests: `_SC_DELAYTIMER_MAX`, `_SC_TTY_NAME_MAX`, `_SC_LOGIN_NAME_MAX`, `_SC_GETPW_R_SIZE_MAX`, `_SC_GETGR_R_SIZE_MAX`, the aio limits and `_PC_SYMLINK_MAX`.
- The glibc 2.43 gaps met here are listed in cross-cutting.md.

## Left undone

- S11, S25, S26, P21-P23, I3 and E4.
- The 6MiB ARG_MAX cap: sysconf-arg_max.c checks it wherever the hard stack limit allows.
- qemu-user and bare metal: neither was run on this host.
  - sysconf-arg_max skips under `is_qemu()`.
  - Deviation 7 should be confirmed on the x86_64 host.

## Test runs (verified by running)

- **aarch64-linux-gnu** (full suite): Ok 1589, Fail 0, Expected Fail 6, Skipped 43.
- **arm-linux-gnueabihf**: Ok 1588, Fail 0, Expected Fail 6, Skipped 45.
- **Native gcc configuration** (`ninja` only): builds cleanly.
- **clang native** (CC=clang CXX=clang++, full suite): Ok 1214, Fail 0, Expected Fail 6, Skipped 384.
- **CMake thumbv7m** (`ninja test`): 624 tests, 100% passed (273 ran, the rest skipped).
- **This unit's tests:** all pass on both Linux targets, for picolibc and `-native`. The exceptions are test-gethostid-native, which skips (no `/etc/hostid`), and test-crypt, which isn't built natively.
- **clang-format:** clean on all 28 changed C files.
- **Fix proofs:** listed with each deviation above.
- **Incidents:** none from the host security software. armhf, CMake and clang-native each caught a test portability bug in a first round of runs (type-limits, getgroups linking, and /proc and /sys checks in the native configuration). These were fixed in their commits before the final runs above.

## Commits (audit/unistd-config, on top of 6edd04ce22)

- b6759b85db limits: Use POSIX.1-2024's minimum values
- fff5bc1bd1 limits: Define the constants POSIX requires that were missing
- 68b2b25a7f unistd.h: Define the sysconf and confstr names POSIX.1-2024 added
- ff36345f56 unistd.h: Don't declare optreset
- 797734c9ab libos/fallback: Report the <limits.h> values from sysconf
- 0df7839a19 libos/linux: Report the kernel's limits from sysconf
- 699e13ea03 libos/semihost: Answer the other sysconf names from the fallback
- c884ef1429 libos/linux: Report allocation, text domain and option values from pathconf
- 68fb4e7c4c confstr: List the width-restricted programming environments
- 47dc139dba getopt: Follow the POSIX rules in getopt
- a89d78f127 posix: Define crypt, encrypt and setkey, failing with ENOSYS
- 13247b3aca unistd.h: Give the V8 sysconf and confstr names their own values
- 8e36b1579f libos/fallback: Report _SC_CLK_TCK from sysconf
- ce732d5675 test-posix: Check the POSIX rules in getopt
- 25ff2bc0c4 test-posix: Check sysconf's limits against <limits.h> and the system
- 167c49a3bd test-posix: Check the configuration part of <unistd.h>
- 46fcad382e test-posix: Check pathconf's options, allocation size and errors
- 341343d18d test-posix: Check every confstr name and the width-restricted environments
- 2e34e550c1 test-posix: Check gethostname's buffers and keep gethostid away from DNS
- 12f81588a9 test-posix: Check swab, crypt, encrypt and setkey
