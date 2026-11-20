# <mqueue.h> audit against POSIX.1-2024 (Issue 8)

Worktree `/agentic/picolibc-project/worktrees/audit-mqueue`, branch `audit/mqueue`,
based on 14ebe3b96b. Implementation: `libos/linux/mq_*.c`, `_mq_notify_thread.c`,
`local-mqueue.h`; header `libc/include/mqueue.h`.
Tests: `test/test-posix/test-mqueue/` (paths below are relative to that directory).

Spec source: pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/mqueue.h,
functions/mq_open, mq_close, mq_unlink, mq_send, mq_receive, mq_notify,
mq_getattr, mq_setattr.

Status key: TESTED (test file:line), UNTESTED, NOT TESTABLE (reason).
Every TESTED row was run and passes for x86_64 picolibc (`test-X`), the
32-bit multilib (`test-X_32`), and against glibc (`test-X-native`), unless it
says picolibc-only.

## Requirement table

### Header (basedefs/mqueue.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | mqd_t defined, not an array type | TESTED | test-mqueue_h.c:56 (a function type returning mqd_t, which fails to compile for an array) |
| H2 | size_t, ssize_t defined | TESTED | test-mqueue_h.c:48 |
| H3 | struct timespec defined as in <time.h> | TESTED | test-mqueue_h.c:48 |
| H4 | `struct sigevent` tag declared | TESTED | test-mqueue_h.c:53 |
| H5 | struct mq_attr with long mq_flags, mq_maxmsg, mq_msgsize, mq_curmsgs | TESTED | test-mqueue_h.c:59-62 (_Generic) |
| H6 | O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_EXCL, O_NONBLOCK defined | TESTED | test-mqueue_h.c:41-44 (only <mqueue.h> included at that point) |
| H7 | Ten function prototypes as specified, including restrict on mq_setattr and mq_timedreceive | TESTED | test-mqueue_h.c:70-83 (function-pointer initialisation; restrict on parameters does not affect type compatibility, so that part is checked by reading the header: it matches) |

### mq_open

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | Returns a descriptor on success | TESTED | test-mq_open.c:97 and every test |
| O2 | O_RDONLY descriptor can receive but not send | TESTED | test-mq_send.c:149-151 (EBADF on send) |
| O3 | O_WRONLY descriptor can send but not receive | TESTED | test-mq_receive.c:123-126 |
| O4 | O_RDWR can do both | TESTED | every test |
| O5 | O_CREAT uses mode and attr; mq_maxmsg/mq_msgsize set from attr | TESTED | test-mq_notify.c:148, test-mq_setattr.c:91 |
| O6 | O_CREAT with NULL attr uses implementation-defined defaults | TESTED | test-mq_open.c:120-123 (only checks positive values) |
| O7 | O_CREAT on an existing queue (no O_EXCL) opens it; attr ignored | TESTED | test-mq_open.c:104-108 |
| O8 | O_CREAT with O_EXCL fails if the queue exists (EEXIST) | TESTED | test-mq_notify.c:144 |
| O9 | O_NONBLOCK at open sets the description flag | TESTED | test-mq_open.c:113, test-mq_receive.c:135-138, test-mq_send.c:156-160 |
| O10 | Open descriptions are distinct (flags are per description) | TESTED | test-mq_open.c:114, test-mq_setattr.c:98 |
| O11 | mq_open neither adds nor removes messages | TESTED | test-mq_open.c:107 (curmsgs 1 after reopen) |
| O12 | Without O_CREAT, a missing queue fails (ENOENT) | TESTED | test-mq_open.c:73, test-mq_notify.c:285 |
| O13 | Mode bits applied, less umask | UNTESTED | Needs fstat on an mqd_t, which POSIX does not provide; Linux handles it in the kernel |
| O14 | Atomic check-and-create with O_EXCL | NOT TESTABLE | race property; inherited from the kernel |
| E-O1 | EACCES: permission denied by mode | TESTED | test-mq_open.c:129-136 (skipped when euid is 0) |
| E-O2 | EACCES: creation denied | NOT TESTABLE | needs a /dev/mqueue owned by someone else or another user |
| E-O3 | EINTR | NOT TESTABLE | Linux mq_open never blocks |
| E-O4 | EINVAL: unsupported name | TESTED | test-mq_notify.c:136 ("no-slash"); picolibc also rejects "/" with EINVAL (glibc: ENOENT), not asserted |
| E-O5 | EINVAL: mq_maxmsg or mq_msgsize <= 0 | TESTED | test-mq_open.c:79-93 |
| E-O6 | EMFILE / ENFILE | NOT TESTABLE | needs exhausting process/system descriptor limits; kernel behaviour |
| E-O7 | ENOSPC | NOT TESTABLE | needs exhausting /proc/sys/fs/mqueue/queues_max or the RLIMIT_MSGQUEUE budget; kernel behaviour |
| E-O8 | ENAMETOOLONG (may fail) | UNTESTED | optional; the kernel returns it for names over NAME_MAX (inferred, not run) |

### mq_close

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Removes the descriptor association; closes the fd | TESTED | test-mq_close.c:97, every test |
| C2 | Removes a notification registration made through mqdes | TESTED | test-mq_close.c:93-99 |
| C3 | Returns 0 on success | TESTED | every test |
| E-C1 | EBADF for an invalid descriptor (-1) | TESTED | test-mq_close.c:79 |
| E-C2 | EBADF for a valid fd that is not a message queue | TESTED (picolibc only) | test-mq_close.c:81-86. **Was a deviation; fixed.** glibc closes the fd and returns 0, so the check is `#ifdef __PICOLIBC__` |

### mq_unlink

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | Removes the name | TESTED | test-mq_unlink.c:80-84 |
| U2 | Destruction postponed while open; descriptor keeps working | TESTED | test-mq_unlink.c:94-98 |
| U3 | Name reuse with O_CREAT creates a new queue | TESTED | test-mq_unlink.c:88-89 |
| U4 | On failure the queue is unchanged | UNTESTED | only failure reachable unprivileged is ENOENT, where there is no queue |
| E-U1 | ENOENT | TESTED | test-mq_unlink.c:73, test-mq_notify.c:287 |
| E-U2 | EACCES | NOT TESTABLE | needs a queue owned by another user in the sticky /dev/mqueue |
| E-U3 | EINTR | NOT TESTABLE | Linux mq_unlink never blocks |
| E-U4 | ENAMETOOLONG (may fail) | UNTESTED | optional |

### mq_send / mq_timedsend

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | Message added; returns 0 | TESTED | test-mq_send.c:118-121 |
| S2 | msg_len <= mq_msgsize accepted (including equal and zero) | TESTED | test-mq_send.c:138-145 |
| S3 | Higher priority inserted before lower | TESTED | test-mq_send.c:118-125, test-mq_notify.c:151-160 |
| S4 | Equal priority inserted after existing (FIFO) | TESTED | test-mq_send.c:118-125 |
| S5 | msg_prio < {MQ_PRIO_MAX} accepted; {MQ_PRIO_MAX} is EINVAL | TESTED | test-mq_send.c:128-135. **Was a deviation; fixed** (sysconf said 32, kernel accepts up to 32767) |
| S6 | Full queue, blocking: blocks until space | TESTED | test-mq_send.c:169-180 (another process makes room) |
| S7 | Full queue, blocking: interrupted by a signal (EINTR) | TESTED | test-mq_send.c:163-166 |
| S8 | Full queue, O_NONBLOCK: EAGAIN, nothing queued | TESTED | test-mq_send.c:155-160, 167 |
| S9 | On error nothing is queued | TESTED | test-mq_send.c:167, test-mq_timedsend.c:99 |
| S10 | Waiting senders woken highest priority/longest first (Priority Scheduling) | NOT TESTABLE | needs SCHED_FIFO threads (privilege) and is kernel behaviour |
| T1 | mq_timedsend: wait ends at abstime (CLOCK_REALTIME) with ETIMEDOUT | TESTED | test-mq_notify.c:183, test-mq_timedsend.c:87 |
| T2 | abstime already passed: ETIMEDOUT immediately if full | TESTED | test-mq_timedsend.c:87 |
| T3 | Never fails with timeout if there is room | TESTED | test-mq_timedsend.c:83 |
| T4 | Deadline before the epoch is a passed deadline (only tv_nsec can be invalid) | TESTED (picolibc only) | test-mq_timedsend.c:109-110, 122. **Was a deviation; fixed** (Linux returns EINVAL for tv_sec < 0) |
| T5 | With O_NONBLOCK, mq_timedsend is equivalent to mq_send | TESTED (picolibc only) | test-mq_timedsend.c:113-117. **Was a deviation; fixed** (Linux returned EINVAL for a bad tv_nsec instead of EAGAIN) |
| T6 | EINVAL: would block and tv_nsec < 0 or >= 1e9 | TESTED | test-mq_timedsend.c:92-96 |
| T7 | Validity of abstime need not be checked when there is room | Not asserted | permissive; picolibc now accepts the message, glibc returns EINVAL. Both conform |
| E-S1 | EBADF: not open for writing | TESTED | test-mq_send.c:151 |
| E-S2 | EMSGSIZE | TESTED | test-mq_send.c:140 |

### mq_receive / mq_timedreceive

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | Removes the oldest message of highest priority | TESTED | test-mq_send.c:122-125, test-mq_notify.c:155-160 |
| R2 | Returns message length; stores priority when msg_prio non-NULL | TESTED | test-mq_notify.c:155-160 |
| R3 | msg_prio NULL accepted | TESTED | test-mq_receive.c:131 |
| R4 | msg_len < mq_msgsize: EMSGSIZE and message stays queued | TESTED | test-mq_receive.c:116-119, test-mq_notify.c:162 |
| R5 | Empty, blocking: blocks until a message is enqueued | TESTED | test-mq_receive.c:147-157 |
| R6 | Empty, blocking: interrupted by a signal (EINTR) | TESTED | test-mq_receive.c:141-145 |
| R7 | Empty, O_NONBLOCK: EAGAIN | TESTED | test-mq_receive.c:134-138, test-mq_notify.c:169 |
| R8 | Waiting receivers woken by priority (Priority Scheduling) | NOT TESTABLE | as S10 |
| TR1 | Timeout expires at abstime on CLOCK_REALTIME, not before | TESTED | test-mq_timedreceive.c:93-104 |
| TR2 | abstime already passed: ETIMEDOUT immediately if empty | TESTED | test-mq_timedreceive.c:90 |
| TR3 | Never fails with timeout if a message is available | TESTED | test-mq_timedreceive.c:115-117 |
| TR4 | Deadline before the epoch is a passed deadline | TESTED (picolibc only) | test-mq_timedreceive.c:126-131. **Was a deviation; fixed** |
| TR5 | With O_NONBLOCK, equivalent to mq_receive | TESTED (picolibc only) | test-mq_timedreceive.c:134-138. **Was a deviation; fixed** |
| TR6 | EINVAL: would block and bad tv_nsec | TESTED | test-mq_timedreceive.c:107-112 |
| TR7 | 64-bit deadlines past 2038 reach the kernel | TESTED | test-mq_notify.c:188-193 (pre-existing) |
| E-R1 | EBADF: not open for reading | TESTED | test-mq_receive.c:125 |
| E-R2 | EBADMSG (may fail) | NOT TESTABLE | data corruption cannot be provoked |

### mq_notify

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | Registration succeeds (SIGEV_NONE, SIGEV_SIGNAL, SIGEV_THREAD) | TESTED | test-mq_notify.c:197-222 |
| N2 | Signal delivered with si_code SI_MESGQ and sigev_value on empty-to-non-empty | TESTED | test-mq_notify.c:209-213 |
| N3 | SIGEV_THREAD calls sigev_notify_function with sigev_value | TESTED | test-mq_notify.c:216-222 |
| N4 | No notification when the queue was not empty | TESTED | test-mq_notify.c:233-244 |
| N5 | Only one process registered: second registration in same process EBUSY | TESTED | test-mq_notify.c:211 |
| N6 | ... and from another process EBUSY | TESTED | test-mq_notify.c:246-253 |
| N7 | NULL removes the registration | TESTED | test-mq_notify.c:200, 272-276 |
| N8 | Registration removed after delivery; queue available again | TESTED | test-mq_notify.c:220 (re-register after the signal), 247 |
| N9 | Blocked receiver takes the message, no notification, registration stays | TESTED | test-mq_notify.c:255-271 (relies on a 200 ms delay for the parent to block; see below) |
| N10 | SIGEV_THREAD honours sigev_notify_attributes | UNTESTED | implementation copies the attributes (read, not run) |
| E-N1 | EBADF | TESTED | test-mq_notify.c:230-231 |
| E-N2 | EBUSY | TESTED | as N5/N6 |
| E-N3 | EINVAL: NULL and not registered (may fail) | Not applicable | Linux returns 0; optional error |

### mq_getattr / mq_setattr

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | mq_getattr: mq_flags from the open description | TESTED | test-mq_open.c:113-114, test-mq_notify.c:167 |
| A2 | mq_getattr: mq_maxmsg, mq_msgsize as created | TESTED | test-mq_notify.c:148, test-mq_setattr.c:91 |
| A3 | mq_getattr: mq_curmsgs current | TESTED | test-mq_notify.c:154, test-mq_setattr.c:91 |
| A4 | mq_getattr EBADF (may fail) | TESTED | test-mq_setattr.c:106-107 |
| A5 | mq_setattr sets mq_flags (O_NONBLOCK) | TESTED | test-mq_setattr.c:86-102, test-mq_notify.c:166-171 |
| A6 | mq_setattr ignores mq_maxmsg, mq_msgsize, mq_curmsgs | TESTED | test-mq_setattr.c:81-95 |
| A7 | omqstat gets the previous attributes, same as mq_getattr | TESTED | test-mq_setattr.c:80-91 |
| A8 | omqstat NULL accepted | TESTED | test-mq_setattr.c:101 |
| A9 | On failure attributes unchanged | UNTESTED | only reachable failure is EBADF, where there is no queue |
| A10 | mq_setattr EBADF | TESTED | test-mq_setattr.c:104-105 |

### Counts

101 requirements:
- TESTED: 83 (5 of them only under `__PICOLIBC__` because glibc differs: E-C2, T4, T5, TR4, TR5)
- UNTESTED: 6 (O13, E-O8, U4, E-U4, N10, A9)
- NOT TESTABLE: 10 (O14, E-O2, E-O3, E-O6, E-O7, E-U2, E-U3, S10, R8, E-R2)
- Permissive, deliberately not asserted: 1 (T7); optional error the implementation doesn't use: 1 (E-N3)

## Deviations found and fixed

All were verified by running: each new test fails with the fix reverted (log
lines quoted below) and passes with it.

1. **Timeout validity checked too early** (libos/linux/local-mqueue.h `_mq_timeout`,
   mq_timedsend.c, mq_timedreceive.c). Linux validates abs_timeout before looking
   at the queue, returning EINVAL for tv_sec < 0 or a bad tv_nsec. POSIX:
   "[EINVAL] The process or thread would have blocked, and the abstime parameter
   specified a nanoseconds field value less than zero or greater than or equal to
   1000 million"; "If O_NONBLOCK is set ... this function shall be equivalent to
   mq_send()"; a deadline "already passed at the time of the call" expires the
   timeout. Fix: when the deadline is invalid or before the epoch, pass the
   kernel {0, 0} (already passed), and turn the resulting ETIMEDOUT into EINVAL
   only for a bad tv_nsec. Failing-before output:
   `full queue and a deadline before the epoch: ... errno == ETIMEDOUT` and
   `empty queue and a deadline before the epoch: ... errno == ETIMEDOUT`.
   glibc has the same behaviour as unfixed picolibc, hence the `__PICOLIBC__` guard.
2. **mq_close closed any file descriptor** (libos/linux/mq_close.c). POSIX:
   "[EBADF] The mqdes argument is not a valid message queue descriptor." Fix:
   query the queue attributes (mq_getsetattr returns EBADF for non-queue fds)
   before closing. Costs one extra syscall per mq_close. Failing-before:
   `mq_close of a pipe: mq_close((mqd_t)fds[0]) == -1 && errno == EBADF`.
   glibc also closes the pipe, hence the guard.
3. **sysconf(_SC_MQ_PRIO_MAX) reported 32** (libos/linux/sysconf.c, via
   libos/fallback/sysconf.c returning `_POSIX_MQ_PRIO_MAX`) while the kernel
   accepts priorities up to 32767, so `mq_send` with a priority sysconf calls
   invalid succeeded. POSIX: "The value of msg_prio shall be less than
   {MQ_PRIO_MAX}" / "[EINVAL] The value of msg_prio was outside the valid
   range." Fix: report the kernel's 32768 (matches glibc). Failing-before:
   `send above the highest priority: ... errno == EINVAL`.

## Other observations (not changed)

- `sysconf(_SC_MQ_OPEN_MAX)` on Linux returns the fallback `_POSIX_MQ_OPEN_MAX`
  (8); Linux has no such per-process limit (glibc returns -1). Conservative, not
  a conformance failure for <mqueue.h>; belongs to a <unistd.h>/<limits.h> audit.
- `MQ_PRIO_MAX` is not defined in <limits.h>; it is allowed to be omitted
  (runtime-invariant, available from sysconf). Also for the <limits.h> audit.
- `mq_open("/")` gives EINVAL in picolibc, ENOENT in glibc; both acceptable.
- mq_notify(NULL) when not registered returns 0 (optional EINVAL not used).
- mq_setattr silently drops mq_flags bits other than O_NONBLOCK; spec allows
  implementation-defined flags and lists no error.
- mq_open/mq_unlink EINTR cannot happen on Linux; mq_send/mq_receive restart
  only under SA_RESTART, which is permitted (tests install handlers without it).
- Read, not run: the SIGEV_THREAD helper creates one detached thread per
  registration, which exits when the kernel reports delivery or removal
  (including on mq_close), so no thread leaks were found by reading.

## Left undone

- O13 (umask/mode) needs fstat on an mqd_t, which is Linux-specific.
- N10 (sigev_notify_attributes) not tested; would need inspecting the helper's
  attributes from inside the notification function.
- N9 relies on timing: the child waits 200 ms before sending so the parent is
  blocked in mq_receive. Under extreme load a false failure ("no signal with a
  blocked receiver") is possible; there's no portable way to observe that a
  process is blocked. Ran 3 times back to back, no failures.
- The tests that use alarm() take about 1 s each (test-mq_send, test-mq_receive).
- CMake build not run; CMakeLists.txt changes mirror meson.build.

## Test runs (verified by running)

- x86_64-linux-gnu (`builds/build-x86_64-linux-gnu`, -Dwerror=true):
  `meson test -t 4 --suite test-mqueue`: 30 Ok, 0 Fail (10 tests x picolibc, _32, -native).
  Full `meson test -t 4`: 1729 tests; 1674 Ok, 9 Expected Fail, 0 Fail, 46 Skipped.
- i686-linux-gnu (`builds/build-i686-linux-gnu`, -Dwerror=true):
  `meson test -t 4 --suite test-mqueue`: 10 Ok, 0 Fail.
- test-mq_notify run three times back to back on x86_64: all passed.
- `clang-format --dry-run` clean on every touched C file.

## Commits (audit/mqueue, on top of 14ebe3b96b)

- 93c9178955 libos/linux: Check mq timeouts only when the call would block
- 4d32d2b60c test-posix: Check deadlines in mq_timedsend and mq_timedreceive
- 1d842d5faf libos/linux: Return EBADF from mq_close for non-queue descriptors
- 5d437cd0bb test-posix: Check descriptors and notification in mq_close
- a0f472bcf0 libos/linux: Report the kernel's MQ_PRIO_MAX from sysconf
- 509f5a0f38 test-posix: Check ordering, limits, blocking and errors in mq_send
- 8b6158f928 test-posix: Check blocking and errors in mq_receive
- ada9f1889e test-posix: Check attributes, access and errors in mq_open
- 697e6f56f4 test-posix: Check deferred destruction in mq_unlink
- 58bd83b0b7 test-posix: Check ignored fields and omqstat in mq_setattr
- 06d8b9d013 test-posix: Check arrival, EBUSY and blocked receivers in mq_notify
- 3e958524f0 test-posix: Check the types, macros and prototypes in mqueue.h

Nothing pushed. No AI attribution in any commit.
