# `<signal.h>` audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-signal`, branch `audit/signal`, based on
2d4e737cfc (`claude/posix-spec-audit`). Host: aarch64 Linux (AWS Graviton2, kernel
5.10). Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43).

Implementation: `libos/linux/` (sigaction.c, signal.c, _sigprocmask.c, sigprocmask.c,
pthread_sigmask.c, pthread_kill.c, kill.c, killpg.c, raise.c, sigqueue.c,
_sigtimedwait.c, sigtimedwait.c, sigwait.c, sigwaitinfo.c, sigsuspend.c, sigpending.c,
sigaltstack.c, _siginfo_from_linux.c, and the `_signal_*`/`_sigmask_*` converters);
`libc/signal/` (set operations, sig2str.c, psignal.c, psiginfo.c); and the headers
`libc/include/signal.h` and `libc/include/sys/syslimits.h` (NSIG_MAX).
Tests: `test/test-posix/test-signal/`. Paths below are relative to that directory
unless they start with `test-pthread/`. Line numbers are for the branch tip.

Spec source: the pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/signal.h and
functions/sigaction, sigprocmask (covers pthread_sigmask), sigaddset, sigdelset,
sigemptyset, sigfillset, sigismember, kill, killpg, raise, signal, sigqueue, sigwait,
sigtimedwait (covers sigwaitinfo), sigsuspend, sigpending, sigaltstack, psignal
(covers psiginfo), pthread_kill and sig2str (covers str2sig).

Status key: TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row was
run and passes for picolibc on aarch64-linux-gnu and arm-linux-gnueabihf (`test-X`),
and against glibc on both (`test-X-native`), unless the row says picolibc-only.
"Verified" means I ran it; "inferred" means I only read the code.

## Requirement table

### Header (basedefs/signal.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | SIG_DFL, SIG_ERR and SIG_IGN are distinct, have the type signal() takes and returns, and differ from any function | TESTED | test-signal_h.c:45, 233-235 |
| H2 | pthread_t, size_t, uid_t, pid_t, pthread_attr_t, sig_atomic_t, sigset_t and struct timespec defined | TESTED | test-signal_h.c:40, 140 (pthread types only where threads exist) |
| H3 | struct sigevent members and types, including `pthread_attr_t *sigev_notify_attributes` | TESTED | test-signal_h.c:52, 141. **Was a deviation (void \*); fixed** |
| H4 | SIGEV_NONE, SIGEV_SIGNAL and SIGEV_THREAD distinct | TESTED | test-signal_h.c:57 |
| H5 | union sigval members | TESTED | test-signal_h.c:62 |
| H6 | SIGRTMIN and SIGRTMAX are positive ints; the range holds at least RTSIG_MAX signals and doesn't overlap the table's signals | TESTED | test-signal_h.c:78, 241, 247-251 (the RTSIG_MAX check is picolibc-only: glibc's RTSIG_MAX of 32 counts its two reserved signals) |
| H7 | SIGRTMAX < sysconf(_SC_NSIG) | UNTESTED | picolibc has no `_SC_NSIG` (cross-cutting); test-signal_h.c:258 checks it where it's defined (glibc) |
| H8 | SIG2STR_MAX defined and usable in #if | TESTED (picolibc only; glibc lacks it) | test-signal_h.c:66-68. **Was a deviation (only visible with `__MISC_VISIBLE`); fixed** |
| H9 | The 27 required signals are distinct positive int constants below NSIG_MAX | TESTED | test-signal_h.c:72-78, 238-244, 255. **NSIG_MAX was a deviation; fixed** |
| H10 | struct sigaction members and types | TESTED | test-signal_h.c:85 |
| H11 | SIG_BLOCK, SIG_UNBLOCK and SIG_SETMASK distinct | TESTED | test-signal_h.c:92 |
| H12 | SA_NOCLDSTOP, SA_ONSTACK, SA_RESETHAND, SA_RESTART, SA_SIGINFO, SA_NOCLDWAIT, SA_NODEFER | TESTED | test-signal_h.c:96-98, 262-263 |
| H13 | SS_ONSTACK, SS_DISABLE, MINSIGSTKSZ, SIGSTKSZ | TESTED | test-signal_h.c:101; the kernel accepting MINSIGSTKSZ is checked at test-sigaltstack.c:126 |
| H14 | mcontext_t, and ucontext_t with uc_link, uc_sigmask, uc_stack and uc_mcontext | UNTESTED | **Deviation, not fixed:** neither type exists (see below) |
| H15 | stack_t members | TESTED | test-signal_h.c:105 |
| H16 | siginfo_t members and types (si_signo, si_code, si_errno, si_pid, si_uid, si_addr, si_status, si_value) | TESTED | test-signal_h.c:110 |
| H17 | si_code constants ILL_\*, FPE_\*, SEGV_\*, BUS_\*, TRAP_\*, CLD_\* and SI_\*, distinct within each signal | TESTED | test-signal_h.c:121-132, 265-270. **Was a deviation (the ILL, FPE, SEGV, BUS and TRAP codes were missing); fixed** |
| H18 | Prototypes for the 24 functions | TESTED | test-signal_h.c:144-188 (by initialising function pointers; sig2str and str2sig are picolibc-only) |

### Signal-specific information (basedefs/signal.h, XSH 2.4.3)

| # | Requirement | Status | Reference |
|---|---|---|---|
| I1 | SIGSEGV si_code is SEGV_ACCERR or SEGV_MAPERR | TESTED | test-si_code.c:110-117. **Was a deviation (codes reported as 0); fixed** |
| I2 | SIGBUS si_code is BUS_ADRERR | TESTED | test-si_code.c:129-130 (same fix) |
| I3 | si_addr is the faulting address (SIGSEGV, SIGBUS) | TESTED | test-si_code.c:112, 118, 131 |
| I4 | SIGILL, SIGFPE and SIGTRAP codes and si_addr | UNTESTED | No portable way to raise these faults (aarch64 doesn't trap on integer division). The mapping uses the same code path as I1 (inferred) |
| I5 | SI_USER from kill and SI_QUEUE from sigqueue, with si_pid and si_uid | TESTED | test-sigaction.c:161-166, test-sigqueue.c:94-96, test-si_code.c:139 |
| I6 | SIGCHLD si_status holds the full exit value, not 8 bits | NOT TESTABLE | Linux puts only the low 8 bits in si_status (kernel limitation) |

### sigaction

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | Installs the action and returns 0 | TESTED | test-sigaction.c:159-162 |
| A2 | With a null act, handling is unchanged and oact gets the current action | TESTED | test-sigaction.c:168-184. **Was a deviation; fixed** (deviation 1) |
| A3 | oact gets the previous action (handler, sa_mask, sa_flags) when a new one is installed | TESTED | test-sigaction.c:176-178, 190 |
| A4 | oact reports SIG_IGN and SIG_DFL | TESTED | test-sigaction.c:194-195 |
| A5 | An SA_SIGINFO handler gets (signo, siginfo_t \*, context) | TESTED | test-sigaction.c:161-166 |
| A6 | The mask in the handler is current ∪ sa_mask ∪ sig, and is restored on return | TESTED | test-sigaction.c:202-206 |
| A7 | SA_NODEFER: sig isn't added to the mask | TESTED | test-sigaction.c:210-213 |
| A8 | SA_RESETHAND: the disposition is reset to SIG_DFL on entry | TESTED | test-sigaction.c:217-221. **Was a deviation (oact reported the stale handler); fixed** (deviation 2) |
| A9 | SA_RESETHAND: SA_SIGINFO is cleared on entry | UNTESTED | Linux keeps SA_SIGINFO in the flags (kernel); see below |
| A10 | SA_RESETHAND doesn't reset SIGILL and SIGTRAP | NOT TESTABLE | Linux resets them anyway (kernel); see below |
| A11 | SA_RESTART restarts an interrupted function; without it, the function fails with EINTR | TESTED | test-sigaction.c:241-242 (read on a pipe, interrupted by alarm(1)) |
| A12 | SA_NOCLDSTOP: no SIGCHLD when a child stops | TESTED | test-sigaction.c:248-263 |
| A13 | SA_NOCLDWAIT: no zombies, so wait fails with ECHILD | TESTED | test-sigaction.c:272-278 |
| A14 | SA_ONSTACK delivers on the alternate stack | TESTED | test-sigaltstack.c:109 |
| A15 | SIGKILL and SIGSTOP are silently left out of the mask | TESTED | test-sigprocmask.c:81-83 (through sigprocmask; the kernel applies the same rule) |
| A16 | A failed call installs nothing | TESTED | test-sigaction.c:237 |
| A17 | SA_SIGINFO signals are queued in FIFO order with their values | TESTED | test-rtsig.c:121-126 |
| A18 | An action set by signal() is restored when its oact is passed back | UNTESTED | inferred: signal() is sigaction() with SA_RESTART |
| E-A1 | EINVAL: invalid signal (negative, past SIGRTMAX) | TESTED | test-sigaction.c:227-229 |
| E-A2 | EINVAL: catching SIGKILL, ignoring SIGSTOP | TESTED | test-sigaction.c:231-234 |
| E-A3 | Querying SIGKILL with a null act succeeds | TESTED | test-sigaction.c:235 |
| E-A4 | EINVAL for setting SIGKILL to SIG_DFL (may fail) | Not asserted | optional; Linux accepts it |

### signal

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | Returns the func of the most recent call (SIG_DFL the first time) | TESTED | test-signal.c:58-73, 83 |
| G2 | SIG_IGN ignores the signal; a handler is called | TESTED | test-signal.c:64, 76-79 |
| G3 | errno is unchanged on success | TESTED | test-signal.c:71-73 |
| G4 | SIG_ERR and EINVAL for an invalid signal, catching SIGKILL, or ignoring SIGSTOP | TESTED | test-signal.c:88-101 |
| G5 | A signal ignored at program start (inherited SIG_IGN) is reported as SIG_IGN | UNTESTED | needs exec; the A8 fix (taking SIG_IGN from the kernel) covers it (inferred) |
| G6 | Thread-safe | NOT TESTABLE | inferred: the handler table is updated atomically |

### sigprocmask / pthread_sigmask

| # | Requirement | Status | Reference |
|---|---|---|---|
| M1 | SIG_BLOCK forms the union | TESTED | test-sigprocmask.c:95-100 |
| M2 | SIG_UNBLOCK removes only those signals | TESTED | test-sigprocmask.c:103-106 |
| M3 | SIG_SETMASK replaces the mask | TESTED | test-sigprocmask.c:107-110 |
| M4 | oset gets the previous mask | TESTED | test-sigprocmask.c:75, 95 |
| M5 | With a null set, how doesn't matter and the mask is unchanged | TESTED | test-sigprocmask.c:113-121. **Was a deviation (EINVAL); fixed** |
| M6 | A pending signal that is unblocked is delivered before the call returns | TESTED | test-sigprocmask.c:78, test-sigpending.c:77 |
| M7 | SIGKILL and SIGSTOP can't be blocked; no error | TESTED | test-sigprocmask.c:81-83 |
| M8 | A failed call leaves the mask unchanged | TESTED | test-sigprocmask.c:85-87 |
| M9 | pthread_sigmask returns the error number; the mask is per thread | TESTED | test-sigprocmask.c:121, test-pthread/test-pthread_sigmask.c:107, 114 |
| E-M1 | EINVAL: non-null set and an invalid how | TESTED | test-sigprocmask.c:85, 121 |
| E-M2 | Never EINTR | NOT TESTABLE | inferred: rt_sigprocmask doesn't block |

### Signal sets (sigemptyset, sigfillset, sigaddset, sigdelset, sigismember)

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | sigemptyset excludes every signal and returns 0 | TESTED | test-sigismember.c:73-84 (both the functions and the macros) |
| S2 | sigfillset includes every signal and returns 0 | TESTED | test-sigismember.c:74-84 |
| S3 | sigaddset adds only that signal and returns 0 | TESTED | test-sigismember.c:89-98 |
| S4 | sigdelset deletes only that signal and returns 0 | TESTED | test-sigismember.c:100-111 |
| S5 | sigismember returns 1 or 0 | TESTED | test-sigismember.c:77-111 |
| E-S1 | EINVAL for invalid signals (may fail) | Not asserted | optional; the macros don't check the number (signal 64 and above is an out-of-range shift) |

### kill / killpg

| # | Requirement | Status | Reference |
|---|---|---|---|
| K1 | Signal 0 only checks | TESTED | test-kill.c:76 |
| K2 | pid > 0 sends to that process; a signal to the sender is delivered before kill returns | TESTED | test-kill.c:86-87 |
| K3 | pid 0 sends to the caller's process group; a negative pid sends to that group | TESTED | test-kill.c:91-93, test-killpg.c:85-90 |
| K4 | pid -1 sends to every process the caller may signal | NOT TESTABLE | it would signal every process of the user |
| K5 | Permission: the real or effective uid must match | TESTED | test-kill.c:110-113 (EPERM sending to init; skipped as root) |
| K6 | SIGCONT exemption within the session | NOT TESTABLE | needs another user's process in the session |
| E-K1 | EINVAL | TESTED | test-kill.c:78-80, test-killpg.c:97 |
| E-K2 | EPERM | TESTED | test-kill.c:113 |
| E-K3 | ESRCH | TESTED | test-kill.c:98-104, test-killpg.c:95 |
| KP1 | killpg(pgrp, sig) is kill(-pgrp, sig) | TESTED | test-killpg.c:85-97 |

### raise

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | Sends to the calling thread, like pthread_kill(pthread_self(), sig) | TESTED | test-raise-thread.c:110-120 |
| R2 | Doesn't return until the handler has | TESTED | test-raise-thread.c:84-88, 106 |
| R3 | Returns 0 on success | TESTED | test-raise-thread.c:106 |
| R4 | A blocked signal stays pending for the thread | TESTED | test-raise-thread.c:124-129 |
| E-R1 | Non-zero return and EINVAL for an invalid signal | TESTED | test-raise-thread.c:132-135 |

### pthread_kill

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | Delivers to the given thread | TESTED | test-pthread/test-pthread_kill.c:96-100 |
| P2 | Not an error for a zombie (exited, unjoined) thread | TESTED | test-pthread/test-pthread_kill.c:105-117. **Was a deviation (ESRCH); fixed** |
| P3 | Signal 0 checks the thread | TESTED | test-pthread/test-pthread_kill.c:101, 111 |
| E-P1 | EINVAL for a non-zero invalid signal, and no signal sent | TESTED | test-pthread/test-pthread_kill.c:102-103, 115 (the exited-thread case is picolibc-only: glibc returns 0 without checking the signal) |
| E-P2 | Never EINTR | NOT TESTABLE | inferred: tgkill doesn't block |

### sigqueue

| # | Requirement | Status | Reference |
|---|---|---|---|
| Q1 | Sends with a value; the SA_SIGINFO handler gets si_value (int and pointer) | TESTED | test-sigqueue.c:92-96, 105-106 |
| Q2 | Signal 0 only checks | TESTED | test-sigqueue.c:97 |
| Q3 | Delivered to the caller before return when not blocked | TESTED | test-sigqueue.c:93 |
| Q4 | Real-time signals are queued; the lowest number goes first | TESTED | test-rtsig.c:105-126, test-sigtimedwait.c:133-141 |
| E-Q1 | EAGAIN once {SIGQUEUE_MAX} signals are queued | UNTESTED | needs exhausting RLIMIT_SIGPENDING; kernel behaviour |
| E-Q2 | EINVAL | TESTED | test-sigqueue.c:99-101 |
| E-Q3 | EPERM | TESTED | test-sigqueue.c:108-112 (skipped as root) |
| E-Q4 | ESRCH | TESTED | test-sigqueue.c:113-115 |

### sigwait / sigwaitinfo / sigtimedwait

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | sigwait returns a pending signal from set, clears it, and returns 0 | TESTED | test-sigwait.c:103-106 |
| W2 | Waits when nothing is pending | TESTED | test-sigwaitinfo.c:105-110 (sigqueue from another process) |
| W3 | The lowest real-time signal goes first; queued values come out FIFO | TESTED | test-rtsig.c:121-126, test-sigtimedwait.c:139-141 |
| W4 | sigwaitinfo and sigtimedwait return the signal; info gets si_signo, si_code and si_value | TESTED | test-sigwaitinfo.c:107-108, test-sigtimedwait.c:110-111, 139 |
| W5 | A null info is accepted | TESTED | test-sigtimedwait.c:142 |
| W6 | The pending indication is reset when nothing more is queued | TESTED | test-sigtimedwait.c:143 |
| W7 | sigtimedwait fails with EAGAIN when the timeout expires | TESTED | test-sigtimedwait.c:113-116 |
| W8 | A zero timeout returns at once | TESTED | test-sigtimedwait.c:125-128 |
| W9 | The timeout is measured on CLOCK_MONOTONIC | NOT TESTABLE | kernel hrtimer; can't be told apart without changing CLOCK_REALTIME |
| W10 | Only the thread a pthread_kill targets returns from sigwait | UNTESTED | needs two waiting threads |
| E-W1 | EINVAL for a bad tv_nsec (may fail) | TESTED | test-sigtimedwait.c:117-122 |
| E-W2 | ... checked only when no signal is pending ("should") | TESTED (picolibc only) | test-sigtimedwait.c:146-159. **Was a deviation; fixed** (glibc passes the timeout to Linux, which checks it first) |
| E-W3 | EINTR (may fail) | UNTESTED | optional; Linux returns it for sigwaitinfo and sigtimedwait (inferred) |
| E-W4 | sigwait EINVAL for invalid signals (may fail) | Not asserted | optional; invalid signals are dropped from the set |

### sigsuspend / sigpending

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | Replaces the mask and waits for a caught signal | TESTED | test-sigsuspend.c:81-88 |
| U2 | Returns -1 with EINTR after the handler, with the mask restored | TESTED | test-sigsuspend.c:87-90 |
| U3 | With a terminating action, never returns | UNTESTED | inferred: the process dies in the kernel |
| U4 | SIGKILL and SIGSTOP can't be blocked; no error | UNTESTED | the kernel enforces it (inferred) |
| SP1 | sigpending reports blocked signals pending on the thread | TESTED | test-sigpending.c:74-78 |
| SP2 | ... and on the process | TESTED | test-sigpending.c:82-88 |

### sigaltstack

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | Installs a stack; SA_ONSTACK handlers run on it | TESTED | test-sigaltstack.c:101-109 |
| T2 | oss reports sp, size and flags, with SS_DISABLE when there's no stack | TESTED | test-sigaltstack.c:97, 102 |
| T3 | oss has SS_ONSTACK while running on the stack | TESTED | test-sigaltstack.c:110 |
| T4 | SS_DISABLE disables the stack; ss_sp and ss_size are ignored | TESTED | test-sigaltstack.c:114-117, 129-135 |
| T5 | A MINSIGSTKSZ stack is accepted | TESTED | test-sigaltstack.c:124-127. **Was a deviation on aarch64; fixed** |
| T6 | No alternate stack after exec | UNTESTED | needs exec; kernel behaviour |
| E-T1 | EINVAL: SS_ONSTACK in ss_flags | TESTED (picolibc only) | test-sigaltstack.c:145-149. **Was a deviation; fixed** (glibc passes it to Linux, which accepts it) |
| E-T2 | EINVAL: unknown flags | TESTED | test-sigaltstack.c:137-144 |
| E-T3 | ENOMEM: smaller than MINSIGSTKSZ | TESTED | test-sigaltstack.c:118-120 |
| E-T4 | EPERM: changing the active stack | TESTED | test-sigaltstack.c:111 |

### psignal / psiginfo

| # | Requirement | Status | Reference |
|---|---|---|---|
| PS1 | Writes "message: description\n" | TESTED | test-psiginfo.c:94, 100 |
| PS2 | A null or empty message writes only the description | TESTED | test-psiginfo.c:97, 103-105 |
| PS3 | psiginfo uses pinfo's signal | TESTED | test-psiginfo.c:94-97 |
| PS4 | The orientation of stderr is unchanged | TESTED | test-psiginfo.c:109-118 (a wide stream stays wide and gets the text) |
| PS5 | errno is unchanged on success | TESTED | test-psiginfo.c:81-86 |
| PS6 | On error, the stream's error indicator and errno are set | UNTESTED | needs a failing stderr |
| PS7 | File timestamps are marked for update | NOT TESTABLE | follows from stdio (inferred) |

### sig2str / str2sig (picolibc only: glibc has neither)

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | Table signals give their name without SIG, and return 0 | TESTED | test-sig2str.c:104-106 |
| N2 | SIGRTMIN and SIGRTMAX give "RTMIN" and "RTMAX" | TESTED | test-sig2str.c:113-114 |
| N3 | The lower half gives "RTMIN+n"; the upper half gives "RTMIN+n" or "RTMAX-m" | TESTED | test-sig2str.c:117-127 |
| N4 | The results fit SIG2STR_MAX and translate back | TESTED | test-sig2str.c:135-142 |
| N5 | -1 for invalid numbers | TESTED | test-sig2str.c:145-146 |
| N6 | sig2str returns -1 exactly when kill would fail with EINVAL | UNTESTED; deviation, not fixed | SIGEMT and SIGLOST have names but Linux doesn't have them (see below) |
| N7 | str2sig takes table names, RTMIN, RTMAX, RTMIN+n and RTMAX-n (n from 1 to SIGRTMAX-SIGRTMIN-1) | TESTED | test-sig2str.c:107, 115-116, 128-133 |
| N8 | str2sig takes decimal signal numbers | TESTED | test-sig2str.c:108-109 |
| N9 | str2sig stores nothing for strings it doesn't recognise | TESTED | test-sig2str.c:147-156. **Was a deviation (accepted "63", " 2" and "+2"); fixed** |

### Counts

143 requirements:
- TESTED: 117. Of these, 11 are picolibc-only (H8, E-W2, E-T1 and N1-N5, N7-N9), and
  2 have one part that is picolibc-only (H6, E-P1).
- UNTESTED: 14 (H7, H14, I4, A9, A18, G5, E-Q1, W10, E-W3, U3, U4, T6, PS6, N6;
  H14 and N6 are deviations that weren't fixed).
- NOT TESTABLE: 9 (I6, A10, G6, E-M2, K4, K6, E-P2, W9, PS7).
- Optional errors deliberately not asserted: 3 (E-A4, E-S1, E-W4).

Before this audit, 45 of these rows were tested (by the 14 existing tests in
test-signal and the two pthread tests). Now 117 are.

## Deviations found and fixed

Each fix was verified by running. On aarch64-linux-gnu, the new check fails when only
the library change is reverted (the message is quoted below) and passes with the fix.

1. **sigaction(sig, NULL, &oact) replaced the handler with garbage**
   (libos/linux/sigaction.c). The local `func` was set only when `act` was non-null,
   but it was always stored in `_sig_func[sig]`, the table the trampolines call
   through. POSIX: "If the argument act is a null pointer, signal handling is
   unchanged." Depending on what a register held, the next signal called a stray
   function. Fix: update the table only when act is given. Failing-before:
   `handler still installed after a query: raise(SIGUSR1) == 0 && caught == SIGUSR1`.
   Whether the check catches the bug depends on the stray register's contents, so the
   test calls sigaction from a `noinline, noclone` helper that keeps five function
   pointers live. With that helper it fails reliably on aarch64 (-Os). Without it, the
   check passed even with the bug present.
2. **oact reported a stale handler after SA_RESETHAND** (libos/linux/sigaction.c).
   oact->sa_handler came from picolibc's table, which doesn't see the kernel reset the
   disposition on delivery, or SIG_IGN inherited across exec. POSIX (SA_RESETHAND):
   "the disposition of the signal shall be reset to SIG_DFL". Fix: take SIG_DFL and
   SIG_IGN from the kernel's old action. Failing-before:
   `SA_RESETHAND resets the disposition: sigaction(SIGUSR1, NULL, &old) == 0 && old.sa_handler == SIG_DFL`.
3. **sigprocmask and pthread_sigmask returned EINVAL for a bad how with a null set**
   (libos/linux/_sigprocmask.c). POSIX: "If set is a null pointer, the value of the
   argument how is not significant"; EINVAL is for "The set argument is not a null
   pointer and the value of the how argument is not equal to one of the defined
   values". Failing-before: `how ignored without a set: sigprocmask(42, NULL, &cur) == 0`.
4. **MINSIGSTKSZ was too small on aarch64** (libc/include/signal.h and the new
   libos/linux/include/sys/_signal.h). MINSIGSTKSZ was 2048 on every target, but the
   aarch64 kernel rejects stacks under 5120 bytes with ENOMEM, and POSIX allows ENOMEM
   only for a stack "less than MINSIGSTKSZ". Fix: libos/linux supplies MINSIGSTKSZ and
   SIGSTKSZ in `<sys/_signal.h>`: 5120 and 16384 on aarch64 (matching glibc), 2048 and
   8192 elsewhere. `<signal.h>` includes the header when present; it is installed next
   to `<sys/_posix.h>`. Failing-before: `MINSIGSTKSZ accepted: sigaltstack(&ss, NULL) == 0`.
5. **sigaltstack accepted SS_ONSTACK** (libos/linux/sigaltstack.c). POSIX: "[EINVAL]
   ... ss_flags ... has SS_ONSTACK or invalid flags set." Linux treats SS_ONSTACK as
   0. Failing-before: `SS_ONSTACK: sigaltstack(&ss, NULL) == -1 && errno == EINVAL`.
   glibc behaves like unfixed picolibc, hence the `__PICOLIBC__` guard.
6. **pthread_kill returned ESRCH for an exited, unjoined thread**
   (libos/linux/pthread_kill.c). POSIX: "It shall not be an error if thread is a
   zombie thread." Fix: return 0 once the kernel has cleared the tid. The signal number
   is now checked with `_signal_to_linux` first, so invalid numbers are still EINVAL.
   Failing-before: `signal 0 to an exited thread: pthread_kill(zombie, 0) == 0`.
   The test waits 100 ms for the thread to exit. If it hasn't exited by then, the
   checks still pass; they just don't exercise the zombie path.
7. **The fault si_code values were missing** (libc/include/signal.h and
   libos/linux/_siginfo_from_linux.c). `<signal.h>` had no ILL_\*, FPE_\*, SEGV_\*,
   BUS_\* or TRAP_\* constants, and the kernel's codes became 0. Fix: define them
   (from 16 upwards, like CLD_\*) and map Linux's 1-based values. Failing-before: with
   the whole fix reverted, the test doesn't compile; with only the mapping reverted,
   `SEGV_ACCERR: code == SEGV_ACCERR`.
8. **SIG2STR_MAX wasn't visible with `_POSIX_C_SOURCE` 202405L**
   (libc/include/signal.h), although sig2str and str2sig were. Failing-before:
   `#error no SIG2STR_MAX` in a strict-POSIX compile, checked by hand.
   test-signal_h.c uses `_XOPEN_SOURCE` 800, which shows the macro either way.
9. **sigev_notify_attributes was void \*** (libc/include/signal.h); POSIX declares it
   `pthread_attr_t *`. It stays void \* on targets without thread types.
   Failing-before: `static assertion failed: "sigev_notify_attributes is a pthread_attr_t pointer"`.
10. **NSIG_MAX was `__LONG_WIDTH__`** (libc/include/sys/syslimits.h): 32 on 32-bit
    targets, where signals go up to 62, and visible only with `__MISC_VISIBLE`. POSIX:
    signal numbers are "less than the value of {NSIG_MAX}". It is now 64 (sigset_t is
    64 bits on every target) and visible for POSIX.1-2024. Failing-before:
    `'NSIG_MAX' undeclared`.
11. **str2sig accepted strings that aren't signals** (libc/signal/sig2str.c): "63",
    which is past SIGRTMAX, and, through strtoul, " 2", "+2" and "RTMIN+ 1". POSIX:
    str2sig stores a value only for the listed forms or a form the implementation
    recognises. Failing-before: `number past SIGRTMAX: rejected(want)`.
12. **sigtimedwait checked the timeout even with a signal pending**
    (libos/linux/_sigtimedwait.c). POSIX: "An implementation should only check for this
    error if no signal is pending in set and it is necessary to wait." Linux checks
    first, and also rejects a negative tv_sec, which is an interval that has already
    expired. Fix: in both cases poll with a zero timeout, and turn EAGAIN into EINVAL
    only for a bad tv_nsec (the approach the pilot used for mq_timed\*).
    Failing-before: `bad timeout with a pending signal: sigtimedwait(&set, &si, &ts) == SIGUSR1`.

## Deviations not fixed

- **No mcontext_t or ucontext_t** (H14). Issue 8 moved these types into `<signal.h>`.
  They would have to match each architecture's kernel signal frame, because the third
  argument to an SA_SIGINFO handler is the kernel's ucontext. That is a bigger,
  per-architecture job, so I left it for a decision.
- **SA_RESETHAND keeps SA_SIGINFO** (A9) **and also resets SIGILL and SIGTRAP** (A10).
  This is Linux behaviour, and glibc behaves the same. sigaction could hide the first
  by clearing SA_SIGINFO in what oact reports, but then an explicit SIG_DFL installed
  with SA_SIGINFO would be misreported.
- **sig2str names SIGEMT and SIGLOST** (N6). Linux doesn't have these signals (kill
  returns EINVAL), so on Linux sig2str should return -1 for them. sig2str is generic
  libc code and can't see which signals the OS has.
- **SIGCHLD si_status is limited to 8 bits** (I6), because the kernel provides only
  those.

## Other observations (not changed)

- `NSIG` is defined whatever the feature-test macros say. The header tests
  `#ifdef __GNU_VISIBLE`, which is always true because the macro is always defined (as
  0 or 1). NSIG is outside the reserved SIG namespace, so this is namespace pollution,
  but picolibc's own code relies on it. Recorded in cross-cutting.md.
- The sigaddset, sigdelset and sigismember macros don't check the range: signal
  numbers of 64 and above are an undefined shift. The EINVAL error is "may fail".
- sigaction updates its handler table after the rt_sigaction syscall. A signal that
  arrives in between runs the old handler through the new trampoline, and if
  SA_SIGINFO is being switched, with the wrong arguments. Inferred, not reproduced.
- glibc differences found along the way, guarded with `__PICOLIBC__` or worked around:
  RTSIG_MAX is 32 against 31 usable signals; pthread_kill on an exited thread doesn't
  check the signal number; raise gives SI_TKILL, so test-si_code.c uses kill to get
  SI_USER; and glibc's `signal()` under `_POSIX_C_SOURCE` resets the handler on
  delivery, so test-sigpending.c reinstalls it.
- `test/test-raise.c` already existed, so the new thread test is named
  `test-raise-thread` to avoid a duplicate test name in meson and CMake.

## Left undone

- H7: there is no `_SC_NSIG` (recorded in cross-cutting.md for the unistd-config
  unit).
- I4: SIGILL, SIGFPE and SIGTRAP faults can't be raised portably.
- E-Q1 (EAGAIN from sigqueue), W10 (sigwait in two threads), PS6 (psignal errors),
  G5 and T6 (state across exec), U3 and U4.
- Timing: the SA_RESTART checks in test-sigaction use alarm(1) and take about 2 s.
  If load delayed the parent's read by more than 1 s, the "EINTR without SA_RESTART"
  check would see the byte and fail. In test-pthread_kill, the 100 ms pause only
  decides whether the zombie path is exercised, not whether the test passes.

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`), full
  `meson test -t 4`: 1227 tests; 1180 Ok, 0 Fail, 6 Expected Fail, 41 Skipped.
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`), full
  `meson test -t 4`: 1226 tests; 1177 Ok, 0 Fail, 6 Expected Fail, 43 Skipped.
- Bare-metal arm (`scripts/do-arm-configure -Dwerror=true -Dmultilib=false`),
  `meson test -t 20 --suite test-signal --suite test-pthread`: 4 Ok (test-sig2str,
  test-sigismember, test-signal, test-signal_h), 0 Fail, 33 Skipped (the OS tests).
  I turned multilib off because the full multilib build is about 117k ninja steps,
  too much for the shared host.
- CMake (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 481 tests, 100%
  passed (232 ran, 249 skipped), including test-sig2str, test-signal_h and
  test-sigismember.
- test-getnameinfo-native, the known DNS failure, passed in these runs.
- `clang-format --dry-run` is clean on every C file and header I touched.

## Commits (audit/signal, on top of 2d4e737cfc)

- 0cf3cb95af libos/linux: Leave the signal handler alone when sigaction has no act
- 944889b901 libos/linux: Report SIG_DFL and SIG_IGN from the kernel in sigaction
- a124f6c305 libos/linux: Ignore how in sigprocmask when set is null
- 3abaf17f21 libos/linux: Use the kernel's MINSIGSTKSZ and SIGSTKSZ
- 041ab976c9 libos/linux: Reject SS_ONSTACK in sigaltstack
- 1cdcece4c7 libos/linux: Let pthread_kill signal exited threads without error
- f409ac591e signal: Add the si_code values for SIGILL, SIGFPE, SIGSEGV, SIGBUS and SIGTRAP
- 55869c4a1c signal: Define SIG2STR_MAX for POSIX.1-2024
- 3a6409ac3e signal: Make sigev_notify_attributes a pthread_attr_t pointer
- 4d0bfea822 limits: Make NSIG_MAX 64 and visible for POSIX.1-2024
- 4d98f7b8bc signal: Accept only signal numbers and plain decimals in str2sig
- 3d720426f5 libos/linux: Check the sigtimedwait timeout only when it would wait
- 5128b59648 test-posix: Check queries, masks, flags and errors in sigaction
- 4835e8a594 test-posix: Check how and the mask operations in sigprocmask
- a35ed48f89 test-posix: Check SS_ONSTACK, EPERM and MINSIGSTKSZ in sigaltstack
- 895357a6f5 test-posix: Check pthread_kill on an exited thread
- fcb5551ff8 test-posix: Check the fault si_code values and si_addr
- 4b57fe1814 test-posix: Check the signal.h types, macros and prototypes
- af100633ae test-posix: Check sig2str and str2sig
- 0586df84fd test-posix: Check timeouts and signal selection in sigtimedwait
- ec636dd18c test-posix: Check delivery, process groups and errors in kill
- 20d688e0f1 test-posix: Check pointer values and errors in sigqueue
- c55fad5124 test-posix: Check return values, SIG_IGN and errors in signal
- 96e0c3abd0 test-posix: Check process-directed signals in sigpending
- b846ba210e test-posix: Check psignal output, errno and orientation
- 44fb8fbd23 test-posix: Check the signal set functions and macros
- f4f6bb74ff test-posix: Check raise in threads and its errors

Nothing pushed. No AI attribution in any commit.

## Process note

`git stash` is shared by all worktrees of a repository, because refs/stash lives in
the common git directory. During this run, this unit and the net-names unit each
popped the other's stash. I caught it at once. The misplaced inet_pton.c change went
back to net-names as `/tmp/net-names-inet_pton-stash.patch`, I told the coordinator,
and I used patch files from then on. No commit was affected.
