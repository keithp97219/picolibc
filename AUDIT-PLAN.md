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
  `#ifdef __PICOLIBC__`, with a comment.

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
> - **Gating:** tests compile everywhere and gate at run time. Copy the
>   preamble from a neighbouring test, including the comment
>   `/* Without the system support this test needs, it is skipped */`, and
>   return 77 when skipping.
> - **glibc:** each test also builds against glibc as `-native`. Keep tests
>   portable, and guard checks where glibc is wrong with
>   `#ifdef __PICOLIBC__` plus a comment.
> - **Style:** `clang-format`, and match the surrounding idiom. libos/linux
>   maps constants with `switch` statements onto the `LINUX_` values and maps
>   flags with the `MAP_*` macros; error-number functions use
>   `_syscall_raw`/`_syscall_errno`. No master/slave terminology.
> - **Robust tests:** use unique file names and avoid timing assumptions.
> - **Never use `git stash`:** the stash is shared by every worktree, and
>   parallel units have swapped changes through it. To prove a fix, reverse
>   it with `git show <fix> -- libc libos | git apply -R`, test, then
>   reapply.
> - **Name lookups:** picolibc never uses DNS. Use the reserved `.invalid`
>   domain for names that must not resolve, and run "no name" checks only
>   under `__PICOLIBC__`.
> - **glibc header gaps:** glibc declares several POSIX.1-2024 additions only
>   with `_GNU_SOURCE`, and defines many constants as enumerators. Check
>   constants with `_Static_assert` rather than `#ifdef`.
> - **Full suite:** run without `--no-rebuild`, which leaves some
>   generated test inputs unbuilt.
> - **qemu-user:** the x86_64 host also runs every test for aarch64 and
>   armhf under qemu-user, which you can't see from real hardware. qemu
>   ignores SA_NOCLDSTOP and SA_NOCLDWAIT, keeps the top real-time signals,
>   and lacks robust futex lists. Where a check depends on such a kernel
>   detail, relax it only under qemu, detected with the `is_qemu()` helper in
>   `test-signal/test-sigaction.c` (BogoMIPS 100.00 in /proc/cpuinfo), and
>   print what was skipped.
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
