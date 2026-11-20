# `<sys/stat.h>`, `<fcntl.h>` and `<sys/statvfs.h>` audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-files`, branch `audit/files`, based on
4af0c34b98 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10;
builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43) as an
unprivileged user.

Implementation: `libos/linux/` (stat.c, lstat.c, fstat.c, fstatat.c, _statbuf.c,
chmod.c, fchmod.c, fchmodat.c, mkdir.c, mkdirat.c, mkfifo.c, mkfifoat.c, mknod.c,
mknodat.c, umask.c, futimens.c, utimensat.c, open.c, openat.c, fcntl.c,
posix_fadvise.c, posix_fallocate.c, statvfs.c, fstatvfs.c, _statvfsbuf.c,
_open_flags_to_linux.c, _open_flags_from_linux.c and the `_at_fd_to_linux` and
`_at_flags_to_linux` helpers in local-linux.h); `libc/posix/fcntl/creat.c`; headers
`libc/include/sys/stat.h`, `libc/include/fcntl.h` → `sys/fcntl.h` →
`sys/_default_fcntl.h`, and `libc/include/sys/statvfs.h`.
Tests: `test/test-posix/test-fcntl/`, `test-sys-stat/` and `test-sys-statvfs/`
(file names below are in those directories). Line numbers are for the branch tip.

Spec source: the pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/fcntl.h,
sys_stat.h and sys_statvfs.h, and functions/stat (covers lstat), fstat, fstatat,
chmod, fchmod (fchmodat is on the chmod page), mkdir (mkdirat), mkfifo (mkfifoat),
mknod (mknodat), umask, futimens (utimensat), open (openat), creat, fcntl,
posix_fadvise, posix_fallocate and fstatvfs (statvfs). All 24 interfaces are in
Issue 8; S_TYPEISTMO belongs to the Typed Memory Objects option, which picolibc
doesn't claim, so it isn't required.

Status key: TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row
was run and passes for picolibc (`test-X`) and against glibc (`test-X-native`) on
aarch64-linux-gnu and arm-linux-gnueabihf, unless it says picolibc only.
Permission-denial checks are skipped when the euid is 0. Every new test works in a
fresh `mkdtemp()` directory (in /tmp, or the build directory for the older tests)
and removes it.

## Requirement table

### `<fcntl.h>` (basedefs/fcntl.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | struct f_owner_ex with `int type` and `pid_t pid` | TESTED (picolibc only; glibc needs _GNU_SOURCE) | test-fcntl_h.c:108. **Was missing; fixed** |
| F2 | struct flock members and types | TESTED | test-fcntl_h.c:73 |
| F3 | mode_t, off_t and pid_t | TESTED | test-fcntl_h.c:69 |
| F4 | The cmd values F_DUPFD … F_SETOWN_EX are unique and usable in #if | TESTED (the OFD and _EX ones picolibc only) | test-fcntl_h.c:80-91, 245. **F_GETOWN_EX and F_SETOWN_EX were missing; fixed.** F_DUPFD_CLOFORK is missing (deviation, not fixed) |
| F5 | FD_CLOEXEC and FD_CLOFORK, bitwise-distinct, usable in #if | FD_CLOEXEC TESTED (test-fcntl_h.c:114); FD_CLOFORK UNTESTED | **FD_CLOFORK missing; deviation, not fixed** |
| F6 | F_RDLCK, F_UNLCK, F_WRLCK unique, usable in #if | TESTED | test-fcntl_h.c:119 |
| F7 | F_OWNER_PID and F_OWNER_PGRP unique | TESTED (picolibc only) | test-fcntl_h.c:110. **Were missing; fixed** |
| F8 | SEEK_SET, SEEK_CUR and SEEK_END defined by `<fcntl.h>` itself | TESTED | test-fcntl_h.c:40-42, 124. **Were missing; fixed** |
| F9 | O_CLOEXEC, O_CLOFORK, O_CREAT, O_DIRECTORY, O_EXCL, O_NOCTTY, O_NOFOLLOW, O_TRUNC, O_TTY_INIT bitwise-distinct, usable in #if (O_TTY_INIT may be 0) | TESTED except O_CLOFORK (O_TTY_INIT picolibc only) | test-fcntl_h.c:49-52, 128-133, 138, 246. **O_TTY_INIT was missing; fixed (0). O_CLOFORK missing; not fixed** |
| F10 | O_APPEND, O_DSYNC, O_NONBLOCK, O_RSYNC, O_SYNC usable in #if | TESTED | test-fcntl_h.c:147, 247 |
| F11 | O_ACCMODE masks the file access modes | TESTED | test-fcntl_h.c:253-256 (O_SEARCH picolibc only). **Didn't cover O_EXEC/O_SEARCH; fixed** |
| F12 | O_EXEC, O_RDONLY, O_RDWR, O_SEARCH, O_WRONLY unique (O_EXEC may equal O_SEARCH) | TESTED (O_EXEC/O_SEARCH picolibc only; glibc has neither) | test-fcntl_h.c:153, 251 |
| F13 | The `<sys/stat.h>` file mode constants | TESTED | test-fcntl_h.c:43-48, 167. **Were missing; fixed** |
| F14 | AT_FDCWD, AT_EACCESS, AT_SYMLINK_NOFOLLOW, AT_SYMLINK_FOLLOW, AT_REMOVEDIR | TESTED | test-fcntl_h.c:175, 258-259 |
| F15 | POSIX_FADV_* | TESTED | test-fcntl_h.c:180, 260 |
| F16 | Prototypes for creat, fcntl, open, openat, posix_fadvise, posix_fallocate | TESTED | test-fcntl_h.c:195-204 |

### `<sys/stat.h>` (basedefs/sys_stat.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | struct stat members and types, including st_atim, st_mtim, st_ctim | TESTED | test-sys_stat_h.c:61 |
| S2 | blkcnt_t, blksize_t, dev_t, ino_t, mode_t, nlink_t, uid_t, gid_t, off_t, time_t, struct timespec | TESTED | test-sys_stat_h.c:54 |
| S3 | st_atime, st_ctime, st_mtime are st_Xtim.tv_sec | TESTED | test-sys_stat_h.c:172-173 |
| S4 | S_IFMT and the S_IF* types, usable in #if, distinct | TESTED | test-sys_stat_h.c:75, 142-145 |
| S5 | The mode bits have the given values, are usable in #if and without a cast as open()'s mode | TESTED | test-sys_stat_h.c:82, 134 |
| S6 | S_ISBLK … S_ISSOCK true for their own type only | TESTED | test-sys_stat_h.c:152-158 |
| S7 | S_TYPEISMQ, S_TYPEISSEM, S_TYPEISSHM (zero when not distinct types) | TESTED | test-sys_stat_h.c:163-165. **Were missing; fixed** |
| S8 | UTIME_NOW and UTIME_OMIT distinct, outside [0, 999999999] | TESTED | test-sys_stat_h.c:89-90 |
| S9 | Prototypes for the 16 functions | TESTED | test-sys_stat_h.c:105-126 |
| S10 | Distinct files have distinct (st_dev, st_ino); hard links share it; st_nlink counts links | TESTED | test-lstat.c:107-111 (see the st_dev note below) |

### stat, lstat, fstat, fstatat

| # | Requirement | Status | Reference |
|---|---|---|---|
| ST1 | stat fills buf; no permission on the file is needed | TESTED | test-lstat.c:94, 196 |
| ST2 | st_mode, st_ino, st_dev, st_uid, st_gid, the times and st_nlink are meaningful | TESTED | test-lstat.c:95-103 (uid is the euid, times near now, tv_nsec in range) |
| ST3 | stat follows symbolic links | TESTED | test-lstat.c:115 |
| ST4 | lstat reports the link: type, st_size = length of its contents, st_nlink, owner | TESTED | test-lstat.c:117-120 |
| ST5 | lstat of a non-link is stat | TESTED | test-lstat.c:121 |
| ST6 | fstat on files and pipes | TESTED | test-lstat.c:103, 125; test-stat.c:127 |
| ST7 | fstatat relative to fd, with and without AT_SYMLINK_NOFOLLOW | TESTED | test-lstat.c:131-132, test-fstatat.c:82-85 |
| ST8 | fstatat(AT_FDCWD) is stat or lstat; an absolute path ignores fd | TESTED | test-lstat.c:134-137 |
| ST9 | fstatat through an O_SEARCH descriptor | TESTED (picolibc only) | test-lstat.c:153-155 |
| ST10 | With an O_SEARCH descriptor the search permission check is skipped (all *at functions) | NOT TESTABLE | Linux checks search permission through O_PATH descriptors (verified by running a glibc O_PATH program); see "Deviations not fixed" |
| ST11 | Time fields updated before writing buf | NOT TESTABLE | kernel-internal |
| E-ST1 | EACCES (search permission on the prefix; fstatat's fd) | TESTED | test-lstat.c:190-192 (skipped as root) |
| E-ST2 | EIO | NOT TESTABLE | needs a failing device |
| E-ST3 | ELOOP | TESTED | test-lstat.c:178 |
| E-ST4 | ENAMETOOLONG (component) | TESTED | test-lstat.c:183 |
| E-ST5 | ENOENT (missing, empty path) | TESTED | test-lstat.c:163-169 |
| E-ST6 | ENOTDIR (prefix, trailing slash) | TESTED | test-lstat.c:171-175 |
| E-ST7 | EOVERFLOW | NOT TESTABLE | off_t, ino_t, blkcnt_t are 64-bit on every target |
| E-ST8 | fstatat EBADF for a relative path and a bad fd; ENOTDIR for a non-directory fd | TESTED | test-lstat.c:140-142 |
| E-ST9 | fstatat with Linux's AT_FDCWD value (-100) is EBADF, not the working directory | TESTED (picolibc only) | test-lstat.c:148. **Was a deviation; fixed** |
| E-ST10 | fstatat EINVAL for a bad flag (may fail) | TESTED | test-lstat.c:185, test-fstatat.c:88 |
| E-ST11 | fstat EBADF | TESTED | test-lstat.c:199 |

### chmod, fchmod, fchmodat

| # | Requirement | Status | Reference |
|---|---|---|---|
| CH1 | Sets the permission bits, S_ISUID, S_ISGID and S_ISVTX | TESTED | test-chmod.c:80-100, test-fchmod.c:80 |
| CH2 | File type bits in mode are ignored | TESTED | test-chmod.c:106-107, test-fchmod.c:80-81 |
| CH3 | Fails (EPERM) unless the euid owns the file | TESTED | test-chmod.c:131, test-fchmod.c:94 (skipped as root) |
| CH4 | S_ISGID cleared for a regular file whose group isn't the caller's | NOT TESTABLE | needs a file in a group we aren't in, which an unprivileged user can't create |
| CH5 | st_ctim marked for update | TESTED | test-chmod.c:110-124 |
| CH6 | On failure the mode is unchanged | TESTED | test-fchmodat.c:127-128 |
| CH7 | fchmodat relative to fd; AT_FDCWD with flag 0 is chmod; absolute paths | TESTED | test-fchmodat.c:86, 111-116 |
| CH8 | AT_SYMLINK_NOFOLLOW on a symbolic link changes it or fails with EOPNOTSUPP | TESTED | test-fchmodat.c:94 |
| CH9 | AT_SYMLINK_NOFOLLOW on anything else changes its mode | TESTED | test-fchmodat.c:98-105. **Was a deviation on kernels before 6.6; fixed** |
| CH10 | fchmod works through a read-only descriptor | TESTED | test-fchmod.c:85 |
| E-CH1 | EACCES, ELOOP, ENOENT (incl. empty), ENOTDIR (prefix, trailing slash) | TESTED | test-chmod.c:136-154 |
| E-CH2 | ENAMETOOLONG | UNTESTED | the kernel's lookup, as in E-ST4 (inferred) |
| E-CH3 | EROFS | NOT TESTABLE | no read-only file system on this host |
| E-CH4 | fchmodat EBADF, ENOTDIR, EINVAL flag, ENOENT empty | TESTED | test-fchmodat.c:120-131 |
| E-CH5 | fchmod EBADF | TESTED | test-fchmod.c:100 |
| E-CH6 | EINTR, EINVAL mode (may fail) | Not asserted | optional |

### mkdir, mkdirat, mkfifo, mkfifoat, mknod, mknodat, umask

| # | Requirement | Status | Reference |
|---|---|---|---|
| MK1 | mkdir: permission bits and S_ISVTX from mode, less the umask | TESTED | test-mkdir.c:125, 139, 144; test-umask.c:84 |
| MK2 | Owner is the euid; group is the parent's or the egid (mkdir, mkfifo, mknod, open) | TESTED | test-mkdir.c:127-128, 166-167, 178-179; test-open.c:116-120 |
| MK3 | The new directory is empty | TESTED | test-mkdir.c:131-134 |
| MK4 | The parent's mtime and ctime are marked (mkdir, mkfifo, mknod) | TESTED | test-mkdir.c:102 (called at 129, 168, 180) |
| MK5 | A symbolic link, even a dangling one, or any existing name is EEXIST for all six; nothing is made at the target | TESTED | test-mkdir.c:195-210 |
| MK6 | mkdirat/mkfifoat/mknodat relative to fd, AT_FDCWD, absolute paths | TESTED | test-mkdir.c:150-158, 170, 182; test-mkdirat.c:76, test-mkfifoat.c:77, test-mknodat.c:79 |
| MK7 | mkfifo makes a FIFO, mode less the umask, that passes data | TESTED | test-mkdir.c:163-164, test-mkfifo.c:78-92 |
| MK8 | mknod makes FIFOs with the mask applied | TESTED | test-mkdir.c:175-176, test-mknod.c:75 |
| MK9 | mknod of other types needs privileges (EPERM) | TESTED for S_IFCHR and S_IFBLK | test-mkdir.c:253-256 (skipped as root). Linux lets anyone make regular files, sockets and the 0:0 "whiteout" character device with mknod (kernel) |
| MK10 | On failure nothing is made | TESTED | test-mkdir.c:292-298 |
| E-MK1 | EACCES (write and search) | TESTED | test-mkdir.c:261-271 (skipped as root) |
| E-MK2 | ELOOP, ENAMETOOLONG, ENOENT (prefix, empty), ENOTDIR | TESTED | test-mkdir.c:214-245 |
| E-MK3 | mkfifo trailing slash: ENOENT or ENOTDIR; not ENOENT for an existing file | TESTED | test-mkdir.c:230-233 |
| E-MK4 | mknod EINVAL | TESTED | test-mkdir.c:249 |
| E-MK5 | *at EBADF and ENOTDIR | TESTED | test-mkdir.c:277-289 |
| E-MK6 | EILSEQ, EMLINK, ENOSPC, EROFS, EIO | NOT TESTABLE | Linux accepts any name bytes; ext4 has no practical link limit; needs a full or read-only file system or a failing device |
| U1 | umask sets the mask and returns the old one; a returned value restores it | TESTED | test-umask.c:71-72 |
| U2 | Only the permission bits are used; S_ISVTX clear in the result | TESTED | test-umask.c:89 |
| U3 | The mask applies to open, openat, creat, mkdir(at), mkfifo(at), mknod(at) | TESTED | test-umask.c:78-84, test-open.c:114, test-openat.c:104, test-creat.c:74, test-mkdir.c:125-182 |

### futimens, utimensat

| # | Requirement | Status | Reference |
|---|---|---|---|
| UT1 | Sets the access and modification times, to the nanosecond | TESTED | test-utimensat.c:94-96, 113-118 |
| UT2 | Rounds down to the file system's resolution, never later | TESTED | test-utimensat.c:117 |
| UT3 | UTIME_NOW is now and UTIME_OMIT leaves the time; tv_sec ignored for both | TESTED | test-utimensat.c:100-102, 125-128; test-futimens.c:91, 96 |
| UT4 | A null times sets both to now | TESTED | test-futimens.c:103-106, test-utimensat.c:104-105 |
| UT5 | Times past 2038 | TESTED | test-futimens.c:87-89 |
| UT6 | utimensat relative to fd, AT_FDCWD, absolute paths | TESTED | test-utimensat.c:94, 181-186 |
| UT7 | AT_SYMLINK_NOFOLLOW sets the link's times, not the target's | TESTED | test-utimensat.c:166-173 |
| UT8 | st_ctim marked for update | TESTED | test-utimensat.c:130-140 |
| UT9 | On failure the times are unchanged | TESTED | test-utimensat.c:145-156, test-futimens.c:116-117 |
| UT10 | Explicit times need the owner (EPERM); NULL/UTIME_NOW need owner or write access (EACCES) | TESTED | test-utimensat.c:193-201 (on "/", skipped as root) |
| UT11 | Both UTIME_OMIT needs no ownership or permission | TESTED | test-utimensat.c:205 |
| UT12 | The owner may use a read-only descriptor | TESTED | test-futimens.c:123 |
| E-UT1 | EINVAL for tv_nsec < 0 or >= 1e9 | TESTED | test-utimensat.c:151-155, test-futimens.c:116 |
| E-UT2 | EINVAL for an unsupported tv_sec; EROFS | NOT TESTABLE | ext4 takes the whole range; no read-only file system |
| E-UT3 | futimens EBADF | TESTED | test-futimens.c:129 |
| E-UT4 | utimensat EBADF, ENOTDIR fd, ENOENT (incl. empty), ENOTDIR trailing slash, EINVAL flag | TESTED | test-utimensat.c:106, 212-222 |
| E-UT5 | utimensat ELOOP, ENAMETOOLONG, EACCES on the prefix | UNTESTED | kernel lookup, as for stat (inferred) |

### open, openat, creat

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | Returns the lowest available descriptor | TESTED | test-open.c:127-130 |
| O2 | A new open file description, offset at 0 | TESTED | test-open.c:139-143 |
| O3 | FD_CLOEXEC clear unless O_CLOEXEC | TESTED | test-open.c:128, 135 |
| O4 | FD_CLOFORK clear unless O_CLOFORK | UNTESTED | **O_CLOFORK doesn't exist; deviation, not fixed** |
| O5 | The access modes limit reading and writing, and F_GETFL reports them | TESTED | test-open.c:149-157 |
| O6 | O_EXEC: not readable, reported as O_EXEC | TESTED (picolibc only) | test-open.c:233-234 |
| O7 | O_SEARCH opens a directory for searching; openat works through it; reported as O_SEARCH | TESTED (picolibc only) | test-open.c:223-228. **Reporting was a deviation (O_ACCMODE); fixed** |
| O8 | O_APPEND writes at the end | TESTED | test-open.c:168-170 |
| O9 | O_CREAT: a regular file, mode less the umask, owned by the euid; parent marked | TESTED | test-open.c:113-123 |
| O10 | O_CREAT on an existing file has no effect | TESTED | test-open.c:161 |
| O11 | O_EXCL: EEXIST, also for links and dangling links | TESTED | test-open.c:164, 206-208 |
| O12 | O_EXCL atomic between threads | NOT TESTABLE | a race in the kernel |
| O13 | O_DIRECTORY: ENOTDIR for other files | TESTED | test-open.c:213, test-openat.c:91 |
| O14 | O_NOCTTY: a terminal doesn't become the controlling terminal | UNTESTED | needs a new session and a terminal (termios unit) |
| O15 | O_NOFOLLOW: ELOOP for a link | TESTED | test-open.c:205 |
| O16 | O_NONBLOCK FIFO: readers return at once, writers without readers ENXIO | TESTED | test-open.c:240-247 |
| O17 | A blocking FIFO open waits for the other end | TESTED | test-open.c:251-261 |
| O18 | O_SYNC, O_DSYNC, O_RSYNC accepted and reported | TESTED | test-open.c:185-188 (the integrity itself is NOT TESTABLE) |
| O19 | O_TRUNC empties a regular file, keeps mode and owner, marks mtime/ctime; no effect on FIFOs | TESTED | test-open.c:174-179, 246 |
| O20 | O_TTY_INIT (or 0): terminal set to a conforming state | NOT TESTABLE | needs a terminal no process has open. picolibc now defines it as 0; Linux doesn't reset non-standard termios state on first open (see below) |
| O21 | openat relative, AT_FDCWD, absolute paths, mode | TESTED | test-open.c:267-269, test-openat.c:76-110 |
| O22 | creat is open(path, O_WRONLY \| O_CREAT \| O_TRUNC, mode) | TESTED | test-creat.c:72-90 |
| O23 | On failure no file is made | TESTED | test-open.c:208, 305 |
| E-O1 | EACCES (read, write, O_TRUNC, create, search; openat's fd) | TESTED | test-open.c:290-301 (skipped as root) |
| E-O2 | EEXIST, EISDIR (O_WRONLY, O_RDWR, O_CREAT), ELOOP, ENAMETOOLONG | TESTED | test-open.c:164, 210-216, 287 |
| E-O3 | EMFILE | TESTED | test-open.c:308-320 (a child with RLIMIT_NOFILE 16) |
| E-O4 | ENOENT (missing, O_CREAT prefix, empty); ENOTDIR (prefix, trailing slash); O_CREAT trailing slash | TESTED | test-open.c:276-282 |
| E-O5 | ENXIO for a FIFO | TESTED | test-open.c:240 |
| E-O6 | openat EBADF, ENOTDIR | TESTED | test-open.c:271-273, test-openat.c:95-99 |
| E-O7 | EINTR, EILSEQ, EINVAL (FIFO O_RDWR, sync I/O), ENFILE, ENOSPC, ENXIO (device), EOVERFLOW, EROFS, ETXTBSY, EAGAIN, EOPNOTSUPP | NOT TESTABLE or not asserted | Linux supports O_RDWR FIFOs and sync I/O; off_t is 64-bit; the rest need system limits, devices or a read-only file system |

### fcntl

| # | Requirement | Status | Reference |
|---|---|---|---|
| FC1 | F_DUPFD: lowest free descriptor >= arg, same open file description, FD_CLOEXEC clear | TESTED | test-fcntl.c:88-89, 111-122 |
| FC2 | F_DUPFD_CLOEXEC sets FD_CLOEXEC | TESTED | test-fcntl.c:92, 120 |
| FC3 | F_DUPFD_CLOFORK; F_SETFD with FD_CLOFORK | UNTESTED | **missing; deviation, not fixed** |
| FC4 | F_GETFD/F_SETFD per descriptor | TESTED | test-fcntl.c:83-85, 97 |
| FC5 | FD_CLOEXEC closes the descriptor on exec | UNTESTED | exec; kernel behaviour (inferred) |
| FC6 | F_GETFL: access mode through O_ACCMODE plus status flags, shared by the open file description | TESTED | test-fcntl.c:71-81, 99-100; test-open.c:150-157, 224, 234 |
| FC7 | F_SETFL ignores the access mode and creation flags | TESTED | test-fcntl.c:105-107 |
| FC8 | F_GETOWN/F_SETOWN: pid > 0, process group < 0, 0 for none | TESTED | test-fcntl-setown.c:71-82. **fcntl rejected both with EINVAL; fixed** |
| FC9 | F_GETOWN_EX/F_SETOWN_EX with F_OWNER_PID and F_OWNER_PGRP | TESTED (picolibc only) | test-fcntl-setown.c:99-122. **Were missing; fixed** |
| FC10 | SIGURG delivery to the owner, with kill() permission checks | UNTESTED | needs out-of-band TCP data (net-sockets) |
| FC11 | F_GETLK reports the blocking lock: type, SEEK_SET, start, length, pid | TESTED | test-fcntl-setlkw.c:180-181, test-fcntl-locks.c:109 |
| FC12 | F_GETLK without a blocker changes only l_type | TESTED | test-fcntl-setlkw.c:186-194 |
| FC13 | F_SETLK fails at once with EACCES or EAGAIN | TESTED | test-fcntl-setlkw.c:212 |
| FC14 | F_SETLKW waits; a caught signal makes it fail with EINTR | TESTED | test-fcntl-setlkw.c:226-233 (an interval timer keeps interrupting, so no timing assumption) |
| FC15 | Shared locks need read access and exclusive locks write access (EBADF) | TESTED | test-fcntl-setlkw.c:265, 271 |
| FC16 | Negative l_len covers [start + len, start - 1]; no lock before offset 0 | TESTED | test-fcntl-setlkw.c:204-208, 279-282 |
| FC17 | l_len 0 reaches the largest offset, past EOF | TESTED | test-fcntl-setlkw.c:249-250 |
| FC18 | A new lock replaces the process's own locks in its range; unlock splits | TESTED | test-fcntl-setlkw.c:241-244 |
| FC19 | Unlocking a range ending at the largest off_t is unlocking to the end | TESTED | test-fcntl-setlkw.c:252-253 |
| FC20 | Process locks: released by closing any descriptor of the file or by exit; not inherited | TESTED | test-fcntl-setlkw.c:233-238, 256-259 |
| FC21 | OFD locks: shared through fork, conflict with process locks even in one process, l_pid -1, released when the description closes | TESTED (glibc needs _GNU_SOURCE, so the native build skips them) | test-fcntl-setlkw.c:296-333, test-fcntl-locks.c:132-140 |
| FC22 | F_OFD_SETLKW EINTR | TESTED (picolibc only) | test-fcntl-setlkw.c:328 |
| FC23 | The range is fixed before F_SETLKW blocks | NOT TESTABLE | needs a known-blocked waiter |
| E-FC1 | EBADF | TESTED | test-fcntl.c:151-153, test-fcntl-setlkw.c:291, test-fcntl-setown.c:147 |
| E-FC2 | EINVAL: unknown cmd, negative arg, arg >= OPEN_MAX, bad lock data, bad f_owner_ex | TESTED | test-fcntl.c:127-147, test-fcntl-locks.c:124, test-fcntl-setlkw.c:277-282, test-fcntl-setown.c:130-137 (OPEN_MAX is taken from sysconf, see cross-cutting) |
| E-FC3 | EMFILE | TESTED | test-fcntl.c:140 |
| E-FC4 | EOVERFLOW for a range past the largest off_t | TESTED | test-fcntl-setlkw.c:285-288 |
| E-FC5 | ESRCH for F_SETOWN/F_SETOWN_EX | TESTED | test-fcntl-setown.c:91, 142 |
| E-FC6 | ENOLCK | NOT TESTABLE | system lock limit |
| E-FC7 | EDEADLK, EPERM (may fail) | UNTESTED | EDEADLK needs knowing another process is waiting; Linux checks SIGURG permission only when sending |

### posix_fadvise, posix_fallocate

| # | Requirement | Status | Reference |
|---|---|---|---|
| FA1 | Each advice value returns 0; len 0 means to the end; the range need not exist | TESTED | test-posix_fadvise.c:81, 88 |
| FA2 | Errors are returned, errno untouched | TESTED | test-posix_fadvise.c:98 |
| E-FA1 | EBADF, EINVAL (advice, len < 0), ESPIPE (pipe, FIFO) | TESTED | test-posix_fadvise.c:82-94 |
| FL1 | Allocates; past the end the file grows to offset + len; inside it doesn't change | TESTED | test-posix_fallocate.c:101-104, 113 |
| FL2 | Writes into the space don't fail for lack of space | TESTED (one write) | test-posix_fallocate.c:116 |
| FL3 | An O_TRUNC open frees the space | TESTED (size) | test-posix_fallocate.c:119 |
| E-FL1 | EBADF (bad, read-only) | TESTED | test-posix_fallocate.c:107, 124 |
| E-FL2 | EFBIG past the largest size, and past RLIMIT_FSIZE with SIGXFSZ | TESTED | test-posix_fallocate.c:128, 131-145 |
| E-FL3 | EINVAL (negative offset or length; zero length may fail) | TESTED | test-posix_fallocate.c:105-106, 129 |
| E-FL4 | ENODEV | TESTED | test-posix_fallocate.c:150 (/dev/null) |
| E-FL5 | ENOTSUP when the file system can't allocate | TESTED (picolibc only) | test-posix_fallocate.c:167 (/proc/self/comm). **Was EINVAL; fixed**. glibc emulates the allocation with writes instead |
| E-FL6 | ESPIPE (pipe, FIFO) | TESTED | test-posix_fallocate.c:109, 157 |
| E-FL7 | EINTR, EIO, ENOSPC | NOT TESTABLE | needs a signal mid-allocation, a failing device, a full file system |

### statvfs, fstatvfs, `<sys/statvfs.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| SV1 | struct statvfs members and types; fsblkcnt_t, fsfilcnt_t (unsigned) | TESTED | test-sys_statvfs_h.c:53-56 |
| SV2 | ST_RDONLY and ST_NOSUID, distinct bits | TESTED | test-sys_statvfs_h.c:70 |
| SV3 | Prototypes | TESTED | test-sys_statvfs_h.c:79-80 |
| SV4 | statvfs and fstatvfs describe the same file system | TESTED | test-fstatvfs.c:160-164, test-statvfs.c:147-150 |
| SV5 | No permission on the file is needed | TESTED | test-fstatvfs.c:170-171 |
| SV6 | Counts are consistent (free ≤ total, available ≤ free) | TESTED | test-fstatvfs.c:72-77 |
| SV7 | ST_RDONLY and ST_NOSUID match each mount's options | TESTED | test-fstatvfs.c:136-140 (9 mounts; none is read-only here, so ST_RDONLY is only checked clear) |
| SV8 | f_flag holds only f_flag values | TESTED (picolibc only) | test-fstatvfs.c:80. **Was a deviation (ST_VALID and others passed through); fixed** |
| E-SV1 | statvfs EACCES, ELOOP, ENAMETOOLONG, ENOENT (incl. empty), ENOTDIR (prefix, trailing slash) | TESTED | test-fstatvfs.c:184-209 |
| E-SV2 | fstatvfs EBADF | TESTED | test-fstatvfs.c:214 |
| E-SV3 | EIO, EINTR, EOVERFLOW | NOT TESTABLE | needs a failing device; the counts are 64-bit |

### Counts

184 requirements:
- TESTED: 159, plus F5 in part (FD_CLOEXEC yes, FD_CLOFORK no). Of these, 10 are
  picolibc only (F1, F7, ST9, E-ST9, O6, O7, FC9, FC22, E-FL5, SV8 are picolibc only
  because glibc lacks the names under `_POSIX_C_SOURCE` or behaves differently;
  F4, F9, F11 and F12 have one picolibc-only part).
- UNTESTED: 8 (O4, FC3 and F5's FD_CLOFORK are the close-on-fork deviation; O14,
  FC5, FC10, E-FC7, E-CH2, E-UT5).
- NOT TESTABLE: 15 (ST10, ST11, E-ST2, E-ST7, CH4, E-CH3, E-MK6, E-UT2, O12, O20,
  E-O7, FC23, E-FC6, E-FL7, E-SV3).
- Optional errors deliberately not asserted: 1 (E-CH6).

Before this audit about 47 of these rows were tested, by the 20 existing tests in
the three directories (none of them checked the headers). Now 159 are. The audit
added 9 test programs (test-fcntl_h, test-fcntl-setown, test-fcntl-setlkw,
test-open, test-sys_stat_h, test-lstat, test-mkdir, test-sys_statvfs_h,
test-fstatvfs) and extended 9 (test-fcntl, test-openat, test-posix_fadvise,
test-posix_fallocate, test-chmod, test-fchmod, test-fchmodat, test-futimens,
test-utimensat). test-sys-statvfs's CMakeLists.txt had no tests; it now registers
the two new ones.

## Deviations found and fixed

Each fix was verified by running: on aarch64-linux-gnu the new check fails with only
the library change reversed (`git show <fix> -- libc libos | git apply -R`, rebuild,
run, reapply), and passes with it. The failing check is quoted.

1. **fchmodat with AT_SYMLINK_NOFOLLOW failed for every file on kernels before 6.6**
   (libos/linux/fchmodat.c). Only fchmodat2 takes flags; without it picolibc
   returned EOPNOTSUPP even for regular files and directories. POSIX allows
   EOPNOTSUPP only when "path names a symbolic link, and the system does not
   support changing the mode of a symbolic link". Fix: on ENOSYS, open the file
   with O_PATH | O_NOFOLLOW, return EOPNOTSUPP if it is a link, and otherwise change
   the mode through /proc/self/fd/N (glibc's approach; without /proc it stays
   EOPNOTSUPP). This host runs 5.10, so the fallback is what ran. Failing-before:
   `AT_SYMLINK_NOFOLLOW on a file: fchmodat(dfd, "file", 0604, AT_SYMLINK_NOFOLLOW) == 0`.
2. **posix_fallocate reported an unsupporting file system as EINVAL**
   (libos/linux/posix_fallocate.c). That was Issue 7; Issue 8 says "[ENOTSUP] The
   underlying file system does not support this operation". picolibc's ENOTSUP
   (134) differs from EOPNOTSUPP (95), so the kernel's EOPNOTSUPP is now mapped to
   ENOTSUP. Failing-before: `ENOTSUP: posix_fallocate(fd2, 0, 1) == ENOTSUP`
   (/proc/self/comm, a regular file whose file system has no fallocate).
   test-posix_fallocate's "can't allocate, skipping" path accepts both errors.
3. **Linux's AT_FDCWD (-100) worked as the working directory in every *at
   function** (libos/linux/local-linux.h, `_at_fd_to_linux`). picolibc's AT_FDCWD
   is -2, so -100 is neither AT_FDCWD nor a descriptor; POSIX requires EBADF. It is
   now passed to the kernel as -1. This changes every *at function in libos/linux
   (faccessat, fchownat, linkat, unlinkat, … in other units too). Failing-before:
   `Linux's AT_FDCWD: fstatat(-100, "file", &st2, 0) == -1 && errno == EBADF`.
4. **O_ACCMODE didn't cover O_EXEC and O_SEARCH** (libc/include/sys/_default_fcntl.h).
   POSIX lists them among the file access modes that "can be extracted from the
   return value [of F_GETFL] using the mask O_ACCMODE". O_ACCMODE now includes
   their bit (as musl's does); `_open_flags_to_linux`, `_open_flags_from_linux` and
   mq_open mask the three classic modes explicitly. Failing-before:
   `O_ACCMODE covers the access modes: (access_modes[i] & O_ACCMODE) == access_modes[i]`
   and `O_SEARCH reported: (fcntl(fd2, F_GETFL) & O_ACCMODE) == O_SEARCH`.
5. **`<fcntl.h>` lacked O_TTY_INIT, SEEK_SET/SEEK_CUR/SEEK_END and the file mode
   constants** (sys/_default_fcntl.h). It now defines O_TTY_INIT as 0 (allowed),
   the SEEK_ values (guarded like `<stdio.h>`'s), and includes `<sys/stat.h>`, which
   POSIX permits. Failing-before (compile): `#error <fcntl.h> must define SEEK_SET,
   SEEK_CUR and SEEK_END`, `... the file mode constants`, `... O_TTY_INIT`. These
   three were proven by deleting the additions by hand, because the next commit
   touches the neighbouring lines and the reverse patch doesn't apply on its own.
6. **fcntl rejected F_GETOWN and F_SETOWN with EINVAL, and F_GETOWN_EX, F_SETOWN_EX,
   struct f_owner_ex, F_OWNER_PID and F_OWNER_PGRP were missing**
   (libos/linux/fcntl.c, sys/_default_fcntl.h). All four commands now go through the
   kernel's F_GETOWN_EX/F_SETOWN_EX, which can report process groups without the
   negative value looking like an error. A negative ID with F_SETOWN_EX is EINVAL
   (POSIX) rather than the kernel's ESRCH; F_SETOWN of INT_MIN is EINVAL; a kernel
   F_OWNER_TID owner reads back as F_OWNER_PID. Failing-before (library part only):
   `no owner: fcntl(fds[0], F_GETOWN) == 0`; with the header part reversed too, the
   test doesn't compile.
7. **S_TYPEISMQ, S_TYPEISSEM and S_TYPEISSHM were missing** (libc/include/sys/stat.h).
   They evaluate to 0 (these objects aren't distinct file types on Linux), using
   glibc's `(buf)->st_mode - (buf)->st_mode` form so buf is still type-checked.
   Failing-before: `implicit declaration of function 'S_TYPEISMQ'`.
8. **statvfs f_flag carried the kernel's other mount flags** (libos/linux/_statvfsbuf.c):
   ST_VALID (0x20), ST_NODEV, ST_NOEXEC, ST_RELATIME and so on, none of which
   `<sys/statvfs.h>` defines. They are now mapped bit by bit (MAP_FLAG), keeping
   ST_RDONLY and ST_NOSUID. Failing-before:
   `f_flag holds only ST_ flags: (sv->f_flag & ~(unsigned long)(ST_RDONLY | ST_NOSUID)) == 0`.
   (glibc keeps the extra bits too, so the check is picolibc-only.) This one is
   more a cleanup than a strict violation: POSIX doesn't forbid other bits, but
   picolibc names none of them.

## Deviations not fixed

- **No close-on-fork** (F4, F5, F9, O4, FC3): O_CLOFORK, FD_CLOFORK and
  F_DUPFD_CLOFORK don't exist. Linux has no close-on-fork flag, so it would need a
  user-space descriptor table that open, dup*, fcntl, socket, accept4, pipe2, close
  and fork all maintain. Defining the names without the behaviour would be worse.
  Left for a decision; it spans several units (recorded in cross-cutting.md).
- **O_SEARCH descriptors don't skip the search-permission check** (ST10, and the
  same sentence in fchmodat, mkdirat, mkfifoat, mknodat, openat, utimensat):
  picolibc's O_SEARCH is O_PATH, and Linux checks search permission on the
  directory even through an O_PATH descriptor. Verified by running a glibc O_PATH
  program here (`fstatat` through an O_PATH descriptor of a mode-0 directory gives
  EACCES). Kernel behaviour; not fixable in the library.
- **mknod makes regular files, sockets and 0:0 character devices without
  privileges** (MK9). POSIX: "Only a process with appropriate privileges may invoke
  mknod() for file types other than FIFO-special." Linux allows S_IFREG, S_IFSOCK
  and the overlayfs whiteout device; glibc passes them through as well. picolibc
  could refuse them, but that would be stricter than the system's own notion of
  "appropriate privileges", so I left it.
- **O_TTY_INIT is 0, but Linux doesn't reset non-standard termios state** (O20).
  POSIX lets O_TTY_INIT be zero only if open() then always initializes a terminal
  that isn't open elsewhere. Linux keeps a tty's termios from its last use unless
  the driver resets it (ptys are always reset, which POSIX requires anyway). glibc
  doesn't define O_TTY_INIT at all; musl defines 0. Belongs with the termios unit.

## Other observations (not changed)

- **st_dev and st_rdev are encoded as `major << 8 | minor`** (libos/linux/_statbuf.c).
  Minors above 255 overlap the next major (0:300 and 1:44 are both 0x12c), so two
  file systems could in principle share an st_dev, which would break "distinct
  files shall have distinct file identities". The kernel's own 32-bit form,
  `(minor & 0xff) | (major << 8) | ((minor & ~0xff) << 12)`, is one-to-one and is
  what mknod's dev argument already passes through, so mknod/stat round trips for
  minors above 255 are also broken. The BSD `major()`/`minor()` macros in
  `<sys/stat.h>` assume yet another layout (a 32-bit shift for a 64-bit dev_t).
  No device or mount on this host has a minor above 255, and creating one needs
  root, so I couldn't show a failure; recorded rather than changed (inferred by
  reading).
- `lstat` and `mknod` are declared under `defined(__BSD_VISIBLE)`, which is always
  true (the macro is always defined, as 0 or 1): the same mistake cross-cutting.md
  already lists for stdio.h and pwd.h. Harmless here since both are POSIX.
- `fcntl(F_SETFD)` passes the argument straight to the kernel; picolibc's
  FD_CLOEXEC is 1, as Linux's is.
- O_DSYNC and O_RSYNC have O_SYNC's value, so F_GETFL can't tell them apart. POSIX
  doesn't require the status flags to be distinct, and O_SYNC is the stronger
  guarantee.
- posix_openpt's flag check uses O_ACCMODE, so it now lets O_SEARCH through to the
  kernel (as O_PATH). Harmless, but worth a look by whoever audits `<stdlib.h>`.
- glibc header differences met along the way, all handled with `__PICOLIBC__`
  guards or `_XOPEN_SOURCE`: no O_EXEC, O_SEARCH, O_TTY_INIT or the CLOFORK names at
  all; F_OFD_*, F_GETOWN_EX, F_SETOWN_EX and struct f_owner_ex only with
  `_GNU_SOURCE`; S_ISVTX and mknod only with `_XOPEN_SOURCE`; realpath isn't
  declared under `_POSIX_C_SOURCE 202405L`.

## Left undone

- The close-on-fork family (see above).
- O14 (O_NOCTTY needs a new session and a terminal; termios unit), FC5 (FD_CLOEXEC
  across exec), FC10 (SIGURG delivery needs TCP out-of-band data; net-sockets),
  E-FC7 (EDEADLK needs knowing that another process is already waiting; polling
  /proc/locks would work but is Linux-only), E-CH2 and E-UT5 (kernel lookup errors
  already covered for stat and open).
- CH4 (S_ISGID clearing) needs a group we aren't in, so it can only run as root
  with a second group, and the tests skip permission checks as root.
- Timing: the two "st_ctim advanced" checks (test-chmod.c, test-utimensat.c) repeat
  the change until the kernel's coarse clock ticks, up to 10^6 times; they take a
  few milliseconds and have no fixed sleep. The F_SETLKW EINTR checks use a 20 ms
  interval timer, so a signal arriving before fcntl blocks is followed by another.

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`), full
  `meson test -t 4` at the branch tip: 1263 tests; 1216 Ok, 0 Fail, 6 Expected Fail,
  41 Skipped.
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`), full
  `meson test -t 4` at the branch tip: 1262 tests; 1213 Ok, 0 Fail, 6 Expected Fail,
  43 Skipped.
- CMake (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 499 tests, 100%
  passed (235 ran, 264 skipped). The three header tests (test-fcntl_h,
  test-sys_stat_h, test-sys_statvfs_h) run there; the behaviour tests skip, as
  intended, for lack of OS support.
- Every new and changed test also passes as `-native` (glibc) on both Linux targets.
- Bare-metal meson targets weren't run here (the x86_64 host does them).
- `clang-format --dry-run --Werror` is clean on every C file and header the branch
  touches.
- Not run as root here; the root paths (permission checks skipped) were only read.

## Commits (audit/files, on top of 4af0c34b98)

- 2ca072b76c libos/linux: Change modes with AT_SYMLINK_NOFOLLOW in fchmodat without fchmodat2
- 32fc5be488 libos/linux: Report ENOTSUP from posix_fallocate
- bc7b91d1e9 libos/linux: Treat Linux's AT_FDCWD value as a bad descriptor
- fee0dab988 fcntl: Include O_EXEC and O_SEARCH in O_ACCMODE
- c48700d596 fcntl: Define O_TTY_INIT, the SEEK_ values and the file modes
- 31c87144a4 libos/linux: Support F_GETOWN, F_SETOWN, F_GETOWN_EX and F_SETOWN_EX
- 2336b20f6e sys/stat: Define S_TYPEISMQ, S_TYPEISSEM and S_TYPEISSHM
- 12f6a3722d libos/linux: Report only ST_RDONLY and ST_NOSUID in f_flag
- 4fa1c42c28 test-posix: Check the constants, structures and prototypes in fcntl.h
- fbfce88ef7 test-posix: Check the types, structure, macros and prototypes in sys/stat.h
- 7909083575 test-posix: Check the structure, flags and prototypes in sys/statvfs.h
- bef3170af0 test-posix: Check counts, mount flags and errors in statvfs and fstatvfs
- 70cf40a873 test-posix: Check links, identities, O_SEARCH and errors in stat, lstat and fstatat
- cc3ee22856 test-posix: Check modes, symbolic links and errors in mkdir, mkfifo and mknod
- 95d6cc25a0 test-posix: Check AT_SYMLINK_NOFOLLOW on files and errors in fchmodat
- b6b511a62d test-posix: Check special bits, change times and errors in chmod and fchmod
- 8f16f7418d test-posix: Check nanoseconds, links, permissions and errors in utimensat and futimens
- 3aa557a466 test-posix: Check flags, descriptors, FIFOs and errors in open and openat
- b3487084e8 test-posix: Check descriptor flags and F_DUPFD limits in fcntl
- e19f074a05 test-posix: Check F_GETOWN, F_SETOWN, F_GETOWN_EX and F_SETOWN_EX
- 49e66357e9 test-posix: Check ranges, waiting, signals and release of fcntl locks
- 07206e8ec5 test-posix: Check EFBIG, ENODEV, ENOTSUP and growth in posix_fallocate
- 592e06961c test-posix: Check lengths, FIFOs and errno in posix_fadvise

Nothing pushed. No AI attribution in any commit.
