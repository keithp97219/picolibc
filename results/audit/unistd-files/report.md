# `<unistd.h>` file interfaces: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-unistd-files`, branch `audit/unistd-files`, based on d14119a43a (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43, clang 21 for the syntax check) as an unprivileged user: 13 supplementary groups including group 0, Seccomp 0, RLIMIT_NOFILE 65535, build directory on ext4, /dev/shm on tmpfs.

Interfaces covered:
- **I/O and descriptors:** read, pread, write, pwrite, close, posix_close, dup, dup2, dup3, pipe, pipe2, lseek, fsync, fdatasync, ftruncate, truncate.
- **Links and paths:** link, linkat, symlink, symlinkat, readlink, readlinkat, unlink, unlinkat, rmdir.
- **Directories:** chdir, fchdir, getcwd.
- **Access and ownership:** access, faccessat, chown, fchown, lchown, fchownat.
- **Other:** lockf (XSI), getentropy (new in Issue 8), sync (XSI), and the `<unistd.h>` constants and types they use.

isatty, ttyname and ttyname_r were covered by the termios unit (its T1-T3 and N5) and aren't repeated. rename and renameat belong to `<stdio.h>`, and `<unistd.h>` doesn't need them. Every interface above is in Issue 8.

Implementation:
- `libos/linux/`: read.c, pread.c, write.c, pwrite.c, close.c, dup.c, dup2.c, dup3.c, pipe.c, pipe2.c, lseek.c, fsync.c, fdatasync.c, ftruncate.c, truncate.c, link.c, linkat.c, symlink.c, symlinkat.c, readlink.c, readlinkat.c, unlink.c, unlinkat.c, rmdir.c, chdir.c, fchdir.c, getcwd.c, access.c, faccessat.c, chown.c, fchown.c, lchown.c, fchownat.c, getentropy.c, sync.c, and the `_at_fd_to_linux`/`_at_flags_to_linux` helpers in local-linux.h.
- `libc/posix/unistd/`: lockf.c, posix_close.c.
- `libos/dummyhost/getentropy.c`.
- Headers: `libc/include/unistd.h` → `sys/unistd.h`, and `sys/syslimits.h` for GETENTROPY_MAX.

Tests:
- They are in `test/test-posix/test-unistd/`. File names below are there and drop the `test-` prefix (`read.c` = `test-read.c`).
- The cancellation probes are in `test/test-posix/test-pthread/test-pthread_testcancel.c` (cited as `pthread_testcancel.c`).
- Line numbers are for the branch tip.
- Every new test works in a fresh `mkdtemp()` directory in the build directory and removes it.
- Directory changes happen in a child (getcwd.c).
- Permission-denial checks are skipped when the euid is 0.
- The chown checks succeed only with the caller's own uid and groups, and use other IDs only to get EPERM.

Spec source: pubs.opengroup.org/onlinepubs/9799919799.
- **basedefs pages:** unistd.h, and limits.h for GETENTROPY_MAX.
- **functions pages:** read (covers pread), write (pwrite), close (posix_close), dup (dup2, dup3), pipe (pipe2), lseek, fsync, fdatasync, ftruncate, truncate, link (linkat), symlink (symlinkat), readlink (readlinkat), unlink (unlinkat), rmdir, chdir, fchdir, getcwd, access (faccessat), chown (fchownat), fchown, lchown, lockf, getentropy, sync.

Kernel behaviour was checked against the v5.10 source on elixir.bootlin.com:
- fs/open.c: close maps ERESTART* to EINTR only after the descriptor is cleared; do_sys_ftruncate.
- fs/namei.c: do_unlinkat and may_delete give EISDIR only for directories; do_rmdir; do_symlinkat uses getname, which gives ENAMETOOLONG at 4096 bytes.
- fs/stat.c: do_readlinkat returns EINVAL for `bufsiz <= 0` before the lookup.
- fs/read_write.c: ksys_pread64 and ksys_pwrite64 return EINVAL for a negative position before looking up the descriptor.
- fs/d_path.c: getcwd gives ERANGE for any short size, including 0, and uses the "(unreachable)" prefix.
- fs/file.c: ksys_dup3 gives EINVAL for equal descriptors.
- fs/locks.c: flock64_to_posix_lock's EOVERFLOW and EINVAL; posix_locks_deadlock.

Status key: TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row was run and passes for picolibc (`test-X`) and against glibc (`test-X-native`) on aarch64-linux-gnu and arm-linux-gnueabihf, unless the row says picolibc only. "(new)" marks checks this unit added; "(existing)" marks checks that were already there.

## Requirement table

### `<unistd.h>` declarations and constants (basedefs/unistd.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | size_t, ssize_t (signed), off_t (signed), uid_t, gid_t, pid_t, intptr_t | TESTED (new) | unistd_h-files.c:61-64 |
| H2 | NULL | TESTED (new) | unistd_h-files.c:67 |
| H3 | F_OK, R_OK, W_OK, X_OK usable in #if; F_OK and the R_OK/W_OK/X_OK combinations distinct | TESTED (new) | unistd_h-files.c:70-82 |
| H4 | SEEK_SET, SEEK_CUR, SEEK_END; SEEK_HOLE and SEEK_DATA distinct from each other and from them | TESTED (new) | unistd_h-files.c:83-93 |
| H5 | F_LOCK, F_TEST, F_TLOCK, F_ULOCK (XSI), distinct | TESTED (new) | unistd_h-files.c:94-99 |
| H6 | ... and not defined without XSI | TESTED (new) | unistd_h-names.c:81 |
| H7 | STDIN_FILENO 0, STDOUT_FILENO 1, STDERR_FILENO 2 | TESTED (new) | unistd_h-files.c:102-104 |
| H8 | O_CLOEXEC defined as in `<fcntl.h>` | TESTED (new, picolibc only; glibc lacks it) | unistd_h-files.c:106-112. **Was missing; fixed** |
| H9 | O_CLOFORK defined as in `<fcntl.h>` | UNTESTED | **Missing; not fixed** (no close-on-fork; see the files unit's cross-cutting entry) |
| H10 | POSIX_CLOSE_RESTART defined; not 0 unless posix_close never gives EINTR | TESTED (new, picolibc only) | unistd_h-files.c:114-119. Its value is 1; picolibc's posix_close never returns EINTR, so either value is allowed |
| H11 | `_POSIX_CHOWN_RESTRICTED` defined, not -1 | UNTESTED | **Not defined.** An option constant, left to unistd-config (cross-cutting.md) |
| H12 | Prototypes of all 40 file functions with the spec's types (restrict in readlink/readlinkat) | TESTED (new) | unistd_h-files.c:132-173 (`_Generic` on each address; posix_close picolibc only) |
| H13 | lockf and sync (XSI) not declared without XSI | TESTED (new) | unistd_h-names.c:58-59 |
| H14 | Prototype parameter names don't collide with application macros (`flags`, `flag`) | TESTED (new) | unistd_h-names.c:40-41, unistd_h-files.c:45-46. **dup3/pipe2 used `flags`; fixed** |
| H15 | GETENTROPY_MAX in `<limits.h>`, at least 256 | TESTED (new, picolibc only) | unistd_h-files.c:121-126. **Was missing; fixed** |

### read, pread

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | nbyte 0 returns 0 with no other result | TESTED (new) | read.c:108-109 |
| R2 | Reading starts at the file offset, which advances by the count read | TESTED (new) | read.c:113-114 |
| R3 | No transfer past end-of-file; 0 at or after it | TESTED (new) | read.c:117-120 |
| R4 | Bytes never written (a gap after lseek and write) read as zero | TESTED (new) | read.c:123-129 |
| R5 | Empty pipe with no writer: 0, also with O_NONBLOCK | TESTED (new) | read.c:196-198 |
| R6 | Empty pipe with a writer and O_NONBLOCK: EAGAIN | TESTED (new) | read.c:168 |
| R7 | Empty pipe with a writer, blocking: waits for data | TESTED (new) | read.c:175-183 |
| R8 | O_NONBLOCK has no effect when data is available | TESTED (new) | read.c:169-170 |
| R9 | A short count from a pipe that has fewer bytes than asked | TESTED (new) | read.c:157 |
| R10 | A signal before any data: -1/EINTR | TESTED (new) | read.c:186-192 |
| R11 | A signal after some data: the count read | NOT TESTABLE | A pipe read returns as soon as it has data, so there is no reliable way to interrupt it partway |
| R12 | On a socket, read is recv with no flags (data; EAGAIN/EWOULDBLOCK) | TESTED (new) | read.c:202-207 |
| R13 | ENOTCONN on an unconnected socket | TESTED (new, TCP) | read.c:212-213. Linux gives EINVAL for AF_UNIX stream sockets (cross-cutting.md) |
| R14 | Last data access time marked for update | UNTESTED | relatime/noatime mounts make it unobservable |
| R15 | EBADF: not open, or not open for reading | TESTED (new) | read.c:141-145 |
| R16 | EISDIR for a directory (XSI, when read can't read directories) | TESTED (new) | read.c:151 |
| R17 | EAGAIN for other non-blocking files; ECONNRESET, ETIMEDOUT; EIO (background tty, termios unit) | UNTESTED | Needs particular sockets or devices; kernel |
| R18 | EOVERFLOW at the offset maximum | NOT TESTABLE | off_t is 64 bits on every target, so the offset maximum is the file system's limit |
| R19 | pread reads at the offset without moving the file offset | TESTED (new) | read.c:131-134 |
| R20 | pread: EINVAL for a negative offset on a regular file; offset unchanged | TESTED | read.c:135-136 (new); pread.c (existing) |
| R21 | pread: ESPIPE for a file that can't seek, whatever the offset | TESTED (new; the negative-offset case picolibc only) | read.c:161-164, 208. **Gave EINVAL for a negative offset; fixed** |
| R22 | read and pread are cancellation points, also with bad arguments | TESTED | pthread_testcancel.c:244 (the probe now uses offset -1). **pread returned before the cancellation point; fixed** |

### write, pwrite

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | nbyte 0 on a regular file returns 0 and changes nothing | TESTED (new) | write.c:143-144 |
| W2 | Writes at the offset and advances it; the size becomes the last byte plus one | TESTED (new) | write.c:147-153 |
| W3 | O_APPEND moves the offset to the end before each write | TESTED (new) | write.c:189-193 |
| W4 | Only what fits under the file size limit is written; the next write fails with EFBIG and SIGXFSZ (XSI) | TESTED (new) | write.c:206-230 (child with RLIMIT_FSIZE 20) |
| W5 | EFBIG at or past the maximum file size or offset maximum | TESTED (new) | write.c:176 (pwrite at 2^62) |
| W6 | A signal before any data: EINTR | TESTED (new) | write.c:319-327 (full pipe) |
| W7 | A signal after some data: the count written | NOT TESTABLE | As R11 |
| W8 | Later reads return the data written; writing again overwrites | TESTED (new) | write.c:149-150 |
| W9 | Pipe data is appended in order | TESTED (new) | write.c:241-256 |
| W10 | Pipe writes of PIPE_BUF bytes or fewer aren't interleaved with other writers | TESTED (new) | write.c:283-316 (two processes, 2000 records each) |
| W11 | A blocking pipe write returns nbyte | TESTED (new) | write.c:255 |
| W12 | Non-blocking, PIPE_BUF bytes or fewer: all of it, or EAGAIN with nothing written | TESTED (new) | write.c:259-265 |
| W13 | Non-blocking, more than PIPE_BUF: what fits, and at least PIPE_BUF once the pipe is empty | TESTED (new) | write.c:266-279 |
| W14 | Modification and change times marked for update | TESTED (new, mtime) | write.c:162-164 |
| W15 | On a socket, write is send with no flags | TESTED (new) | write.c:338-339 |
| W16 | EPIPE and SIGPIPE for a pipe with no reader and for a stream socket that is shut down | TESTED (new) | write.c:330-343 |
| W17 | EBADF: not open for writing, or not open | TESTED (new) | write.c:180-184 |
| W18 | ENOSPC, EIO, ENOBUFS, ENXIO, ECONNRESET, ENETDOWN, ENETUNREACH, EACCES | NOT TESTABLE | Needs a full or failing device, or a network |
| W19 | S_ISUID/S_ISGID may be cleared | n/a | "may" |
| W20 | pwrite writes at the offset and leaves the file offset unchanged | TESTED | write.c:168-171 (new); pwrite.c (existing) |
| W21 | pwrite leaves the offset unchanged even with O_APPEND | TESTED (new) | write.c:194-200 |
| W22 | pwrite with O_APPEND writes at the given position | UNTESTED | **Linux appends; not fixed** (needs RWF_NOAPPEND, Linux 6.9) |
| W23 | pwrite: EINVAL for a negative offset; offset unchanged | TESTED (new) | write.c:172-173 |
| W24 | pwrite: ESPIPE for a file that can't seek, whatever the offset | TESTED (new; the negative-offset case picolibc only) | write.c:234-237, 344. **Fixed with R21** |
| W25 | write and pwrite are cancellation points, also with bad arguments | TESTED | pthread_testcancel.c:275 (the probe now uses offset -1). **Fixed with R22** |

### close, posix_close

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | The descriptor is freed, and the lowest-numbered one is reused | TESTED (new) | posix_close.c:85-91 |
| C2 | EBADF for a descriptor that isn't open | TESTED (new) | posix_close.c:92-93, 108 |
| C3 | The process's locks on the file are released | TESTED (new) | lockf.c:224-225 (closing another descriptor for the file) |
| C4 | After the last close of a pipe's write end, readers see end-of-file | TESTED (new) | posix_close.c:96-100 |
| C5 | Interrupted close: EINTR means still open (POSIX_CLOSE_RESTART non-zero); EINPROGRESS means closed | NOT TESTABLE | Needs a file system whose flush can be interrupted (NFS, FUSE). **Linux's EINTR always means closed; close now reports EINPROGRESS** (inferred from fs/open.c, not run) |
| C6 | Never EAGAIN or EWOULDBLOCK | UNTESTED | Inferred: close maps only EINTR |
| C7 | Unlinked file's space freed at last close; memory objects; SO_LINGER sockets; pty manager SIGHUP; aio | UNTESTED | Kernel |
| C8 | posix_close(fd, 0) closes and never returns EINTR | TESTED | posix_close.c:106-108 (new); posix_close.c (existing) |
| C9 | posix_close(fd, POSIX_CLOSE_RESTART) closes | TESTED | posix_close.c:110-112 |
| C10 | An invalid flag may give EINVAL, but the descriptor is closed as for 0 | TESTED (new, picolibc only; glibc has no posix_close) | posix_close.c:116-121. **Left the descriptor open; fixed** |
| C11 | close and posix_close are cancellation points | TESTED | pthread_testcancel.c:482 (posix_close now probed with an invalid flag; close existing). **posix_close skipped it for a bad flag; fixed with C10** |

### dup, dup2, dup3

| # | Requirement | Status | Reference |
|---|---|---|---|
| D1 | dup returns the lowest free descriptor (fcntl F_DUPFD 0) | TESTED (new) | dup2.c:110-111 |
| D2 | dup shares the open file description (offset, status flags); FD_CLOEXEC clear | TESTED (new) | dup2.c:112-116 |
| D3 | dup: EBADF | TESTED (new) | dup2.c:118-119 |
| D4 | dup: EMFILE | TESTED (new) | dup2.c:172-191 (child with a low RLIMIT_NOFILE) |
| D5 | dup2 closes fildes2 first, makes it refer to fildes, and returns fildes2 | TESTED (new) | dup2.c:122-124, 129 |
| D6 | dup2 clears FD_CLOEXEC on fildes2 | TESTED (new) | dup2.c:125, 133-134 |
| D7 | dup2(fd, fd) returns fd without closing it; flags unchanged | TESTED (new) | dup2.c:137-142. **Failed with EINVAL on aarch64; fixed** |
| D8 | dup2 with an invalid fildes doesn't close fildes2 | TESTED (new) | dup2.c:146-149 |
| D9 | dup2/dup3: EBADF for fildes2 < 0 or >= {OPEN_MAX} | TESTED (new, against the kernel's limit) | dup2.c:152-154, 165-166. picolibc's OPEN_MAX is 64 (already on the cross-cutting list) |
| D10 | dup2: the copy shares locks | TESTED (new) | lockf.c:224-225 |
| D11 | If closing fildes2 fails, it is left unchanged; EINTR; EIO | NOT TESTABLE | Linux's dup2 can't fail to close |
| D12 | dup3: EINVAL for equal descriptors | TESTED | dup2.c:157-158 (new); dup3.c (existing) |
| D13 | dup3: FD_CLOEXEC set only from the flag | TESTED | dup2.c:159-161 (new); dup3.c (existing) |
| D14 | dup3: O_CLOFORK | UNTESTED | **No close-on-fork; not fixed** |
| D15 | dup3: an invalid flag may give EINVAL; nothing allocated | TESTED | dup2.c:167-168 (new); dup3.c (existing) |
| D16 | dup3: EBADF; fildes2 left open on failure | TESTED (new) | dup2.c:163-164 |

### pipe, pipe2

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | Two descriptors, the lowest available (XSH 2.6) | TESTED (new) | pipe.c:135-137 |
| P2 | FD_CLOEXEC and O_NONBLOCK clear | TESTED (new) | pipe.c:140-142 |
| P3 | fildes[1] writes and fildes[0] reads, first in, first out | TESTED (new) | pipe.c:143-145 |
| P4 | Owned by the effective uid and gid | TESTED (new) | pipe.c:150 |
| P5 | Access, modification and change times marked | TESTED (new) | pipe.c:151-152 |
| P6 | pipe2 sets O_NONBLOCK and FD_CLOEXEC exactly as asked | TESTED | pipe.c:156-171 (new); pipe2.c (existing) |
| P7 | pipe2: O_CLOFORK | UNTESTED | **No close-on-fork; not fixed** |
| P8 | pipe2: an invalid flag may give EINVAL; nothing allocated, fildes unchanged | TESTED (new) | pipe.c:174-181 |
| P9 | EMFILE with no descriptor free or only one free; fildes unchanged, nothing left open | TESTED (new) | pipe.c:62-90, 184-185 |
| P10 | ENFILE | NOT TESTABLE | Needs the system-wide file limit |

### lseek

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | SEEK_SET, SEEK_CUR and SEEK_END return the new offset | TESTED (new) | lseek.c:94-98 |
| L2 | Seeking past the end is allowed and doesn't extend the file | TESTED (new) | lseek.c:101-102 |
| L3 | After data is written past a gap, the gap reads as zeros | TESTED (new) | read.c:123-129 |
| L4 | SEEK_DATA finds data at or after the offset | TESTED (new) | lseek.c:105-106, 124 |
| L5 | SEEK_HOLE finds a hole, including the virtual hole at the end of the file | TESTED (new) | lseek.c:107-108, 122 |
| L6 | ENXIO for SEEK_DATA/SEEK_HOLE at or past the size; offset unchanged | TESTED (new) | lseek.c:110-114, 125 |
| L7 | EINVAL for a bad whence or a negative result (file and directory); offset unchanged | TESTED (new) | lseek.c:128-133, 143 |
| L8 | EBADF | TESTED (new) | lseek.c:137-138 |
| L9 | ESPIPE for pipes, FIFOs and sockets | TESTED (new) | lseek.c:148-157 |
| L10 | EOVERFLOW | NOT TESTABLE | off_t is 64 bits everywhere, and ext4 caps lseek at 16 TiB |

### fsync, fdatasync, sync

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | fsync and fdatasync succeed on a written file | TESTED (existing) | fsync.c:74, fdatasync.c:74 |
| S2 | EBADF | TESTED (new) | fsync.c:79, fdatasync.c:79 |
| S3 | EINVAL for a file that can't be synchronized | TESTED (new) | fsync.c:84-87, fdatasync.c:84-87 (a pipe: success or EINVAL; Linux gives EINVAL) |
| S4 | EINTR, EIO, errors from queued I/O; the durability itself | NOT TESTABLE | Needs a failing device; storage can't be observed |
| S5 | fsync and fdatasync are cancellation points | TESTED (existing) | pthread_testcancel.c:151-158 |
| S6 | sync schedules the write-out and returns nothing | TESTED (existing) | sync.c (calls it); the effect can't be observed |

### ftruncate, truncate

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | Shrinking removes the data past the length | TESTED (new) | ftruncate.c:129-132, 148 |
| T2 | Growing fills the new part with zeros | TESTED (new) | ftruncate.c:135-140, 149-150 |
| T3 | The file offset, and other open offsets, are unchanged | TESTED (new) | ftruncate.c:131, 151 |
| T4 | Modification and change times marked | TESTED (new) | ftruncate.c:143-145, 152-154 |
| T5 | Past the file size limit: EFBIG and SIGXFSZ; file unchanged | TESTED (new) | ftruncate.c:188-215 |
| T6 | EINVAL for a negative length; file unchanged | TESTED (new) | ftruncate.c:157-159 |
| T7 | EFBIG or EINVAL past the maximum file size | TESTED (new) | ftruncate.c:162-168 |
| T8 | ftruncate: EBADF or EINVAL for a descriptor not open for writing | TESTED (new) | ftruncate.c:171-178 |
| T9 | EINVAL for a pipe (ftruncate) or a FIFO (truncate) | TESTED (new) | ftruncate.c:181-185 |
| T10 | truncate: EISDIR; ENOENT (also for an empty path); ENOTDIR (prefix, trailing slash); ELOOP; ENAMETOOLONG | TESTED (new) | ftruncate.c:218-229 |
| T11 | truncate: EACCES without write or search permission | TESTED (new, unprivileged) | ftruncate.c:232-238 |
| T12 | truncate follows symbolic links | TESTED (new) | ftruncate.c:226-227 |
| T13 | Sizes past 32 bits | TESTED (existing) | truncate.c |
| T14 | Shared memory objects; mapped pages discarded, with SIGBUS | UNTESTED | memory-sched area |
| T15 | EINTR, EIO, EROFS | NOT TESTABLE | Needs a failing or read-only file system |

### link, linkat, symlink, symlinkat

| # | Requirement | Status | Reference |
|---|---|---|---|
| K1 | link adds a name for the same file and raises the link count by one | TESTED | symlink.c:190-192 (new); link.c (existing) |
| K2 | link of a symbolic link is implementation-defined; Linux links the link itself | TESTED (new) | symlink.c:194-195 |
| K3 | EPERM for a directory | TESTED (new) | symlink.c:210 |
| K4 | On failure the link count is unchanged and nothing is created | TESTED (new) | symlink.c:213 |
| K5 | EEXIST for any existing path2, symbolic links included | TESTED | symlink.c:198-200 (new); link.c (existing) |
| K6 | ENOENT: missing path1, missing prefix, empty path1 or path2 | TESTED | symlink.c:201-204 (new); link.c (existing) |
| K7 | ENOTDIR: a prefix, or a trailing slash on path1; ENOENT or ENOTDIR for a trailing slash on path2 | TESTED (new) | symlink.c:205-209 |
| K8 | ELOOP, ENAMETOOLONG | TESTED (new) | symlink.c:211-212 |
| K9 | EACCES without write permission in the directory | TESTED (new, unprivileged) | symlink.c:173 |
| K10 | EXDEV across file systems | TESTED (new) | symlink.c:216-226 (build directory against /dev/shm) |
| K11 | Times of the file and the directory marked | UNTESTED | Change time only; low value |
| K12 | EMLINK, ENOSPC, EROFS, EILSEQ | NOT TESTABLE | ext4 has no practical link limit; needs a full or read-only file system; Linux accepts any name bytes |
| K13 | linkat resolves each path relative to its descriptor; AT_FDCWD | TESTED | symlink.c:229 (new); linkat.c (existing) |
| K14 | linkat without a flag links the symbolic link; AT_SYMLINK_FOLLOW links its target | TESTED | symlink.c:230-234 (new); linkat.c (existing) |
| K15 | linkat: EBADF for either descriptor; ENOTDIR for a file descriptor; absolute paths ignore the descriptors | TESTED (new) | symlink.c:236-242 |
| K16 | linkat: EINVAL for a bad flag (may) | TESTED | symlink.c:243 (new); linkat.c (existing) |
| K17 | O_SEARCH descriptors skip the search check | NOT TESTABLE | O_SEARCH is O_PATH, and Linux still checks (files unit) |
| K18 | symlink stores its contents as given, without validating them | TESTED (new) | symlink.c:126-128 |
| K19 | The link is owned by the euid; its group is the process's or the directory's | TESTED (new) | symlink.c:131-133 |
| K20 | symlink marks the directory's times | TESTED (new) | symlink.c:125, 129 |
| K21 | symlink: EEXIST for an existing path2 (file, link, dangling link, directory), which is unaffected | TESTED | symlink.c:136-141 (new); symlinkat.c (existing) |
| K22 | symlink: ENOENT (empty path2, missing prefix); ENOTDIR; ENOENT or ENOTDIR for a trailing slash; ELOOP; ENAMETOOLONG for a component | TESTED (new) | symlink.c:144-155 |
| K23 | symlink: ENAMETOOLONG for contents longer than {SYMLINK_MAX}; contents up to that length are kept | TESTED (new) | symlink.c:158-167. **pathconf said 255 while Linux takes 4095; fixed** |
| K24 | symlink: EACCES without write or search permission | TESTED (new, unprivileged) | symlink.c:170-176 |
| K25 | symlinkat: relative to the descriptor; AT_FDCWD; EBADF; ENOTDIR | TESTED (new) | symlink.c:181-187 |
| K26 | symlink: EIO, ENOSPC, EROFS, EILSEQ | NOT TESTABLE | As K12 |
| K27 | An empty path1 is stored like any other string | UNTESTED | **Linux gives ENOENT; can't be fixed in the library** |

### readlink, readlinkat

| # | Requirement | Status | Reference |
|---|---|---|---|
| RL1 | Places the contents, without adding a null, and returns the count | TESTED | readlink.c:110-114 (new); readlinkat.c (existing) |
| RL2 | A short buffer gets the first bufsize bytes | TESTED | readlink.c:117-118 (new); readlinkat.c (existing) |
| RL3 | With bufsize 0, places 0 bytes and returns 0 | TESTED (new, picolibc only) | readlink.c:120-123, 148. **Linux's EINVAL was passed through; fixed** |
| RL4 | On error, returns -1 and leaves the buffer unchanged | TESTED (new) | readlink.c:74-77 (applies to every error check) |
| RL5 | EINVAL for a file that isn't a symbolic link | TESTED | readlink.c:126-127 (new); readlinkat.c (existing) |
| RL6 | ENOENT (missing, empty path); ENOTDIR (prefix, trailing slash); ELOOP; ENAMETOOLONG | TESTED (new) | readlink.c:128-138 |
| RL7 | EACCES when a prefix lacks search permission | TESTED (new, unprivileged) | readlink.c:156-158 |
| RL8 | readlinkat: relative to the descriptor; AT_FDCWD; EBADF; ENOTDIR; absolute paths | TESTED (new) | readlink.c:143-153 |
| RL9 | Last access time marked | UNTESTED | relatime |
| RL10 | EIO | NOT TESTABLE | Needs a failing device |

### unlink, unlinkat, rmdir

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | unlink removes the name and lowers the link count | TESTED (new) | unlink.c:122-123 |
| U2 | A symbolic link is removed, not its target | TESTED (new) | unlink.c:127 |
| U3 | An open file stays usable after its last link is removed | TESTED (new) | unlink.c:130-133 |
| U4 | Parent directory times marked | TESTED (new) | unlink.c:121, 124 |
| U5 | unlink of a directory: EPERM, and the directory stays | TESTED (new, picolibc only; glibc gives EISDIR) | unlink.c:137-140. **Linux's EISDIR was passed through; fixed** |
| U6 | ENOENT (missing, empty path); ENOTDIR (prefix, trailing slash); ELOOP; ENAMETOOLONG; file kept | TESTED (new) | unlink.c:144-152 |
| U7 | EACCES without write permission in the directory or search permission on the prefix | TESTED (new, unprivileged) | unlink.c:156-163 |
| U8 | EPERM or EACCES for another user's file in a sticky directory (XSI) | NOT TESTABLE | Needs a file owned by another user in a directory the test can write |
| U9 | EBUSY, EROFS, ETXTBSY | NOT TESTABLE | Kernel; needs a read-only file system |
| U10 | unlinkat: relative to the descriptor; AT_FDCWD | TESTED | unlink.c:175, 185 (new); unlinkat.c (existing) |
| U11 | unlinkat with AT_REMOVEDIR removes a directory | TESTED | unlink.c:176 (new); unlinkat.c (existing) |
| U12 | AT_REMOVEDIR: EEXIST or ENOTEMPTY; ENOTDIR for a file | TESTED | unlink.c:171-174 (new); unlinkat.c (existing) |
| U13 | unlinkat of a directory without AT_REMOVEDIR: EPERM | TESTED (new, picolibc only) | unlink.c:139, 170. **Fixed with U5** |
| U14 | unlinkat: EBADF; ENOTDIR for a file descriptor; absolute paths ignore the descriptor | TESTED (new) | unlink.c:177-184 |
| U15 | unlinkat: EINVAL for a bad flag (may) | TESTED (existing) | unlinkat.c |
| U16 | rmdir removes an empty directory and marks the parent's times | TESTED (new) | unlink.c:207-212 |
| U17 | rmdir: a final dot gives EINVAL; dot-dot fails | TESTED (new) | unlink.c:189-192 |
| U18 | rmdir: EEXIST or ENOTEMPTY | TESTED (new) | unlink.c:193-195 |
| U19 | rmdir of a symbolic link: ENOTDIR, and the directory stays | TESTED (new) | unlink.c:197-199 |
| U20 | rmdir: ENOTDIR for a file; ENOENT for a missing name or an empty path | TESTED (new) | unlink.c:200-203 |
| U21 | An open directory is removed, and no new entries can be made in it | TESTED (new) | unlink.c:207-216 |
| U22 | rmdir of the root or a working directory: success or EBUSY | n/a | Unspecified |

### chdir, fchdir, getcwd

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | chdir changes the working directory | TESTED (new) | getcwd.c:94-99 |
| G2 | chdir: ENOENT (missing, empty path); ENOTDIR (file, prefix); ELOOP; ENAMETOOLONG; working directory unchanged | TESTED (new) | getcwd.c:124-133 |
| G3 | chdir: EACCES | TESTED (new, unprivileged) | getcwd.c:134-137 |
| G4 | fchdir to a directory descriptor | TESTED | getcwd.c:142-144 (new); fchdir.c (existing) |
| G5 | fchdir: ENOTDIR, EBADF, EACCES; working directory unchanged | TESTED (new; EACCES unprivileged) | getcwd.c:147-164 |
| G6 | getcwd gives an absolute name and returns buf | TESTED (new) | getcwd.c:95-99, 103 |
| G7 | No dot, dot-dot or symbolic-link components; one leading slash and no extra slashes | TESTED (new) | getcwd.c:101-109 |
| G8 | An exact-size buffer works; ERANGE for a size above 0 that is too small | TESTED (new) | getcwd.c:112-118 |
| G9 | EINVAL for a size of 0 | TESTED (new) | getcwd.c:119-121. **Linux's ERANGE was passed through; fixed** |
| G10 | Always an absolute pathname, even for a directory outside the root (Linux returns "(unreachable)/...") | NOT TESTABLE | Needs chroot or another mount namespace. **Now ENOENT** (inferred; glibc does the same) |
| G11 | EACCES, ENOMEM (may) | UNTESTED | Linux's getcwd doesn't need search permission |

### access, faccessat

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | F_OK tests only whether the file exists | TESTED | faccessat.c:107-108 (new); access.c (existing) |
| A2 | R_OK, W_OK and X_OK are each checked, and a combination needs all of them | TESTED (new; denials unprivileged) | faccessat.c:111-122 |
| A3 | A privileged X_OK may succeed with no execute bit set | TESTED (new) | faccessat.c:124 (only enforced when unprivileged) |
| A4 | Checks use the real IDs; AT_EACCESS uses the effective IDs | TESTED (new, with equal IDs) | faccessat.c:153, 156. NOT TESTABLE with differing IDs (needs a set-ID program) |
| A5 | ENOENT (empty path); ENOTDIR (prefix, trailing slash); ELOOP; ENAMETOOLONG | TESTED (new) | faccessat.c:127-133 |
| A6 | EACCES when a prefix lacks search permission | TESTED (new, unprivileged) | faccessat.c:136-139 |
| A7 | EINVAL for an invalid amode (may) | TESTED (new) | faccessat.c:143-146 |
| A8 | EROFS, ETXTBSY | NOT TESTABLE | Needs a read-only file system; Linux's access doesn't report ETXTBSY |
| A9 | faccessat: relative to the descriptor; AT_FDCWD behaves as access | TESTED | faccessat.c:150-152 (new); faccessat.c (existing) |
| A10 | faccessat: EACCES through a descriptor without search permission | TESTED (new, unprivileged) | faccessat.c:157-159 |
| A11 | faccessat: EBADF; ENOTDIR; absolute paths ignore the descriptor | TESTED (new) | faccessat.c:161-165 |
| A12 | faccessat: EINVAL for a bad flag (may) | TESTED | faccessat.c:167 (new); existing |

### chown, fchown, lchown, fchownat

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | The owner may set its own uid and group; -1 leaves an ID unchanged | TESTED | chown.c:157-160 (new); chown.c, fchown.c (existing) |
| O2 | The group may change to one of the process's supplementary groups | TESTED (new) | chown.c:167-170 |
| O3 | Change time marked | TESTED (new) | chown.c:163-164 |
| O4 | chown follows links; lchown and AT_SYMLINK_NOFOLLOW change the link itself | TESTED (new) | chown.c:172-183 |
| O5 | An unprivileged chown of an executable regular file clears S_ISUID and S_ISGID | TESTED (new) | chown.c:187-196 |
| O6 | EPERM for another uid, or a group the process isn't in; nothing changes | TESTED (new, unprivileged) | chown.c:199-217 |
| O7 | EPERM on a file owned by someone else | TESTED (new, with a real group change) | chown.c:211-215. Linux lets chown(path, -1, -1) succeed on such a file (not fixed) |
| O8 | Privileged changes of the uid | NOT TESTABLE | Needs privilege |
| O9 | fchown: EBADF for a descriptor that isn't open, including AT_FDCWD's value | TESTED | chown.c:221-223 (new); fchown.c (existing, -1). **fchown(-2) changed the working directory's owner; fixed** |
| O10 | ENOENT (missing, empty path); ENOTDIR (prefix, trailing slash); ELOOP; ENAMETOOLONG; lchown doesn't follow links | TESTED (new) | chown.c:226-236 |
| O11 | EACCES when a prefix lacks search permission | TESTED (new, unprivileged) | chown.c:238-242 |
| O12 | fchownat: relative to the descriptor; EBADF; ENOTDIR; EINVAL for a bad flag; EACCES through the descriptor | TESTED | chown.c:246-256 (new); fchownat.c (existing) |
| O13 | EINVAL for an unsupported ID; EROFS, EIO, EINTR | NOT TESTABLE | Linux takes every ID except -1 in the initial namespace; needs a read-only or failing file system |

### lockf (XSI)

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | F_LOCK and F_TLOCK lock size bytes from the offset; other processes see the lock, the owner doesn't | TESTED | lockf.c:171-177 (new); lockf.c (existing basics) |
| F2 | The file offset isn't moved | TESTED (new) | lockf.c:173 |
| F3 | Adjacent and overlapping sections merge; unlocking the middle leaves two | TESTED (new) | lockf.c:180-187 |
| F4 | A negative size locks the bytes before the offset | TESTED (new) | lockf.c:190-193 |
| F5 | Size 0 locks through any future end of the file | TESTED (new) | lockf.c:196-198 |
| F6 | F_ULOCK up to the maximum offset trims a size-0 lock | TESTED (new) | lockf.c:201-204 |
| F7 | F_TEST detects another process's lock: EACCES or EAGAIN | TESTED | lockf.c:72-90 (new); lockf.c (existing) |
| F8 | F_TLOCK fails with EACCES or EAGAIN when another process holds the section | TESTED (new) | lockf.c:86-88 |
| F9 | F_LOCK blocks, and a signal interrupts it with EINTR | TESTED (new) | lockf.c:247-273 |
| F10 | EDEADLK when two processes wait for each other | TESTED (new) | lockf.c:227-246 (exactly one of the two gets EDEADLK) |
| F11 | Locks are released on the first close of any descriptor for the file | TESTED (new) | lockf.c:224-225 |
| F12 | Locks are removed when the process exits | TESTED (new) | lockf.c:187, 225 (the children's locks are gone after they exit) |
| F13 | EINVAL for a bad function, or when size plus offset is below 0 | TESTED | lockf.c:209-211 (new); lockf.c (existing, bad function) |
| F14 | EOVERFLOW for a section past the maximum offset | TESTED (new) | lockf.c:212-213 |
| F15 | Existing locks are unchanged on failure | TESTED (new) | lockf.c:214 |
| F16 | EBADF for a descriptor that isn't open, or for F_LOCK/F_TLOCK on one not open for writing; F_TEST is allowed there | TESTED (new) | lockf.c:215-222 |
| F17 | ENOLCK; EAGAIN for mapped files (may); EOPNOTSUPP | NOT TESTABLE | Needs the system lock limit; Linux doesn't refuse mapped files |
| F18 | lockf (F_LOCK) may be a cancellation point | TESTED (existing) | pthread_testcancel.c:163 |

### getentropy

| # | Requirement | Status | Reference |
|---|---|---|---|
| E1 | Writes exactly length bytes, up to GETENTROPY_MAX | TESTED (new) | getentropy.c:79-89 |
| E2 | A zero length writes nothing | TESTED (new) | getentropy.c:75-77 |
| E3 | The data is unpredictable | TESTED (new, weakly) | getentropy.c:84-86 (two calls differ) |
| E4 | EINVAL past GETENTROPY_MAX, with nothing written | TESTED (new; EINVAL picolibc only, glibc gives EIO) | getentropy.c:91-99. **Looped for any length; fixed** |
| E5 | ENOSYS without an entropy source (may) | NOT TESTABLE | Linux always has getrandom; dummyhost returns ENOSYS (read, not run) |

### Counts

248 requirements: 210 TESTED, 14 UNTESTED, 22 NOT TESTABLE and 2 n/a. Before this unit about 37 of them had a test (the rows marked "existing", mostly success paths); the other 173 TESTED rows are new. Eleven fixes were made. All but close's EINTR, which can't be produced here, were proven by running. Five deviations are left (below).

## Deviations found and fixed

Each fix except the first was verified on aarch64-linux-gnu. Only its library change was reversed (`git show <fix> -- libc libos | git apply -R`); then the build was redone, the named test run, and the change reapplied. The failing check is quoted for each.

1. **close reported EINTR after the descriptor was gone** (libos/linux/close.c).
   - **Cause:** Linux clears the descriptor before the flush that can be interrupted, and then maps ERESTART* to EINTR "because file table entry was cleared" (v5.10 fs/open.c).
   - **Spec:** with POSIX_CLOSE_RESTART defined as 1, an EINTR from close means "fildes remaining open"; EINPROGRESS means it is closed.
   - **Fix:** close now reports EINPROGRESS instead of EINTR.
   - **Not run:** producing it needs a file system whose flush can be interrupted (NFS, FUSE), and picolibc has no prctl, so a test can't install a seccomp filter instead.

2. **posix_close left the descriptor open for an invalid flag** (libc/posix/unistd/posix_close.c).
   - **Spec:** "If flag is invalid, posix_close() may fail with errno set to [EINVAL], but shall otherwise behave as if flag had been 0 and close fildes."
   - **Cause:** it returned EINVAL before closing, which also skipped the cancellation point.
   - **Fix:** it now closes the descriptor, then reports EINVAL. The old test-posix_close expected the old behaviour and is updated in the same commit.
   - **Failing-before:** `closed despite the invalid flag: fcntl(fd, F_GETFD) == -1 && errno == EBADF`, and in test-pthread_testcancel `posix_close is not a cancellation point`.

3. **dup2(fd, fd) failed with EINVAL on aarch64** (libos/linux/dup2.c).
   - **Cause:** targets without a dup2 system call use dup3, which rejects equal descriptors.
   - **Spec:** "unless fildes is equal to fildes2 in which case dup2() shall return fildes2 without closing it".
   - **Fix:** for equal descriptors it checks with F_GETFD and returns the descriptor, or EBADF if it isn't open.
   - **Failing-before:** `dup2 of a descriptor to itself: dup2(50, 50) == 50 (errno 22)`. arm has the system call and passed before.

4. **pread and pwrite checked a negative offset before the cancellation point, and gave the wrong error for pipes** (libos/linux/pread.c, pwrite.c).
   - **Cause:** the early EINVAL return skipped a pending cancellation, against the net-sockets rule.
   - **Spec:** EINVAL is only for "a regular file or block special file"; "[ESPIPE] The file is incapable of seeking" applies to pipes, FIFOs and sockets whatever the offset.
   - **Fix:** the check now comes after `_cancel_enter`, and the error is lseek's when lseek fails (ESPIPE, EBADF).
   - **Failing-before:** `pread on a pipe with a negative offset: ... errno == (29) (errno 22)`, the same for pwrite, and in test-pthread_testcancel `pread is not a cancellation point`.

5. **readlink and readlinkat failed for a zero buffer size** (libos/linux/readlink.c, readlinkat.c).
   - **Cause:** Linux returns EINVAL for `bufsiz <= 0` before looking up the path.
   - **Spec:** "the first bufsize bytes shall be placed in buf", and the count is returned. EINVAL is only for a file that isn't a symbolic link.
   - **Fix:** a zero size now reads into a one-byte scratch buffer and returns 0, or the real error. readlink now calls readlinkat.
   - **Failing-before:** `zero-size buffer: readlink("link", buf, 0) == 0 && buf[0] == '#' (errno 22)`.

6. **unlink and unlinkat gave EISDIR for a directory** (libos/linux/unlinkat.c, unlink.c).
   - **Spec:** "[EPERM] The file named by path is a directory, and either the calling process does not have appropriate privileges or the implementation prohibits using unlink() on directories". The same applies to unlinkat without AT_REMOVEDIR.
   - **Fix:** EISDIR now becomes EPERM. In v5.10 every EISDIR from do_unlinkat or may_delete means the target is a directory (or ".", ".." or "/"). unlink now calls unlinkat.
   - **Failing-before:** `unlink a directory: ... errno == (1) (errno 21)`.

7. **getcwd gave ERANGE for a zero size** (libos/linux/getcwd.c).
   - **Spec:** "[EINVAL] The size argument is 0."
   - **Fix:** the size is now checked before the system call.
   - **Also in this commit:** a result that doesn't start with `/` (Linux's "(unreachable)" prefix for a directory outside the root) now gives ENOENT, since POSIX requires an absolute pathname. That part can't be run here (G10).
   - **Failing-before:** `a zero size: ret == NULL && errno == EINVAL (errno 34)`.

8. **fchown(-2, ...) changed the owner of the working directory** (libos/linux/fchown.c).
   - **Cause:** it called fchownat(fd, "", ..., AT_EMPTY_PATH), and fchownat turns picolibc's AT_FDCWD (-2) into Linux's, so the empty path named the working directory.
   - **Spec:** "[EBADF] The fildes argument is not an open file descriptor."
   - **Fix:** negative descriptors now give EBADF, and the system call is made directly.
   - **Failing-before:** `fchown(AT_FDCWD): ... fchown(-2, (uid_t)-1, (gid_t)-1)) == -1 && errno == (9) (errno 0)`.
   - fstat and fexecve have the same flaw with Linux's -100 (cross-cutting.md).

9. **getentropy had no GETENTROPY_MAX and accepted any length** (libos/linux/getentropy.c, libos/dummyhost/getentropy.c, libc/include/sys/syslimits.h).
   - **Spec:** POSIX.1-2024 adds GETENTROPY_MAX to `<limits.h>` (minimum 256), and getentropy "shall fail if: [EINVAL] The value of length is greater than {GETENTROPY_MAX}".
   - **Fix:** GETENTROPY_MAX is now 256, visible where NSIG_MAX is, and getentropy checks it first.
   - **Failing-before:** `#error GETENTROPY_MAX` from the header test, and, with only the libos part reversed, `GETENTROPY_MAX + 1 fails: getentropy(buf, GETENTROPY_MAX + 1) == -1 (errno 0)`.

10. **`<unistd.h>` didn't define O_CLOEXEC, and dup3/pipe2 used a plain parameter name** (libc/include/sys/unistd.h).
    - **Spec:** "The <unistd.h> header shall define the symbolic constants O_CLOEXEC and O_CLOFORK as described in <fcntl.h>."
    - **Fix:** O_CLOEXEC is defined for `_POSIX_C_SOURCE >= 202405`, with the same tokens as sys/_default_fcntl.h, so the two headers can be included in either order. The parameter `flags`, reported by unistd-process, is now `__flag`.
    - **Failing-before (compile):** `test-unistd_h-names.c:40: expected ';', ',' or ')' before '+' token` and `#error O_CLOEXEC missing from <unistd.h>`.

11. **pathconf(_PC_SYMLINK_MAX) reported 255 on Linux** (libos/linux/local-statfs.h).
    - **Cause:** it fell through to the fallback's `_POSIX_SYMLINK_MAX`, but Linux stores link contents up to its PATH_MAX (4096 bytes including the null; do_symlinkat via getname). So a 300-byte link worked, though symlink "shall fail [ENAMETOOLONG] ... the length of the path1 argument is longer than {SYMLINK_MAX}".
    - **Fix:** it now returns 4095. pathconf belongs to unistd-config; it was changed only because symlink depends on it, and that is recorded in cross-cutting.md.
    - **Failing-before:** `contents longer than SYMLINK_MAX: ... errno == (91) (errno 0)` (the 256-byte link was created).

## Deviations not fixed

- **pwrite on an O_APPEND descriptor writes at the end of the file** (W22).
  - POSIX: pwrite "writes into a given position and does not change the file offset (regardless of whether O_APPEND is set)".
  - Linux only offers RWF_NOAPPEND for pwritev2, from 6.9. On 5.10 the library would have to clear O_APPEND around the call, which races with other threads.
  - The offset part does hold, and is checked.
- **O_CLOFORK is missing from `<unistd.h>`, dup3 and pipe2** (H9, D14, P7). picolibc has no close-on-fork yet; see the files unit's cross-cutting entry.
- **symlink("", path2) fails with ENOENT** (K27), because Linux rejects empty contents.
- **chown(path, -1, -1) succeeds on another user's file** (O7).
  - POSIX's EPERM, "The effective user ID does not match the owner of the file", reads as applying here.
  - But nothing changes, glibc behaves the same, and a library check would need a stat that races with the chown.
- **read on an unconnected AF_UNIX stream socket gives EINVAL** (R13), as recv does. This is net-sockets' area (cross-cutting.md).

## Other observations (not changed)

- Left to unistd-config (cross-cutting.md):
  - `_POSIX_CHOWN_RESTRICTED`, `_POSIX_NO_TRUNC` and `_POSIX_VDISABLE` aren't defined.
  - `NGROUPS_MAX` isn't in `<limits.h>`, and `_POSIX_NGROUPS_MAX` is 0 where POSIX.1-2024 says 8.
  - `optreset` is declared in strict mode.
- lockf's F_TEST probes with a read lock, so it doesn't report another process's fcntl read lock. POSIX leaves the interaction between lockf and fcntl locks unspecified, and glibc and musl do the same.
- `<stdio.h>`'s FILE members (`flags`, `unget`, `put`, ...) have plain names, so the header tests `#undef` their macros before including `<stdio.h>`. This is the stdio unit's area (cross-cutting.md).
- test-ftruncate had been skipped on every Linux configuration, left over from the old fixed-name test-ftrunc. It is rewritten to use a temporary directory and now runs.
- glibc 2.43 differences met here, guarded with `__GLIBC__`:
  - negative pread/pwrite offsets on a pipe give EINVAL;
  - unlink of a directory gives EISDIR;
  - a zero readlink size gives EINVAL;
  - getentropy past 256 bytes gives EIO, and GETENTROPY_MAX isn't defined;
  - there is no posix_close, and `<unistd.h>` doesn't define O_CLOEXEC;
  - dup3, pipe2, getentropy, SEEK_DATA and SEEK_HOLE need `_GNU_SOURCE`, which the tests set only for TEST_GLIBC.

## Left undone

- The UNTESTED kernel rows (R14, R17, C7, T14, K11, RL9, G11), and the not-testable error rows that need failing, full or read-only file systems.
- Rows that need a set-ID program, root or a second user: differing real and effective IDs (A4), privileged chown (O8) and removal in a sticky directory (U8).

## Test runs (verified by running)

| Target | Tests | Ok | Fail | Expected Fail | Skipped |
|---|---|---|---|---|---|
| aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`), full `meson test -t 4` at the branch tip | 1591 | 1543 | 0 | 6 | 42 |
| arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`), full `meson test -t 4` at the branch tip | 1592 | 1542 | 0 | 6 | 44 |

- Every new and changed test passes on both targets, for picolibc and as `-native` against glibc.
- **Native configuration** (`scripts/do-native-configure -Dwerror=true`, ninja only): builds cleanly.
- **CMake** (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 599 tests, 100% passed (260 ran, 339 skipped). test-unistd_h-files and test-unistd_h-names run there; the behaviour tests skip.
- **clang:** `/tmp/arm64run/clang-check builds/build-aarch64-linux-gnu <the 33 changed .c files>` gave 53 compile entries (picolibc and -native), all OK, with no FAIL.
- **clang-format:** `clang-format --dry-run --Werror` is clean on every file the branch touches.
- **Fix proofs** (aarch64, library change reversed): quoted with each deviation above.
- **Not run here:** the bare-metal meson targets; the x86_64 host runs them.
- **Incidents:** no host security software kills and no timeouts. Each signal check uses a single alarm(), never repeated signals. The EXDEV check creates nothing in /dev/shm unless link unexpectedly succeeds, and then removes it at once.

## Commits (audit/unistd-files, on top of d14119a43a)

- 43ef91ce87 libos/linux: Report EINPROGRESS from an interrupted close
- f7068c61f4 unistd: Close the descriptor in posix_close when the flag is invalid
- 6ffe404654 libos/linux: Return the descriptor from dup2 when both are equal
- f61e9baefe libos/linux: Check pread and pwrite offsets inside the cancellation point
- 5f2b248a6f libos/linux: Accept a zero buffer size in readlink and readlinkat
- 9f139a2d19 libos/linux: Report EPERM from unlink and unlinkat for a directory
- 2444a9d7b6 libos/linux: Report EINVAL from getcwd for a zero size
- dd6942d976 libos/linux: Fail with EBADF from fchown for negative descriptors
- a853328062 getentropy: Define GETENTROPY_MAX and reject longer requests
- 75c97a11de unistd.h: Define O_CLOEXEC and use a reserved name in dup3 and pipe2
- e9ccccb492 libos/linux: Report the kernel's symbolic link length in pathconf
- b6b0a9e8e9 test-posix: Check offsets, pipes, signals, sockets and errors in read and pread
- cf21eeee24 test-posix: Check O_APPEND, the file size limit, pipes and errors in write and pwrite
- 79d496a648 test-posix: Check each whence, holes and errors in lseek
- e48aa88729 test-posix: Check equal descriptors, FD_CLOEXEC and errors in dup, dup2 and dup3
- 354251cb6d test-posix: Check flags, ownership and EMFILE in pipe and pipe2
- 31ac278140 test-posix: Check descriptor reuse and invalid flags in close and posix_close
- 69ea8f88f0 test-posix: Check growth, offsets, the file size limit and errors in ftruncate and truncate
- 16c2bbbc02 test-posix: Check EBADF and pipes in fsync and fdatasync
- 0e3f532b87 test-posix: Check links, open files, directories and errors in unlink, unlinkat and rmdir
- e240398bbf test-posix: Check contents, SYMLINK_MAX and errors in symlink, symlinkat, link and linkat
- a3d86bc1a3 test-posix: Check truncation, zero sizes and untouched buffers in readlink and readlinkat
- 4a272d6035 test-posix: Check names, sizes and errors in getcwd, chdir and fchdir
- 5a79ecdf3c test-posix: Check each permission, AT_EACCESS and errors in access and faccessat
- 5ec643b098 test-posix: Check groups, set-ID bits, links and errors in chown, fchown, lchown and fchownat
- bbedcfd398 test-posix: Check sections, deadlock, signals and errors in lockf
- f51b3757c4 test-posix: Check lengths and GETENTROPY_MAX in getentropy
- d6de0d9c9a test-posix: Check the file declarations and constants in `<unistd.h>`