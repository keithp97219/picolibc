# `<stdlib.h>`, `<inttypes.h>`: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-stdlib`, branch `audit/stdlib`, based on 6edd04ce22 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43, clang 21) as an unprivileged user.

**Interfaces.** Everything `<stdlib.h>` declares in Issue 8 except setkey (done by unistd-config) and the details of the multibyte functions (wide and locale units): the environment (getenv, secure_getenv, setenv, unsetenv, putenv), mkstemp/mkostemp/mkdtemp, realpath, the allocation functions (malloc, calloc, realloc, reallocarray, aligned_alloc, posix_memalign, free), qsort/qsort_r/bsearch, the rand48 family, random/srandom/initstate/setstate, rand/srand, the strto* integer functions and ato*, a64l/l64a, getsubopt, posix_openpt/grantpt/unlockpt/ptsname/ptsname_r (beyond termios's terminal parts), system, abort, exit/_Exit/quick_exit/atexit/at_quick_exit, mblen/mbtowc/wctomb/mbstowcs/wcstombs basics, and the header's macros, types and namespace. `<inttypes.h>`: imaxdiv_t, wchar_t, the PRI/SCN macros, imaxabs, imaxdiv, strtoimax, strtoumax, wcstoimax, wcstoumax.
- Not in Issue 8, so only checked for namespace: rand_r (removed in Issue 8), mktemp, ecvt/fcvt/gcvt (removed in Issue 7), valloc, free_sized/free_aligned_sized (C23 only).

**Implementation:** `libc/stdlib/` (setenv.c, putenv.c, unsetenv.c, _findenv.c, getenv.c, random.c, srandom.c, initstate.c, setstate.c, rand.c, srand.c, rand48*.c, a64l.c, l64a.c, getsubopt.c, realloc.c, malloc.c, memalign.c, posix-memalign.c, system.c, abort.c, exit*.c, quick-exit.c, mbstowcs.c, wcstombs.c), `libc/stdio/mktemp.c`, `mkdtemp.c`, `strtoi.h`, `libc/posix/stdlib/realpath.c`, `libos/linux/` (ptsname*.c, grantpt.c, unlockpt.c, posix_openpt.c, pthread_exit.c, and the new abort.c and system.c). Headers: `libc/include/stdlib.h`, `inttypes.h`, `sys/_wait.h`, `sys/wait.h`, `sys/_default_fcntl.h` and the new `sys/_open_flags.h`.

**Tests.** `test/test-posix/test-stdlib/` and the new `test/test-posix/test-inttypes/`. File names below drop the `test-` prefix; line numbers are for the branch tip.
- Environment changes, exit-path checks, abort and the FIFO case of mkstemp run in forked children.
- mkstemp and realpath work under a fresh `mkdtemp()` directory.
- system needs `/bin/sh` and skips without it.
- The random-number tests check the specified congruences and the state rules, not statistics.

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799: basedefs/stdlib.h, inttypes.h, XBD 6.2; functions/ for every interface above; XSH 2.9.1 (thread safety: random, srandom, initstate, setstate, srand48, seed48, lcong48, erand48, nrand48, jrand48, getenv and mkstemp are not in the list of functions that need not be thread-safe).

**Status key.** TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row was verified by running on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and, unless noted, the `-native` glibc build. "(new)" marks checks this unit added; "(existing)" checks that were there before.

## Requirement table

### `<stdlib.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | EXIT_FAILURE (1 to 125), EXIT_SUCCESS (0), RAND_MAX (>= 32767), integer constants usable in `#if` | TESTED (new) | stdlib_h.c:62-67 |
| H2 | MB_CUR_MAX: size_t, 1 in the POSIX locale | TESTED (new) | stdlib_h.c:111-114; mblen.c:65 |
| H3 | NULL; div_t, ldiv_t, lldiv_t members; size_t, wchar_t | TESTED (new) | stdlib_h.c:56-58, 96-106 |
| H4 | The `<sys/wait.h>` macros, including WCOREDUMP (new in Issue 8) | TESTED (new) | stdlib_h.c:70-79, 116-121. **Deviation 2, fixed** |
| H5 | O_APPEND, O_CLOEXEC, O_DSYNC, O_NOCTTY (XSI), O_RDWR, O_RSYNC, O_SYNC | TESTED (new; picolibc only, glibc lacks them) | stdlib_h.c:80-83. **Deviation 1, fixed** |
| H6 | O_CLOFORK | NOT TESTABLE | close-on-fork isn't implemented (files unit's cross-cutting item) |
| H7 | Every prototype, XSI ones with `_XOPEN_SOURCE 800` | TESTED (new) | stdlib_h.c:125-199 (`_Generic`); strtold and setkey aren't listed (setkey is unistd-config's) |
| H8 | Strict POSIX: no XSI interface, and nothing POSIX doesn't reserve (rand_r, valloc, ecvt/fcvt/gcvt, mktemp, suboptarg, alloca, BSD/GNU extensions) | TESTED (new; rand_r picolibc only) | stdlib_h-names.c:52-97. **Deviation 3, fixed** |
| H9 | Strict POSIX: the Issue 8 additions are declared (mkostemp, reallocarray, realpath, secure_getenv, qsort_r) | TESTED (new; picolibc only) | stdlib_h-names.c:115-125 |
| H10 | Strict ISO C: no POSIX names (W* macros, O_* flags, setenv, mkstemp, ...) | TESTED (new; picolibc only, skipped for `-native`) | stdlib_h-iso.c:45-60. **Part of deviation 2** |
| H11 | May make `<fcntl.h>`, `<limits.h>`, `<math.h>`, `<stddef.h>`, `<sys/wait.h>` visible | n/a | permission |

### getenv, secure_getenv, setenv, unsetenv, putenv

| # | Requirement | Status | Reference |
|---|---|---|---|
| E1 | getenv finds a variable, NULL when missing | TESTED | secure_getenv.c (existing); setenv.c:87-92 (new) |
| E2 | getenv's pointer points into environ's data | TESTED (new) | setenv.c:110-119 |
| E3 | getenv is thread-safe (not in XSH 2.9.1's list) | inferred by reading | takes the libc lock |
| E4 | secure_getenv returns NULL when euid != ruid or egid != rgid at startup | NOT TESTABLE | needs a set-user-ID program run by another user. **Deviation 12, not fixed** |
| E5 | setenv adds and replaces; overwrite 0 leaves the value | TESTED (new) | setenv.c:90-108 |
| E6 | setenv updates the list environ points to | TESTED (new) | setenv.c:93, 106 |
| E7 | setenv copies the strings | TESTED (new) | setenv.c:95-99 |
| E8 | setenv EINVAL for an empty name or '=', environment unchanged | TESTED (new) | setenv.c:122-127 |
| E9 | setenv ENOMEM, environment unchanged | UNTESTED | no reliable way to make only setenv's allocation fail |
| E10 | unsetenv removes (all definitions); a missing name succeeds | TESTED (new) | setenv.c:87, 134-135, 150-157 |
| E11 | unsetenv EINVAL for an empty name or '=', unchanged | TESTED (new) | setenv.c:128-132 |
| E12 | putenv's string becomes part of the environment; altering it alters the environment | TESTED (new) | setenv.c:171-182. **Deviation 5, fixed** |
| E13 | putenv of a name alone removes it or fails with EINVAL | TESTED (new) | setenv.c:193-198. **Part of deviation 5** (it returned 1, errno unset) |
| E14 | putenv ENOMEM | UNTESTED | as E9 |
| E15 | An environ assigned by the application is used and isn't freed; environ may be NULL | TESTED (new) | setenv.c:207-242. **Deviation 6, fixed** |
| E16 | setenv, unsetenv, putenv need not be thread-safe | n/a | they take the libc lock anyway |

### mkstemp, mkostemp, mkdtemp

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | mkstemp creates a regular file, open O_RDWR, mode S_IRUSR\|S_IWUSR | TESTED (new; existing for "it opens") | mkstemp.c:170-185; test-stdio/mktemp.c (existing) |
| T2 | Six or more trailing Xs are replaced by as many portable filename characters | TESTED (new) | mkstemp.c:110-117, 186-198 |
| T3 | The name doesn't duplicate an existing file | TESTED (new) | mkstemp.c:200-212 (64 distinct names), 277-287 (a FIFO with the first name). **Deviation 7, fixed** (the existence check blocked on a FIFO) |
| T4 | mkostemp: O_APPEND, O_CLOEXEC, O_SYNC take effect, mode 0600 | TESTED (new; O_APPEND existing) | mkstemp.c:214-239 |
| T5 | mkostemp: O_CLOFORK, O_DSYNC and O_RSYNC | NOT TESTABLE / inferred | O_CLOFORK is missing; O_DSYNC and O_RSYNC equal O_SYNC in picolibc |
| T6 | mkdtemp makes a directory, mode S_IRWXU, returns template | TESTED | mkdtemp.c (existing); mkstemp.c:241-247 (new, exact mode) |
| T7 | EINVAL when the template doesn't end in six Xs | TESTED | mkdtemp.c, test-stdio/mktemp.c (existing); mkstemp.c:249-260 (new, all three) |
| T8 | The errors of open and mkdir (ENOENT, ENOTDIR) | TESTED (new) | mkstemp.c:262-275 |
| T9 | mkostemp EINVAL (may fail) for invalid flags | UNTESTED | optional; picolibc masks other flags out |
| T10 | mkstemp is thread-safe | inferred by reading | the name generator steps under the libc lock |
| T11 | No function calls rand: mkstemp leaves rand's sequence alone | TESTED (new) | mkstemp.c:289-298. **Deviation 8, fixed** |

### realpath

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | An absolute name without '.', '..' or links, into the buffer or allocated | TESTED (existing) | realpath.c:124-151, 196-202 |
| R2 | '..' after links and at the root; trailing slashes after directories | TESTED (new) | realpath.c:166-174 |
| R3 | EINVAL for a null name | TESTED (new) | realpath.c:159-165 |
| R4 | ENOENT (missing, empty), ENOTDIR (component, trailing slash after a file, through a link), ELOOP | TESTED (existing; the link case new) | realpath.c:152-158, 169 |
| R5 | ENAMETOOLONG for a component longer than NAME_MAX | TESTED (new) | realpath.c:176-183 |
| R6 | EACCES for a directory without search permission | TESTED (new, unprivileged) | realpath.c:185-195 |
| R7 | EIO; ENOMEM (may fail); ENAMETOOLONG beyond PATH_MAX (may) | UNTESTED | no way to cause EIO or ENOMEM; PATH_MAX is the known 1024 cross-cutting item |

### malloc, calloc, realloc, reallocarray, aligned_alloc, posix_memalign, free

| # | Requirement | Status | Reference |
|---|---|---|---|
| M1 | malloc, calloc: NULL and ENOMEM when memory can't be had; calloc overflow is ENOMEM | TESTED (new) | realloc.c:79-87 |
| M2 | calloc's memory is zero | TESTED | test-stdlib/malloc.c (existing); realloc.c:88-92 |
| M3 | free doesn't modify errno | TESTED (new) | realloc.c:94-102 |
| M4 | realloc(NULL) is malloc; contents kept up to the smaller size | TESTED | malloc tests (existing); realloc.c:104-112 |
| M5 | A failed realloc (ENOMEM) keeps the old object | TESTED (new) | realloc.c:113-117 |
| M6 | reallocarray: overflow is ENOMEM and keeps the object; otherwise realloc(nelem * elsize) | TESTED (new) | realloc.c:119-129 |
| M7 | realloc(ptr, 0) and reallocarray with a zero factor: an object, or NULL with errno EINVAL | TESTED (new; not glibc) | realloc.c:131-147. **Deviation 4, fixed** |
| M8 | aligned_alloc: aligned; EINVAL for an unsupported alignment; ENOMEM | TESTED (new) | realloc.c:150-159 |
| M9 | posix_memalign: EINVAL for alignments that aren't power-of-two multiples of sizeof(void *), ENOMEM, memptr unchanged on error, result aligned | TESTED (new) | realloc.c:161-178 |
| M10 | The single total order of allocation calls (data races) | inferred by reading | the allocator holds its lock; test-malloc-stress (existing) |

### qsort, qsort_r, bsearch

| # | Requirement | Status | Reference |
|---|---|---|---|
| Q1 | Ascending order; whole elements moved | TESTED | qsort-r-posix.c, bsearch.c, libc-testsuite qsort (existing); qsort.c:119-122 (new) |
| Q2 | nel 0: no call to compar, no rearrangement | TESTED (new) | qsort.c:112-117, 130 |
| Q3 | qsort_r passes arg as the third argument | TESTED | qsort-r-posix.c (existing); qsort.c:133-137 |

### drand48 family

| # | Requirement | Status | Reference |
|---|---|---|---|
| D1 | X(n+1) = (a X(n) + c) mod 2^48 with a = 0x5DEECE66D, c = 0xB | TESTED (new) | rand48.c:129-217, against 64-bit arithmetic, seeds up to 2^48 - 1 |
| D2 | drand48/erand48 = 2^-48 X in [0, 1); lrand48/nrand48 = X >> 17; mrand48/jrand48 = X >> 16 signed | TESTED (new) | rand48.c:131-165 |
| D3 | A constant default state without initialization | TESTED (new) | rand48.c:126-127 |
| D4 | srand48: high 32 bits from the seed, low 16 bits 0x330E | TESTED (new) | rand48.c:170-180 |
| D5 | seed48 sets X and returns the previous X | TESTED (new) | rand48.c:152-168 |
| D6 | lcong48 sets X, a, c for all functions; srand48 and seed48 restore a and c | TESTED (new) | rand48.c:182-208 |
| D7 | erand48/nrand48/jrand48 streams are independent | TESTED (new) | rand48.c:210-217 |
| D8 | srand48, seed48, lcong48, erand48, nrand48, jrand48 are thread-safe | inferred by reading | the state is per thread (see cross-cutting) |

### random, srandom, initstate, setstate

| # | Requirement | Status | Reference |
|---|---|---|---|
| RN1 | random returns 0 to 2^31 - 1 | TESTED (new) | random.c:140-144 |
| RN2 | The default sequence is srandom(1)'s, and initstate(1, state, 128)'s | TESTED (new) | random.c:127-138 |
| RN3 | Sizes 8, 32, 64, 128, 256; others round down; 8 to 31 bytes use a linear congruential generator | TESTED (new, the rounding) | random.c:146-151; the generator type inferred by reading |
| RN4 | initstate with size < 8 returns NULL | TESTED (new) | random.c:153-154 |
| RN5 | initstate and setstate return the previous array; setstate resumes an array where it was | TESTED | initstate.c (existing); random.c:174-184 (new) |
| RN6 | srandom seeds the array in use | TESTED (new) | random.c:156-165 |
| RN7 | A non-linear additive feedback generator, period > 2^69 with 256 bytes, growing with the size | inferred by reading | **Deviation 9, fixed** (it was a 64-bit LCG for every size) |
| RN8 | random, srandom, initstate, setstate are thread-safe, with one process state | TESTED (new) | random-thread.c:107-136. **Part of deviation 9** |

### rand, srand

| # | Requirement | Status | Reference |
|---|---|---|---|
| RA1 | rand in [0, RAND_MAX]; srand(seed) repeats the sequence | TESTED (new; rand's statistics existing) | random.c:186-194 |
| RA2 | Without srand, the sequence of srand(1) | TESTED (new) | random.c:87-110, in a fresh child |
| RA3 | As if no function calls rand or srand (srandom, random, initstate, mkstemp) | TESTED (new; the random part not glibc) | random.c:196-208; mkstemp.c:289-298. **Deviation 8, fixed** |
| RA4 | Period at least 2^32 (XSI) | inferred by reading | 64-bit LCG |
| RA5 | rand avoids data races with thread-safe functions | inferred by reading | per-thread state |

### strtol, strtoll, strtoul, strtoull, ato*

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | White space, sign, base 0 prefixes, bases 2 to 36, letters either case | TESTED (libc-testsuite strtol existing; new) | strtol.c:66-77 |
| S2 | strtoul/strtoull negate in the return type | TESTED (new) | strtol.c:79-80 |
| S3 | "0x" without a hex digit converts "0" | TESTED (new) | strtol.c:83-85 |
| S4 | No conversion: 0 and endptr = nptr | TESTED (new) | strtol.c:87-96 |
| S5 | errno unchanged on success | TESTED (new) | strtol.c:65-81 |
| S6 | EINVAL and 0 for an unsupported base | TESTED (new) | strtol.c:98-106 |
| S7 | ERANGE with LONG_MIN/MAX, LLONG_MIN/MAX, ULONG_MAX, ULLONG_MAX, the subject consumed | TESTED (new; partly existing) | strtol.c:108-132 |
| S8 | Base 0 and 2 subject sequence: no "0b" prefix in POSIX.1-2024 (C17) | UNTESTED | **Deviation 13, not fixed** (picolibc follows C23) |
| S9 | atoi, atol, atoll | TESTED (new) | strtol.c:134-135 |

### a64l, l64a

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | The digit set, least significant first, at most six digits | TESTED (new) | a64l.c:69-95 |
| A2 | a64l uses the first six characters or up to the null; "" is 0; l64a(0) is "" | TESTED (new) | a64l.c:78-81, 96-99 |
| A3 | Only the low 32 bits, sign-extended for a wider long | TESTED (new; a64l not glibc) | a64l.c:101-113. **Deviation 10, fixed** |

### getsubopt

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | Commas separate suboptions; *optionp moves to the next one or to the final null | TESTED (new) | getsubopt.c:66-76 |
| G2 | The first '=' separates the value; later ones are part of it; *valuep NULL without '=' | TESTED (new) | getsubopt.c:70, 78-82 |
| G3 | Nothing else is special: blanks, empty suboptions | TESTED (new) | getsubopt.c:84-110. **Deviation 11, fixed** |
| G4 | Index of the matching token, or -1; keylistp unchanged | TESTED (new; the value on a mismatch not glibc) | getsubopt.c:112-126 |

### posix_openpt, grantpt, unlockpt, ptsname, ptsname_r (beyond termios)

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | posix_openpt flags, EMFILE, EINVAL; grantpt; unlockpt; ptsname; ptsname_r ERANGE/EBADF/EINVAL | TESTED (existing, termios round) | posix_openpt.c, grantpt.c, unlockpt.c, ptsname.c, ptsname_r.c; termios report P1-N5 |
| P2 | Prototypes, XSI visibility, hidden in strict mode | TESTED (new) | stdlib_h.c:144-189; stdlib_h-names.c:76-80 |
| P3 | posix_openpt ENFILE, EAGAIN; grantpt EACCES | NOT TESTABLE | system-wide limits |
| P4 | ptsname need not be thread-safe; its string may be overwritten | n/a | static buffer |

### system

| # | Requirement | Status | Reference |
|---|---|---|---|
| SY1 | system(NULL) is non-zero | TESTED (new) | system-signals.c:124 |
| SY2 | Runs sh -c -- command | TESTED (new) | system-signals.c:126-129. **Deviation 14, fixed** |
| SY3 | Returns the shell's status in waitpid format | TESTED (existing; new) | system.c (existing); system-signals.c:131-133 |
| SY4 | Ignores SIGINT and SIGQUIT and blocks SIGCHLD while waiting; restores them | TESTED (new) | system-signals.c:135-158. **Deviation 14** |
| SY5 | The command gets the caller's dispositions | TESTED | system.c "kill -INT" (existing); system-signals.c:160-164 |
| SY6 | Doesn't affect other children's status | TESTED (new) | system-signals.c:166-176 |
| SY7 | Doesn't return until the child ends, also when a signal interrupts the wait | TESTED (new) | system-signals.c:146-148. **Deviation 14** (an uninitialized status) |
| SY8 | Shell can't run: as exit(127) | inferred by reading | `_exit(127)` after execve; can't remove /bin/sh |
| SY9 | -1 with fork's errno when no child can be made | UNTESTED | picolibc has no RLIMIT_NPROC to make fork fail |
| SY10 | Concurrent calls and cancellation | n/a | unspecified |

### abort

| # | Requirement | Status | Reference |
|---|---|---|---|
| AB1 | Abnormal termination with SIGABRT's status | TESTED | test-stdlib/abort.c (existing, expected failure); abort-status.c:149-150 |
| AB2 | Overrides blocking or ignoring SIGABRT | TESTED (new) | abort-status.c:152-156. **Deviation 15, fixed** |
| AB3 | A handler that returns: still terminated by SIGABRT, never returns | TESTED (new) | abort-status.c:158-161. **Deviation 15** |
| AB4 | A handler that doesn't return keeps the process | TESTED (new) | abort-status.c:163-164 |
| AB5 | SIGABRT goes to the calling thread | inferred by reading | raise uses tgkill |

### exit, _Exit, quick_exit, atexit, at_quick_exit

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | atexit functions in reverse order; one registered during exit runs after those already called | TESTED (new; order existing) | exit.c:242-247; test-stdlib/atexit.c (existing) |
| X2 | Each registration is called | TESTED (new) | exit.c:248-249 |
| X3 | At least 32 functions | TESTED (new) | exit.c:251-253 |
| X4 | A function that doesn't return ends the processing | TESTED (new) | exit.c:255-257 |
| X5 | exit calls no at_quick_exit functions; quick_exit no atexit ones; _Exit neither | TESTED (new) | exit.c:265-273; quick-exit.c (existing) |
| X6 | at_quick_exit order, late registration, 32 functions | TESTED (new; order existing) | exit.c:259-263 |
| X7 | quick_exit and _Exit don't flush streams | TESTED (new, quick_exit) | exit.c:278-279 |
| X8 | exit flushes all streams after the atexit functions | TESTED by the stdio unit | not in this base (stdio round 7) |
| X9 | waitpid sees status & 0377 | TESTED (new) | exit.c:275-276 |
| X10 | waitid and SIGCHLD's siginfo see the full status | NOT TESTABLE | Linux keeps 8 bits (kernel/exit.c, `(code & 0xff) << 8`) |
| X11 | atexit functions run on the last thread's termination | TESTED (new) | atexit-thread.c:110-119 |

### mblen, mbtowc, wctomb, mbstowcs, wcstombs (basics; the wide unit has the details)

| # | Requirement | Status | Reference |
|---|---|---|---|
| MB1 | A null s: no shift states (0) | TESTED (new) | mblen.c:67-70 |
| MB2 | mblen, mbtowc, wctomb on portable characters and the null character | TESTED (new) | mblen.c:72-82 |
| MB3 | mbstowcs and wcstombs convert, and count without storing for a null destination (XSI) | TESTED (new) | mblen.c:84-87. **Deviation 16, fixed** |
| MB4 | The POSIX locale has 256 single-byte characters (XBD 6.2) | UNTESTED | **Deviation 17, not fixed** (locale/wide units) |

### `<inttypes.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| I1 | Includes `<stdint.h>` | TESTED (new) | inttypes_h.c:55-57 |
| I2 | imaxdiv_t with intmax_t quot and rem | TESTED (new) | inttypes_h.c:574-577 |
| I3 | wchar_t | TESTED (new; not glibc) | inttypes_h.c:564 (the test uses wchar_t with no other header). **Deviation 18, fixed** |
| I4 | Every PRI and SCN macro for N = 8/16/32/64, LEAST, FAST, MAX, PTR | TESTED (new) | inttypes_h.c:59-526 (`#ifndef`), 579-606 (snprintf and sscanf with each) |
| I5 | The six prototypes | TESTED (new) | inttypes_h.c:568-573 |
| I6 | imaxabs, imaxdiv values | TESTED (new) | inttypes_h.c:608-612 |
| I7 | strtoimax/strtoumax/wcstoimax/wcstoumax: as strtol: errno untouched on success, ERANGE with the limits, EINVAL for a bad base, no conversion | TESTED (new) | inttypes_h.c:614-654 |
| I8 | Strict mode: nothing beyond the reserved PRI/SCN prefixes | UNTESTED | PRIB* leak (cross-cutting); the known MALLOC_ALIGNMENT and `<stdint.h>` leaks |

### Counts

137 requirements: 110 TESTED (about 22 of them, wholly or in part, by tests that existed before), 10 inferred by reading, 8 UNTESTED, 5 NOT TESTABLE, 4 n/a.

## Deviations found and fixed

Each fix was proved on aarch64-linux-gnu: with only its library change reversed (`git show <fix> -- libc libos | git apply -R`, or by hand where a later commit touches the same lines), the test was rebuilt and run and failed; with the fix it passes.

1. **`<stdlib.h>` didn't define the open flags mkostemp takes** (O_APPEND, O_CLOEXEC, O_DSYNC, O_NOCTTY, O_RDWR, O_RSYNC, O_SYNC). The `_F*` bits and `O_*` names moved from `sys/_default_fcntl.h` into the new `sys/_open_flags.h`, which `<stdlib.h>` includes for POSIX.1-2024. Failing before (compile): `#error "an open flag for mkostemp is missing"`. Reversed by hand (the include removed), as the next commit touches the same lines.
2. **WCOREDUMP was missing from `<stdlib.h>`, and strict ISO C got the W* macros.** WCOREDUMP moved to `sys/_wait.h`; `<stdlib.h>` includes that only when POSIX is visible. Failing before (compile): `#error "WCOREDUMP is missing"`; `#error "<sys/wait.h> macros in ISO C mode"`.
3. **Strict-mode namespace leaks in `<stdlib.h>`:** rand_r (removed in Issue 8), valloc, ecvt/fcvt/gcvt and their `_r`/float/long double variants, suboptarg (BSD), and alloca under gnu modes. They now need `_DEFAULT_SOURCE` or the old standard that had them; getsubopt's prototype also got Issue 8's restrict. Failing before (compile): `'rand_r' redeclared as different kind of symbol`, `'valloc' ...`, `conflicting types for 'suboptarg'`.
4. **realloc(ptr, 0) freed and returned NULL without setting errno** (libc/stdlib/realloc.c). POSIX.1-2024: "A null pointer shall be returned and, if ptr is not a null pointer, errno shall be set to [EINVAL]". reallocarray with a zero factor goes the same way. Failing before: `realloc(ptr, 0): q != NULL || errno == EINVAL`. glibc behaves as picolibc did; the check is `#ifndef __GLIBC__`.
5. **putenv copied its string** (libc/stdlib/putenv.c) into setenv, so altering it didn't alter the environment, and a string without '=' returned 1 without errno. Now the string itself goes into environ; no '=' (or an empty name) is EINVAL. Failing before: `the string itself is in the environment`.
6. **setenv freed an environ array the application had assigned, and crashed on a null environ** (libc/stdlib/setenv.c). It remembered only "I allocated an array once", so after `environ = mine` the next new name freed `mine`; and it counted entries through a null environ. It also overwrote values in place, which would write into putenv's strings (or a read-only literal) once 5 was fixed. setenv now records the array and the entries it allocated (`__env_put`, `__env_disown`), reuses or frees only those, and unsetenv frees removed entries. Allocation stays outside the libc lock (with the sequence-number retry), as the time unit's lock fix requires. Fixed together with 5 (one commit); failing before: `the string itself is in the environment` (putenv group) and `the application's array is intact` (environ group).
7. **mkstemp's existence check blocked on a FIFO** (libc/stdio/mktemp.c). It opened each candidate with O_RDONLY, which waits for a writer when the name is a FIFO (and could acquire a terminal). It now opens with O_NONBLOCK | O_NOCTTY, lets the create call decide when that open fails for any reason (no more 46656 retries when the directory can't be searched), leaves a template with fewer than six Xs untouched, and replaces every X. The pre-check stays because semihosting's open ignores O_EXCL. Failing before: the child hung until its 20-second alarm, `mkstemp in a child, without hanging`.
8. **mkstemp moved rand's sequence.** Names came from random(), and rand() was random() & RAND_MAX. POSIX (rand): "The implementation shall behave as if no function defined in this volume of POSIX.1-2024 calls rand() or srand()". mkstemp now has its own generator (splitmix64 seeded from the stack address, stepped under the libc lock), which also stops it disturbing random. With the mkstemp fix reversed, test-mkstemp stops earlier, at the FIFO hang (7), so its `mkstemp leaves rand's sequence alone` check isn't reached; the rand side is proved by reversing 9 (`random and srandom leave rand's sequence alone`).
9. **random was a 64-bit LCG for every state size, per thread by default, and unlocked with initstate** (libc/stdlib/random.c and friends). POSIX describes a non-linear additive feedback generator whose period grows with the state ("With 256 bytes ... greater than 2^69"), and random, srandom, initstate and setstate aren't in XSH 2.9.1's list, so they must be thread-safe. srandom in one thread didn't seed another; initstate's array was shared without a lock. Now: the BSD generator (types 0 to 4, header word, memcpy access so unaligned arrays work), one state under the libc lock, the built-in state seeded lazily as initstate(1, 128 bytes). rand got its own state with its previous sequence, so srandom/random/initstate don't move it. Failing before: `a thread uses the process's seed` (random-thread), `random and srandom leave rand's sequence alone` (random).
10. **a64l returned up to 36 bits** (libc/stdlib/a64l.c). POSIX: "only the low-order 32 bits shall be used ... the resulting value is sign-extended". Failing before (aarch64): `a64l sign-extends the low 32 bits`. 32-bit targets were already right. glibc returns 36 bits; guarded.
11. **getsubopt split at blanks and skipped empty suboptions** (libc/stdlib/getsubopt.c, BSD code). POSIX names only commas and the first '='. Failing before: `blanks in the token and the value`.
14. **system didn't follow POSIX on Linux** (generic libc/stdlib/system.c). No "--", no ignoring SIGINT/SIGQUIT or blocking SIGCHLD, the child not restoring them, an uninitialized status when a signal interrupted waitpid, and system(NULL) ran `sh -c` with a null argument. New `libos/linux/system.c`. Failing before: `a command starting with '-' (sh -c -- command)`.
15. **abort gave exit status 1 when SIGABRT was blocked, ignored or caught by a handler that returned** (generic libc/stdlib/abort.c: raise, then `_exit(1)`). POSIX: "The abort() function shall override blocking or ignoring the SIGABRT signal" and the status is that of termination by SIGABRT. New `libos/linux/abort.c`: unblock, raise; then restore SIG_DFL, unblock, raise again; `_exit(127)` only if all that returns. Failing before: `abort with SIGABRT blocked`.
16. **mbstowcs and wcstombs without __MB_CAPABLE wrote through a null destination** (or mbstowcs returned 0 for n 0). XSI: with a null destination they return the length needed, whatever n. The separate non-multibyte loops are gone; every configuration uses the general code. Failing before (the Linux builds aren't __MB_CAPABLE): `mbstowcs counts without a buffer`.
18. **`<inttypes.h>` didn't define wchar_t** (it typedefs `_wchar_t` from `__WCHAR_TYPE__`). POSIX requires it; it is now defined when POSIX is visible, as ISO C doesn't ask for it. Failing before (compile): `unknown type name 'wchar_t'`.

The rand48 family looked wrong and isn't: `__dorand48` overflows its 32-bit accumulator, but only by 2^48, which the result drops. A fix was written, its test passed without it, and it was dropped; test-rand48 stays to check the congruence. A change to the generic system.c ("--", NULL, EINTR) was also dropped, because nothing on this host runs it (Linux has its own now, native builds skip the POSIX tests, and hexagon has its own system.c), so it couldn't be proved.

## Deviations not fixed

12. **secure_getenv ignores the security criteria** (libc/stdlib/secure_getenv.c is `getenv`). POSIX: it "shall return a null pointer if the calling process does not meet" euid == ruid and egid == rgid at startup. On Linux, `getauxval(AT_SECURE)` (libos/linux has getauxval) gives exactly that, so a libos/linux/secure_getenv.c would be one line. Not made: it can't be tested without running a set-user-ID program as another user, and fixes must be proved.
13. **strtol and friends accept a "0b" prefix for base 0 and 2** (libc/stdio/strtoi.h). That is C23; POSIX.1-2024 is aligned with C17, where "0b1" in base 0 converts "0" and leaves endptr at "b1". glibc picks the behaviour by the C standard requested (`__isoc23_strtol`). Needs a decision; read in the source, not run.
17. **The POSIX locale has only 128 characters** (cross-cutting; locale and wide units). XBD 6.2 requires 256 single-byte characters; `__ascii_mbtowc` and `__ascii_wctomb` reject 0x80 and up.
- **O_CLOFORK** in `<stdlib.h>` and mkostemp: the known close-on-fork gap.
- **The known header leaks**: MALLOC_ALIGNMENT and `<stdint.h>` through `<sys/types.h>` (not included by `<stdlib.h>` here, but `<inttypes.h>` legitimately has `<stdint.h>`); PRIB* in strict C17 (cross-cutting).

## Other observations

- The rand48 state, rand's state and l64a's buffer are per thread; allowed (those functions need not be thread-safe), but a seed in one thread doesn't reach another (cross-cutting).
- system isn't cancellation-safe: a thread cancelled in its waitpid leaves SIGINT/SIGQUIT ignored and SIGCHLD blocked. POSIX leaves cancellation inside system unspecified except for the child.
- exit's flushing of fopen streams on os-linux is the stdio unit's round-7 change, not in this base; test-exit doesn't check it.
- The "known failure" test-mktemp in the clang native notes was a race between test-mktemp and test-mktemp-native in the same directory: mktemp (no O_EXCL) handed both processes the same first name from random()'s fixed seed. The private generator seeded from the stack address makes that unlikely.

## Left undone

- E9/E14 (ENOMEM from setenv and putenv), SY9 (fork failure), R7 (EIO, ENOMEM), T9 (mkostemp EINVAL, optional).
- The generic system.c and abort.c (see above).
- `<inttypes.h>`'s PRIB* in strict C17 and the `_restrict` parameter name in wcstoimax_l/wcstoumax_l.

## Files touched that other units' patches also touch

None of the round-7 patches (unistd-config, logging-misc, stdio) touch the files changed here; unistd-config's getopt patch is in libc/stdlib/getopt.c, which is untouched. `libc/stdio/mktemp.c` is shared with stdio's area (tmpnam calls mktemp), but no stdio patch changes it. `libos/linux/meson.build` gains abort.c and system.c, so a conflict there would be a list-merge only.

## Test runs (verified by running)

- **aarch64-linux-gnu** (`-Dwerror=true`, full `meson test -t 4`): Ok 1616, Expected Fail 6, Fail 0, Skipped 42.
- **arm-linux-gnueabihf** (same): Ok 1613, Expected Fail 6, Fail 0, Skipped 44, **Timeout 2**: test-a64l and test-a64l-native, whose round-trip loop overflowed a 32-bit long (`v * 3 + 1`; the compiler then took `v > 0` as always true). The loop now uses long long (squashed into the test commit); rerun, both pass. The test-stdlib and test-inttypes suites were rerun on both targets at the branch tip: Ok 86, Expected Fail 2, Fail 0 each.
- **Native** (`scripts/do-native-configure`, ninja only): builds clean.
- **Clang native** (`CC=clang CXX=clang++`, `-Dwerror=true`, full test): Ok 1235, Expected Fail 6, Fail 0, Skipped 389. The first build caught `aligned_alloc(24, ...)` (`-Wnon-power-of-two-alignment`); the test now passes the alignment through a volatile object.
- **CMake thumbv7m** (`ninja test`): 637 tests, 100% passed (357 skipped, the POSIX tests without an OS). Incident: that run started before the test-a64l fix and its qemu looped for 350 s until I killed that qemu process (my own, by PID), which run-arm reported as a pass; test-a64l rebuilt and rerun alone under CMake: Passed.
- The proofs (fix reversed, test failing) were run on aarch64-linux-gnu; the failing checks are quoted above.
- Host security software didn't interfere.

## Commits (audit/stdlib, on top of 6edd04ce22)

- 035ef9f184 stdlib.h: Define the open flags that mkostemp takes
- c28592299b stdlib.h: Define WCOREDUMP, and the wait macros only for POSIX
- 7cadfb77b9 stdlib.h: Hide names POSIX.1-2024 doesn't define in strict mode
- 96f6ecdd43 inttypes.h: Define wchar_t for POSIX
- 6348eca49d stdlib: Set errno to EINVAL when realloc frees for a zero size
- 8136d5aca1 stdlib: Put putenv's string in the environment, and track setenv's storage
- 7f1b31d306 stdlib: Give rand its own sequence, and make random the shared BSD generator
- 9a0c121b97 stdlib: Make a64l use only the low-order 32 bits
- 3d8ecbb7f7 stdlib: Split getsubopt suboptions at commas only
- 6afbb70160 stdio: Give mkstemp a name generator of its own, and don't block on FIFOs
- fffda32363 libos/linux: Give system the POSIX signal handling, -- and waiting
- 2388814244 libos/linux: Make abort override a blocked or ignored SIGABRT
- 4e1d1d1ec8 stdlib: Count without storing in mbstowcs and wcstombs without a buffer
- 81870c9d0d test-posix: Check <stdlib.h>'s macros, types, prototypes and namespace
- 5e7023b3ef test-posix: Check the strtol family's subject, bases, EINVAL and ERANGE
- 39050c92d8 test-posix: Check setenv, unsetenv and putenv, and an environ the application sets
- 879ba9902b test-posix: Check mkstemp, mkostemp and mkdtemp names, modes, flags and errors
- bf5d57af8a test-posix: Check ENOMEM, EINVAL and errno in the allocation functions
- cc9d0a804f test-posix: Check the rand48 congruence, random's state arrays and threads, and rand
- 529dd15a6b test-posix: Check a64l, getsubopt, the multibyte basics and qsort_r
- 9e9b3df4a1 test-posix: Check system's signal handling, -- and waiting
- 1d852ca6df test-posix: Check abort's status and the exit functions' order
- c813d0e556 test-posix: Check <inttypes.h>: wchar_t, the PRI and SCN macros, and the functions
- f931615860 test-posix: Check realpath's EINVAL, EACCES and ENAMETOOLONG, and slashes and '..'
