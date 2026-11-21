# `<unistd.h>` process interfaces: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-unistd-process`, branch `audit/unistd-process`, based on
40921d6cce (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10; builds and tests
ran in the Debian sid chroot (gcc 16.2, glibc 2.43) as an unprivileged user (no capabilities,
13 supplementary groups including group 0, Seccomp 0, RLIMIT_STACK 10 MiB, pid_max 32768).

Interfaces: fork, _Fork, execl, execle, execlp, execv, execve, execvp, fexecve, _exit (and _Exit
where the page shares it); getpid, getppid, getuid, geteuid, getgid, getegid, setuid, seteuid,
setgid, setegid, setreuid, setregid, getresuid, getresgid, setresuid, setresgid (new in Issue 8,
XSI), getgroups; setsid, getsid, setpgid, getpgid, getpgrp, tcgetpgrp, tcsetpgrp; getlogin,
getlogin_r, nice (XSI), pause, alarm, sleep; and the `<unistd.h>` declarations they need.
setpgrp is not in Issue 8 (its page is gone, and `<unistd.h>` no longer lists it); ualarm, usleep
and vfork went in Issue 7.

Implementation: `libos/linux/` (fork.c, _Fork.c, _fork_raw.c, execve.c, fexecve.c, _exit.c, the
get/set ID wrappers, getgroups.c, setsid.c, getsid.c, setpgid.c, getpgid.c, getpgrp.c,
tcgetpgrp.c, tcsetpgrp.c, nice.c, pause.c, alarm.c, _pthread_setxid.c), `libc/posix/unistd/`
(execl.c, execle.c, execlp.c, execv.c, execvp.c, execvpe.c, getlogin.c, getlogin_r.c, sleep.c),
header `libc/include/sys/unistd.h`.

Tests: `test/test-posix/test-unistd/`; file names drop the `test-` prefix (`execl.c` =
`test-execl.c`). Line numbers are for the branch tip. Exec tests run the test binary again
through `/proc/self/exe`, never system programs (the older test-execle, test-execvp and
test-fexecve still use `/bin/sh`, and the execvp shell fallback needs it).

Spec source: pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/unistd.h, functions/fork
(covers _Fork), exec (covers fexecve), _exit, getpid, getppid, getuid, geteuid, getgid, getegid,
setuid, seteuid, setgid, setegid, setreuid, setregid, getresuid, getresgid, setresuid, setresgid,
getgroups, setsid, getsid, setpgid, getpgid, getpgrp, tcgetpgrp, tcsetpgrp, getlogin, nice,
pause, alarm, sleep, and XSH 2.4.3 (async-signal-safe functions). Kernel behaviour was checked
against v5.10 `drivers/tty/tty_jobctrl.c` (tiocspgrp, tiocgpgrp) and `kernel/sys.c`
(setreuid, setresuid).

Status key: TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row was verified by
running it on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and (unless noted) for the
`-native` glibc build. "(new)" marks checks this unit added; "(existing)" ones that were there.

## Requirement table

### `<unistd.h>` declarations (basedefs/unistd.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | pid_t, uid_t, gid_t defined; pid_t signed | TESTED (new) | unistd_h-process.c:61-62 |
| H2 | Prototypes of all 40 process functions with the spec's types (incl. `_Noreturn _exit`, restrict in getres*id) | TESTED (new) | unistd_h-process.c:64-107 (`_Generic` on each function's address, no link dependency) |
| H3 | XSI-only process functions (nice, setre*id, getres*id, setres*id) hidden without XSI | TESTED (new) | unistd_h-names.c:51-57 |
| H4 | Removed interfaces not declared: setpgrp (Issue 8), ualarm, usleep, vfork (Issue 7) | TESTED (new) | unistd_h-names.c:60-63 (strict), unistd_h-process.c:54-57 (XSI 800, picolibc). **Deviation 7, fixed** |
| H5 | No extensions in strict mode (execvpe, execlpe, setgroups, issetugid, daemon) | TESTED (new) | unistd_h-names.c:66-70 |
| H6 | Prototype parameter names don't collide with application macros | TESTED (new) | unistd_h-names.c:38-39 (`name`, `namesize`). **Deviation 7, fixed** |
| H7 | `environ` is declared by no header (XBD: "All identifiers ... except environ") | UNTESTED | picolibc declares it for `_POSIX_C_SOURCE >= 202405` on purpose (commit 7e8126df52); see "Deviations not fixed" |

### fork, _Fork

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | Returns 0 in the child and the child's pid in the parent | TESTED | fork.c:208-211 (new); _Fork.c:84-92 (existing) |
| F2 | Unique child pid, matching no active process group | TESTED (new) | fork.c:211,214 (`kill(-pid, 0)` probe, no signal sent) |
| F3 | Child's parent pid is the caller | TESTED (new) | fork.c:104 |
| F4 | Child's descriptors share the parent's open file descriptions | TESTED (new) | fork.c:107, 222-223 (shared offset) |
| F5 | FD_CLOFORK descriptors not inherited | NOT TESTABLE | picolibc has no FD_CLOFORK (files unit, cross-cutting) |
| F6 | Directory streams and message catalogs copied | UNTESTED | follows from the address-space copy (inferred) |
| F7 | tms_utime/stime/cutime/cstime set to 0 | TESTED (new, cutime and cstime) | fork.c:127 after fork.c:173 shows the parent's are non-zero; utime/stime not checked (the child itself runs) |
| F8 | Alarm reset and cancelled | TESTED (new) | fork.c:117, 225 |
| F9 | semadj values cleared (XSI) | UNTESTED | kernel (sysv-ipc area) |
| F10 | Process-owned file locks not inherited | TESTED (new) | fork.c:139-141 (child can't take the lock; F_GETLK shows the parent) |
| F11 | Pending signals empty in the child | TESTED (new) | fork.c:112, 224 |
| F12 | Interval timers reset (XSI) | TESTED (new) | fork.c:119, 226 |
| F13 | Semaphores open in the parent open in the child | UNTESTED | memory-sched area |
| F14 | No memory locks inherited (ML) | UNTESTED | kernel |
| F15 | MAP_PRIVATE data copied, later changes private | TESTED (new) | fork.c:144-146, 221 (static data) |
| F16 | Per-process timers not inherited | TESTED (new) | fork.c:121-125, 227 |
| F17 | Message queue descriptors copied (MSG) | UNTESTED | mqueue area |
| F18 | No AIO inherited | UNTESTED | aio area |
| F19 | One thread in the child; only async-signal-safe calls until exec | TESTED (new) | execl-thread.c:194-214 (_Fork child execs while another thread allocates) |
| F20 | Process-shared locks not held by the child | UNTESTED | pthread-sync area |
| F21 | CPU-time clock of the child starts at zero (CPT) | TESTED (new) | fork.c:130 after fork.c:174 |
| F22 | Signal mask and dispositions copied ("all other characteristics the same") | TESTED (new) | fork.c:110, 114 |
| F23 | _Fork runs no pthread_atfork handlers | TESTED (existing) | _Fork.c:81-92 |
| F24 | _Fork is async-signal-safe | UNTESTED | _Fork.c calls only the system call and `_pthread_fork_reset`, which takes no locks (inferred) |
| F25 | EAGAIN when out of processes / CHILD_MAX | NOT TESTABLE | needs RLIMIT_NPROC, which picolibc lacks (threads-c11 note), and is unsafe on a shared host |
| F26 | ENOMEM (may fail) | NOT TESTABLE | can't exhaust memory safely |

### exec family (execl, execle, execlp, execv, execve, execvp, fexecve)

The new image's checks are in `helper()`, run once for each of the eight call forms
(execl.c:347; fexecve twice, plain and with O_EXEC, skipped for glibc, which has no O_EXEC).

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | No return on success; image replaced | TESTED | execl.c:347-360 (new) |
| X2 | fexecve: equivalent to execve on fd; file offset ignored | TESTED (new) | execl.c:249-254 (offset at end of file) |
| X3 | argc/argv passed exactly, argv[0] as given | TESTED (new) | execl.c:99-103 |
| X4 | environ from envp (execle, execve, fexecve) | TESTED | execl.c:105-111 (new); execle.c (existing) |
| X5 | environ inherited for execl, execv, execlp, execvp | TESTED (new) | execl.c:105-108 |
| X6 | execlp/execvp: name with a slash used as the path | TESTED | execl.c:380, 400 (new); execvp.c (existing) |
| X7 | execlp/execvp: PATH search | TESTED | execl.c:238, 247 (new, through PATH=dir); execvp.c (existing) |
| X8 | PATH unset: implementation-defined | NOT TESTABLE | implementation-defined; picolibc searches `/bin:/usr/bin` (inferred, execvpe.c) |
| X9 | ENOEXEC in execlp/execvp runs the shell with file and the arguments | TESTED (existing) | execvp.c (script via PATH) |
| X10 | Combined argument and environment size limited by {ARG_MAX} | TESTED (new) | execl.c:471-487 (E2BIG) |
| X11 | Descriptors stay open, offset and flags kept, except FD_CLOEXEC | TESTED (new) | execl.c:120-124 |
| X12 | FD_CLOFORK kept across exec | NOT TESTABLE | no FD_CLOFORK |
| X13 | Locks dropped only by FD_CLOEXEC closes | UNTESTED | kernel |
| X14 | Directory streams closed | TESTED (new) | execl.c:126-128 (opendir's descriptor is closed) |
| X15 | Floating-point environment set to default | UNTESTED | kernel |
| X16 | setlocale(LC_ALL, "C") at start-up | TESTED (new) | execl.c:156-158 |
| X17 | SIG_DFL stays default, SIG_IGN stays ignored, caught signals become default | TESTED (new) | execl.c:134-141 |
| X18 | SIGCHLD ignored: unspecified | n/a | unspecified |
| X19 | Alternate signal stacks dropped, SA_ONSTACK cleared (XSI) | TESTED (new) | execl.c:134-136, 139-141, 148-150 (covers the signal unit's T6) |
| X20 | atexit/at_quick_exit/pthread_atfork registrations dropped | UNTESTED | follows from the image replacement (inferred) |
| X21 | Set-ID bits, ST_NOSUID, saved IDs from the new effective IDs (XSI) | NOT TESTABLE | needs a set-ID file owned by another user |
| X22 | Shared memory detached, semaphores closed, typed memory, memory locks, mappings removed | UNTESTED | kernel |
| X23 | Scheduling for SCHED_FIFO/RR unchanged | NOT TESTABLE | no real-time privilege (RLIMIT_RTPRIO 0) |
| X24 | Per-process timers deleted | UNTESTED | kernel |
| X25 | Message queue descriptors closed | UNTESTED | Linux opens them close-on-exec (inferred) |
| X26 | AIO not affecting the new image | UNTESTED | aio area |
| X27 | CPU-time clock inherited; thread CPU-time starts at 0 | UNTESTED | kernel |
| X28 | Initial thread: deferred, enabled cancellation; no TSD; joinable | UNTESTED | follows from picolibc's start-up (inferred) |
| X29 | Inherited: nice value | TESTED (new) | execl.c:159-161 |
| X30 | Inherited: pid, ppid, process group, session | TESTED (new) | execl.c:113-118 |
| X31 | Inherited: real IDs, supplementary groups | UNTESTED | kernel; only observable with differing IDs |
| X32 | Inherited: time left on the alarm | TESTED (new) | execl.c:152-154 |
| X33 | Inherited: cwd, umask | TESTED (new) | execl.c:129-132 |
| X34 | Inherited: root directory, file size and other resource limits, controlling terminal, times | UNTESTED | kernel |
| X35 | Inherited: signal mask, pending signals | TESTED (new) | execl.c:143-147 |
| X36 | Exec from a multi-threaded process ends all threads, no destructors or cleanup handlers | TESTED (new) | execl-thread.c:150-173, 217 |
| X37 | argv and envp arrays not modified | TESTED (new, after failures) | execl.c:489-492 |
| X38 | execl and execle are async-signal-safe (XSH 2.4.3) | TESTED (new) | execl-thread.c:194-214. **Deviation 1, fixed** |
| X39 | Return -1 and set errno on failure | TESTED | every check_err in execl.c:364-487 |
| XE1 | E2BIG | TESTED (new) | execl.c:484 |
| XE2 | EACCES: not a regular file | TESTED (new) | execl.c:384 (directory), 436 (fexecve of a directory) |
| XE3 | EINVAL: recognised format the system doesn't support | NOT TESTABLE | no such binary at hand |
| XE4 | EACCES: search permission on the prefix, or no execute permission | TESTED | execl.c:383, 391 (new, skipped as root); execvp.c (existing) |
| XE5 | ELOOP: symbolic link loop | TESTED (new) | execl.c:399-400 |
| XE6 | ENAMETOOLONG: a component longer than {NAME_MAX} | TESTED (new) | execl.c:409-414. **Deviation 2, fixed** (execvp) |
| XE7 | ENOENT: missing component or empty string | TESTED (new) | execl.c:364-370 |
| XE8 | ENOTDIR: prefix not a directory, or trailing slash on a non-directory | TESTED (new) | execl.c:376-380 |
| XE9 | ENOEXEC (except execlp/execvp) | TESTED (new) | execl.c:426-430 (execv, execve, fexecve) |
| XE10 | fexecve EBADF: fd not open for executing | TESTED | execl.c:440 (new, closed descriptor); fexecve.c (existing, -1; glibc gives EINVAL) |
| XE11 | ENOMEM (may) | NOT TESTABLE | can't exhaust memory safely |
| XE12 | ELOOP beyond {SYMLOOP_MAX} (may) | UNTESTED | the loop case is checked |
| XE13 | ENAMETOOLONG for a path over {PATH_MAX} (may) | TESTED (new) | execl.c:415-419 (8 KiB path) |
| XE14 | ETXTBSY (may) | TESTED (new) | execl.c:442-468 (copy of the test open for writing; runs once closed) |

### _exit, _Exit

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | Low 8 bits available to wait | TESTED (new) | _exit.c:129, 132 |
| T2 | Full status from waitid and SIGCHLD's si_status | NOT TESTABLE | Linux keeps only 8 bits (signal unit, I6) |
| T3 | _Exit and _exit equivalent | TESTED (new) | _exit.c:127-132 |
| T4 | No atexit/at_quick_exit functions, no signal handlers | TESTED (new) | _exit.c:128, 131 |
| T5 | Streams not flushed | TESTED (new) | _exit.c:128, 131 |
| T6 | Descriptors closed | TESTED (new) | _exit.c:105-108 (EOF on the pipe) |
| T7 | SIGCHLD ignored (XSI): status discarded, ECHILD | TESTED (new) | _exit.c:181-195 (relaxed under qemu) |
| T8 | SA_NOCLDWAIT | UNTESTED | sys-wait area; qemu ignores it |
| T9 | Zombie until waited for; SIGCHLD sent to the parent | TESTED (new) | _exit.c:139-150 |
| T10 | Children inherited by a system process | TESTED (new) | _exit.c:152-179 |
| T11 | Shared memory detached, semadj applied, semaphores closed, memory locks, mappings, mqueues | UNTESTED | kernel |
| T12 | Controlling process: SIGHUP to the foreground group, terminal released | UNTESTED | termios G5 |
| T13 | Orphaned group with a stopped member gets SIGHUP and SIGCONT | UNTESTED | stopping processes in a shared test run is risky |
| T14 | Threads end without cleanup handlers or destructors | TESTED (new) | execl-thread.c:150-173, 218 |

### getpid, getppid, IDs

| # | Requirement | Status | Reference |
|---|---|---|---|
| I1 | getpid returns the caller's pid | TESTED | fork.c:104, execl.c:113 (new) |
| I2 | getppid returns the parent's pid | TESTED | fork.c:104, execl.c:115 (new); _Fork.c:87 |
| I3 | getuid/geteuid/getgid/getegid return the IDs | TESTED (existing) | getuid.c, getgid.c (file owner) |
| I4 | setuid: unprivileged, uid equal to the real or saved ID sets the effective ID only | TESTED | setuid.c:78-79 (equal IDs only) |
| I5 | setuid: privileged sets all three | NOT TESTABLE | needs privilege |
| I6 | setuid/setgid don't change the supplementary groups | TESTED (new) | setuid.c:76-83, setgid.c same lines |
| I7 | setuid/setgid EINVAL for an invalid ID | TESTED (new) | setuid.c:86, setgid.c:86 |
| I8 | setuid/setgid EPERM for another ID without privilege | TESTED (new) | setuid.c:91-92, setgid.c:91-92 |
| I9 | seteuid/setegid set the effective ID to the real or saved ID | TESTED | seteuid.c:73-75, setegid.c same (equal IDs only) |
| I10 | seteuid/setegid leave the real and saved IDs unchanged | TESTED when privileged (new) | getresuid.c:88-104, getresgid.c same; NOT TESTABLE unprivileged here. **Deviation 3, fixed** |
| I11 | seteuid/setegid EINVAL for an invalid ID | TESTED (new) | seteuid.c:78, setegid.c:78. **Deviation 3, fixed** |
| I12 | seteuid/setegid EPERM | TESTED (new) | seteuid.c:83-84, setegid.c same |
| I13 | setreuid/setregid: -1 leaves an ID alone; swap allowed | TESTED | setreuid.c:75-79, setregid.c same |
| I14 | setreuid/setregid: saved ID set when the real ID changes or effective != real | NOT TESTABLE | needs differing IDs; the kernel implements it (kernel/sys.c __sys_setreuid) |
| I15 | setreuid/setregid EINVAL for out-of-range IDs | NOT TESTABLE | in the initial user namespace only -1 is invalid, and it means "unchanged" here |
| I16 | setreuid/setregid EPERM | TESTED (new) | setreuid.c:84-87, setregid.c same |
| I17 | setresuid/setresgid: -1 leaves an ID alone; any of the three values allowed | TESTED | getresuid.c:81-85, getresgid.c same |
| I18 | setresuid/setresgid: on failure none of the IDs change | TESTED (new) | getresuid.c:108-117, getresgid.c same |
| I19 | setresuid/setresgid EPERM; EINVAL | TESTED (new, EPERM); EINVAL NOT TESTABLE | as I15 |
| I20 | getresuid/getresgid store the three IDs | TESTED | getresuid.c:79-80, 85 |
| I21 | getgroups: fills the list, returns the count | TESTED (existing) | getgroups.c:69-73 |
| I22 | getgroups(0, ...) counts without touching the array | TESTED (new) | getgroups.c:75-79 |
| I23 | getgroups EINVAL for a short array | TESTED (new) | getgroups.c:82-84 (13 groups here) |
| I24 | Whether getgroups includes the effective group ID | n/a | implementation-defined; Linux doesn't add it |
| I25 | All set*id functions are async-signal-safe (XSH 2.4.3) | UNTESTED | with threads, `_pthread_setxid` takes `_pthread_all_lock`; see "Deviations not fixed" |
| I26 | Credentials change for the whole process (all threads) | TESTED (existing) | setuid-thread.c |

### Sessions and process groups

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | setsid: new session and group led by the caller; returns the new group ID | TESTED (existing) | setsid.c:65-80 |
| S2 | setsid: no controlling terminal afterwards | TESTED (new) | tcsetpgrp.c:267-280, 333-337 (tcgetpgrp ENOTTY, /dev/tty fails) |
| S3 | setsid EPERM: caller is a group leader | TESTED (existing) | setsid.c:73-75 |
| S4 | setsid EPERM: another process's group ID equals the caller's pid | TESTED (new) | setsid.c:82-114 |
| S5 | getsid(0) and getsid(pid) return the session leader's group ID | TESTED (existing) | getsid.c:61-65 |
| S6 | getsid ESRCH | TESTED (existing) | getsid.c:66 |
| S7 | getsid/getpgid EPERM for other sessions | NOT TESTABLE | Linux allows the access (allowed by "and the implementation does not allow") |
| S8 | setpgid: new group with pgid 0, join an existing group in the session | TESTED (new) | setpgid.c:166-172 |
| S9 | setpgid: pid 0 is the caller | TESTED (new) | setpgid.c:188-199 |
| S10 | setpgid: a session leader's group doesn't change (EPERM) | TESTED (new) | setpgid.c:134-145 (session_leader_self), 181 |
| S11 | setpgid EACCES: child has exec'd | TESTED (new) | setpgid.c:230-258 |
| S12 | setpgid EINVAL: pgid < 0 | TESTED (new) | setpgid.c:175 |
| S13 | setpgid EPERM: child in another session | TESTED (new) | setpgid.c:179-181 |
| S14 | setpgid EPERM: no such group in the session | TESTED (new) | setpgid.c:177, 182 |
| S15 | setpgid ESRCH: not the caller or a child | TESTED (new) | setpgid.c:202, 204-228 (a grandchild) |
| S16 | getpgid(0)/getpgid(pid) | TESTED (existing) | getpgid.c:61-62 |
| S17 | getpgid ESRCH; EINVAL (may) | TESTED (existing, ESRCH) | getpgid.c:63 |
| S18 | getpgrp returns the group, never fails | TESTED | getpgid.c:61, fork.c:214 |
| S19 | tcgetpgrp returns the foreground group | TESTED (existing) | tcsetpgrp.c:294 |
| S20 | tcgetpgrp with no foreground group: a value > 1 naming no existing group | TESTED (new) | tcsetpgrp.c:226-264 |
| S21 | tcgetpgrp allowed from a background group | TESTED (new) | tcsetpgrp.c:127-129 |
| S22 | tcgetpgrp EBADF, ENOTTY | TESTED (existing) | tcsetpgrp.c:319-326, 351 |
| S23 | tcsetpgrp sets the foreground group | TESTED | tcsetpgrp.c:296 (existing), 146-163 (new) |
| S24 | From a background group: SIGTTOU to the group | TESTED (new) | tcsetpgrp.c:131-140 (handler runs, call fails EINTR) |
| S25 | SIGTTOU blocked or ignored: allowed, no signal | TESTED (new) | tcsetpgrp.c:141-163 |
| S26 | tcsetpgrp EBADF | UNTESTED | same ioctl path as tcgetpgrp's EBADF (inferred) |
| S27 | tcsetpgrp EINTR | TESTED (new) | tcsetpgrp.c:137-138 |
| S28 | tcsetpgrp EINVAL for an unsupported value | TESTED (existing) | tcsetpgrp.c:303-305 |
| S29 | tcsetpgrp EIO: orphaned background group, SIGTTOU not blocked or ignored | TESTED (new, picolibc) | tcsetpgrp.c:166-223. **Deviation 4, fixed**; glibc gives ENOTTY |
| S30 | tcsetpgrp ENOTTY | TESTED (existing) | tcsetpgrp.c:322-323 |
| S31 | tcsetpgrp EPERM: group in another session, or no such group | TESTED | tcsetpgrp.c:300-302 (existing), 306-313 (new, picolibc). **Deviation 4, fixed**; glibc gives ESRCH |

### getlogin, getlogin_r

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | Name associated by login with the controlling terminal | NOT TESTABLE | no utmp and no controlling terminal under meson; picolibc uses getpwuid(getuid()) (see "Deviations not fixed") |
| L2 | getlogin_r stores the same name | TESTED (existing) | getlogin.c:68 |
| L3 | getlogin_r ERANGE for a short buffer | TESTED (existing) | getlogin.c:70, getlogin_r.c:67 |
| L4 | getlogin NULL with errno on failure; EMFILE, ENFILE, ENOTTY, ENXIO (may) | TESTED (existing, the error results accepted) | getlogin.c:72, getlogin_r.c:69 |

### nice, pause, alarm, sleep

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | nice adds incr, returns the new value - NZERO | TESTED (existing) | nice.c:68, 85-86 |
| N2 | Values above the limit are set to the limit | TESTED | nice.c:88-91 (existing), 93-99 (new, INT_MAX; picolibc only, glibc overflows). **Deviation 5, fixed** |
| N3 | Values below the limit set to the limit (needs privilege) | TESTED when privileged | nice.c:100-106 |
| N4 | Only a privileged process can lower the value: EPERM, value unchanged | TESTED (new) | nice.c:70-80, 104-106 |
| N5 | nice applies to the process: all threads | TESTED (new, picolibc) | nice-thread.c:96-106. **Deviation 6, fixed**; glibc changes only the calling thread |
| N6 | No effect on SCHED_FIFO/RR | NOT TESTABLE | no real-time privilege |
| P1 | pause returns -1/EINTR after a handler | TESTED (existing) | pause.c:82-86 |
| P2 | pause doesn't return when the signal terminates the process | TESTED (new) | pause.c:89-103 |
| A1 | alarm sends SIGALRM after the time | TESTED (existing) | alarm.c:76-101 |
| A2 | alarm(0) cancels a pending request | TESTED (new) | alarm.c:104-114 |
| A3 | Requests not stacked; returns the time left of the previous one | TESTED | alarm.c:82-86 (existing), 106-109 (new) |
| A4 | Returns 0 with no previous request | TESTED | alarm.c:76-80 (existing), 111-114 (new) |
| A5 | Non-zero for any pending request, even the longest | TESTED (new) | alarm.c:116-128 (glibc on 32-bit targets fails this) |
| Z1 | sleep waits the full time; returns 0 | TESTED (new) | sleep.c:93-107 |
| Z2 | Ends on a caught signal; returns the unslept seconds | TESTED (new) | sleep.c:110-121 |
| Z3 | Ignored signals don't end it | TESTED (new) | sleep.c:95-108 |
| Z4 | A terminating signal ends the process | TESTED (new) | sleep.c:123-133 |
| Z5 | Multi-threaded: sleep doesn't use SIGALRM | UNTESTED | picolibc's sleep is nanosleep (inferred) |
| Z6 | sleep, alarm and pause are async-signal-safe | UNTESTED | each is one system call (inferred) |

### Counts

180 requirements: 133 TESTED, 29 UNTESTED, 16 NOT TESTABLE, 2 n/a (unspecified or
implementation-defined). Before this unit about 43 of them had a test (rows marked "existing", or
with an existing test alongside the new one); the other 90 TESTED rows are new. Seven deviations
were fixed (one of them, seteuid's saved ID, is only checked when the test runs as root) and five
are left (below).

## Deviations found and fixed

Each fix was verified by running it on aarch64-linux-gnu: with only the library change reversed
(`git show <fix> -- libc libos | git apply -R`, rebuild, run, reapply), the new check fails; with
the fix it passes. The failing check is quoted.

1. **execl and execle called calloc** (libc/posix/unistd/execl.c, execle.c). XSH 2.4.3 lists execl
   and execle as async-signal-safe, and fork's page says a multi-threaded process's child may only
   call async-signal-safe functions until exec. calloc takes the C library lock, which another
   thread may hold at the time of a _Fork; the child then blocks forever. The argument array is now
   built with `__builtin_alloca` (the library builds with `-Werror=vla`), with the old calloc path
   kept only for compilers without it. Failing-before: test-execl-thread "child hung" in 3 of 3
   runs (the child of _Fork never ran the program while another thread called malloc; the parent
   killed it after 10 s).
2. **execvp gave ENOENT for a name too long for any PATH entry** (libc/posix/unistd/execvpe.c).
   A name longer than PATH_MAX - 2 was skipped for every directory and the search ended with
   ENOENT. POSIX: "[ENAMETOOLONG] The length of a component of a pathname is longer than
   {NAME_MAX}" (shall fail), and "the length of the pathname constructed from the file argument
   exceeds {PATH_MAX}" (may fail). A skipped name now gives ENAMETOOLONG when nothing else was
   found. Shorter names over NAME_MAX already got ENAMETOOLONG from the kernel. Failing-before:
   `execvp(longname, argv_err): returned -1 errno 2 (No such file or directory), expected
   ENAMETOOLONG`.
3. **seteuid and setegid could change the saved ID, and accepted -1** (libos/linux/seteuid.c,
   setegid.c). They called setreuid(-1, uid)/setregid(-1, gid); Linux's setreuid sets the saved
   ID to the new effective ID whenever that differs from the real ID (v5.10 kernel/sys.c
   `__sys_setreuid`). POSIX: "the real user ID and saved set-user-ID shall remain unchanged". So a
   set-user-ID program that called seteuid(other) lost its saved ID and couldn't switch back.
   They now call setresuid(-1, id, -1)/setresgid(-1, id, -1), as glibc and musl do. To the kernel
   -1 means "unchanged", but for seteuid it is just an invalid ID; POSIX: "[EINVAL] The value of
   the uid argument is invalid". It now gives EINVAL. Failing-before (unprivileged, here):
   `an invalid ID gives EINVAL: seteuid((uid_t)-1) == -1 && errno == EINVAL`, and the same for
   setegid. The saved-ID part needs privilege: getresuid.c:88-104 checks it when run as root
   (with the old code it exits 2); it couldn't run here.
4. **tcsetpgrp's EPERM and EIO came back as ESRCH and ENOTTY** (libos/linux/tcsetpgrp.c). Linux's
   TIOCSPGRP returns ESRCH for a group that doesn't exist, and turns tty_check_change's EIO for an
   orphaned background group into ENOTTY (v5.10 tty_jobctrl.c tiocspgrp: `if (retval == -EIO)
   return -ENOTTY;`). POSIX: "[EPERM] ... does not match the process group ID of a process in the
   same session" and "[EIO] The process group of the writing process is orphaned, the calling
   thread is not blocking SIGTTOU, and the process is not ignoring SIGTTOU". ESRCH now becomes
   EPERM. ENOTTY becomes EIO when the descriptor is the caller's controlling terminal (TIOCGPGRP,
   which checks only that, succeeds on it) and isn't a pty manager (TIOCGPTN fails); every other
   ENOTTY path in tiocspgrp needs the descriptor not to be the caller's controlling terminal.
   Failing-before: `child check 12 failed` (EPERM) with the whole fix reversed, and `child check
   44 failed` (EIO) with only the EIO mapping removed.
5. **nice(INT_MAX) overflowed** (libos/linux/nice.c). `prio += incr` overflowed for a positive
   nice value, wrapped negative and asked for -20. POSIX: "Requests for values above or below
   these limits shall result in the nice value being set to the corresponding limit". incr is now
   limited to ±39 first. glibc has the same overflow, so that check is picolibc-only.
   Failing-before: `nice(INT_MAX): errno == 0 && ret == 19` (it returned -1/EPERM).
6. **nice changed only the calling thread** (libos/linux/nice.c). Linux nice values are per
   thread. POSIX: "The nice value set with nice() shall be applied to the process. If the process
   is multi-threaded, the nice value shall affect all system scope threads in the process." With
   more than one thread, nice now sets the new value in every listed thread (setpriority by tid)
   while holding `_pthread_all_lock`, which pthread_create holds across clone and listing, so no
   new thread is missed. glibc behaves like the old code; the check is guarded. Failing-before:
   `the other thread has the new value: seen == prio + 1`.
7. **`<unistd.h>` declared removed interfaces and used application names**
   (libc/include/sys/unistd.h). setpgrp (removed in Issue 8) was declared for
   `_XOPEN_SOURCE 800`, and ualarm (removed in Issue 7) for `_POSIX_C_SOURCE 202405L` alone,
   both names POSIX doesn't reserve. getlogin_r's parameters were `name` and `namesize`, so an
   application macro of either name broke the header. Failing-before (compile):
   `test-unistd_h-names.c: expected ';', ',' or ')' before '+' token`, `'ualarm' redeclared
   as different kind of symbol`, and in test-unistd_h-process.c `'setpgrp' redeclared as
   different kind of symbol`.

## Deviations not fixed

- **getlogin and getlogin_r return the password-file name for the real user ID**
  (libc/posix/unistd/getlogin.c, getlogin_r.c). POSIX wants "the user name associated by the
  login activity with the controlling terminal", the name the user logged in under "even if there
  are several login names with the same user ID", and allows ENOTTY/ENXIO without a controlling
  terminal. picolibc answers even with no terminal, and gives the first passwd entry for the uid.
  A real implementation would look up ttyname(0..2) in utmpx, which belongs with the users unit's
  utmpx work; the meson tests have neither utmp nor a terminal, so the existing tests accept both
  results.
- **tcgetpgrp works on a pty manager** (libos/linux/tcgetpgrp.c). Linux's TIOCGPGRP on the manager
  side reports the subsidiary's foreground group to any process (tiocgpgrp checks the controlling
  terminal only for `tty == real_tty`). POSIX: "[ENOTTY] ... the file is not the controlling
  terminal", and a manager is never one. glibc and musl pass it through, and terminal emulators
  rely on it, so I left it; a fix would fail with ENOTTY when TIOCGPTN succeeds on the descriptor.
- **set\*id functions with threads aren't async-signal-safe.** XSH 2.4.3 lists setuid, seteuid,
  setgid, setegid, setreuid, setregid, setresuid and setresgid. With more than one thread,
  `_pthread_setxid` takes `_pthread_all_lock` (a pthread mutex), so a signal handler that calls one
  while the interrupted thread is in pthread_create or pthread_exit deadlocks. glibc has the same
  shape. Fixing it means blocking signals around the lock in every holder; it is the
  pthread-threads area's lock, so I left it.
- **`environ` is declared in `<unistd.h>`** for `_POSIX_C_SOURCE 202405L`. Commit 7e8126df52 did
  this on purpose ("POSIX.1-2024 requires unistd.h to declare environ"), but the 2024 `<unistd.h>`
  page lists only optarg, opterr, optind and optopt, and XBD says "All identifiers in this volume
  of POSIX.1-2024, except environ, are defined in at least one of the headers". It does no harm to
  a program that declares environ itself. Left for the maintainer.
- **sleep rounds the unslept time down** (libc/posix/unistd/sleep.c). With 0.9 s left it returns
  0, the same as a full sleep. POSIX only says "in seconds"; glibc and musl also truncate, FreeBSD
  rounds up. Not changed.

## Other observations (not changed)

- setpgrp and ualarm are declared under `_DEFAULT_SOURCE` but neither picolibc nor libos defines
  them, so a program that uses them fails to link.
- `libos/linux/sleep.c` contains only a licence comment and isn't in any build file.
- `<unistd.h>` declares `dup3(..., int flags)` and `pipe2(..., int flags)` in strict mode, so an
  application macro named `flags` breaks the header. Those belong to unistd-files (recorded in
  cross-cutting.md).
- On the Linux configurations `stdio-exit-flush` is off, so exit() doesn't flush streams opened
  with fopen or fdopen (ISO C requires it); test-_exit.c only checks that exit runs atexit
  functions. For the stdio unit (cross-cutting.md).
- glibc 2.43 differences met here, guarded with `__GLIBC__`: nice overflows for large increments
  and changes only the calling thread; tcsetpgrp gives ESRCH and ENOTTY for EPERM and EIO; on
  32-bit targets setitimer fails with EOVERFLOW for UINT_MAX seconds, so alarm(UINT_MAX) sets no
  alarm and alarm(0) then returns 0; no O_EXEC, so the fexecve O_EXEC case is skipped; fexecve(-1)
  gives EINVAL; _Fork, getresuid and friends need `_GNU_SOURCE`.
- picolibc's opendir uses O_CLOEXEC, so directory streams are closed by exec, as checked.
- vfork is still defined in libos/linux (declared only for BSD and old XSI), which is fine.

## Left undone

- The UNTESTED kernel inheritance rows (X13, X15, X22, X24, X27, X31, X34, F9, F13, F14, F17,
  F18, T11) and the other units' areas they touch.
- setreuid/setresuid EINVAL and the privileged ID paths need a user namespace with more than one
  mapped ID, or root.
- T12, T13 (hang-up and orphaned stopped groups) were not attempted.

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`), full `meson test -t 4`
  at the branch tip: 1560 tests; 1511 Ok, 0 Fail, 6 Expected Fail, 43 Skipped.
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`), full
  `meson test -t 4` at the branch tip: 1561 tests; 1510 Ok, 0 Fail, 6 Expected Fail, 45 Skipped.
- Every new and changed test passes on both, for picolibc and as `-native` against glibc; the
  only partial skip is the fexecve O_EXEC case in test-execl-native (glibc has no O_EXEC).
- Native configuration (`scripts/do-native-configure -Dwerror=true`, `ninja` only): builds
  cleanly.
- CMake (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 584 tests, 100% passed (256
  ran, 328 skipped). test-unistd_h-names and test-unistd_h-process run there (the type checks use
  `_Generic`, so they need no process functions at link time); the behaviour tests skip.
- Bare-metal meson targets weren't run here; the x86_64 host does them.
- `clang-format --dry-run --Werror` is clean on every C file and header the branch touches.
- Fix proofs (each fix's library change reversed, aarch64): listed with each deviation above.
  The execl fix failed 3 of 3 runs without it ("child hung" after 10 s).
- No host security software kills were seen; no other incidents. The exec tests use only the
  test binary (/proc/self/exe) and temporary directories under the build directory, every child
  is reaped (orphaned grandchildren end when a pipe closes), and no signal goes to any process
  group the tests didn't create (the only group probe is `kill(-pid, 0)`, which sends nothing).

## Commits (audit/unistd-process, on top of 40921d6cce)

- a0bc3998f9 unistd: Build the argv of execl and execle without malloc
- 717273e4a7 unistd: Report ENAMETOOLONG from execvp for names too long to try
- 59034c2234 libos/linux: Keep the saved IDs in seteuid and setegid
- ea41dc3695 libos/linux: Map the tcsetpgrp errors Linux reports differently
- 47dc1016cc libos/linux: Limit the nice increment so the sum can't overflow
- a329a111c0 libos/linux: Apply nice to every thread
- 45430fd499 unistd.h: Hide setpgrp and ualarm, and use reserved names in getlogin_r
- 1ebe7da1ca test-posix: Check what the exec functions pass on, and their errors
- c3105ba246 test-posix: Check execl and execle in the child of _Fork with threads
- 867d2f2a85 test-posix: Check the errors of the set-ID functions
- 248b0b0dbf test-posix: Check nice's limits and that it changes every thread
- 3c24bfdb49 test-posix: Check setpgid, setsid and tcsetpgrp in sessions and groups
- 5a4a2ab986 test-posix: Check what fork copies and resets, and how _exit ends a process
- 464ed65a52 test-posix: Check sleep, and alarm(0) and pause ending the process
- 69e0e524f7 test-posix: Check the process declarations in <unistd.h>
