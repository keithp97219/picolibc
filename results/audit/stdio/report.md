# `<stdio.h>`: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-stdio`, branch `audit/stdio`, based on 6edd04ce22 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot as an unprivileged user, with gcc 16.2, clang 21 and glibc 2.43.

**Interfaces.**
- The 71 functions of the Issue 8 `<stdio.h>` synopsis, plus asprintf and vasprintf (added by Austin Group 1496), and the header's types and macros.
- The POSIX additions came first: fdopen, fileno, popen/pclose, getline/getdelim, dprintf/vdprintf, fmemopen/open_memstream, %n$, the locking and _unlocked functions, ctermid, tmpfile/tmpnam, renameat, fseeko/ftello. Then the errno and descriptor rules of XSH 2.5 and 2.5.1.
- printf and scanf conversions were only checked for %n$, *m$ and the POSIX error returns.

**Out of scope.**
- Not in Issue 8 (removed by Austin Group 1330): tempnam, P_tmpdir, gets, getw/putw, L_cuserid, cuserid. They are checked only as namespace items.
- mkstemp, mkdtemp and mkostemp belong to `<stdlib.h>` (the stdlib unit). mkostemp exists, in mktemp.c.
- open_wmemstream, fwide and the wide functions belong to the wide unit.

**Implementation.**
- `libc/stdio/`: tinystdio. bufio.c and its helpers serve descriptor streams; also fdopen, fopen, freopen, popen, pclose, fmemopen, open_memstream, getdelim, vfprintf, fseek, ftell and the rest.
- `libc/posix/stdio/ctermid.c`.
- Headers: `libc/include/stdio.h`, `stdio-bufio.h`, `stdio-posix.h`.
- Locks: `libc/include/sys/lock.h`, `libc/misc/lock.c`, `libos/linux/lock.c`.

**Tests.** All in `test/test-posix/test-stdio/`. File names below drop the `test-` prefix (`fseek.c` = `test-fseek.c`), and line numbers are for the branch tip.
- Every test that uses files works in a fresh `mkdtemp()` directory and removes it.
- popen runs `/bin/sh` commands that touch only that directory, and the test skips if there is no `/bin/sh`.
- The locking test uses threads with pipe handshakes. The one deadline loop only decides when to report a failure.
- Output on stdout and stderr is checked through fork and pipes.
- Existing picolibc tests in `test/test-stdio/` are cited where they cover a row.

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799: basedefs/stdio.h, functions/ for each interface, and XSH 2.5, 2.5.1 and 2.5.2.

**Status key.**
- TESTED (file:line), UNTESTED, or NOT TESTABLE (with the reason).
- TESTED rows were run on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and, unless noted, for the `-native` glibc build.
- "(new)" means added by this unit.

## Requirement table

### `<stdio.h>` header

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | FILE, fpos_t, off_t, size_t, ssize_t, va_list | TESTED (new) | stdio_h.c:52-64 |
| H2 | BUFSIZ > 0; L_ctermid and L_tmpnam are integer constants | TESTED (new) | stdio_h.c:67-69 |
| H3 | _IOFBF, _IOLBF, _IONBF are distinct | TESTED (new) | stdio_h.c:70 |
| H4 | SEEK_CUR, SEEK_END, SEEK_SET are distinct | TESTED (new) | stdio_h.c:71-72 |
| H5 | FILENAME_MAX; FOPEN_MAX >= 8; TMP_MAX >= 25 | TESTED (new) | stdio_h.c:73-75 |
| H6 | TMP_MAX >= 10000 on XSI systems | TESTED (new) | stdio_h-xsi.c:42. **Deviation 20, fixed** (it was 32) |
| H7 | EOF is a negative int | TESTED (new) | stdio_h.c:76-77 |
| H8 | NULL; stdin, stdout and stderr are macros of type FILE * | TESTED (new) | stdio_h.c:79-84, 162-168 |
| H9 | Prototypes of every function | TESTED (new) | stdio_h.c:87-158, checked with `_Generic` so nothing is linked |
| H10 | The function-like macros take the function's arguments | TESTED (new) | stdio_h.c:171-175. **Deviation 4, fixed** |
| H11 | asprintf and vasprintf are declared in POSIX.1-2024 mode | TESTED (new) | stdio_h.c:156-157. glibc needs `_GNU_SOURCE` |
| H12 | Strict mode declares no extensions | TESTED (new) | stdio_h-names.c:48-131. **Deviation 19, fixed** (asnprintf, vasnprintf). setbuffer and setlinebuf were already fixed. fdevopen, FDEV_* and printf_float are left |
| H13 | P_tmpdir is removed in Issue 8 | TESTED (new, not glibc) | stdio_h-xsi.c:44-47. **Deviation 19, fixed** |
| H14 | FILE's members don't collide with application macros | UNTESTED | **Left**: the members have plain names |
| H15 | The header may make `<stddef.h>` symbols visible | n/a | a permission |

### Streams, XSH 2.5 and 2.5.1

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | exit and returning from main flush every stream, after the atexit functions | TESTED (new) | fflush.c:160-163. **Deviation 6, fixed** |
| S2 | _exit doesn't flush | TESTED (new) | fflush.c:170-172 |
| S3 | Every function that reads, writes or positions a stream takes its lock, as if by flockfile | TESTED (new, through fgetc) | flockfile.c:183-199. **Deviation 6, fixed** |
| S4 | Each stream has its own lock | TESTED (new) | flockfile.c:169. **Deviation 6** (flockfile used one global lock) |
| S5 | stderr is not fully buffered; stdin and stdout are fully buffered only if they aren't interactive | UNTESTED | stdout is always line buffered (left, below) |
| S6 | Deadlock detection for line-buffered input requests | NOT TESTABLE | the clause only allows behaviour |
| S7 | A stream on an fdopen'd or inherited descriptor starts at the descriptor's offset | TESTED (new) | fdopen.c:87-94. **Deviation 7, fixed** |
| S8 | fflush on a seekable input stream sets the offset to the stream's position and drops ungetc bytes | TESTED | fflush.c:173-180 (new); fdopen.c:96; test-stdio/test-ungetc-fflush.c |
| S9 | fclose on an input stream sets the offset the same way | TESTED (new) | fflush.c:182-183 |
| S10 | After fflush, fseek sets the offset even if another handle moved it | TESTED (new, not glibc) | fseek.c:134-157. **Deviation 9, fixed** |
| S11 | Memory streams behave like file streams within their bounds | TESTED | fmemopen.c (new); test-stdio/test-fmemopen.c |
| S12 | No data is lost or duplicated when the handle rules are followed | TESTED (in part) | fdopen.c, fseek.c, fflush.c |
| S13 | freopen clears the orientation; the standard streams start unoriented | TESTED (freopen only, weakened) | freopen.c:108-112. picolibc's fwide(f, 0) never returns 0 (wide unit) |

### fopen, freopen, fdopen, fileno

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | The mode table: open flags for r, w and a, with + and b | TESTED (new) | fopen.c:117-128 |
| O2 | 'a' writes at end-of-file even after fseek | TESTED (new) | fopen.c:132-137 |
| O3 | A new file is created with mode 0666 less the umask | TESTED (new) | fopen.c:140-143 |
| O4 | 'e' sets FD_CLOEXEC; without it the flag is clear | TESTED | fopen.c:125, 148, 161 |
| O5 | 'x' with w or a gives O_EXCL and EEXIST | TESTED (new) | fopen.c:146-159. **Deviation 1, fixed** ('ax') |
| O6 | The error and EOF indicators are clear after opening | TESTED (new) | fopen.c:126 |
| O7 | 'w' marks st_mtime; creating a file marks its directory | TESTED (new) | fflush.c:200-211 |
| O8 | ENOENT (missing file, missing prefix, empty path), EISDIR, ENOTDIR, trailing slash, ELOOP, ENAMETOOLONG, EACCES | TESTED (new) | fopen.c:164-189. EACCES is skipped when running as root |
| O9 | EINVAL for an invalid mode (may fail) | TESTED (existing) | test-stdio/test-fopen.c |
| O10 | EMFILE, ENFILE, ENOSPC, ENXIO, EOVERFLOW, EROFS, ETXTBSY, EINTR, EILSEQ | UNTESTED | open's errors are passed through (inferred by reading) |
| R1 | freopen flushes first, then closes, then opens | TESTED (new) | freopen.c:93-98. **Deviation 18, fixed** |
| R2 | The original stream is closed even if the open fails | TESTED (new) | freopen.c:120-126. **Deviation 18, fixed** |
| R3 | The new descriptor comes from an open after the close, so stdout keeps 1 | TESTED (new) | freopen.c:148-166. **Deviation 18, fixed** |
| R4 | The indicators and the orientation are cleared | TESTED (new) | freopen.c:101-112. **Deviation 18, fixed** |
| R5 | The mode works as for fopen ('e', 'x') | TESTED (new) | freopen.c:114-117 |
| R6 | A null pathname changes the mode; whether the handle is reused is implementation-defined | TESTED (new; picolibc keeps the descriptor) | freopen.c:128-137 |
| R7 | EBADF for a null pathname with an invalid descriptor | TESTED (new) | freopen.c:139-144. **Deviation 21, fixed** |
| R8 | The other open errors | TESTED (ENOENT and EEXIST) | freopen.c:117, 124 |
| D1 | fdopen's stream starts at the descriptor's offset | TESTED (new) | fdopen.c:87-98. **Deviation 7, fixed** |
| D2 | 'w' doesn't truncate; 'x' has no effect | TESTED (new) | fdopen.c:122-135 |
| D3 | O_APPEND stays set; 'a' on a descriptor without O_APPEND leaves the offset alone | TESTED (new; the offset check is not run for glibc) | fdopen.c:138-156. **Deviation 8, fixed** |
| D4 | 'e' sets FD_CLOEXEC; without it the flag is unchanged | TESTED (new; 'e' not for glibc) | fdopen.c:105-120. **Deviation 8, fixed** |
| D5 | The indicators are cleared | TESTED (new) | fdopen.c:90 |
| D6 | EBADF (may fail) | TESTED (new, picolibc) | fdopen.c:158-163 |
| D7 | EMFILE when {STREAM_MAX} streams are open | NOT TESTABLE | there is no stream limit |
| F1 | fileno gives the descriptor; 0, 1 and 2 for the standard streams | TESTED | fileno.c:62; fdopen.c:91 |
| F2 | EBADF for a stream without a file | TESTED (new) | fileno.c:64-75. **Deviation 14, fixed** |

### fclose, fflush

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | fflush and fclose write buffered output | TESTED (new) | fflush.c:185-198 |
| C2 | Both mark st_mtime when they write data | TESTED (new) | fflush.c:194, 198 |
| C3 | fflush(NULL) flushes every output stream | TESTED (new) | fflush.c:165-168. **Deviation 6** |
| C4 | fclose closes the descriptor; the stream is closed even when fclose fails | TESTED (new) | fdopen.c:102-103; fflush.c:219-220 |
| C5 | On failure: EOF, the error indicator and errno | TESTED (new) | fflush.c:217-227; fseek.c:180-185 |
| C6 | EBADF, EPIPE, ENOSPC | TESTED (new) | fflush.c:213-227; fseek.c:180-185 (ENOSPC on /dev/full) |
| C7 | EAGAIN, EFBIG, EINTR, EIO, ENOMEM, ENXIO | UNTESTED | write's errors are passed through (inferred by reading) |

### Positioning: fseek, fseeko, ftell, ftello, fgetpos, fsetpos, rewind

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | SEEK_SET, SEEK_CUR, SEEK_END | TESTED | fseek.c:95-98 |
| P2 | A successful seek clears EOF and drops ungetc bytes | TESTED (new) | fseek.c:104-109 |
| P3 | Seeking past the end; the gap reads as zeros | TESTED (new) | fseek.c:116-118 |
| P4 | After fflush, fseek adjusts the file offset | TESTED (new, not glibc) | fseek.c:134-157. **Deviation 9, fixed** |
| P5 | A write error during the seek sets the error indicator | TESTED (new) | fseek.c:170-176. **Deviation 11, fixed** |
| P6 | EINVAL for a negative result or a bad whence | TESTED (new) | fseek.c:121-127 |
| P7 | ESPIPE on pipes, for fseek, ftell and ftello | TESTED (new) | fseek.c:158-168 |
| P8 | EOVERFLOW from fseek and ftell past LONG_MAX | TESTED (new, arm32 only; the fseek check is not run for glibc) | fseek.c:188-212. **Deviation 10, fixed** |
| P9 | EOVERFLOW from fseeko when off_t would overflow | UNTESTED | implemented in fseek.c; ext4 rejects offsets near OFF_MAX |
| P10 | ftell doesn't change errno when it succeeds | TESTED (new) | fseek.c:100-102 |
| P11 | ungetc moves the position back | TESTED | fseek.c:108; test-stdio/test-ungetc-ftell.c |
| P12 | fgetpos and fsetpos round trip, also past LONG_MAX | TESTED (new) | fseek.c:111-114, 201-202. Part of deviation 10 |
| P13 | rewind seeks to 0 and clears the error indicator | TESTED (new) | fseek.c:177-178 |
| P14 | EBADF, EAGAIN, EFBIG, EIO, ENOMEM | UNTESTED | only ENOSPC (on /dev/full) is tested |
| P15 | fmemopen: EINVAL for a position outside the buffer | TESTED (new) | fmemopen.c:106-114. **Deviation 15, fixed** |

### Byte I/O

| # | Requirement | Status | Reference |
|---|---|---|---|
| B1 | Reading a stream not open for reading gives EBADF and sets the error indicator | TESTED (new) | fgetc.c:80-87. **Deviation 13, fixed** |
| B2 | Writing a stream not open for writing gives EBADF and sets the error indicator | TESTED (new) | fgetc.c:89-97. **Deviation 13, fixed** |
| B3 | With the EOF indicator set, fgetc returns EOF; so do fread, fgets and getline | TESTED (new) | fgetc.c:114-131. **Deviation 12, fixed** (buffered files) |
| B4 | ungetc: one byte, clears EOF, EOF itself fails, fseek and fflush drop the byte | TESTED | fgetc.c:131; fseek.c:108-109; test-stdio/test-ungetc*.c |
| B5 | The same EBADF for the wide functions | TESTED (new) | fgetc.c:99-112 |
| B6 | The first read marks st_atime | NOT TESTABLE | relatime and noatime mounts |
| B7 | Read errors: EINTR | TESTED (new) | getline.c:103-127. EIO, ENOMEM, ENXIO, EOVERFLOW and EAGAIN are UNTESTED |

### getline, getdelim

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | Allocates or grows *lineptr and updates *n | TESTED | getline.c:93-101; getdelim.c |
| G2 | Stores the delimiter and a NUL; null bytes in the data are counted | TESTED (new) | getline.c:96-99 |
| G3 | A last line without a delimiter | TESTED | getline.c:99 |
| G4 | -1 at end-of-file, and when the EOF indicator is already set | TESTED (new) | getline.c:100; fgetc.c:121-126 |
| G5 | A read error part way through: -1, errno and the error indicator | TESTED (new; glibc returns the partial count) | getline.c:103-127. **Deviation 16, fixed** |
| G6 | EINVAL for a null lineptr or n | TESTED (new; for glibc only getline with a null lineptr is checked) | getline.c:76-91. **Deviation 16, fixed** (it crashed) |
| G7 | ENOMEM; EOVERFLOW (may fail) | UNTESTED | present in getdelim.c (inferred by reading) |

### popen, pclose

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | 'r': the child's stdout is the pipe and the parent holds the readable end | TESTED (new) | popen.c:124-130 |
| X2 | 'w': the child's stdin is the pipe and the parent holds the writable end | TESTED (new) | popen.c:132-140 |
| X3 | The shell runs as `execl(sh, "sh", "-c", "--", command)` | TESTED (new) | popen.c:166-176. **Deviation 17, fixed** |
| X4 | Streams from earlier popen calls are closed in the new child | TESTED (new) | popen.c:156-165. **Deviation 17, fixed** |
| X5 | 'e' sets FD_CLOEXEC; without it the flag is clear | TESTED (new) | popen.c:128, 148-154. **Deviation 17, fixed** |
| X6 | FD_CLOFORK is cleared | NOT TESTABLE | picolibc has no O_CLOFORK |
| X7 | If the shell can't run, the status is as for exit(127) | TESTED (new; glibc fails popen instead) | popen.c:178-199. **Deviation 17, fixed** (it was 1) |
| X8 | pclose returns the termination status | TESTED | popen.c:142-146 |
| X9 | pclose doesn't return before the child ends, even after EINTR | TESTED (new) | popen.c:201-215. **Deviation 17, fixed** |
| X10 | pclose leaves other children alone | TESTED (new) | popen.c:217-227 |
| X11 | ECHILD when the status was already collected | TESTED (existing) | pclose.c:66-72 |
| X12 | The parent and the child run independently | TESTED (inferred) | |
| X13 | Whether pthread_atfork handlers run is implementation-defined | n/a | they run (popen uses fork) |
| X14 | EINVAL for a bad mode (may fail) | TESTED (inferred) | popen.c rejects a mode not starting with r or w (inferred by reading) |

### Locking and the _unlocked functions

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | flockfile keeps a count; the owner may nest | TESTED (new) | flockfile.c:161-171 |
| L2 | ftrylockfile returns non-zero while another thread holds the lock | TESTED (new) | flockfile.c:165-167, 177. **Deviation 3, fixed** (it blocked) |
| L3 | flockfile waits for the owner | TESTED (new) | flockfile.c:174-181 |
| L4 | Stream functions take the stream lock | TESTED (new, through fgetc) | flockfile.c:183-199. **Deviation 6** |
| L5 | getc_unlocked, getchar_unlocked, putc_unlocked and putchar_unlocked work under flockfile | TESTED (new) | flockfile.c:201-224; stdio_h.c:171-175 |
| L6 | The library's own nested locking is benign | TESTED | test-open_wmemstream, with lock-valid.c. **Deviation 2, fixed** |

### printf and scanf families

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | %n$ in any order and repeated; *m$ width and precision; %% mixed with %n$; NL_ARGMAX of at least 9; v* functions; mixed types | TESTED (new) | fprintf.c:94-113 |
| N2 | asprintf returns an allocated result and its length | TESTED (new) | fprintf.c:115-118 |
| N3 | snprintf with n 0 and a null s | TESTED (new) | fprintf.c:120-124 |
| N4 | EOVERFLOW when the count would pass INT_MAX | TESTED (new; skipped under qemu and on bare metal) | fprintf.c:126-141. **Deviation 22, fixed** |
| N5 | fprintf on a read-only stream gives EBADF and sets the error indicator | TESTED (new) | fgetc.c:96 |
| N6 | dprintf and vdprintf write to the descriptor and return the count | TESTED (new) | dprintf.c:90-101 |
| N7 | dprintf: EBADF (may fail) | TESTED (new) | dprintf.c:104-105 |
| N8 | dprintf marks st_mtime | TESTED (new) | dprintf.c:107-112 |
| N9 | Repeated vdprintf calls don't keep memory | TESTED (new, picolibc mallinfo) | dprintf.c:114-127. **Deviation 5, fixed** |
| N10 | scanf %n$ | TESTED (new) | fprintf.c:144-149 |
| N11 | asprintf ENOMEM; EILSEQ for %lc | UNTESTED | |
| N12 | Conversion details | TESTED (existing) | the test/test-stdio printf and scanf suites |

### fmemopen, open_memstream

| # | Requirement | Status | Reference |
|---|---|---|---|
| M1 | 'r': reads stop at max_size, null bytes are ordinary, the buffer is unchanged | TESTED (new) | fmemopen.c:62-71 |
| M2 | 'w' truncates by writing a null byte | TESTED (new, not glibc) | fmemopen.c:73-80 |
| M3 | A null byte after writes when it fits, never past max_size | TESTED (new; glibc writes past) | fmemopen.c:81-88 |
| M4 | 'a' starts at the first null byte and writes at the end | TESTED (new) | fmemopen.c:90-98 |
| M5 | A null buf with '+' is allocated and starts at 0 | TESTED (new) | fmemopen.c:100-104 |
| M6 | Seeks outside the buffer fail with EINVAL | TESTED (new) | fmemopen.c:106-114. **Deviation 15, fixed** |
| M7 | A null buf without '+' gives EINVAL (may fail) | TESTED (existing) | test-stdio/test-fmemopen.c |
| M8 | open_memstream: growth; *bufp and *sizep after fflush and fclose; the size is the smaller of length and position; seeks past the end are zero-filled | TESTED (existing) | test-stdio/test-open_memstream.c. SEEK_END is relative to the size fflush would set (allowed) |
| M9 | fileno fails with EBADF on memory streams | TESTED (new) | fileno.c |

### ctermid, tmpfile, tmpnam, perror, remove, rename, renameat, setvbuf

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | ctermid(buf) and ctermid(NULL) fit in L_ctermid | TESTED (existing) | ctermid.c:63-70; stdio_h.c:178 |
| T2 | tmpfile gives an update stream on a file already removed | TESTED (new) | tmpnam.c:80-86 |
| T3 | tmpnam gives TMP_MAX different names, none of an existing file | TESTED (new, 10000 names) | tmpnam.c:88-99 |
| T4 | tmpnam(NULL) uses an internal object | TESTED (new) | tmpnam.c:89-90 |
| T5 | No library function calls tmpnam | TESTED (inferred) | tmpfile uses mkstemp |
| T6 | perror writes "s: message\n", or only the message for a null or empty s | TESTED (new) | perror.c:70-96 |
| T7 | remove unlinks files and removes directories as rmdir does | TESTED (new) | perror.c:98-110. **Deviation 23, fixed** |
| T8 | rename and renameat | TESTED (existing) | rename.c, renameat.c |
| T9 | setbuf and setvbuf | TESTED (existing) | test-stdio/test-setvbuf.c |
| T10 | tmpfile's errors | UNTESTED | |

### Counts

About 150 rows: 124 TESTED (about 100 by new checks), 14 UNTESTED, 6 NOT TESTABLE, 4 n/a and 2 partly tested. Before this unit about 35 had a test. 23 deviations were fixed and 6 are left.

## Deviations found and fixed

How each fix was proved:
- Reverse only the library change, with `git show <fix> -- libc libos | git apply -R` or by hand where later commits change the same lines, then rebuild and run the test. The check quoted below then fails, and it passes again with the fix.
- Most were proved on aarch64-linux-gnu. Fix 10 was proved on arm-linux-gnueabihf, and fix 4 in the native configuration, which builds without stdio-locking.

1. **fopen 'x' with 'a'** didn't set O_EXCL; sflags.c only handled 'w'. Failing check: `fopen.c: ax fails when the file exists`.
2. **vfwprintf took the stream lock a second time** through putwc. Once stdio-locking was on, lock-valid.c aborted test-open_wmemstream with "assertion failed". It now calls putwc_unlocked.
3. **ftrylockfile blocked.**
   - It now calls a new optional `__retarget_lock_try_acquire_recursive`: pthread_mutex_trylock in libos/linux, a stub in libc/misc.
   - Lock implementations that don't provide it keep the old blocking behaviour.
   - Failing check: test-flockfile hangs (timeout exit 124) at `ftrylockfile fails while another thread holds the lock`.
4. **getchar_unlocked and putchar_unlocked macros had the wrong number of arguments** without stdio-locking (1 and 2). Failing in the native build: `macro 'putchar_unlocked' requires 2 arguments, but only 1 given`.
5. **vdprintf leaked a lock on every call**: the lock of the FILE on its stack was allocated and never freed. Failing check: test-dprintf crashes (SIGSEGV) once the test's lock pool is used up.
6. **os-linux now always turns on stdio-locking and stdio-exit-flush.**
   - Changed in meson.build, with the options described in meson_options.txt, doc/build.md and doc/locking.md.
   - Before, flockfile took the global libc lock, which other threads' stdio calls never take, and exit flushed only stdout.
   - This fixes the cross-cutting "exit doesn't flush" item.
   - Failing checks: test-flockfile times out at `fgetc holds the stream lock while it waits`; test-fflush doesn't compile, because fflush was declared `__nonnull` without exit flush (`argument 1 null where non-null expected`).
7. **fdopen and the standard streams started at position 0** instead of the descriptor's offset.
   - ftell was wrong, fflush on an input stream moved the descriptor to the wrong place, and an fseek inside the buffer read the wrong data.
   - A new bufio flag, `__BSYNC`, fixes this. fdopen and freopen with a null path read the offset at once; the static standard streams read it before their first read, write or seek.
   - Failing check: `fdopen.c: ftell starts at the offset`.
8. **fdopen ignored 'e', didn't check the descriptor, and seeked to the end for 'a'** (which moved the descriptor). fopen now does that seek itself. Failing check: `fdopen.c: e sets FD_CLOEXEC`.
9. **fseek after fflush didn't call lseek**, because the in-buffer shortcut also applied to an empty buffer, and ftell went through the same path.
   - fseek now turns SEEK_CUR into SEEK_SET itself, with EOVERFLOW if off_t would overflow.
   - A seek function called with SEEK_CUR and 0 now only reports the position; it calls lseek(SEEK_CUR) only to detect pipes (ESPIPE).
   - Failing check: `fseek.c: fseek after fflush sets the offset`.
10. **On 32-bit targets, ftell truncated and fseek succeeded past LONG_MAX.** fgetpos and fsetpos now use ftello and fseeko. Failing check (arm32): `fseek.c: ftell EOVERFLOW`.
11. **fseek didn't set the error indicator** when its flush failed. rewind now clears the indicator after its seek. Failing check: `fseek.c: fseek sets the error indicator`.
12. **End-of-file wasn't sticky for buffered files**: data appended after EOF was read without clearerr, which ISO C fgetc forbids. Changed in `__bufio_get` and the fast fread path only. Failing check: `fgetc.c: fgetc with the end-of-file indicator set`.
13. **Reads and writes in the wrong direction** returned EOF without EBADF, and writes didn't set the error indicator. Fixed in fgetc, fgets, fread, fgetwc, fgetws, fputc, fputs, puts, fwrite, fputwc, fputws and vfprintf. Failing check: `fgetc.c: fgetc on a write-only stream`.
14. **fileno** returned -1 without setting errno for memory streams, and returned the cookie pointer for funopen and fopencookie streams. Failing check: `fileno.c: fileno of an fmemopen stream`.
15. **Failed fmemopen seeks didn't set errno.** Failing check: `fmemopen.c: negative position`.
16. **getdelim crashed on null pointers, and returned the partial count after a read error.** Failing: test-getline crashes (SIGSEGV) at the null-pointer checks.
17. **popen and pclose:**
    - Earlier popen streams stayed open in the child, so a second "w" child kept the first stream's write end open.
    - 'e' was ignored, there was no "--", argv[0] was "/sh", and the child exited with 1 instead of 127 when the shell couldn't run.
    - pclose returned -1 on EINTR instead of waiting.
    - The pipe is now created with O_CLOEXEC, which is cleared on the parent's end unless 'e' is given. Each stream is listed before fork, and fork holds `__LIBC_LOCK`, so the child sees a consistent list.
    - Failing check: `popen.c: re`.
18. **freopen:**
    - It opened before flushing and closing. The old data was written after a truncating open, and the new descriptor didn't reuse the old number.
    - It left the stream open when the open failed, and kept a wide orientation.
    - It now closes through the stream's close function, not `close` on a cookie.
    - Failing check, with fix 21 also reversed: `freopen.c: orientation cleared`.
19. **Namespace:** asnprintf and vasnprintf were visible in strict mode, and P_tmpdir with `_XOPEN_SOURCE 800`. asprintf and vasprintf are now declared under `__POSIX_VISIBLE >= 202405 || __MISC_VISIBLE`, with restrict. Failing at compile time: `conflicting types for 'asnprintf'` and `#error P_tmpdir was removed in Issue 8`.
20. **TMP_MAX was 32**; XSI requires at least 10000. Failing at compile time: `static assertion failed: "TMP_MAX on XSI systems"`.
21. **freopen(NULL, ...) succeeded on a closed descriptor.** Failing check: `freopen.c: freopen NULL EBADF`.
22. **printf's count overflowed past INT_MAX** instead of failing with EOVERFLOW. Failing check: `fprintf.c: EOVERFLOW`.
23. **remove failed for directories**: it only called unlink. Failing check: `perror.c: remove a directory`.

## Deviations left

- **FILE's members have plain names**: `unget`, `flags`, `put`, `get`, `flush`, `lock`, `close`, `file`, `cfile`, `seek`, `setvbuf` (stdio.h:91-142). An application macro such as `#define flags 1` before `<stdio.h>` breaks the header. A rename touches about 270 uses and any code that builds FILEs by hand, so it is left for the maintainer.
- **picolibc's embedded API is visible in strict mode**: fdevopen, fdev_close, FDEV_SETUP_*, `_FDEV_*` and printf_float. Hiding them would break `-std=c11` users, so this is left for a decision.
- **stdout is always line buffered**, even on a file or pipe. It would need an isatty check at startup.
- **Sticky EOF applies only to buffered files.** fdevopen device streams keep reading after `_FDEV_EOF`, since embedded consoles may rely on that.
- **fwide(f, 0) never returns 0** (picolibc has no "unoriented" state). For the wide unit.
- **The fseeko EOVERFLOW check is untested**, because ext4 rejects offsets near OFF_MAX.

## glibc 2.43 differences (the tests skip these checks for glibc with `__GLIBC__`)

- fdopen ignores 'e', and seeks to the end for 'a'.
- fmemopen with "w" doesn't write the initial null byte, and writes one past max_size.
- fseek after fflush doesn't call lseek when the position looks unchanged.
- getline and getdelim don't check every null pointer, and return the partial count after a read error.
- On 32-bit targets, fseek past LONG_MAX succeeds.
- popen returns NULL when the shell can't be executed.
- asprintf needs `_GNU_SOURCE`.
- P_tmpdir is still defined for XSI.

## Verified by running versus inferred

Every TESTED row was run on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and the `-native` glibc build. The CMake thumbv7m build ran the tests that build for bare metal, and the native and clang native builds were checked as below. Rows marked "(inferred)" or "passed through" were read in the code, not run.

## Commits

Library fixes:
```
7027598735 stdio: Make fopen's 'x' flag exclusive with 'a' as well as 'w'
d6ba829625 stdio: Use putwc_unlocked inside vfwprintf
96e520eb3e stdio: Make ftrylockfile fail when another thread holds the lock
fad6571691 stdio.h: Give getchar_unlocked and putchar_unlocked the right arguments
9e441fb1c3 stdio: Don't leak a lock for each vdprintf call
d6d8530888 Always use stdio-locking and stdio-exit-flush for os-linux
75e84415f1 stdio: Start fdopen and standard streams at the descriptor's offset
e5cfe4e8aa stdio: Make fdopen honour 'e', check the descriptor and leave the offset alone
08cc756633 stdio: Set the descriptor offset in fseek after fflush, and never seek in ftell
d3c0aecf02 stdio: Fail with EOVERFLOW when fseek or ftell's offset doesn't fit in long
f40fb1feda stdio: Set the error indicator when fseek can't flush the stream
316a86ce60 stdio: Keep the end-of-file indicator sticky for buffered files
f01fb2776e stdio: Report EBADF when reading a write-only stream or writing a read-only one
193e4be00b stdio: Fail with EBADF in fileno for streams without a descriptor
67089b7932 stdio: Set errno when fmemopen streams can't seek
39c7e006bd stdio: Check getdelim's pointers and return -1 after a read error
be2b871fe1 stdio: Bring popen and pclose in line with POSIX.1-2024
809ef70a94 stdio: Make freopen flush, close and then open, and close the stream on failure
79bb70bebe stdio.h: Hide asnprintf, vasnprintf and P_tmpdir in strict POSIX modes
dad1f857f5 stdio.h: Make TMP_MAX at least 10000
5798f31a82 stdio: Fail with EBADF in freopen with a null pathname and an invalid descriptor
c6a8db23c5 stdio: Fail with EOVERFLOW when printf's count would pass INT_MAX
518ee2384f stdio: Make remove remove directories
```

Tests: 16 "test-posix: Check ..." commits, one per test file:
- New files: stdio_h, stdio_h-names, stdio_h-xsi, fdopen, fseek, fgetc, fileno, fmemopen, getline, flockfile, fflush, freopen, dprintf, fprintf, tmpnam, perror.
- Rewritten: fopen and popen (now in mkdtemp directories).

## Verification

| Build | Ok | Fail | Expected Fail | Skipped |
|---|---|---|---|---|
| aarch64-linux-gnu, full suite | 1613 | 0 | 6 | 40 |
| arm-linux-gnueabihf, full suite | 1612 | 0 | 6 | 42 |
| clang native, full `meson test` | 1227 | 0 | 6 | 392 |

- The base had Ok 1579 and 42 Skipped on aarch64. Two tests that need stdio-locking now run.
- gcc native configuration: builds (ninja only).
- CMake thumbv7m `ninja test`: 100% of 634 tests passed (274 passed, 360 skipped).
