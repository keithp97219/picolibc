# <aio.h> audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-aio`, branch `audit/aio`, based on 40921d6cce (`claude/posix-spec-audit`). Host: aarch64 Linux (Graviton2, kernel 5.10), with builds and tests in the Debian sid chroot.

**Scope:**
- `<aio.h>`: struct aiocb, the AIO_/LIO_ constants, the types the header must provide, and the eight functions aio_read, aio_write, aio_fsync, aio_error, aio_return, aio_cancel, aio_suspend and lio_listio.
- The notification rules of XSH 2.4.1 as aio uses them (SIGEV_NONE, SIGEV_SIGNAL with SI_ASYNCIO and si_value, SIGEV_THREAD).
- The ordering and priority rules of XSH 2.8.2, and how requests interact with close().
- AIO_LISTIO_MAX, AIO_MAX and AIO_PRIO_DELTA_MAX, with their sysconf names.

**Implementation:**
- `libc/posix/aio/*.c` and `local-aio.h`: each request runs in a detached thread of its own, with every signal blocked.
  - `__aio_submit.c` holds the request list, the worker and the lock helpers.
  - `__aio_notify.c` delivers notifications.
- `libos/linux/__aio_signal.c` sends SIGEV_SIGNAL notifications with si_code SI_ASYNCIO through rt_sigqueueinfo.
- The header is `libc/include/aio.h`, the limits are in `libc/include/sys/syslimits.h`, and sysconf is in `libos/fallback/sysconf.c`.

**Tests:** in `test/test-posix/test-aio/`. File names below drop the `test-` prefix.
- New: aio_h.c (header, prototypes, limits).
- Extended: aio_read.c, aio_write.c, aio_fsync.c, aio_error.c, aio_return.c, aio_cancel.c, aio_suspend.c, lio_listio.c.

**Spec source:** pubs.opengroup.org/onlinepubs/9799919799.
- Base definitions: basedefs/aio.h.
- Function pages: aio_cancel, aio_error, aio_fsync, aio_read, aio_return, aio_suspend, aio_write, lio_listio.
- XSH 2.4.1 and 2.4.2 (sigevent, si_code), 2.8.2 (asynchronous I/O), 2.9.5.2 (cancellation points).

**Status key:**
- TESTED (file:line), UNTESTED, NOT TESTABLE (with the reason). "(new)" marks rows this audit added tests for.
- Every TESTED row was run and passes on aarch64-linux-gnu and arm-linux-gnueabihf, both for picolibc and against glibc 2.43 (`-native`), unless the row says otherwise.
- "Read" means inferred from the code, not run.

## Requirement table

### Header (basedefs/aio.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | struct aiocb has aio_fildes (int), aio_offset (off_t), aio_buf (volatile void *), aio_nbytes (size_t), aio_reqprio (int), aio_sigevent (struct sigevent) and aio_lio_opcode (int) | TESTED (new) | aio_h.c:59-64 (`_Generic` on each member) |
| H2 | off_t, pthread_attr_t, size_t and ssize_t are defined | TESTED (new) | aio_h.c:42-45 (only `<aio.h>` included) |
| H3 | struct timespec is defined | TESTED (new) | aio_h.c:42-45 |
| H4 | struct sigevent and union sigval are as in `<signal.h>`, with sigev_notify_attributes a `pthread_attr_t *` | TESTED (new) | aio_h.c:47-55 |
| H5 | AIO_ALLDONE, AIO_CANCELED and AIO_NOTCANCELED are defined and distinct | TESTED (new) | aio_h.c:66-68 |
| H6 | LIO_NOP, LIO_READ and LIO_WRITE, and LIO_WAIT and LIO_NOWAIT, are defined and distinct | TESTED (new) | aio_h.c:69-71 |
| H7 | The eight prototypes are as specified (aio_fsync under FSC or SIO) | TESTED (new) | aio_h.c:98-117, by function-pointer initialisation. glibc's lio_listio lacks restrict on the list elements, so the native build checks glibc's form |
| H8 | The header may make `<fcntl.h>`, `<signal.h>` and `<time.h>` symbols visible; picolibc's includes `<signal.h>` | TESTED (new, picolibc only) | aio_h.c:73-78 (the SIGEV_ values) |
| H9 | In strict mode, nothing POSIX doesn't reserve is declared | Read (`-E -dM`) | See the note below the table |
| H10 | The header is clean under `-Wundef` | TESTED | headers suite, check-wundef-aio.h |

H9 note: with `_POSIX_C_SOURCE 202405L`, the header declares only:
- aio_, lio_, AIO_ and LIO_ names, and the `__aio_` private members;
- `<signal.h>` and `<time.h>` symbols;
- the known `<stdint.h>` macros, wint_t and MALLOC_ALIGNMENT;
- **CLK_TCK** from `<time.h>` (see cross-cutting.md).

### aio_read

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | Reads aio_nbytes at aio_offset into aio_buf | TESTED | aio_read.c:129-138 |
| R2 | Returns once queued, even when the data can't be delivered yet | TESTED (new) | aio_read.c:163-174 (an empty pipe stays EINPROGRESS) |
| R3 | [PIO] Submitted at the base priority minus aio_reqprio | NOT APPLICABLE | `_POSIX_PRIORITIZED_IO` isn't defined on Linux |
| R4 | aio_error and aio_return can be used while the request runs | TESTED | aio_error.c:121-138 |
| R5 | After a queuing error, nothing is queued | Read | Every early return in `__aio_submit` comes before the list insertion |
| R6 | The read happens as if lseek(aio_offset, SEEK_SET) came first; on a file that can't seek, the offset doesn't apply | TESTED (new) | aio_read.c:163-174 (a pipe with aio_offset 1000). **Was a deviation (ESPIPE); fixed** |
| R7 | aio_sigevent says how completion is notified | TESTED (new) | aio_read.c:176-204 (rows N1 and N2) |
| R8 | aio_lio_opcode is ignored | TESTED (new) | aio_read.c:155-159 |
| R9 | [SIO] synchronized I/O completion | NOT TESTABLE | Durability can't be observed from user space |
| R10 | No transfer past the offset maximum | NOT TESTABLE | off_t is 64 bits and every open uses O_LARGEFILE |
| R11 | Returns 0, or -1 with errno set | TESTED | throughout |
| E-R1 | EAGAIN when there are no resources | TESTED (new) | lio_listio.c:202 (a child with RLIMIT_AS at its current size; skipped under qemu) |
| E-R2 | EBADF, now or as the request's status | TESTED | Invalid descriptor: aio_read.c:208-216 (now reported at once). Write-only descriptor: aio_error.c:105-117 (as the status) |
| E-R3 | EINVAL for an invalid offset, aio_reqprio [PIO] or aio_nbytes | TESTED (new) for the offset and aio_reqprio | aio_read.c:140-153. aio_nbytes is NOT TESTABLE: Linux clamps any count |
| E-R4 | ECANCELED after aio_cancel | TESTED (new) | aio_cancel.c:167-206 (on a write; same code) |
| E-R5 | EOVERFLOW | NOT TESTABLE | as R10 |

### aio_write

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | Writes at aio_offset when O_APPEND isn't set | TESTED | aio_write.c:244-247 |
| W2 | Returns once queued | TESTED (new) | aio_write.c:316-338 (blocked on a full pipe) |
| W3 | [PIO] priority | NOT APPLICABLE | as R3 |
| W4 | With O_APPEND, the offset doesn't apply and writes append in the order of the calls | TESTED (new) | aio_write.c:296-314 (32 one-byte writes). **Was a deviation; fixed** |
| W5 | On a device that can't seek, writes append in the order of the calls | TESTED (new) | aio_write.c:316-338 (an empty write queued behind a blocked one must wait). **Was a deviation (ESPIPE, no ordering); fixed** |
| W6 | aio_lio_opcode is ignored | Read | Same submission path as R8 |
| W7 | [SIO] synchronized I/O | NOT TESTABLE | as R9 |
| W8 | Past the file size limit the write fails, and [XSI] SIGXFSZ is generated | TESTED (new) | aio_write.c:169-217 (a child with RLIMIT_FSIZE 10). EFBIG is checked everywhere; SIGXFSZ for picolibc only. **Was a deviation; fixed** |
| W9 | Completion is notified | TESTED (new) | aio_write.c:253-278 (SIGEV_THREAD) |
| E-W1 | EAGAIN, EBADF, EINVAL (aio_reqprio) | TESTED | EINVAL: aio_write.c:249-251. EAGAIN and EBADF through the shared submission path (E-R1, E-R2) |
| E-W2 | ECANCELED | TESTED (new) | aio_cancel.c:167-206 |
| E-W3 | EFBIG at the offset maximum or the implementation's maximum file size | NOT TESTABLE | as R10 |

### aio_fsync

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | O_SYNC completes as fsync, O_DSYNC as fdatasync | TESTED (the status only) | aio_fsync.c:141-146. Durability is NOT TESTABLE |
| F2 | The I/O queued when aio_fsync was called completes first | TESTED (new) | aio_fsync.c:167-188. **Was a deviation; fixed** |
| F3 | Returns once queued; the status is EINPROGRESS, then the result | TESTED | aio_fsync.c:181-188 |
| F4 | aio_sigevent notification | Read | Same path as W9 |
| F5 | Only aio_fildes and aio_sigevent are used | TESTED (new, not glibc) | aio_fsync.c:150-157 (aio_reqprio -1, aio_lio_opcode 42). **Was a deviation (EINVAL); fixed**. glibc checks aio_reqprio |
| E-F1 | EAGAIN | Read | Shared submission path, tested through E-R1 |
| E-F2 | EBADF, shall fail | TESTED (new) | aio_fsync.c:159-165. **Was only reported as the status; fixed** |
| E-F3 | EINVAL for a bad op | TESTED | aio_fsync.c:147 |
| E-F4 | EINVAL when the file can't be synchronized | TESTED, as the status | aio_fsync.c:188 (pipe). **Deviation left**, see below |
| E-F5 | A queued request's failure is reported in the status | Read | |

### aio_error and aio_return

| # | Requirement | Status | Reference |
|---|---|---|---|
| ER1 | 0, the error status, or EINPROGRESS while running | TESTED | aio_error.c:114, 125, 135 (EINPROGRESS is now deterministic, on a pipe) |
| ER2 | May fail with EINVAL once the status has been retrieved | Not done | picolibc returns the value EINVAL; "may" allows that |
| ER3 | Async-signal-safe | Read | Lock-free atomic loads and a compare-exchange |
| RT1 | Returns the value read/write/fsync would; -1 for a failed request | TESTED | aio_return.c:110, 117-123 (new) |
| RT2 | Can be called once; then may fail with EINVAL | TESTED (new, picolibc only) | aio_return.c:113-115 |
| RT3 | An aiocb can be reused for another request | TESTED (new) | aio_return.c:125-129 |

### aio_cancel

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Cancels the given request, or with NULL every cancelable one on fildes | TESTED (new) | aio_cancel.c:167-218 |
| C2 | A canceled request gets ECANCELED and return -1 | TESTED (new) | aio_cancel.c:201-202 |
| C3 | A canceled request still gets its normal notification | TESTED (new) | aio_cancel.c:203-205 (the SIGEV_SIGNAL value) |
| C4 | Requests not canceled complete normally and their aiocb isn't touched | TESTED (new) | aio_cancel.c:209-223 |
| C5 | Returns AIO_CANCELED, AIO_NOTCANCELED or AIO_ALLDONE | TESTED | aio_cancel.c:156-164, 196-225 |
| C6 | Which requests are cancelable is implementation-defined | TESTED (new, picolibc only) | aio_cancel.c:196-198. Requests that haven't started can be canceled. Before, nothing could be, which was allowed |
| E-C1 | EBADF | TESTED | aio_cancel.c:165 |

### aio_suspend

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | Waits until one listed request completes | TESTED | aio_suspend.c:168-172, 204-207 |
| S2 | Returns at once if one has already completed | TESTED | aio_suspend.c:173 |
| S3 | NULL entries are ignored | TESTED (picolibc only) | aio_suspend.c:161-164. glibc returns at once for a list of NULLs |
| S4 | EAGAIN on timeout | TESTED (new, with a real pending request) | aio_suspend.c:185-189 |
| S5 | The timeout uses CLOCK_MONOTONIC | Read | Uses sem_clockwait(CLOCK_MONOTONIC) |
| S6 | EINTR when a signal interrupts it | TESTED (new) | aio_suspend.c:191-201. **Was a deviation; fixed** |
| S7 | Is a cancellation point, also when a request has already completed | TESTED (new for the completed case, not glibc) | aio_suspend.c:176-182. The blocked case is in test-pthread_testcancel. **Was a deviation; fixed**. glibc returns 0 |
| S8 | Async-signal-safe | Read | **Deviation found by reading:** it held the aio lock with signals unblocked, so a handler calling it could deadlock. Fixed by the EINTR change; not tested |

### lio_listio

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | LIO_WAIT waits for every request and ignores sig | TESTED (new for sig) | lio_listio.c:247-250, 313-315 |
| L2 | LIO_NOWAIT returns at once and notifies when every request is done | TESTED | Signal: lio_listio.c:252-271. SIGEV_THREAD with a pending request (new): 325-346 |
| L3 | A NULL sig means no notification | Read | SIGEV_NONE is stored |
| L4 | NULL entries and LIO_NOP are ignored | TESTED | lio_listio.c:240-250 |
| L5 | Entries are submitted as aio_read and aio_write would; each has its own status | TESTED | lio_listio.c:248-250, 275-285 |
| L6 | EIO when a request failed | TESTED | lio_listio.c:281 |
| L7 | Errors other than EAGAIN, EINTR and EIO start nothing | TESTED (new, not glibc) | lio_listio.c:319-322. glibc doesn't check sig |
| E-L1 | EAGAIN when there are no resources | TESTED (new) | lio_listio.c:203. **LIO_WAIT said EIO; fixed** |
| E-L2 | EAGAIN when AIO_MAX would be exceeded | NOT APPLICABLE | There is no limit; sysconf says -1 |
| E-L3 | EINVAL for a bad mode or nent > AIO_LISTIO_MAX | TESTED (new for nent) | lio_listio.c:273, 302-310 |
| E-L4 | EINTR during LIO_WAIT; the requests keep running | TESTED (new, not glibc) | lio_listio.c:349-366. **Was a deviation; fixed**. glibc returns EINTR, then one of its request threads crashes with SIGSEGV |
| E-L5 | Request status EAGAIN or EINPROGRESS | TESTED | lio_listio.c:204 |

### Notification (XSH 2.4.1, 2.4.2, 2.8.2)

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | SIGEV_NONE sends nothing but sets the statuses | TESTED (new) | aio_read.c:197-204 |
| N2 | SIGEV_SIGNAL generates sigev_signo with si_code SI_ASYNCIO and si_value | TESTED (new) | aio_read.c:176-195 |
| N3 | SIGEV_THREAD runs a new thread with sigev_value, using the given attributes or none | TESTED (new) | aio_write.c:253-278 (runs after completion, in another thread) |
| N4 | The thread's signal mask (implementation-defined) | Read | All signals blocked |

### Limits and close()

| # | Requirement | Status | Reference |
|---|---|---|---|
| LM1 | sysconf(_SC_AIO_LISTIO_MAX) is AIO_LISTIO_MAX when `<limits.h>` defines it | TESTED (new) | aio_h.c:184. **Was 2 against 64; fixed** |
| LM2 | sysconf(_SC_AIO_MAX) is AIO_MAX, at least 1, or -1 for no limit | TESTED (new) | aio_h.c:186. **Was 1 with no limit enforced; now -1** |
| LM3 | sysconf(_SC_AIO_PRIO_DELTA_MAX) is AIO_PRIO_DELTA_MAX | TESTED (new) | aio_h.c:188-189. **Was 0 against 20; fixed** |
| LM4 | _SC_ASYNCHRONOUS_IO reports the option | TESTED (new) | aio_h.c:182 |
| X1 | close(): a request that isn't canceled completes as if the close hadn't happened yet | TESTED (new, picolibc only) | aio_write.c:280-294, 340-356. **Was a deviation; fixed**. glibc fails both with EBADF |

### Counts

- Requirements: 90 rows.
- Tested before this audit: 27.
- Tested after: 70. Ten of them run for picolibc only or not against glibc: H8, W8's signal, F5, RT2, C6, S3, S7, L7, E-L4 and X1.
- Read only: 11.
- NOT TESTABLE: 5.
- NOT APPLICABLE: 3.
- Optional, not done: 1 (ER2).

## Deviations found and fixed

How each was proven:
- With only the library change undone on top of the final tree, the new test fails 3 of 3 runs on aarch64-linux-gnu; with the change, it passes.
- Fixes 1-4 no longer reverse cleanly over later commits, so for those only the fixing lines were disabled, by a script in /tmp/aio-proof.

1. **ESPIPE on pipes, sockets and terminals** (`__aio_submit.c`, pread/pwrite).
   - Spec:
     - aio_write: on "a device that is incapable of seeking, write operations append to the file in the same order as the calls were made".
     - aio_read: the error status is one "normally set by the read() function", and read() never sets ESPIPE.
     - XSH 2.8.2 applies the "as if lseek()" rule only to "a device that is capable of seeking".
   - So ESPIPE doesn't conform, and pthread-threads' note that it was defensible doesn't hold.
   - Fix: fall back to read/write, as glibc does.
   - Before: aio_read.c "still waiting", aio_error.c "done" and aio_write.c "empty write waits its turn" all failed.
2. **close() right after queuing** made the request fail with EBADF, or act on another file that reused the number.
   - Spec (close()): a request that isn't canceled "completes as if the close() operation had not yet occurred".
   - Fix: each request duplicates the descriptor (F_DUPFD_CLOEXEC) when queued, and closes it when done. That also reports a bad descriptor at once, which aio_fsync needs (E-F2).
   - The request's post-completion state now lives in a private `struct __aio_req`. struct aiocb's private members shrank to `__aio_error` and `__aio_return`: an ABI change in this unreleased header.
   - Before: aio_write.c "written after close" and aio_fsync.c "EBADF" failed.
3. **No ordering.** Appending writes landed in any order, and aio_fsync finished before the requests queued ahead of it.
   - Fix: a write that appends (O_APPEND set, or lseek fails with ESPIPE, decided when it's queued) waits for earlier writes on the same descriptor. aio_fsync waits for every earlier request on its descriptor.
   - Before: aio_fsync.c "waits for the write" failed 3 of 3. aio_write.c "appended in order" failed 2 of 3, since the 32 O_APPEND writes only race without the fix; "empty write waits its turn" failed the third run.
4. **aio_suspend and lio_listio(LIO_WAIT) never returned EINTR**, which both list under "shall fail".
   - Fix: each waiter registers a semaphore, which every completion and aio_cancel posts, and waits with sem_wait or sem_clockwait.
   - An interrupted lio_listio hands its list to the last request to free.
   - This also fixes S8.
   - Before: both tests hung until their alarm(60) watchdog.
5. **aio_suspend didn't act on a pending cancellation** when a request had already completed (XSH 2.9.5.2).
   - Fix: pthread_testcancel() on entry.
   - Before: aio_suspend.c "cancelled" failed.
6. **aio_fsync failed with EINVAL for a bad aio_reqprio**, although "all other members ... are ignored".
   - Before: aio_fsync.c "aio_reqprio ignored" failed.
7. **lio_listio(LIO_WAIT) said EIO instead of EAGAIN** when requests couldn't be queued.
   - Before: lio_listio.c "LIO_WAIT EAGAIN" failed.
8. **sysconf gave the POSIX minimums for the aio limits** (libos/fallback/sysconf.c).
   - It said 2 for _SC_AIO_LISTIO_MAX and 0 for _SC_AIO_PRIO_DELTA_MAX, while `<limits.h>` defines 64 and 20 and the code accepts them. It said 1 for _SC_AIO_MAX although nothing limits the number of requests.
   - Fix: report AIO_LISTIO_MAX and AIO_PRIO_DELTA_MAX, and -1 (no limit) for _SC_AIO_MAX.
   - Before: aio_h.c "AIO_LISTIO_MAX is 64, sysconf says 2".
9. **SIGXFSZ was lost** when aio_write passed the file size limit.
   - Spec (XSI): "the implementation shall generate a SIGXFSZ signal for the thread".
   - The kernel sends it to the request's thread, which blocks every signal and then exits.
   - Fix: when the write fails with EFBIG and SIGXFSZ is pending, the worker sends SIGXFSZ to the process.
   - Before: aio_write.c "SIGXFSZ" failed.

Not a deviation fix:

10. **aio_cancel cancels requests that haven't started**: ones waiting behind an earlier request on the descriptor, or whose thread hasn't run.
    - Before, nothing could be canceled. POSIX allows that, but it left ECANCELED and AIO_CANCELED untestable.
    - aio_cancel sets the status itself; the request's thread then sends the normal notification without touching the aiocb.
    - Before: aio_cancel.c "queued request canceled" failed.

## Deviations found, not changed

- **aio_fsync reports "the file can't be synchronized" (EINVAL) only as the request's status** (E-F4).
  - The page lists it under "shall fail", which reads as synchronous.
  - On Linux only the kernel knows whether the file has an fsync operation, and finding out means doing it. fstat could catch FIFOs and sockets, but not character devices.
  - glibc behaves the same. Left for a decision.

## Other observations (not changed)

- aio_error after aio_return returns the value EINVAL rather than -1 with errno set; "may fail" allows either.
- aio_cancel with a mismatched fildes fails with EINVAL; POSIX leaves that unspecified.
- Each request costs a thread (with the default 128 KiB stack) and a duplicate descriptor until it completes.
  - So a request blocked on a pipe keeps the write end open after the application closes its own, which is what the close() rule implies.
  - A child forked meanwhile also inherits the duplicate. It is close-on-exec, so exec drops it.
- Reads aren't ordered among themselves; POSIX states the order rule only for writes.
- glibc 2.43 deviations, which the tests guard:
  - A request queued before a close() fails with EBADF.
  - aio_suspend ignores a pending cancellation when a request has completed.
  - lio_listio doesn't check sig.
  - aio_fsync checks aio_reqprio.
  - An interrupted LIO_WAIT crashes a request thread.
  - SIGXFSZ never arrives.
  - A forked child of a process that has used aio waits forever for request threads it doesn't have, so the fork-based checks run first in their tests.

## Left undone

- E-F4, above.
- The PSE53 profile (`_POSIX_AEP_REALTIME_DEDICATED`, sys/unistd.h:782-783) defines `_POSIX_PRIORITIZED_IO`, but aio_reqprio is only range-checked ("accepted but unused" in syslimits.h). Such a build would claim priority ordering it doesn't have, including FIFO order for same-priority reads of character special files. It isn't testable here anyway: there is no SCHED_FIFO/RR in the chroot.
- ER2 (optional).

## Timing and robustness of the new tests

- Every test sets alarm(60), and every forked child alarm(30).
- Blocking is deterministic:
  - Reads wait on an empty pipe.
  - Writes wait on a pipe filled by O_NONBLOCK writes until EAGAIN, then switched back to blocking.
  - Every request is waited for or canceled, every pipe drained, and every descriptor closed before exit.
- The "still waiting" checks use aio_suspend with a 50-100 ms timeout. A correct implementation can't fail them, because the request can't complete until the test unblocks it.
- The EINTR checks use a helper thread that signals the main thread every 5 ms until the wait returns; signals that arrive before the wait starts are harmless.
- No SIGKILLs from the host security software were seen.
- The O_APPEND order check only probabilistically catches a missing fix; the pipe checks prove fix 3 deterministically.
- aio_cancel accepts AIO_CANCELED for the blocked first request too, because its thread may not have started yet. That race failed once during development.
- The EAGAIN check lowers RLIMIT_AS to the child's current size, read from /proc/self/statm without allocating. It is skipped under qemu.
- The aio tests passed 10 back-to-back suite runs on aarch64.

## Test runs (verified by running)

All in the Debian sid chroot at the final commit, full suites without `--no-rebuild`:

| Configuration | Ok | Fail | Expected Fail | Skipped |
|---|---|---|---|---|
| aarch64-linux-gnu (`meson test -t 4`) | 1495 | 0 | 6 | 43 |
| arm-linux-gnueabihf | 1494 | 0 | 6 | 45 |
| CMake thumbv7m (`ninja test`) | 254 passed | 0 | – | 322 |

- The CMake run reports "100% tests passed out of 576"; the aio tests skip there, as there are no threads.
- Native configuration (`do-native-configure`, `ninja` only): builds.
- `clang-format --dry-run --Werror` is clean on every touched C file. For `aio.h`, which clang-format 21 here doesn't leave unchanged even in its original form, I kept only the struct change.

## Commits (audit/aio, on top of 40921d6cce)

- 4d4e199fff aio: Read and write files that can't seek with read and write
- 8afe766403 aio: Give each request its own descriptor
- a57cd3d27e aio: Keep appending writes and synchronization in queued order
- 0ffdd6358e aio: Let signals interrupt aio_suspend and lio_listio
- cf52b2cd5d aio: Act on a pending cancellation in aio_suspend
- d67fd90907 aio: Ignore aio_reqprio in aio_fsync
- 53670ab0b0 aio: Report EAGAIN from lio_listio(LIO_WAIT) when requests can't be queued
- 421f494a7c libos/fallback: Report the aio limits from sysconf
- 8d1daecbbf aio: Cancel requests that haven't started
- a016217a22 aio: Deliver SIGXFSZ when aio_write passes the file size limit
- a8f00c2359 test-posix: Check <aio.h> and the aio limits
- 6574adc60f test-posix: Check aio on pipes, appending writes, close and notification
- 78732a7101 test-posix: Check aio_fsync ordering, EBADF and ignored members
- 64c4e3713f test-posix: Check interrupted waits, limits and EAGAIN in aio_suspend and lio_listio
- 65b1e9f488 test-posix: Check canceling queued aio requests

Nothing pushed, and no AI attribution in any commit.
