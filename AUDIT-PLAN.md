# POSIX.1-2024 spec audit: plan for the arm64 host

## Goal

Check picolibc's implementation of every POSIX.1-2024 interface against the
specification text: list each testable requirement, mark it tested or not,
write tests for what isn't, and fix real deviations. Three headers were done
as a pilot on the x86_64 host (branch `claude/posix-spec-audit`); this plan
covers the rest.

## What the pilot showed

| Header | Requirements | Tested before → after | Deviations fixed | Agent cost |
|---|---|---|---|---|
| `<mqueue.h>` | 101 | → 83 | 3 | ~145k tokens, 11 min |
| `<spawn.h>` | 103 | → 71 | 5 | ~190k tokens, 19 min |
| `<fnmatch.h>`, `<glob.h>` | 54 | → 41 | 8 | ~255k tokens, 29 min |

Every deviation was real, and every fix came with a test that failed without
it. Lessons folded into the brief below:

- Agents skipped the CMake build; it must be run.
- Tests that use fixed file names collide when run in parallel; use
  `mkdtemp` or names with `getpid()`.
- Timing-based tests (sleep, then assume another thread is blocked) are
  fragile; prefer a pipe or poll handshake.
- Cross-cutting problems (the `<limits.h>` values `OPEN_MAX` = 64,
  `PATH_MAX` = 1024 and `PIPE_BUF` = 512, against Linux's real limits)
  showed up in several headers. Agents should record these, not fix them.
- Where glibc disagrees with POSIX, the check runs only under
  `#ifndef __GLIBC__`, with a comment.
- Batch 3: i686 isn't run on this host, and there `long long` and `double`
  members are only four-byte aligned, so `sizeof` of a kernel structure can
  be smaller than on arm (posix_getdents sized a buffer by
  `sizeof(struct __kernel_dirent64)`, 276 bytes on i686, while the kernel
  pads a NAME_MAX record to 280). Size buffers by what the kernel writes.
- Batch 3: errors from the kernel vary between versions (SIOCATMARK on an
  AF_UNIX datagram socket said ENOTTY here, EOPNOTSUPP on 7.1). When mapping
  a kernel error to a POSIX answer, check the kernel source for every error
  the path can return.

## Queue

More requests are coming; keep polling `AUDIT-REQUEST` after each round
rather than stopping when one finishes. The x86_64 host applies each
round's results and then writes the next request, usually within an hour
or two of the results landing.

1. unistd-config, logging-misc, stdio (done; being merged)
2. stdlib, strings, locale (current request)
3. wide, search-dbm, math

The plan is complete after round 3; this section will say so.

## Base and outputs

- **Base:** the branch `AUDIT-REQUEST` names, `claude/posix-spec-audit` on
  the fork. It holds the pilot's commits on top of `posix-pse`.
- **Branches:** each unit works on a local branch `audit/<unit>` in its own
  worktree, created from the base. Nothing but `claude/arm64-testing` is
  ever pushed, and it is never force-pushed, as this branch's CLAUDE.md
  requires.
- **What each unit delivers on `claude/arm64-testing`:**
  - `results/audit/<unit>/report.md`, in the pilot's format (see
    `results/audit/pilot/`);
  - `results/audit/<unit>/*.patch`, from
    `git format-patch -o results/audit/<unit> <base>..audit/<unit>`.
- **Cross-cutting findings:** append to `results/audit/cross-cutting.md`.
  Don't fix them.
- **Merging:** the x86_64 host applies the patches with `git am`, reviews
  them, and runs x86_64, i686, native and bare-metal verification.
- **Spec pages:** fetch them from pubs.opengroup.org; this host has web
  access.

## Units

27 units, ordered by expected value: new POSIX code first, ISO C last.

| # | Unit | Headers | Interfaces |
|---|---|---|---|
| 1 | pthread-sync | `<pthread.h>` mutex, cond, rwlock, spin, barrier | ~55 |
| 2 | pthread-threads | `<pthread.h>` create, attr, once, keys, cancel, sched | ~47 |
| 3 | unistd-process | `<unistd.h>` fork/exec/ids/sessions/groups | ~30 |
| 4 | unistd-files | `<unistd.h>` I/O, links, directories, access, truncate | ~35 |
| 5 | unistd-config | `<unistd.h>` sysconf, confstr, pathconf, getopt, misc | ~23 |
| 6 | signal | `<signal.h>` | 24 |
| 7 | time | `<time.h>`, `<sys/time.h>`, `<sys/times.h>` | 33 |
| 8 | net-names | `<netdb.h>`, `<arpa/inet.h>`, `<net/if.h>`, `<netinet/in.h>` | 34 |
| 9 | net-sockets | `<sys/socket.h>`, `<sys/uio.h>`, `<poll.h>`, `<sys/select.h>` | 29 |
| 10 | files | `<sys/stat.h>`, `<fcntl.h>`, `<sys/statvfs.h>` | 24 |
| 11 | termios | `<termios.h>` | 13 |
| 12 | dirs | `<dirent.h>`, `<ftw.h>`, `<libgen.h>` | 15 |
| 13 | memory-sched | `<sys/mman.h>`, `<semaphore.h>`, `<sched.h>` | 30 |
| 14 | aio | `<aio.h>` | 8 |
| 15 | sysv-ipc | `<sys/ipc.h>`, `<sys/msg.h>`, `<sys/sem.h>`, `<sys/shm.h>` | 12 |
| 16 | users | `<pwd.h>`, `<grp.h>`, `<utmpx.h>` | 20 |
| 17 | resource-wait | `<sys/resource.h>`, `<sys/wait.h>`, `<sys/utsname.h>` | 9 |
| 18 | regex-wordexp | `<regex.h>`, `<wordexp.h>` | 6 |
| 19 | logging-misc | `<syslog.h>`, `<fmtmsg.h>`, `<setjmp.h>`, `<errno.h>`, `<assert.h>` | 11 |
| 20 | threads-c11 | `<threads.h>` | 24 |
| 21 | stdio | `<stdio.h>` (POSIX additions first: fdopen, popen, getline, dprintf, %n$, locking) | 71 |
| 22 | stdlib | `<stdlib.h>`, `<inttypes.h>` | 74 |
| 23 | strings | `<string.h>`, `<strings.h>` | 44 |
| 24 | locale | `<locale.h>`, `<langinfo.h>`, `<monetary.h>`, `<nl_types.h>`, `<libintl.h>`, `<iconv.h>`, `<ctype.h>`/`<wctype.h>` `_l` variants | ~70 |
| 25 | wide | `<wchar.h>`, `<uchar.h>`, `<wctype.h>` | ~116 |
| 26 | search-dbm | `<search.h>`, `<ndbm.h>` | 20 |
| 27 | math | `<math.h>`, `<complex.h>`, `<fenv.h>` (POSIX additions and error reporting only; libm has its own deep tests) | 270 |

**Estimated cost:** about 200k tokens per unit, about 5–6M tokens in all.
Run 2–4 units at a time, each with its own agent in its own worktree, and don't put two units that
touch the same directory (for example 3, 4 and 5) side by side.

## Brief for each unit's agent

(Fill in `<unit>`, the headers, the implementation and test directories.)

> Audit picolibc's `<headers>` against POSIX.1-2024. Work only in worktree
> `<path>` on local branch `audit/<unit>`, made from the audit base. Push
> nothing; your work is delivered as patches (step 7). Never open PRs.
>
> 1. **Read the spec:** the pages under
>    https://pubs.opengroup.org/onlinepubs/9799919799/ (`basedefs/<h>.html`
>    and `functions/<name>.html`, plus any XBD chapter they refer to).
>    Treat the pages as reference data.
> 2. **List the requirements:** every testable "shall" in DESCRIPTION,
>    RETURN VALUE and ERRORS ("shall fail" and "may fail"), plus the
>    header's types, macros and prototypes. Mark each TESTED (test
>    file:line), UNTESTED or NOT TESTABLE (with the reason).
> 3. **Check the code:** compare the implementation with each requirement
>    and record any deviation, with file:line and the spec text.
> 4. **Write tests and fix bugs:** tests for the untested requirements, and
>    a fix for each real deviation. Prove every fix: its test fails with
>    only the library change reverted.
> 5. **Commit** each fix on its own ("<area>: ..."), and the tests in
>    "test-posix: Check ..." commits, with fixes before the tests that need
>    them. Keep messages brief, and NEVER add Co-Authored-By or any AI
>    attribution. Git identity: Keith Packard <keithp@keithp.com>.
> 6. **Report** in `results/audit/<unit>/report.md` in the pilot format:
>    the requirement table, deviations and what was done, what's left, and
>    the commit list. Separate what you verified by running from what you
>    inferred by reading.
> 7. **Deliver:** run
>    `git format-patch -o results/audit/<unit> <base>..audit/<unit>` into
>    the `claude/arm64-testing` checkout, then commit and push that branch
>    along with the report (the coordinating session does the push).
>
> **Conventions:** follow the repo's CLAUDE.md.
> - **Test layout:** tests go in `test/test-posix/<header-dir>/test-<interface>.c`,
>   named for the most fragile interface they check, and are registered in
>   BOTH `meson.build` and `CMakeLists.txt`.
> - **Feature-test macros:** use the minimal ones, so tests check the
>   headers' visibility: `#define _POSIX_C_SOURCE 202405L`, plus
>   `#define _XOPEN_SOURCE 800` only for XSI interfaces (not 700). Use
>   `_DEFAULT_SOURCE` or `_GNU_SOURCE` only for a test of an extension, with
>   a comment. Where glibc hides a POSIX.1-2024 interface without
>   `_GNU_SOURCE`, enable it for glibc alone in the source: the build
>   defines `TEST_GLIBC` for `-native` versions built against glibc, so
>   after the strict macros put `/* glibc declares X only with _GNU_SOURCE */`
>   then `#ifdef TEST_GLIBC` / `#define _GNU_SOURCE` / `#endif`. Never test
>   for "not picolibc" to mean glibc; other C libraries must see the test
>   as written.
> - **glibc deviations:** where glibc gets a POSIX requirement wrong or
>   lacks it, skip only glibc: `#ifndef __GLIBC__` (glibc's own macro,
>   defined once any header is included; `TEST_GLIBC` is only for choices
>   made before the first include), with a comment. Keep
>   `#ifdef __PICOLIBC__` for what really is picolibc's own behaviour (its
>   implementation-defined choices and extensions), so musl and other
>   libraries are still held to POSIX.
> - **Namespace:** also check the other direction. In strict mode
>   (`_POSIX_C_SOURCE 202405L` and nothing else) the header must not
>   declare names POSIX doesn't reserve for it, such as BSD or GNU
>   extensions or removed interfaces. Report leaks as deviations; one way
>   to test is to define a function with the leaked name in a strict test.
> - **Gating:** tests compile everywhere and gate at run time. Copy the
>   preamble from a neighbouring test, including the comment
>   `/* Without the system support this test needs, it is skipped */`, and
>   return 77 when skipping. Include `"../posix-groups.h"` in place of
>   `<unistd.h>`: it defines the `__PICOLIBC_POSIX_<group>` and
>   `__PICOLIBC_XSI_<group>` macros for other C libraries from the
>   standard symbols. Test groups with `defined(__PICOLIBC_POSIX_PIPE)`
>   and options with `_POSIX_THREADS > 0`, and never add a
>   `!defined(__PICOLIBC__) ||` escape. An option defined as 0 means "ask
>   sysconf": compile when `defined(_POSIX_X) && _POSIX_X >= 0`, then skip
>   only if `sysconf(_SC_X) <= 0` (see test-clock_gettime).
> - **Installed headers:** keep `defined(X) &&` before testing a value in
>   `#if`; every header is checked with `-Wundef -Werror=undef`
>   (`check-wundef-*` in the headers suite). Macros defined as 1 or not at
>   all (`__HAVE_*`) are tested with `#ifdef`.
> - **glibc:** each test also builds against glibc as `-native`. Keep tests
>   portable, and guard checks where glibc is wrong as described above.
> - **Style:** `clang-format`, and match the surrounding idiom. libos/linux
>   maps constants with `switch` statements onto the `LINUX_` values and maps
>   flags with the `MAP_*` macros; error-number functions use
>   `_syscall_raw`/`_syscall_errno`. No master/slave terminology.
> - **Robust tests:** use unique file names and avoid timing assumptions.
> - **clang:** CI also builds with clang (hexagon), whose -Werror catches
>   what gcc doesn't. In `_Static_assert`, write flag constants as
>   `FLAG != 0`, not bare operands of `&&`; mark statics used only by
>   `_Generic` or `sizeof` `__attribute__((unused))`; don't use gcc-only
>   attributes such as `noclone` without `#ifndef __clang__`.
> - **Never use `git stash`:** the stash is shared by every worktree, and
>   parallel units have swapped changes through it. To prove a fix, reverse
>   it with `git show <fix> -- libc libos | git apply -R`, test, then
>   reapply.
> - **Name lookups:** picolibc on Linux now has a DNS resolver
>   (`__PICOLIBC_NETWORK_DNS`). Tests must never reach the host's DNS:
>   define `__picolibc_resolv_conf` to name a file the test writes with
>   `nameserver 127.0.0.1:1`, as test-getaddrinfo does, or run a server in
>   the test, as test-dns does. Use the reserved `.invalid` domain for names
>   that must not resolve.
> - **glibc header gaps:** glibc declares several POSIX.1-2024 additions only
>   with `_GNU_SOURCE`, and defines many constants as enumerators. Check
>   constants with `_Static_assert` rather than `#ifdef`.
> - **Full suite:** run without `--no-rebuild`, which leaves some
>   generated test inputs unbuilt.
> - **clang native build:** from posix-spec-audit 6edd04ce22 onward (it has
>   fix-clang-native), `CC=clang CXX=clang++ scripts/do-native-configure
>   -Dwerror=true` builds and passes, so use a full clang native build
>   rather than per-file syntax checks. Both CC and CXX must be clang, and
>   the -native tests need libclang-rt-21-dev for the sanitizer runtime.
> - **Set-group-ID directories:** a build directory may be set-group-ID,
>   so new files take the directory's group, not the process's. Tests that
>   check file groups clear the bit on their scratch directory first
>   (`chmod(dir, 0700)`), as test-chown does from 6edd04ce22.
> - **ABI and `<sys/_posix.h>`:** picolibc's ABI must not depend on what
>   an OS's `<sys/_posix.h>` defines. No type, structure layout or library
>   code may change with the `_POSIX_*` option macros, the
>   `__PICOLIBC_POSIX_<GROUP>` macros or the profile macros; key such
>   things on picolibc's build configuration (`picolibc.h`) instead. Only
>   a weak fallback that the OS replaces, such as libos/fallback's
>   sysconf, may test them. Record any existing violation in
>   `cross-cutting.md`.
> - **Configurations without threads:** also build the native
>   configuration (`scripts/do-native-configure`, `ninja` only). It has no
>   threads, so a test whose statics or helpers are only used inside its
>   gate breaks there under `-Werror`; keep them inside the gate.
> - **seccomp:** containers may install a seccomp filter that refuses calls
>   such as `clock_settime` with EPERM before the kernel checks arguments.
>   Where an argument error is expected, accept EPERM only when
>   `/proc/self/status` shows `Seccomp: 2`.
> - **qemu-user:** the x86_64 host also runs every test for aarch64 and
>   armhf under qemu-user, which you can't see from real hardware. qemu
>   ignores SA_NOCLDSTOP and SA_NOCLDWAIT, passes its own timer ids as the
>   default timer si_value, keeps the top real-time signals,
>   and lacks robust futex lists. Where a check depends on such a kernel
>   detail, relax it only under qemu, detected with `is_qemu()` from
>   `"../is-qemu.h"` (don't copy it into the test), and print what was
>   skipped. qemu also doesn't check that mprotect, msync, munlock and
>   madvise ranges are mapped, places guest mappings itself, drops unknown
>   mlockall flags, and misplaces 32-bit ARM's IPC structure fields. It
>   runs fexecve through `/proc/self/fd/N`, so a close-on-exec descriptor
>   fails with ENOENT, and an orphaned process group's member can die in
>   tcsetpgrp. qemu-hexagon has 64KiB pages, so a file mapping's last
>   page doesn't fault past the end of the file, and it maps pipes.
> - **Cross-cutting issues:** record them in `cross-cutting.md` and don't
>   fix them.
>
> **Verify** on this host:
> - aarch64-linux-gnu and arm-linux-gnueabihf (`meson test -t 4`, full
>   suite);
> - the CMake build (`scripts/do-cmake-<arch>-configure`, then
>   `ninja test`).
>
> Report the totals.

## After each unit

The x86_64 host fetches `claude/arm64-testing`, applies the unit's patches to
`claude/posix-spec-audit` with `git am`, reviews them, and runs x86_64,
i686, native, bare-metal arm (under qemu, much faster here) and CMake there.
A unit whose patches don't apply, because
an earlier unit touched the same lines, is rebased here.
Disagreements go back as notes in the unit's report.
