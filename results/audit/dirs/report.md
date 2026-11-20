# `<dirent.h>`, `<ftw.h>` and `<libgen.h>` audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-dirs`, branch `audit/dirs`, based on
a687616faa (`claude/posix-spec-audit`). The host is an AWS Graviton2, aarch64
Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43)
as an unprivileged user, who is also in group 0. Seccomp is 0.

**Implementation:**
- `libos/linux/`: opendir.c, fdopendir.c, closedir.c, readdir.c,
  rewinddir.c, seekdir.c, telldir.c, posix_getdents.c and local-dirent.h.
- `libc/posix/dirent/`: dirfd.c, readdir_r.c, scandir.c and alphasort.c.
- `libc/posix/ftw/nftw.c`.
- `libc/posix/libgen/`: basename.c and dirname.c.
- Headers: `libc/include/dirent.h`, `sys/dirent.h`, `ftw.h`, `libgen.h` and
  `sys/types.h` (for reclen_t).

**Tests:** `test/test-posix/test-dirent/`, `test-ftw/` and the new `test-libgen/`.
File names below are in those directories; line numbers are for the branch tip.

**Spec source:** the pubs.opengroup.org/onlinepubs/9799919799 pages
- basedefs: dirent.h, ftw.h, libgen.h and sys_types.h;
- functions: opendir (which also covers fdopendir), closedir, dirfd, readdir
  (which also covers readdir_r), rewinddir, seekdir, telldir, posix_getdents,
  scandir (which also covers alphasort), nftw, basename and dirname.

**What's out of scope:** ftw() is gone from Issue 8 (its page is a 404, and
Defect 1330 removed it from `<ftw.h>`), so it isn't audited. readdir_r is
obsolescent but still there, so it is audited. DT_TMO belongs to the Typed
Memory Objects option, which picolibc doesn't claim. `<ftw.h>`, `<libgen.h>`,
seekdir, telldir, nftw, basename and dirname are XSI.

**Status key:** TESTED (file:line), UNTESTED, NOT TESTABLE (reason).
- Every TESTED row was run and passes for picolibc (`test-X`) on aarch64-linux-gnu
  and arm-linux-gnueabihf.
- It also passes against glibc (`test-X-native`) unless the row says "picolibc
  only" or names a glibc guard.
- Permission checks are skipped when euid is 0.
- Every directory test builds its tree in a fresh `mkdtemp()` directory and
  removes it at the end.
- Results are compared as sets, never in readdir order.

## Requirement table

### `<dirent.h>` (basedefs/dirent.h, plus reclen_t in sys_types.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | DIR is defined (it may be incomplete) | TESTED | test-dirent_h.c:191 |
| H2 | struct dirent has `ino_t d_ino` and `char d_name[]` | TESTED | test-dirent_h.c:128-129 |
| H3 | struct posix_dent has d_ino, `reclen_t d_reclen`, `unsigned char d_type` and d_name | TESTED (picolibc only; glibc 2.43 lacks it) | test-dirent_h.c:139-143 |
| H4 | d_name holds a name of NAME_MAX bytes plus the NUL | TESTED for posix_dent | test-posix_getdents.c:136, 157. For struct dirent it was only read: `d_name[__NAME_MAX + 1]` |
| H5 | `<dirent.h>` defines ino_t, reclen_t, size_t and ssize_t; reclen_t is unsigned | TESTED | test-dirent_h.c:126, 138 |
| H6 | `<sys/types.h>` defines reclen_t | TESTED (picolibc only) | test-dirent_h.c:41. **It was missing; fixed** |
| H7 | DT_BLK … DT_UNKNOWN are distinct and usable in #if | TESTED (picolibc only; glibc defines them only with `_DEFAULT_SOURCE`) | test-dirent_h.c:156 |
| H8 | DT_MQ, DT_SEM and DT_SHM are distinct from each other and from the others, or never returned | TESTED (picolibc only) | test-dirent_h.c:164. That MAP_DT never produces them was only read |
| H9 | Prototypes for alphasort, closedir, dirfd, fdopendir, opendir, posix_getdents, readdir, rewinddir and scandir | TESTED (posix_getdents picolibc only) | test-dirent_h.c:173-184 |
| H10 | Prototypes for readdir_r (OB), seekdir and telldir (XSI) | TESTED | They compile in test-readdir_r.c and in test-telldir.c (`_XOPEN_SOURCE 800`) |
| H11 | Namespace: DIR's members don't use names the application may define as macros | TESTED | test-dirent_h.c:49-55. **The members were fd, offset, count, dirent, buf and align; fixed** |
| H12 | Namespace: strict POSIX hides seekdir, telldir, fdclosedir, scandirat, versionsort and getdents | TESTED | test-dirent_h.c:74-109, 192 |

### opendir, fdopendir

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | opendir opens a stream positioned at the first entry | TESTED | test-opendir.c:137, 152 |
| O2 | opendir's descriptor is allocated as if with O_DIRECTORY and O_CLOEXEC | TESTED | test-opendir.c:120. **FD_CLOEXEC wasn't set; fixed** |
| O3 | The descriptor counts toward OPEN_MAX | NOT TESTABLE | The kernel enforces it |
| O4 | opendir works through a symbolic link to a directory | TESTED | test-opendir.c:173 |
| O5 | fdopendir: the descriptor's offset determines the entries | TESTED | test-fdopendir.c:123, where the file system can seek to the end (ext4); test-telldir.c:89-98 starts a stream at 2^32 on tmpfs |
| O6 | fdopendir doesn't clear FD_CLOEXEC | TESTED | test-fdopendir.c:113 |
| O7 | closedir closes the descriptor given to fdopendir | TESTED | test-fdopendir.c:91 |
| E-O1 | fdopendir EBADF: not a descriptor, or not open for reading | TESTED | test-fdopendir.c:95, 97, 104 (O_SEARCH only where defined, so not glibc). **An O_SEARCH (O_PATH) descriptor was accepted; fixed** |
| E-O2 | fdopendir ENOTDIR | TESTED | test-fdopendir.c:80 |
| E-O3 | opendir EACCES: no read permission, or no search permission on the prefix | TESTED | test-opendir.c:201, 205 (skipped as root) |
| E-O4 | opendir ELOOP | TESTED | test-opendir.c:191 |
| E-O5 | opendir ENAMETOOLONG for a component | TESTED | test-opendir.c:195 |
| E-O6 | opendir ENOENT: missing name, missing prefix, empty name, dangling link | TESTED | test-opendir.c:177-183 |
| E-O7 | opendir ENOTDIR: a file, a file in the prefix, a link to a file | TESTED | test-opendir.c:185-189 |
| E-O8 | Optional errors: EMFILE, ENFILE, ELOOP for SYMLOOP_MAX, ENAMETOOLONG for PATH_MAX | UNTESTED | All are the kernel's |

### closedir, dirfd

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | closedir closes the stream and its descriptor, and returns 0 | TESTED | test-opendir.c:166-168 |
| C2 | closedir returns -1 and sets errno on failure | UNTESTED | **Deviation (only read):** closedir ignores close()'s result. See "Deviations not fixed" |
| E-C1 | Optional errors: EBADF, EINTR | NOT TESTABLE | Making close fail needs undefined behaviour (closing the stream's descriptor behind its back) |
| DF1 | dirfd returns the stream's descriptor | TESTED | test-dirfd.c:81, test-opendir.c:120 |
| DF2 | With no descriptor, dirfd opens one (O_RDONLY, O_DIRECTORY, O_CLOEXEC) | NOT TESTABLE on Linux | Every Linux stream has a descriptor. See dirfd's ENOTSUP under "Other observations" |
| DF3 | closedir closes the descriptor dirfd returned | TESTED | test-opendir.c:168 |
| E-DF1 | EMFILE, ENFILE, EINVAL | NOT TESTABLE | No new descriptor is ever needed |

### readdir, readdir_r, rewinddir

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | readdir returns the entry at the current position and moves to the next | TESTED | test-opendir.c:122-138 |
| R2 | readdir returns NULL at the end and leaves errno alone | TESTED | test-opendir.c:142 |
| R3 | d_ino is the file's serial number, and a link's own for a symbolic link | TESTED | test-opendir.c:133 |
| R4 | d_name is the name as created; no empty names | TESTED | test-opendir.c:124, 129 |
| R5 | Dot and dot-dot are each returned once | TESTED | test-opendir.c:137 |
| R6 | readdir on another stream leaves the returned entry alone | TESTED | test-opendir.c:162 |
| R7 | readdir marks the directory's access time when it really reads | NOT TESTABLE | /local and /tmp are mounted noatime |
| R8 | After fork, either parent or child may go on using the stream | UNTESTED | It is a permission; there is little to check |
| E-R1 | EOVERFLOW | NOT TESTABLE | ino_t is 64-bit on every target |
| E-R2 | ENOMEM | NOT TESTABLE | readdir doesn't allocate |
| E-R3 | Optional errors: EBADF, ENOENT | NOT TESTABLE | Undefined behaviour, or the kernel's |
| RR1 | readdir_r fills entry and sets `*result = entry` | TESTED | test-readdir_r.c:95 |
| RR2 | At the end, `*result` is NULL and readdir_r returns 0 | TESTED | test-readdir_r.c:99 |
| RR3 | readdir_r returns an error number on error | UNTESTED | readdir can't be made to fail (as for E-R1 to E-R3); the code was read |
| RW1 | rewinddir goes back to the first entry | TESTED | test-opendir.c:152, test-telldir.c:167 |
| RW2 | rewinddir shows the directory's current state | TESTED | test-opendir.c:148, 150 |

### seekdir, telldir

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | telldir gives the current position, and seekdir returns to it | TESTED | test-telldir.c:135-142 |
| T2 | Every position returns to its own entry, in any order, including the end | TESTED | test-telldir.c:146-162 |
| T3 | telldir right after seekdir returns that seekdir's loc | TESTED | test-telldir.c:140, 95 |
| T4 | telldir has no errors: positions beyond LONG_MAX work where long is 32 bits | TESTED | test-telldir.c:92-98 (a tmpfs directory offset of 2^32). **armhf failed with EOVERFLOW; fixed** |

### posix_getdents

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | Fills buf with posix_dent records, up to nbyte bytes | TESTED | test-posix_getdents.c:91 |
| G2 | Returns at least one entry when nbyte is greater than sizeof(struct posix_dent) + NAME_MAX | TESTED | test-posix_getdents.c:152-157, with a NAME_MAX name. **armhf failed with EINVAL; fixed** |
| G3 | Records are aligned, and d_reclen includes the padding, the last record's too | TESTED | test-posix_getdents.c:97-98 |
| G4 | d_ino, and a link's own for a symbolic link | TESTED | test-posix_getdents.c:110 |
| G5 | d_type is DT_REG, DT_DIR, DT_LNK or DT_FIFO (a link's own type), or DT_UNKNOWN | TESTED | test-posix_getdents.c:107 |
| G6 | d_name is the name as created; no empty names | TESTED | test-posix_getdents.c:99-105 |
| G7 | After a call, the offset follows the last entry returned | TESTED | test-posix_getdents.c:164-175 |
| G8 | Reading from 0 to the end gives dot and dot-dot once each | TESTED | test-posix_getdents.c:157 |
| G9 | Returns 0 at the end | TESTED | test-posix_getdents.c:115, 161 |
| G10 | Non-zero flags are implementation-defined; picolibc rejects them with EINVAL | TESTED (picolibc only) | test-posix_getdents.c:145 |
| G11 | buf needs no stricter alignment than malloc gives | Read only | `_Alignof(struct posix_dent)` is 8 |
| G12 | Marks the access time; concurrent changes are atomic | NOT TESTABLE | noatime mounts; the second is the kernel's |
| E-G1 | EBADF: not a descriptor, or not open for reading (O_SEARCH) | TESTED | test-posix_getdents.c:180, 191 |
| E-G2 | EINVAL when nbyte is too small | TESTED | test-posix_getdents.c:148 |
| E-G3 | ENOENT when the offset isn't at an entry | NOT TESTABLE | ext4 and tmpfs accept any offset |
| E-G4 | ENOTDIR | TESTED | test-posix_getdents.c:184 |
| E-G5 | EOVERFLOW | NOT TESTABLE | ino_t is 64-bit |
| E-G6 | Optional errors: EIO, ENOMEM | NOT TESTABLE | |

glibc 2.43 has no posix_getdents, so test-posix_getdents-native skips.

### scandir, alphasort

| # | Requirement | Status | Reference |
|---|---|---|---|
| SC1 | alphasort compares names as strcoll does | TESTED (C locale) | test-alphasort.c:59-61 |
| SC2 | alphasort leaves errno alone | TESTED | test-alphasort.c:65 |
| SC3 | scandir calls sel on every entry | TESTED | test-scandir.c:165 |
| SC4 | The entries and the array are allocated as if by malloc (free works) | TESTED | test-scandir.c:137-146, 172-175 |
| SC5 | The entries are sorted with compar | TESTED | test-scandir.c:137 |
| SC6 | A null sel selects everything | TESTED | test-scandir.c:144 |
| SC7 | compar need not be a total order | TESTED | test-scandir.c:170 |
| SC8 | scandir returns the count and the array, or -1 | TESTED | test-scandir.c:135, 150 |
| E-SC1 | ENOENT (including an empty name) and ENOTDIR | TESTED | test-scandir.c:156-160 |
| E-SC2 | EACCES, ELOOP, ENAMETOOLONG | UNTESTED | They are opendir's errors, passed through (only read) |
| E-SC3 | ENOMEM; EOVERFLOW beyond INT_MAX entries | NOT TESTABLE | |

Until this branch, the picolibc builds of test-scandir, test-readdir and
test-dirfd were skipped on every Linux configuration; see "Other changes".

### `<ftw.h>` (basedefs/ftw.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| FH1 | struct FTW has `int base` and `int level` | TESTED | test-ftw_h.c:45 |
| FH2 | FTW_F, FTW_D, FTW_DNR, FTW_DP, FTW_NS, FTW_SL and FTW_SLN are distinct | TESTED | test-ftw_h.c:55 (`_Static_assert`, since glibc uses enumerators) |
| FH3 | FTW_PHYS, FTW_MOUNT, FTW_DEPTH and FTW_CHDIR are distinct bits | TESTED | test-ftw_h.c:68 |
| FH4 | FTW_XDEV | TESTED (picolibc only; glibc lacks it) | test-ftw_h.c:75. **It was missing; fixed** |
| FH5 | struct stat, the st_mode names and the file type macros | TESTED | test-ftw_h.c:81, 98 |
| FH6 | The nftw prototype | TESTED | test-ftw_h.c:89 |
| FH7 | XSI-only: hidden in strict POSIX mode | TESTED (before this branch) | test-integration/test-namespace.c (FTW_PHYS) |

### nftw

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | Walks the whole tree; FTW_D, FTW_F and the levels | TESTED | test-nftw.c:256-266 |
| N2 | Without FTW_DEPTH, a directory comes before its contents | TESTED | test-nftw.c:263-264 |
| N3 | With FTW_DEPTH, a directory comes after its contents, as FTW_DP | TESTED | test-nftw.c:275-279, 319 |
| N4 | FTW_PHYS doesn't follow links (FTW_SL, even for dangling links); without it, links are followed | TESTED | test-nftw.c:265-266, 277, 345 |
| N5 | FTW_SLN only without FTW_PHYS, and its stat buffer describes the link | TESTED | test-nftw.c:121, 278 |
| N6 | Without FTW_PHYS, with FTW_DEPTH: no directory is reported inside itself | TESTED | test-nftw.c:328. **nftw descended until ENAMETOOLONG; fixed** |
| N7 | Without FTW_PHYS or FTW_DEPTH, that directory is reported but not its contents | TESTED (`#ifndef __GLIBC__`) | test-nftw.c:336-337. **Fixed by the same change.** A link to a sibling directory is walked (line 333) |
| N8 | FTW_CHDIR: the working directory is the file's directory | TESTED | test-nftw.c:282 |
| N9 | Without FTW_CHDIR, the working directory doesn't change | TESTED | test-nftw.c:118 |
| N10 | FTW_MOUNT: nothing with another st_dev, and no descent into it | TESTED | test-nftw.c:230 (walks /dev, where /dev/shm and /dev/pts are other file systems; skipped elsewhere) |
| N11 | FTW_XDEV: the directory with another st_dev is reported, but nothing below it | TESTED (picolibc only) | test-nftw.c:236. **It was missing; fixed** |
| N12 | base and level in struct FTW | TESTED | test-nftw.c:116-117, 262-265 |
| N13 | FTW_DNR for a directory that can't be read, and nothing inside it | TESTED | test-nftw.c:357-358 (skipped as root) |
| N14 | FTW_NS when stat fails for lack of permission | TESTED | test-nftw.c:360 (skipped as root) |
| N15 | Any other stat failure is an error, returning -1 | TESTED | test-nftw.c:343 (an ELOOP link). **It was reported as FTW_NS; fixed** |
| N16 | fd_limit caps the descriptors nftw uses, at most one per level | TESTED | test-nftw.c:300-311 (limits 1 to 3, depth 6). **The limit was ignored; fixed** |
| N17 | nftw's descriptors are FD_CLOEXEC | TESTED | test-nftw.c:310. **Fixed through opendir** |
| N18 | Every descriptor is closed before nftw returns | TESTED | test-nftw.c:311 |
| N19 | Stops at fn's first non-zero result and returns it | TESTED | test-nftw.c:285-290 (stopping after 1 to 6 calls, with and without FTW_DEPTH) |
| N20 | Returns 0 when the tree is done | TESTED | throughout |
| E-N1 | EACCES when the path can't be searched | TESTED | test-nftw.c:362 (skipped as root) |
| E-N1b | EACCES when the starting directory can't be read | UNTESTED | **Ambiguous; not changed.** nftw reports it as FTW_DNR, as glibc does; see "Deviations not fixed" |
| E-N2 | ELOOP, ENAMETOOLONG (component), ENOENT (including an empty path), ENOTDIR | TESTED | test-nftw.c:374-384 |
| E-N3 | EOVERFLOW | NOT TESTABLE | 64-bit stat fields |
| E-N4 | Optional errors: EMFILE, ENFILE, ENAMETOOLONG for PATH_MAX | UNTESTED | |

### basename, dirname, `<libgen.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| B1 | basename: the last component, without trailing slashes | TESTED | test-basename.c:67-75 |
| B2 | basename: all slashes give "/"; exactly "//" gives "/" or "//" | TESTED | test-basename.c:76-80 |
| B3 | basename: NULL or "" gives "." | TESTED | test-basename.c:81-90 |
| B4 | basename and dirname are thread-safe (Issue 8) | Read only | Neither keeps static state; they return into the argument or a string literal |
| D1 | dirname: the containing directory, ignoring trailing slashes that aren't leading ones | TESTED | test-dirname.c:68-78 |
| D2 | dirname keeps ".." components before the last one | TESTED | test-dirname.c:80-81 |
| D3 | dirname may remove redundant slashes; "//" and "//foo" give "/" or "//" | TESTED | test-dirname.c:83-87 |
| D4 | dirname doesn't resolve the path | TESTED | test-dirname.c:89 |
| D5 | dirname: no '/', NULL or "" gives "." | TESTED | test-dirname.c:71, 90-94 |
| L1 | `<libgen.h>` prototypes | TESTED | test-libgen_h.c:56-57, 73-74 |
| L2 | After `#undef basename` or `#undef dirname`, the functions are still declared | TESTED (`#ifndef __GLIBC__`) | test-libgen_h.c:61-80. **basename was undeclared; fixed** |

### Counts

- Rows: 126 (some group a few related requirements). Before this branch, 35
  were tested, counted from the old test-readdir, test-fdopendir,
  test-posix_getdents, test-telldir, test-scandir, test-alphasort,
  test-readdir_r, test-dirfd and test-nftw, plus libc-testsuite's basename and
  dirname. Three of those (test-readdir, test-dirfd and test-scandir) never
  actually ran against picolibc on Linux.
- After this branch: 103 TESTED, 14 NOT TESTABLE, 7 UNTESTED and 2 checked by
  reading only.
- Deviations: 11 fixed (13 requirement rows), and 2 left: closedir's result and
  the unreadable starting directory in nftw.

## Deviations found and fixed

Each fix was verified by running it. With only the library change reversed
(`git show <fix> -- libc libos | git apply -R`, rebuild, run, reapply), the new
check fails, and with the fix it passes. The failing check is quoted. The runs
were on aarch64, except where armhf is named.

1. **DIR's member names weren't reserved** (libc/include/sys/dirent.h).
   - `#define count 3` (or fd, offset, buf, align) before `#include <dirent.h>`
     broke the header.
   - The members are now `__dd_fd`, `__dd_offset` and so on, and every user
     changed with them (libos/linux, dirfd.c, the hexagon semihost files).
   - The reverse patch doesn't apply on its own, because later commits touch the
     same lines, so the old names were put back by hand in the header.
   - Failing before: `test-dirent_h.c:49: error: expected identifier or '(' before numeric constant` (rerun with the final macros).
2. **reclen_t was missing from `<sys/types.h>`** (libc/include/sys/types.h).
   POSIX.1-2024 (Defect 697) adds it there, and `<sys/dirent.h>` now guards its
   own typedef. Failing before: `'reclen_t' undeclared` at test-dirent_h.c:41.
3. **opendir's descriptor wasn't close-on-exec** (libos/linux/opendir.c). Issue 8
   says "as if the O_DIRECTORY and O_CLOEXEC flags were passed to open()". This
   also gives nftw the required FD_CLOEXEC. Failing before:
   - `FD_CLOEXEC: fd >= 0 && (fcntl(fd, F_GETFD) & FD_CLOEXEC)` (test-opendir);
   - `FD_CLOEXEC: extra_inherited == 0` (test-nftw).
4. **fdopendir accepted O_SEARCH descriptors** (libos/linux/fdopendir.c).
   - picolibc's O_SEARCH is O_PATH, which fstat accepts, so the stream was made
     and every read then failed.
   - POSIX: "[EBADF] The fd argument is not a valid file descriptor open for
     reading".
   - fdopendir now checks F_GETFL for O_PATH.
   - Failing before: `EBADF for O_SEARCH: fdopendir(dfd) == NULL && errno == EBADF`.
5. **posix_getdents failed with EINVAL on armhf for the smallest buffer the spec
   allows** (libos/linux/posix_getdents.c).
   - The spec requires at least one entry when nbyte is greater than the size
     of struct posix_dent plus NAME_MAX.
   - On 32-bit targets sizeof(struct posix_dent) is 16, so 272 bytes have to be
     enough, but the kernel's record for a 255-byte name takes 280.
   - Buffers smaller than a whole kernel record now read through a 280-byte
     buffer on the stack, convert what fits, and lseek to the d_off of the last
     record converted.
   - aarch64 isn't affected (its posix_dent header is 24 bytes) and passes
     either way.
   - Failing before (armhf): `end of directory: n == 0` (the call returned -1).
6. **telldir failed with EOVERFLOW for offsets beyond LONG_MAX on armhf**
   (libos/linux/telldir.c, seekdir.c, closedir.c, sys/dirent.h).
   - telldir has no errors defined.
   - Where long is 32 bits, offsets that don't fit are now kept in a table in
     the DIR and reported as `-2 - index`. Kernel offsets are never negative.
     seekdir looks them up, and closedir frees the table.
   - On real AArch32 hardware, ext4 gives a compat task 31-bit hashes, so the
     test makes a large offset itself: on tmpfs (/dev/shm) it seeks a directory
     descriptor to 2^32 and hands it to fdopendir.
   - Under qemu-user on x86_64, every ext4 offset is 64-bit, which is why
     test-telldir used to skip there. With this fix it should pass under qemu
     too. The glibc-only EOVERFLOW skip remains, guarded with `__GLIBC__`.
   - Failing before (armhf): `telldir at 2^32: pos != -1`.
7. **nftw reported every stat failure as FTW_NS, and every opendir failure as
   FTW_DNR** (libc/posix/ftw/nftw.c).
   - POSIX: FTW_NS is for "lack of appropriate permission"; "Failure of stat()
     for any other reason is considered an error and nftw() shall return -1".
   - Now only EACCES gives FTW_NS or FTW_DNR. ENOENT on a link still gives
     FTW_SLN. Read errors are errors too.
   - glibc agrees on ELOOP: the test passes natively.
   - Failing before (with only the EACCES test reversed by hand):
     `ELOOP inside the tree: nftw("lp", record, 4, 0) == -1 && errno == ELOOP`.
     The DNR and readdir parts aren't separately testable (they need EMFILE or
     an I/O error).
8. **nftw walked a directory inside itself through a link to a parent**
   (nftw.c).
   - Without FTW_PHYS it descended `cyc/a/up/a/up/…` until the path was too
     long, then returned -1.
   - It now keeps the chain of directories being walked (dev and ino). Under
     FTW_DEPTH it doesn't report a link back to one of them at all; otherwise
     it reports the link as FTW_D and doesn't descend.
   - Failing before (ancestor check disabled by hand, since the reverse patch
     conflicts with the next commit): `nftw: nftw(path, record, fd_limit,
     flags) == 0` for the FTW_DEPTH walk of cyc.
9. **nftw ignored fd_limit** (nftw.c). It kept one directory open for every
   level. At the last level fd_limit allows, it now reads the directory into
   memory and closes it before descending. Failing before:
   `no more than fd_limit descriptors: max_fds >= 1 && max_fds <= i`.
10. **FTW_XDEV was missing** (ftw.h, nftw.c). It is new in Issue 8 (Defect 1133).
    Failing before (code only reversed): `FTW_XDEV reports the mount point only:
    mine == 1 && other_dev > 0`. With the header reversed too, test-ftw_h and
    test-nftw don't compile.
11. **`#undef basename` left basename undeclared** (libc/include/libgen.h).
    - The header defined basename as `__xpg_basename` and declared only that.
      XSH 2.1.1: "The use of #undef to remove any macro definition will also
      ensure that an actual function is referred to."
    - It now declares `char *basename(char *)` and does `#define basename basename`,
      which still keeps `<string.h>`'s GNU version out.
    - The `__xpg_basename` form is kept for when `<string.h>` with `_GNU_SOURCE`
      came first.
    - The three include orders were checked by compiling them to assembly:
      string.h then libgen.h calls `basename`; libgen.h then string.h calls
      `basename`, also after #undef; string.h alone with `_GNU_SOURCE` calls
      `__gnu_basename`.
    - Failing before: `'basename' undeclared` at test-libgen_h.c:64.

## Deviations not fixed

- **closedir ignores close()'s result** (libos/linux/closedir.c, only read). It
  always returns 0, even if close fails (EINTR or EIO). Making close fail without
  undefined behaviour isn't possible here, so the fix couldn't be proved. It is a
  one-line change: return -1 when close does.
- **nftw on an unreadable starting directory** reports FTW_DNR and returns fn's
  result. ERRORS lists "read permission is denied for path" under EACCES, while
  FTW_DNR describes the same case. glibc does the same as picolibc. I left it.
- **O_SEARCH skipping the search check** doesn't apply to this unit's functions;
  there are no *at functions here. Recorded for completeness: NOT TESTABLE, as in
  files.

## Other observations (not changed)

- dirfd returns -1/ENOTSUP for a stream without a descriptor (libc/posix/dirent/dirfd.c).
  - Issue 8 removed ENOTSUP and requires dirfd to open one instead.
  - No Linux stream lacks a descriptor, and the semihost fake opendir never makes
    a stream, so it can't happen in practice; the hexagon semihost stream always
    has one.
  - Only read.
- `<dirent.h>` declares fdclosedir (`__BSD_VISIBLE`) and scandirat and
  versionsort (`__GNU_VISIBLE`), but none of them is implemented. They are hidden
  in strict mode (test-dirent_h.c checks this), but a program that asks for them
  fails to link.
- struct dirent's d_type is a picolibc extension. It is fine, because d_ is
  reserved.
- Under FTW_XDEV, a directory on another file system is reported as FTW_D even
  with FTW_DEPTH, because its contents weren't visited (FTW_DP would claim they
  were). The spec doesn't say which.
- Under FTW_CHDIR, nftw changes to each file's directory around the call and
  back to the starting directory, which it records with getcwd. A starting
  directory longer than PATH_MAX makes FTW_CHDIR fail.
- seekdir with a value telldir never returned is unspecified. A negative value
  that isn't in the table is passed to lseek, which fails, and the position is
  left unchanged.

## Other changes

- **test-posix: Run the readdir, dirfd and scandir tests on Linux.**
  test-dirent/meson.build skipped the picolibc builds of these tests whenever
  `native-tests` was on, which every Linux configuration sets. It now skips them
  only with `use-stdlib` (the native configuration), as its comment intended. All
  three pass against picolibc on both targets.

## Left undone

- C2 (closedir's result), RR3 (readdir_r's error number) and R8 (fork): see above.
- The optional EMFILE, ENFILE and SYMLOOP_MAX errors (opendir, scandir, nftw).
  They come from the kernel.
- E-SC2: scandir passes opendir's errors through, and that code was only read.
- FTW_MOUNT and FTW_XDEV need a mount point inside a tree, which an unprivileged
  user can't make. The test walks /dev instead, where /dev/shm is tmpfs and
  /dev/pts is devpts, and skips those checks on hosts without a separate /dev/shm.
  glibc passes the FTW_MOUNT part.

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`): full
  `meson test -t 4` at the branch tip, 1314 tests. **1267 Ok, 0 Fail, 6 Expected
  Fail, 41 Skipped.** In this unit's suites, only test-posix_getdents-native skips
  (glibc has no posix_getdents).
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`): full
  `meson test -t 4` at the branch tip, 1313 tests. **1264 Ok, 0 Fail, 6 Expected
  Fail, 43 Skipped.** test-telldir-native stays in test_skip_native on arm.
- Native configuration (`scripts/do-native-configure`, `builds/build-native`):
  `ninja` builds cleanly.
- CMake (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 523 tests,
  100% passed (247 ran, 276 skipped).
  - test-dirent_h, test-ftw_h, test-basename, test-dirname and test-libgen_h run
    there.
  - The behaviour tests skip there for lack of OS support.
- Every new and changed test also passes as `-native` (glibc) on both Linux
  targets, except where the table says picolibc only.
- When run directly, test-nftw and test-opendir printed "success" with no skip
  messages, so the FTW_MOUNT/FTW_XDEV checks (/dev and /dev/shm) and the
  permission checks really ran.
- `clang-format --dry-run --Werror` is clean on every C file and header the
  branch touches.
- What was not run:
  - Nothing was run as root, so the root paths that skip permission checks were
    only read.
  - Bare-metal meson targets weren't run here; the x86_64 host does them.
  - The qemu-user behaviour of test-telldir on armhf (now expected to pass rather
    than skip) wasn't run here.

## Commits (audit/dirs, on top of a687616faa)

- `130241842d` dirent: Give the DIR members reserved names
- `0f9f37e2e9` sys/types: Define reclen_t
- `5a626bbb37` libos/linux: Open directory streams close-on-exec
- `7b4689291e` libos/linux: Fail fdopendir with EBADF for O_SEARCH descriptors
- `f094438502` libos/linux: Return an entry from posix_getdents in the smallest buffer
- `565fb3cdea` libos/linux: Report every directory position from telldir
- `00e151147e` ftw: Report FTW_NS and FTW_DNR only for EACCES in nftw
- `4614a58b9c` ftw: Don't walk a directory inside itself in nftw
- `814078543c` ftw: Use no more than fd_limit descriptors in nftw
- `0a655899a7` ftw: Add FTW_XDEV
- `71cd96a190` libgen: Declare basename under its own name
- `bdea700a08` test-posix: Run the readdir, dirfd and scandir tests on Linux
- `d08946af5b` test-posix: Check the types, constants, prototypes and namespace of <dirent.h>
- `2b8da02965` test-posix: Check close-on-exec, entries, rewinding and errors in opendir
- `70fa7902e5` test-posix: Check EBADF, FD_CLOEXEC and the offset in fdopendir
- `c6998079e0` test-posix: Check small buffers, types, offsets and errors in posix_getdents
- `c8c87ae148` test-posix: Check every position and 64-bit offsets in telldir
- `de288acbfd` test-posix: Check errors and selection in scandir, and errno in alphasort
- `08c73fcbd1` test-posix: Check the structure, constants and prototype in <ftw.h>
- `1230ef8fa5` test-posix: Check flags, links, descriptors, permissions and errors in nftw
- `1cfb66ff9d` test-posix: Check basename, dirname and <libgen.h>
