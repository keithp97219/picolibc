# <threads.h> audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-threads-c11`, branch `audit/threads-c11`, based on d40e4ae0e4 (`claude/posix-spec-audit`). Host: aarch64 Linux (Graviton2, kernel 5.10), with builds and tests in the Debian sid chroot.

**Scope:**
- `<threads.h>` as POSIX.1-2024 includes it: the 25 functions (call_once, cnd_*, mtx_*, thrd_*, tss_*), and the header's types, constants and macros.
- The CX additions: thrd_t is pthread_t, signal behaviour and cancellation.
- The pthread objects underneath were covered by pthread-sync and pthread-threads and aren't retested. This unit tests the C11 layer's own mapping and semantics.

**Implementation:**
- `libc/threads/*.c` and `local-threads.h`: thin wrappers over pthreads, with `__thrd_result` mapping error numbers to `thrd_*` results.
- `libc/stdlib/call_once.c` (generic). New `libos/linux/call_once.c` and `libos/linux/thrd_create.c` replace the generic versions when POSIX threads are built.
- Header: `libc/include/threads.h`.

**Tests:** in `test/test-posix/test-threads/`. File names below drop the `test-` prefix.
- Existing: call_once.c, cnd_timedwait.c, mtx_timedlock.c, thrd_create.c, tss_create.c.
- New: threads_h.c, mtx_trylock.c, cnd_wait.c, thrd_sleep.c, thrd_join.c, thrd_exit.c, tss_delete.c.
- Extended: call_once.c and thrd_create.c.

**Spec source:** pubs.opengroup.org/onlinepubs/9799919799.
- Base definitions: basedefs/threads.h.
- Function pages: call_once, cnd_broadcast, cnd_destroy, cnd_timedwait, mtx_destroy, mtx_lock, thrd_create, thrd_current, thrd_detach, thrd_equal, thrd_exit, thrd_join, thrd_sleep, thrd_yield, tss_create, tss_delete, tss_get.
- XSH 2.2.2 (reserved names) and 2.9.5.2 (cancellation points).

**Status key:** TESTED (file:line), UNTESTED, NOT TESTABLE (with the reason). "(new)" marks rows this audit added tests for. Every TESTED row was run and passes on aarch64-linux-gnu and arm-linux-gnueabihf, both for picolibc and against glibc 2.43 (`-native`), unless the row says otherwise. "Read" means inferred from the code, not run.

## Requirement table

### Header (basedefs/threads.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | `__STDC_NO_THREADS__` is not defined (outside subprofiles) | TESTED (new) | threads_h.c:149-151 |
| H2 | thread_local expands to `_Thread_local` (before C23) | TESTED (new) | threads_h.c:127-131, 179, 265-268 (a per-thread object) |
| H3 | ONCE_FLAG_INIT initializes a once_flag | TESTED (new) | threads_h.c:225, 261-263 |
| H4 | TSS_DTOR_ITERATIONS is an integer constant usable in #if, equal to PTHREAD_DESTRUCTOR_ITERATIONS when <limits.h> defines it, at least 4, and no more than sysconf allows | TESTED (new) | threads_h.c:123-125, 145-147, 250-252 |
| H5 | cnd_t, mtx_t, once_flag, thrd_t and tss_t are complete object types | TESTED (new) | threads_h.c:81-83 |
| H6 | thrd_start_t is `int (*)(void *)` and tss_dtor_t is `void (*)(void *)` | TESTED (new) | threads_h.c:84-85 |
| H7 | [CX] thrd_t is the same type as pthread_t | TESTED (new) | threads_h.c:154 (`_Generic`) |
| H8 | mtx_plain, mtx_recursive, mtx_timed, thrd_busy, thrd_error, thrd_nomem, thrd_success and thrd_timedout are enumeration constants with distinct values, and the four mutex types are distinct | TESTED (new) | threads_h.c:88-120 (checks they aren't macros; duplicate case labels wouldn't compile) |
| H9 | The 25 functions have the specified prototypes | TESTED (new) | threads_h.c:197-222, 254-260 (through pointers of the exact types) |
| H10 | Including <threads.h> makes the <time.h> symbols visible | TESTED (new) | threads_h.c:75-78, 243-246 (types, macros and functions with only <threads.h> included) |
| H11 | Namespace: no `pthread_` or `sched_` names, which XSH 2.2.2 reserves only for <pthread.h> and <sched.h> | TESTED (new) | threads_h.c:60-72, 172 (static definitions that a declaration would conflict with) |

### call_once

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | init_routine runs exactly once per flag, across threads | TESTED | call_once.c:179-183; test-stdlib/test-call-once*.c |
| O2 | Completion synchronizes with later calls: callers return after it completes | TESTED | call_once.c:182; test-call-once-threads.c |
| O3 | [CX] call_once is not a cancellation point | TESTED (new) | thrd_join.c:111-112, 234-237 |
| O4 | [CX] A cancelled init_routine leaves the flag as if call_once had never been called | TESTED (new) | call_once.c:186-199. **Was a deviation; fixed** |
| O5 | [CX] A signal handler run during the call doesn't affect it | TESTED (new) | call_once.c:205-212 |
| O6 | Callers using other flags don't wait for a running init_routine. Derived: otherwise an init_routine that waits for a thread using call_once deadlocks | TESTED (new) | call_once.c:201-203. **Was a deviation; fixed along with O4** |
| O7 | A nested call with the same flag returns at once (picolibc's choice; the spec says such a call won't return) | TESTED (picolibc's choice) | test-stdlib/test-call-once.c count_nested; the new code keeps this |

### cnd_*

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | cnd_init succeeds, and a destroyed variable can be initialized again | TESTED | cnd_timedwait.c; (new) cnd_wait.c:276-279 |
| C2 | cnd_init returns only thrd_success, thrd_nomem or thrd_error | Read | pthread_cond_init has no failure path on Linux; an EAGAIN would now map to thrd_error |
| C3 | cnd_signal and cnd_broadcast with no waiters have no effect and return thrd_success | TESTED (new) | cnd_wait.c:256-258 |
| C4 | cnd_signal wakes at least one waiter, cnd_broadcast all of them | TESTED | cnd_timedwait.c (broadcast with two waiters); signal ordering is pthread-sync's |
| C5 | Unblocked threads own the mutex when they return | TESTED | cnd_timedwait.c; (new) cnd_wait.c:165-173 |
| C6 | cnd_timedwait returns thrd_timedout after the TIME_UTC deadline, not before, and still reacquires the mutex | TESTED (new) | cnd_wait.c:261-266 |
| C7 | A deadline in the past gives thrd_timedout | TESTED (new) | cnd_wait.c:267-269 |
| C8 | An invalid deadline gives thrd_error, with the mutex still held | TESTED (new) | cnd_wait.c:270-273 |
| C9 | [CX] For a waiter in cnd_wait or cnd_timedwait, a signal either resumes the wait or returns thrd_success | TESTED (new) | cnd_wait.c:145-175, 285-286 |
| C10 | [CX] cnd_wait and cnd_timedwait are cancellation points, and the mutex is reacquired before the first cleanup handler | TESTED (new) | blocked: cnd_wait.c:291-301; pending request: cnd_wait.c:207-240, 303-304. Closes the cnd_wait gap left in pthread-threads X11 |
| C11 | [CX] A cancelled waiter doesn't consume a signal | NOT TESTABLE | a race with no observable handshake (as pthread-threads X17) |
| C12 | Release is atomic with respect to cnd_signal and cnd_broadcast; 2.9.9 copies | Read | inherited from pthread_cond_*; pthread-sync covers it |

### mtx_*

| # | Requirement | Status | Reference |
|---|---|---|---|
| M1 | mtx_init accepts the four valid types, returns thrd_success and gives an unlocked mutex | TESTED | mtx_timedlock.c; (new) mtx_trylock.c:224-253 |
| M2 | mtx_init returns only thrd_success or thrd_error | TESTED (picolibc only) | mtx_timedlock.c (type 42 gives thrd_error). **Fixed by reading**: an ENOMEM or EAGAIN from the pthread layer could have come back as thrd_nomem |
| M3 | A destroyed mutex can be initialized again | TESTED (new) | mtx_trylock.c:232-235 |
| M4 | mtx_trylock on a non-recursive mutex the caller holds returns thrd_busy without blocking | TESTED (new) | mtx_trylock.c:226-228 (mtx_plain and mtx_timed) |
| M5 | mtx_trylock on a mutex another thread holds returns thrd_busy, and succeeds after the unlock | TESTED | mtx_timedlock.c; (new) mtx_trylock.c:229-231 |
| M6 | Recursive mutexes: the owner's mtx_lock, mtx_trylock and mtx_timedlock each add one to the lock count, and the mutex is free only at zero | TESTED (new) | mtx_trylock.c:238-253 |
| M7 | At the lock-count limit, mtx_lock and mtx_timedlock return thrd_error (never thrd_nomem), and mtx_trylock returns thrd_error or thrd_busy | TESTED (new, picolibc only) | mtx_trylock.c:257-277. **Was a deviation (thrd_nomem); fixed.** The test starts near UINT_MAX by setting picolibc's `__count` |
| M8 | mtx_timedlock times out with thrd_timedout, not before the TIME_UTC deadline | TESTED | mtx_timedlock.c; (new) mtx_trylock.c:126-140, 292 |
| M9 | [CX] Never a timeout if the mutex can be locked at once, even with a past deadline | TESTED | mtx_timedlock.c; (new) mtx_trylock.c:280-283 |
| M10 | A past deadline on a held mutex gives thrd_timedout | TESTED (new) | mtx_trylock.c:293 |
| M11 | An invalid deadline on a held mutex gives thrd_error; on a free mutex it need not be checked | TESTED (new) | mtx_trylock.c:284-289, 294 |
| M12 | [CX] A signal to a thread waiting in mtx_lock or mtx_timedlock resumes the wait | TESTED (new) | mtx_trylock.c:177-210, 302-303 |
| M13 | mtx_unlock returns thrd_success and makes the mutex available | TESTED | mtx_timedlock.c; mtx_trylock.c:230-231 |
| M14 | The mtx_* functions are not cancellation points | TESTED (new) | thrd_join.c:117-126, 234-237 |
| M15 | Lock and unlock synchronize, in a total order; the scheduling policy picks the next owner | Read / NOT TESTABLE | pthread-sync's area; real-time policies can't be used here (RLIMIT_RTPRIO 0) |

### thrd_*

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | thrd_create runs func(arg) and stores the new thread's ID | TESTED | thrd_create.c:181-183 |
| T2 | Returning from func is thrd_exit with its return value | TESTED | thrd_create.c:183; (new) thrd_join.c:193-198 (0, 1, -1, -12345, INT_MAX, INT_MIN) |
| T3 | [CX] A new thread's signal mask, pending signals, locale, floating-point environment and alternate stack | Read | thrd_create calls pthread_create with default attributes; pthread-threads C6-C10 test these |
| T4 | thrd_nomem when no memory could be allocated for the thread | TESTED (new, picolibc only) | thrd_create.c:205-217 (RLIMIT_AS in a child). **Gave thrd_error once T5 was fixed; fixed.** glibc returns thrd_error. Skipped under qemu, which ignores RLIMIT_AS |
| T5 | [CX] thrd_error at the thread limit ({PTHREAD_THREADS_MAX}) | TESTED (new, glibc only) | thrd_create.c:194-204 (RLIMIT_NPROC in a child). picolibc has no RLIMIT_NPROC, so the check can't run there. **Was thrd_nomem (read); fixed** |
| T6 | If thrd_create fails, no thread is created | Read | pthread_create unmaps the memory and restores the thread count |
| T7 | thrd_current identifies the caller, and thrd_equal compares IDs | TESTED | thrd_create.c:180-182; thread_local per thread: threads_h.c:265-268 |
| T8 | thrd_detach returns thrd_success | TESTED | thrd_create.c:188-189 |
| T9 | thrd_join waits, stores the exit status (any int), and accepts a NULL res | TESTED | thrd_create.c:183; (new) thrd_join.c:193-200 |
| T10 | [CX] If thrd_join detects a deadlock (a thread joining itself), it returns thrd_error | TESTED (new, picolibc only) | thrd_join.c:202-205. Undefined behaviour in general, so picolibc only |
| T11 | [CX] thrd_join isn't affected by a signal handler | TESTED (new) | thrd_join.c:207-222 |
| T12 | [CX] thrd_join is a cancellation point, and a cancelled joiner leaves the target joinable | TESTED (new) | thrd_join.c:224-230. Closes the thrd_join gap left in pthread-threads X11 |
| T13 | Thread exit runs the destructors of every key, tss_t and pthread_key_t alike, setting each value to NULL first | TESTED | tss_create.c; (new) tss_delete.c:207-227 |
| T14 | Destructor rounds repeat up to PTHREAD_DESTRUCTOR_ITERATIONS times | TESTED (new) | tss_delete.c:218-222. A destructor that always resets its value runs 4 times with both libraries; one that resets it twice runs 3 times |
| T15 | [CX] Thread exit releases no mutexes and runs no atexit handlers | TESTED (new) | thrd_exit.c:85-96, 116-121, 159 |
| T16 | [CX] thrd_exit from the initial thread: the other threads keep running and destructors run. The process exits with status 0 (as exit(0) would, so atexit handlers run) after the last thread ends | TESTED (new) | thrd_exit.c:123-128, 155-159 (events "MDTA", status 0 rather than 7 or 5) |
| T17 | thrd_sleep returns 0 after at least the duration, as TIME_UTC measures it; duration and remaining may be the same object | TESTED | thrd_create.c:191; (new) thrd_sleep.c:203-217 |
| T18 | thrd_sleep interrupted by a caught signal returns -1 and stores the remaining time (requested minus slept) | TESTED (new) | thrd_sleep.c:124-160, 238-239 (with two timespecs and with one shared). See the timer-slack note below |
| T19 | [CX] When thrd_sleep returns a negative value, errno is EINTR after an interruption and EINVAL for tv_nsec < 0 or >= 1000000000 | TESTED (new; errno for picolibc and other libraries, not glibc) | thrd_sleep.c:139-143, 219-231. glibc 2.43 leaves errno alone |
| T20 | An ignored signal doesn't end the sleep | TESTED (new) | thrd_sleep.c:241-250 |
| T21 | A signal whose action is to terminate the process ends it | NOT TESTABLE | indistinguishable from the process being killed before the call |
| T22 | [CX] thrd_sleep is a cancellation point | TESTED (new) | thrd_sleep.c:252-255 |
| T23 | [CX] thrd_yield gives up the processor until the thread is again at the head of its thread list | NOT TESTABLE | needs SCHED_FIFO; it calls sched_yield (read) |
| T24 | thrd_create, thrd_detach, thrd_current, thrd_equal and thrd_yield aren't cancellation points | TESTED (new) | thrd_join.c:127-133, 234-237 |

### tss_*

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | tss_create sets a unique key and returns thrd_success | TESTED | tss_create.c; (new) tss_delete.c:207-214 |
| S2 | A new key reads NULL in existing threads and in new threads | TESTED | tss_create.c (new thread); (new) tss_delete.c:235-242 (a thread that existed before the key) |
| S3 | Destructors don't run at process termination | TESTED (new) | tss_delete.c:166-191, 244-256 |
| S4 | tss_delete runs no destructors, and the key's destructor no longer runs at thread exit | TESTED (new) | tss_delete.c:107-108, 226 |
| S5 | tss_delete can be called from a destructor | TESTED (new) | tss_delete.c:143-148, 229-233 |
| S6 | tss_get returns the value, or NULL when none is set | TESTED | tss_create.c |
| S7 | tss_set doesn't invoke the destructor on the value it replaces | TESTED (new) | tss_delete.c:103-104, 222 |
| S8 | [CX] tss_get in a key's destructor returns NULL unless the value was reset; tss_set from a destructor is allowed | TESTED (new) | tss_delete.c:78-80, 87-91, 224 |
| S9 | No destructor call for NULL values or for keys with a NULL destructor | TESTED (new) | tss_delete.c:100, 225 |
| S10 | tss_create and tss_set return thrd_error when the request can't be honored | NOT TESTABLE | the key table holds __PTHREAD_KEYS_MAX keys. pthread-threads K7 covers the EAGAIN at that limit, which maps to thrd_error (read) |
| S11 | The tss_* functions aren't cancellation points | TESTED (new) | thrd_join.c:134-137, 234-237 |

### Counts

86 requirements (rows):
- **TESTED: 71.** 15 were tested before (some only partly) and 56 are new. Rows that were partly tested got stronger checks too: C1, C5, M1, M5, M8, M9, T2, T9, T13, T17, S1 and S2.
- **Checks limited to one library:**
  - picolibc only: M2 (invalid type), M7 (uses picolibc's lock count), T4 (glibc gives thrd_error), T10 (undefined behaviour in general), O7 (picolibc's choice).
  - glibc only: T5 (picolibc has no RLIMIT_NPROC).
  - Under `#ifndef __GLIBC__`: the errno checks of T19.
- **Read only: 4** (C2, C12, T3, T6), plus the read part of M15.
- **NOT TESTABLE: 5** (C11, M15's scheduling part, T21, T23, S10).

## Deviations found and fixed

Each one was verified by running. I reverted only the library change (`git show <fix> -- libc libos | git apply -R`; for the thrd_create change I also removed the file from libos/linux/meson.build), rebuilt, ran the test three times, and reapplied the change. This was on aarch64-linux-gnu.

1. **EAGAIN came back as thrd_nomem** (libc/threads/local-threads.h, mtx_init.c).
   - `__thrd_result` mapped EAGAIN to thrd_nomem. pthread functions give EAGAIN when a limit is reached: the lock count of a recursive mutex, or the number of threads.
   - Spec:
     - mtx_lock and mtx_unlock "shall return thrd_success on success, or thrd_error if the request could not be honored". mtx_timedlock adds only thrd_timedout, and mtx_trylock only thrd_busy.
     - thrd_create [CX]: "thrd_error if the request could not be honored, such as if the system-imposed limit on the total number of threads in a process {PTHREAD_THREADS_MAX} would be exceeded".
     - mtx_init "shall return thrd_success on success or thrd_error if the request could not be honored".
   - Fix: EAGAIN maps to thrd_error, as in glibc, and mtx_init returns only thrd_success or thrd_error.
   - Before: `mtx_lock past the lock count limit: mtx_lock(&mtx) == thrd_error` in 3 of 3 runs.
2. **thrd_create couldn't report thrd_nomem for a thread it couldn't map** (libos/linux/pthread_create.c, new libos/linux/thrd_create.c).
   - pthread_create must report a failed stack mapping as EAGAIN, so after fix 1 that case gave thrd_error.
   - Spec (ISO C and POSIX): "thrd_nomem if no memory could be allocated for the thread requested".
   - Fix:
     - pthread_create's body is now `_pthread_create`. It returns ENOMEM when the mapping fails or the sizes overflow, and ENOMEM or EAGAIN for a failed clone.
     - pthread_create turns ENOMEM into EAGAIN, so it behaves as before.
     - A Linux thrd_create replaces the generic one. It calls `_pthread_create`, maps ENOMEM to thrd_nomem and maps everything else to thrd_error.
     - In the pthread layer, this only splits the function.
   - Before (with fix 1 applied): `thrd_create without memory: res == thrd_nomem` in 3 of 3 runs (the result was thrd_error).
   - Before both fixes, every failure gave thrd_nomem, including the thread limit.
3. **call_once held the global libc lock while init_routine ran** (libc/stdlib/call_once.c; new libos/linux/call_once.c).
   - The flag was set, and `__LIBC_LOCK` held, across init_routine.
   - A cancelled init_routine left the lock held and the flag set. Every later call_once then hung, as did every other user of that lock in any thread.
   - Callers using other flags also waited, so an init_routine that waited for a thread calling call_once deadlocked.
   - Spec [CX]: "if init_routine is a cancellation point and is canceled, the effect on flag shall be as if call_once() was never called."
   - Fix (Linux with POSIX threads):
     - The flag holds 0, the ID of the thread running init_routine, or -1 once done. Other callers wait on the flag with a futex.
     - A cleanup handler resets the flag to 0 and wakes the waiters, as pthread_once does.
     - A nested call on the same flag from init_routine still returns at once, as picolibc documents and test-stdlib/test-call-once checks.
     - The generic version still serves builds without POSIX threads.
   - Before: test-call_once hung and was killed by its alarm (exit 142) in 3 of 3 runs.
   - The cancellation check runs first, so it is the one that hung. I didn't run the other-flag check on its own against the old code.

## Deviations found, not changed

- **Linux can report more remaining time than was requested** (T18).
  - The kernel computes nanosleep's remaining time from the timer's hard expiry, which includes the thread's timer slack (50 us by default).
  - When the signal arrived 47 us after the start, an interrupted 30 s sleep reported 30.000008 s left.
  - Spec: "the amount of time remaining (the requested interval minus the time actually slept)".
  - picolibc and glibc both pass the kernel's value through. The test allows 1 ms over.
  - Clamping belongs in nanosleep, which is the time unit's; noted in cross-cutting.md.

## Other observations (not changed)

- glibc 2.43's thrd_sleep doesn't set errno, which the CX text requires. Its thrd_create reports a stack it can't map as thrd_error. Both are in cross-cutting.md, and the tests skip only glibc for them.
- thrd_create mallocs a small block to pass func and arg to the new thread. If that fails, thrd_create returns thrd_nomem, which is correct.
- thrd_join of a thread that called pthread_exit or was cancelled is undefined. picolibc truncates the `void *` to an int.
- cnd_timedwait and mtx_timedlock work on an mtx_plain mutex. The application "shall ensure" the mutex supports timeouts, so this is allowed.
- In a child forked while another thread is running a call_once init_routine, the flag keeps that thread's ID. A call_once on that flag in the child would then wait forever. pthread_once behaves the same way, and after fork a multithreaded process may only make async-signal-safe calls.

## Left undone

- C11, T21, T23 and the scheduling part of M15, which can't be tested here.
- C2, C12, T3 and T6, which were only read.
- T5 can't run against picolibc, because picolibc has no RLIMIT_NPROC.

## Timing and robustness of the new tests

- Every new test sets alarm(60) (test-thrd_sleep sets 120), so a call that wrongly blocks ends the test.
- Handshakes use semaphores. In the check that cancels a thread blocked in cnd_wait, main locks the mutex the waiter released, which proves the waiter is blocked.
- The signal checks send 5 signals 10 ms apart. A signal that arrives before the thread blocks only weakens the check.
- A signal sent before thrd_sleep starts doesn't end the sleep, so the interrupted-sleep check keeps signalling every 20 ms until the sleeper returns. The host security software didn't interfere.
- The call_once cancellation check gives the second caller 20 ms to start waiting. If it arrives later under load, the check only gets weaker.
- Timeouts are checked as "not before the deadline", never "within N ms".
- The thrd_nomem check runs in a child. It sets RLIMIT_AS to the current VmSize plus 32 KiB, which is less than the 128 KiB default stack. It is skipped under qemu.

## Test runs (verified by running)

All runs were in the Debian sid chroot on the aarch64 host, at the final commit, without `--no-rebuild`.

| Target | Command | Ok | Expected Fail | Fail | Skipped |
|---|---|---|---|---|---|
| aarch64-linux-gnu (`-Dwerror=true`) | `meson test -t 4 --num-processes 3` | 1322 | 6 | 0 | 42 |
| arm-linux-gnueabihf (`-Dwerror=true`) | same | 1319 | 6 | 0 | 44 |

- The native configuration (`scripts/do-native-configure -Dwerror=true`, `ninja` only) builds.
- CMake thumbv7m (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 551 tests, 100% passed (249 ran, 302 skipped). The 7 new tests are registered and skip on bare metal.
- Bare-metal arm under meson was not run on this host, as the brief specifies.
- `clang-format --dry-run -Werror` is clean on every touched C file and header.

## Commits (audit/threads-c11, on top of d40e4ae0e4)

- 5f348841b0 threads: Report EAGAIN as thrd_error
- b145ee72d8 libos/linux: Report thrd_nomem when thrd_create can't map a thread
- ec216528a6 libos/linux: Wait on each call_once flag separately
- a162ca2e6d test-posix: Check threads.h, C11 thread results, destructors and cancellation
- 6c5c3b18bb test-posix: Check cancelled call_once functions and thrd_create errors

Nothing pushed, and no AI attribution in any commit.
