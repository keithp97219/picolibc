# `<syslog.h>`, `<fmtmsg.h>`, `<setjmp.h>`, `<errno.h>`, `<assert.h>`: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-logging-misc`, branch `audit/logging-misc`, based on 6edd04ce22 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43, clang 21) as an unprivileged user.

**Interfaces.**
- closelog, openlog, setlogmask and syslog (XSI), with the LOG_* constants and LOG_MASK/LOG_UPTO.
- fmtmsg (XSI), with the MM_* constants and MSGVERB.
- setjmp, longjmp, sigsetjmp and siglongjmp, with jmp_buf and sigjmp_buf.
- errno and the E* constants.
- assert and NDEBUG, plus static_assert (ISO C).

**What's out of scope:**
- `_setjmp` and `_longjmp` were removed in Issue 8 (Austin Group Defect 1330; their pages are gone). picolibc defines them only in the Cygwin/RTEMS block of `machine/setjmp.h`.
- SEV_LEVEL isn't in POSIX (it is SVID), so it isn't checked; picolibc ignores it.
- vsyslog and LOG_PERROR are extensions, used only to observe syslog.

**Implementation:**
- syslog: `libc/posix/syslog/` (openlog.c, closelog.c, setlogmask.c, syslog.c, vsyslog.c, local-syslog.h) and `libc/include/syslog.h`.
- fmtmsg: `libc/posix/fmtmsg/fmtmsg.c` and `libc/include/fmtmsg.h`.
- setjmp: `libc/include/setjmp.h`, `machine/setjmp.h`, `libc/machine/aarch64/setjmp.S`, `libc/machine/arm/setjmp.S`, and in `libc/signal/` sigjmp_getsigs.c, sigjmp_setsigs.c and the new siglongjmp.c.
- errno: `libc/include/errno.h`, `sys/errno.h` and `libc/errno/errno.c`, plus `libc/string/strerror.c` for the messages.
- assert: `libc/include/assert.h` and `libc/stdlib/assert*.c`.

**Tests.** File names below drop the `test-` prefix, and line numbers are for the branch tip.
- `test/test-posix/test-syslog/`: syslog_h.c (new), syslog.c (rewritten), syslog-thread.c (new).
- `test/test-posix/test-fmtmsg/fmtmsg.c` (rewritten).
- `test/test-posix/test-setjmp/`: setjmp_h.c, setjmp_h-iso.c, sigsetjmp.c (all new).
- `test/test-posix/test-errno/errno.c` (new).
- `test/test-posix/test-assert/assert-message.c` (new; the name avoids a clash with the top-level test-assert).
- Existing tests still relied on: `test/test-setjmp.c`, `test/test-assert.c`, `test-signal/test-si_code.c` (sigsetjmp from a SIGSEGV handler), and `test-pthread/test-pthread_create.c` (errno per thread).

**Nothing reaches the host's system log or console.**
- The chroot's `/dev/log` is the host's logger socket (`srw-rw-rw-`), and the old test-syslog sent its messages there on every run.
- Every syslog and fmtmsg call in the new tests runs in a child whose RLIMIT_NOFILE soft limit is 3, with descriptors 0, 1 and 2 in use. socket() and open() then fail with EMFILE, so neither `/dev/log` nor `/dev/console` can be opened. Each child checks that `open("/dev/null")` fails before it calls the library.
- syslog output is seen through LOG_PERROR on a pipe, and fmtmsg's through standard error on a pipe. MM_CONSOLE is only ever seen failing.
- This matters for glibc too: its fmtmsg sends MM_CONSOLE messages through syslog().

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799:
- basedefs/syslog.h, fmtmsg.h, setjmp.h, errno.h and assert.h;
- functions/closelog, fmtmsg, setjmp, longjmp, sigsetjmp, siglongjmp, errno and assert.

**Status key.**
- TESTED (file:line), UNTESTED, NOT TESTABLE (reason).
- TESTED rows were verified by running them on aarch64-linux-gnu and arm-linux-gnueabihf, both for picolibc and, unless noted, for the `-native` glibc build.
- "(new)" marks checks this unit added; "(existing)" marks checks that were already there.

## Requirement table

### `<syslog.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| SH1 | LOG_PID, LOG_CONS, LOG_NDELAY, LOG_ODELAY, LOG_NOWAIT, distinct | TESTED (new) | syslog_h.c:55-61 (a duplicate `case` doesn't compile) |
| SH2 | The 17 facilities, including LOG_DAEMON, LOG_AUTH, LOG_LPR, distinct | TESTED (new) | syslog_h.c:63-81. **Deviation 1, fixed** |
| SH3 | The 8 severities, distinct | TESTED (new) | syslog_h.c:83-93 |
| SH4 | LOG_MASK and LOG_UPTO give int for an int argument, in strict XSI mode | TESTED (new) | syslog_h.c:98-99. **LOG_UPTO was hidden (deviation 1)** |
| SH5 | LOG_UPTO(pri) covers LOG_EMERG..pri and no other priority | TESTED (new) | syslog_h.c:140-155 |
| SH6 | The mask example on setlogmask's page gives equal masks | TESTED (new) | syslog_h.c:102-104 |
| SH7 | Prototypes | TESTED (new) | syslog_h.c:107-110 (`_Generic`) |
| SH8 | Strict mode: vsyslog isn't declared, and the `<stdarg.h>` names aren't defined | TESTED (new) | syslog_h.c:44-49, 131-134. **Deviation 2, fixed** |

### closelog, openlog, setlogmask, syslog

| # | Requirement | Status | Reference |
|---|---|---|---|
| SL1 | syslog sends the message to the logging facility | NOT TESTABLE | the only facility is `/dev/log`, the host's logger, which the tests must not write to |
| SL2 | The message header has at least a timestamp and a tag | NOT TESTABLE | the header only goes to `/dev/log`. By reading: the timestamp is there, but there is no tag without an ident (left; see below) |
| SL3 | The body is formatted as printf would | TESTED | syslog.c:129-134 (existing check, now in the isolated child) |
| SL4 | %m gives the message for errno on entry | TESTED | syslog.c:130-135 (existing), 138-142 (new) |
| SL5 | %m mixes with `%n$` conversions | TESTED (new) | syslog.c:138-142 |
| SL6 | `%%` stays a percent sign, also before m | TESTED (new) | syslog.c:145-148 |
| SL7 | With no facility in priority, the default facility is used | NOT TESTABLE | the facility is only in the header. By reading: vsyslog.c ORs in `__syslog_facility` |
| SL8 | openlog's ident is prepended to every message | TESTED | syslog.c:129-134 (existing) |
| SL9 | A later openlog replaces the ident and the options | TESTED (new) | syslog.c:175-179 |
| SL10 | LOG_PID logs the process ID | TESTED | syslog.c:133-134 (existing); 179, no PID without the option (new) |
| SL11 | LOG_CONS writes to the console when the message can't be sent, without acquiring a controlling terminal | NOT TESTABLE | writing to `/dev/console` isn't allowed. Only its failure is run: syslog goes on (syslog.c:129). By reading: O_NOCTTY |
| SL12 | LOG_NDELAY opens the connection at once | NOT TESTABLE | it would connect to the host's `/dev/log`. Only its failure is run (syslog.c:129) |
| SL13 | LOG_ODELAY; LOG_NOWAIT | n/a | the open is delayed unless LOG_NDELAY is given; picolibc creates no children |
| SL14 | openlog's facility sets the default; LOG_USER initially | NOT TESTABLE | header only. By reading: correct, except that facility 0 (LOG_KERN) leaves the default alone, as in glibc |
| SL15 | syslog without openlog | TESTED (new) | syslog.c:123-126 |
| SL16 | A descriptor syslog allocates has FD_CLOEXEC | NOT TESTABLE | needs a connected `/dev/log` socket. By reading: SOCK_CLOEXEC, and O_CLOEXEC for the console |
| SL17 | closelog closes the descriptors | NOT TESTABLE | same reason. By reading: correct. closelog followed by openlog works (syslog.c:190-196, new) |
| SL18 | setlogmask sets the mask and returns the old one | TESTED | syslog.c:151 (existing), 162 (new) |
| SL19 | A mask of 0 leaves the mask unchanged | TESTED | syslog.c:159 (existing) |
| SL20 | A priority outside the mask is rejected, whatever its facility bits | TESTED | syslog.c:150-158 (the facility part is new), 161-168 (LOG_UPTO, new) |
| SL21 | The default mask allows every priority | TESTED (new) | syslog.c:120-121, 169-172 |
| SL22 | setlogmask without openlog | TESTED (new) | syslog.c:120 |
| SL23 | EMFILE/ENFILE (may fail) | TESTED (new) | every call in syslog.c runs with no free descriptor and goes on |
| SL24 | Thread safety: a thread cancelled in syslog leaves logging usable (syslog is only an optional cancellation point) | TESTED (new) | syslog-thread.c:107-169. **Deviation 3, fixed.** The `-native` run is skipped on armhf, where glibc hangs |

### `<fmtmsg.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| FH1 | The 10 classification identifiers and MM_NULLMC, distinct | TESTED (new) | fmtmsg.c:63-76 |
| FH2 | MM_HALT, MM_ERROR, MM_WARNING, MM_INFO, MM_NOSEV, distinct | TESTED (new) | fmtmsg.c:77-84 |
| FH3 | MM_NULLLBL, MM_NULLSEV, MM_NULLMC, MM_NULLTXT, MM_NULLACT, MM_NULLTAG have the table's types and values | TESTED (new) | fmtmsg.c:44-53 |
| FH4 | MM_OK, MM_NOTOK, MM_NOMSG, MM_NOCON, distinct | TESTED (new) | fmtmsg.c:85-91 |
| FH5 | Prototype | TESTED (new) | fmtmsg.c:54-58 |

### fmtmsg

| # | Requirement | Status | Reference |
|---|---|---|---|
| FM1 | MM_PRINT writes to standard error | TESTED | fmtmsg.c:200-201 (existing, extended) |
| FM2 | MM_CONSOLE writes to the console | NOT TESTABLE | writing to `/dev/console` isn't allowed; only its failure is run (FM14) |
| FM3 | With both, standard error still gets the message | TESTED (new) | fmtmsg.c:274-275 |
| FM4 | The HALT, ERROR, WARNING and INFO strings; none for MM_NOSEV | TESTED | fmtmsg.c:200 (existing, ERROR), 225-238 (new) |
| FM5 | The action is preceded by "TO FIX:" | TESTED | fmtmsg.c:201 (existing), 241 (new) |
| FM6 | The text and the action aren't limited in size | TESTED (new) | fmtmsg.c:245-255 (5000 bytes each). **Deviation 5, fixed** |
| FM7 | MSGVERB selects the components for standard error, in any order | TESTED (new) | fmtmsg.c:204-212. **Deviation 4, fixed** |
| FM8 | Every component when MSGVERB is unset, empty, malformed or has an unknown keyword | TESTED (new) | fmtmsg.c:200, 215-218 |
| FM9 | A component with its null value is left out | TESTED (new) | fmtmsg.c:221-227 |
| FM10 | The console gets every component, whatever MSGVERB says | NOT TESTABLE | console. By reading: the console uses `MM_VERB_ALL` |
| FM11 | MM_OK on success; nothing written without a display classification | TESTED | fmtmsg.c:257-260 (MM_NULLMC existing, the rest new) |
| FM12 | MM_NOTOK when the call failed completely | TESTED (new) | fmtmsg.c:262-270 and 279-280 (picolibc). With only standard error asked for, picolibc says MM_NOTOK and glibc MM_NOMSG; both readings are accepted |
| FM13 | MM_NOMSG when only standard error failed | NOT TESTABLE | the console would have to work |
| FM14 | MM_NOCON when only the console failed | TESTED (new, picolibc) | fmtmsg.c:272-278. glibc logs console messages through syslog and never sees a failure |
| FM15 | The example message on fmtmsg's page | TESTED (new) | fmtmsg.c:176-201. The check allows for the spaces before the tag: picolibc and glibc print two, while the informative example shows one |

### `<setjmp.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| JH1 | jmp_buf and sigjmp_buf are array types | TESTED (new) | setjmp_h.c:42-48 |
| JH2 | longjmp and siglongjmp are `_Noreturn` functions with prototypes | TESTED (new) | setjmp_h.c:50-86: `&siglongjmp`, `(siglongjmp)(...)`, a call through a pointer, and -Werror=return-type. **Deviation 6, fixed** |
| JH3 | setjmp and sigsetjmp may be macros | n/a | setjmp is a function, sigsetjmp a macro |
| JH4 | The CX names aren't visible in strict ISO C | TESTED (new, picolibc) | setjmp_h-iso.c:36-67. **Deviation 7, fixed.** The `-native` build is skipped, because `-pthread` makes glibc expose POSIX |

### setjmp, longjmp, sigsetjmp, siglongjmp

| # | Requirement | Status | Reference |
|---|---|---|---|
| J1 | setjmp returns 0 from a direct call | TESTED | test/test-setjmp.c (existing); sigsetjmp.c:137-174 |
| J2 | After longjmp(env, val), execution continues as if setjmp had returned val | TESTED | sigsetjmp.c:176-191, with 1, 42, -1, INT_MAX and INT_MIN, four frames down (new); test-setjmp.c (existing) |
| J3 | A val of 0 makes setjmp return 1 | TESTED | sigsetjmp.c:190 (new, also for sigsetjmp); test-setjmp.c (existing) |
| J4 | The caller's state (callee-saved registers) is restored | TESTED (new) | sigsetjmp.c:83-119, 221-241. Before the jump, assembly puts junk in x19-x28 and d8-d15 (aarch64), or r4-r6, r8-r10 and d8-d15 (arm). Ten integers and eight doubles live in those registers in the caller |
| J5 | Volatile automatic objects changed after setjmp keep their new values | TESTED (new) | sigsetjmp.c:197-215 |
| J6 | Unchanged automatic objects keep their values | TESTED (new) | sigsetjmp.c:217 |
| J7 | Static objects have their values as of the longjmp | TESTED (new) | sigsetjmp.c:216 |
| J8 | The floating-point status flags and the rounding mode are as of the longjmp | TESTED (new) | sigsetjmp.c:244-273 |
| J9 | longjmp is async-signal-safe and can leave a handler | TESTED (new) | sigsetjmp.c:380-392 |
| J10 | What longjmp does to the mask is unspecified | n/a | observed: picolibc and glibc both leave it unchanged, so the handler's mask stays |
| J11 | The undefined cases (no setjmp, the function returned, VLAs, another thread) | NOT TESTABLE | undefined behaviour |
| SJ1 | sigsetjmp returns 0 directly and non-zero after siglongjmp; a val of 0 gives 1 | TESTED | sigsetjmp.c:176-191 (new); test-si_code.c (existing, from a SIGSEGV handler) |
| SJ2 | A non-zero savemask saves the calling thread's mask | TESTED (new) | sigsetjmp.c:296-320, and 438-462 in a thread with its own mask |
| SJ3 | siglongjmp restores the mask if, and only if, savemask was non-zero | TESTED (new) | sigsetjmp.c:296-320: both ways, and a mask saved while blocked is restored |
| SJ4 | Out of a handler, the saved mask undoes the handler's mask; without savemask the handler's mask stays | TESTED (new) | sigsetjmp.c:353-378 |
| SJ5 | A handler on the alternate stack can be left; the alternate stack is then free and is used on the next delivery | TESTED (new) | sigsetjmp.c:405-433 |
| SJ6 | Repeated delivery and jump | TESTED (new) | sigsetjmp.c:394-401 |

### `<errno.h>`, errno

| # | Requirement | Status | Reference |
|---|---|---|---|
| EH1 | errno is a modifiable lvalue of type int | TESTED (new) | errno.c:128-140 |
| EH2 | errno is thread-local | TESTED (existing) | test-pthread/test-pthread_create.c:93-104, 190-200 |
| EH3 | Every E* POSIX lists is defined as a positive integer constant of type int, usable in `#if` | TESTED (new) | errno.c:57-72 (`#if`), 76-97, 142-146 |
| EH4 | The values are distinct, except EAGAIN/EWOULDBLOCK and ENOTSUP/EOPNOTSUPP | TESTED (new) | errno.c:102-107, 151-158. In picolibc, EWOULDBLOCK is EAGAIN, and ENOTSUP (134) differs from EOPNOTSUPP (95) |
| EH5 | errno is 0 at startup in the initial thread | TESTED (new) | errno.c:116-126. Skipped when a sanitizer runtime runs before main (the clang `-native` build) |
| EH6 | No function sets errno to 0 | TESTED (new; spot check only) | errno.c:167-177 (strtol, strerror, malloc/free, getenv, snprintf) |
| EH7 | Each distinct E* has its own strerror message (the strings unit's area, checked cheaply here) | TESTED (new) | errno.c:159-164. **Deviation 8, fixed** |
| EH8 | Namespace: nothing but E* and errno in strict mode | TESTED (by `-E -dM`, not in a test) | only E* names (reserved), plus the known `MALLOC_ALIGNMENT` leak |

### `<assert.h>`, assert

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | assert is a macro | TESTED (new) | assert-message.c:49-52 |
| A2 | It expands to a void expression and returns no value | TESTED (new) | assert-message.c:62-69 |
| A3 | Scalar arguments of any type | TESTED (new) | assert-message.c:72-84 |
| A4 | A true assertion does nothing | TESTED | test/test-assert.c (existing); assert-message.c:189-191 |
| A5 | A false one writes the expression text, `__FILE__`, `__LINE__` and `__func__` to stderr, and nothing to stdout | TESTED (new) | assert-message.c:171-180, 196-198 |
| A6 | Then abort is called | TESTED (new) | assert-message.c:197 (SIGABRT) |
| A7 | With NDEBUG, assert is `((void)0)` and its argument isn't evaluated | TESTED (new) | assert-message.c:87-97, 201-202 |
| A8 | assert is redefined each time `<assert.h>` is included | TESTED (new) | assert-message.c:99-108, 204-206 |
| A9 | static_assert (a macro in C17's `<assert.h>`) | TESTED (new) | assert-message.c:110-114 |

### Counts

There are 90 requirement rows:
- 75 TESTED; 17 of these were already tested before this unit.
- 12 NOT TESTABLE.
- 3 n/a.
- 0 UNTESTED.

The 17 earlier checks were in test-syslog, test-fmtmsg, test-setjmp, test-si_code, test-assert and test-pthread_create. The old test-syslog checks also wrote to the host's log each time they ran.

## Deviations found and fixed

1. **`<syslog.h>` lacked LOG_DAEMON, LOG_AUTH and LOG_LPR, and hid LOG_UPTO** behind `__BSD_VISIBLE || __GNU_VISIBLE`. POSIX lists all three facilities, and LOG_UPTO next to LOG_MASK.
   - Fix: added with glibc's values (3, 4 and 6, shifted left by 3).
   - Failing-before: test-syslog_h doesn't compile ("'LOG_DAEMON' undeclared").
2. **`<syslog.h>` always included `<stdarg.h>`**, so strict XSI mode defined va_start, va_arg, va_end and va_copy, which aren't reserved for the header.
   - Fix: only vsyslog, an extension, needs va_list, so the include moved under the extension guard.
   - Failing-before: "<syslog.h> defines the <stdarg.h> macros".
3. **openlog, closelog and syslog called cancellation points with the libc lock held** (connect, close, send, write, open). A thread cancelled there exited holding `__LIBC_LOCK`. Every other thread's next syslog then hung, as did any other libc call that takes the lock.
   - Fix: cancellation is disabled around the locked part with pthread_setcancelstate, as wordexp and the DNS code do. These functions are therefore not cancellation points, which POSIX allows.
   - The lock calls stay in each function, because clang's -Wthread-safety rejects lock helpers.
   - Failing-before: the child is killed by its 30-second SIGALRM ("child killed by signal 14").
4. **fmtmsg ignored MSGVERB.** POSIX: MSGVERB "shall determine for fmtmsg() which message components it is to select when writing messages to standard error".
   - Fix: it is parsed on each call. Keywords may come in any order. Every component is shown when MSGVERB is unset, empty or malformed, or names an unknown keyword. The console still gets everything.
   - Failing-before: `severity:text:action: strcmp(out, "ERROR: illegal option\n" SECOND "\n") == 0`.
5. **fmtmsg cut messages at 1023 bytes.** POSIX says the text and the action are "not limited to a specific size".
   - Fix: the message goes into a 1024-byte stack buffer, or into a malloc'd buffer when it is longer.
   - This is in the same commit as deviation 4. test-fmtmsg's MSGVERB check runs first, so the proof shows that one.
6. **siglongjmp was only a macro.** POSIX says siglongjmp "shall be declared as a function", `_Noreturn` and with a prototype; it "may also be defined as a macro". `&siglongjmp` and `(siglongjmp)(env, val)` didn't compile.
   - Fix: a new `libc/signal/siglongjmp.c` and a `__noreturn` prototype; the macro is gone.
   - Failing-before: test-setjmp_h doesn't compile ("'siglongjmp' undeclared").
7. **The POSIX names in `<setjmp.h>` were visible in strict ISO C, and some names were plain.** sigjmp_buf, sigsetjmp and siglongjmp showed up without any POSIX feature macro. sigjmp_buf's members (`savesigs`, `sigs`, `jmpb`) and the helper's parameter names were plain, so an application macro with one of those names broke the header.
   - Fix: the POSIX part is under `__POSIX_VISIBLE`, and those names are `__`-prefixed.
   - Failing-before: test-setjmp_h-iso doesn't compile ("conflicting types for 'sigjmp_buf'", "macro 'siglongjmp' requires 2 arguments").
8. **strerror gave "Unknown error" for EDQUOT and ESTALE**, two POSIX error numbers that Linux returns.
   - Fix: glibc's messages, "Disk quota exceeded" and "Stale file handle".
   - Failing-before: `EDQUOT and ESTALE share the message "Unknown error"`.

**Also fixed: test-syslog itself.** It sent every message to `/dev/log`, which in this chroot is the host's system logger. It now runs isolated, as described above.

**How the fixes were proved.** For each one, I reversed only its library change (`git show <fix> -- libc libos | git apply -R`), rebuilt the test on aarch64-linux-gnu, saw the failure quoted above, and reapplied the change.

## Deviations not fixed

- **No tag when openlog had no ident** (SL2). POSIX: the header "contains at least a timestamp and a tag string".
  - glibc uses the program name. picolibc has no equivalent (no `__progname`), but libos/linux could read `/proc/self/comm`.
  - This needs a decision on the default tag, and it can't be tested without `/dev/log`.
- **vsyslog reads `__syslog_mask` and `__syslog_facility` without the lock.** This is a benign data race with setlogmask and openlog in other threads. A test can't observe it, so I didn't change it.
- **fmtmsg's MM_CONSOLE path** opens and writes `/dev/console` without holding off cancellation, so a cancel there leaks the descriptor (fmtmsg is an optional cancellation point). This is minor, and the console path can't be tested here.
- **`-Dassert-verbose=false`** (`__ASSERT_QUIET`) makes a failed assert print only "assertion failed", without the expression, file, line and function POSIX requires. It is a size option and verbose is the default, so I only note it.

## Other observations

- picolibc's sigsetjmp and siglongjmp save and restore the mask with sigprocmask. That is per-thread on Linux, as "the current signal mask of the calling thread" requires (SJ2).
- With savemask set, siglongjmp restores the mask before it jumps. A pending signal that the restore unblocks is therefore handled on the current stack before the jump. glibc does the same, and POSIX doesn't order the two.
- In the tested cases, fmtmsg's layout matches glibc's: label, severity and text joined by ": ", then "TO FIX: action", two spaces and the tag. When there are null components the separators differ slightly from glibc's. POSIX leaves that unspecified, and nothing checks it.
- glibc 2.43 differences met here (the tests guard them):
  - fmtmsg reports MM_NOMSG, not MM_NOTOK, when only standard error was asked for and writing it failed. Both readings are accepted.
  - MM_CONSOLE goes through syslog, so MM_NOCON never comes back.
  - On 32-bit ARM, after a thread is cancelled in syslog, the next syslog call hangs (aarch64 is fine).
  - `-pthread` defines `_REENTRANT`, which makes glibc expose POSIX in "strict" ISO C mode.
- In the clang native build, the ubsan runtime runs before main and leaves errno at 2 (ENOENT) in the `-native` tests. A two-line program shows it: built with `clang -fsanitize=bounds` it prints 2, built with plain clang it prints 0. test-errno skips its "zero at startup" check when `__has_feature(undefined_behavior_sanitizer)` (or `address_sanitizer`) is true.

## Left undone

- SL1, SL2, SL7, SL11, SL12, SL14, SL16 and SL17 need a logger socket other than `/dev/log`. picolibc hard-codes that path, and the chroot can't create a mount namespace to substitute a private socket.
- FM2, FM10 and FM13 need a working console.
- The x86_64 host should run test-sigsetjmp under qemu-user:
  - The register-clobbering assembly exists only for aarch64 and arm; other targets get only the C-level checks.
  - The alternate-stack check relies on qemu's sigaltstack emulation.

## Test runs (verified by running)

- **aarch64-linux-gnu** (`-Dwerror=true`, full `meson test -t 4`): 1592 Ok, 0 Fail, 6 Expected Fail, 42 Skipped.
- **arm-linux-gnueabihf**: 1590 Ok, 0 Fail, 6 Expected Fail, 45 Skipped.
  - The extra skip is test-syslog-thread-native (the glibc armhf hang).
  - An earlier armhf run found that hang as a Fail; the test now skips there.
- **clang native** (`CC=clang CXX=clang++ do-native-configure -Dwerror=true`, full suite): builds; 1214 Ok, 1 Fail, 6 Expected Fail, 385 Skipped.
  - The Fail was test-errno-native, from the sanitizer's errno described above.
  - After the fix, this unit's suites on clang native gave 12 Ok and 5 Skipped (the OS tests skip in the native configuration).
  - The two clang failures known on earlier bases (test-math-errhandling, test-mktemp) didn't appear.
  - Before that, clang's -Wthread-safety had rejected the first version of the syslog lock helpers. The fix was reworked to keep the lock calls inline.
- **Native configuration** (`do-native-configure -Dwerror=true`, `ninja` only): builds cleanly.
- **CMake** (`do-cmake-thumbv7m-configure`, `ninja test`): 625 tests, 100% passed.
- **After the final rebase**, which changed only test-errno.c, I rebuilt and reran this unit's suites (syslog, fmtmsg, setjmp, errno, assert), not the full suites:
  - aarch64: 17 Ok.
  - armhf: 16 Ok, 1 Skipped.
  - clang native: 12 Ok, 5 Skipped.
  - The native build still builds.
- `clang-format --dry-run -Werror` is clean on every changed C file.
- The fix proofs are listed with each deviation above.
- The host security software killed nothing.

## Commits (audit/logging-misc, on top of 6edd04ce22)

- 2331ae8c48 syslog.h: Define LOG_DAEMON, LOG_AUTH, LOG_LPR, and LOG_UPTO in strict mode
- d63d79f04f syslog: Hold off cancellation while the lock is held
- 5b36d4ddbe test-posix: Check <syslog.h>, and keep test-syslog away from the system logger
- 9442be42df fmtmsg: Honour MSGVERB, and don't limit the message length
- 07332ab37e test-posix: Check MSGVERB, null components, long messages and results in fmtmsg
- e38777ce90 setjmp.h: Make siglongjmp a function, and hide the POSIX names from ISO C
- c4ca3da7fc test-posix: Check <setjmp.h>, and what setjmp and sigsetjmp keep and restore
- 2a963bc541 strerror: Give EDQUOT and ESTALE messages
- 8f967811be test-posix: Check the <errno.h> names, their messages, and errno
- 24a771b570 test-posix: Check assert's report, NDEBUG and re-including <assert.h>
- e263648aee syslog.h: Don't define the <stdarg.h> names in strict mode
- 8352bc9abe test-posix: Check that <syslog.h> leaves the <stdarg.h> names alone

The patches 0001-0012 are in `/local/home/keithpac/arm64-testing/results/audit/logging-misc/`, made with `git format-patch -o ... 6edd04ce22..audit/logging-misc`. Nothing was committed to or pushed from the arm64-testing checkout.
