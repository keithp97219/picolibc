# <pthread.h> thread management audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-pthread-threads`, branch `audit/pthread-threads`, based on 4af0c34b98 (`claude/posix-spec-audit`). Host: aarch64 Linux (Graviton2), with builds and tests in the Debian sid chroot.

**Scope:**
- creating, terminating, joining and detaching threads, plus pthread_self and pthread_equal
- the pthread_attr_* functions
- pthread_once
- thread-specific data
- cancellation: pthread_cancel, setcancelstate, setcanceltype, testcancel, the cleanup macros, and the cancellation points of XSH 2.9.5
- pthread_atfork
- the scheduling functions
- the thread-specific part of pthread_getcpuclockid
- the non-synchronization part of the header

The synchronization objects were the pthread-sync unit's and are untouched here. Two pairs are no longer in POSIX but are still provided, and their existing tests still run: pthread_getconcurrency/setconcurrency (removed in Issue 8) and pthread_attr_getstackaddr/setstackaddr (removed in Issue 7).

**Implementation:**
- `libos/linux/pthread_*.c`, `_pthread_*.c`, `__pthread_cleanup_*.c`, `local-pthread.h`, `local-cancel.h`, and the cancellation-point wrappers in libos/linux
- headers `libc/include/pthread.h` and `libc/include/sys/_pthreadtypes.h`

**Tests:** in `test/test-posix/test-pthread/`. File names below drop the `test-pthread_` prefix, so `testcancel.c` is `test-pthread_testcancel.c`.

**Spec source:** pubs.opengroup.org/onlinepubs/9799919799.
- Base definitions: basedefs/pthread.h and basedefs/sys_types.h.
- XSH sections 2.9.2 and 2.9.5.
- Function pages:
  - thread lifecycle: pthread_create, pthread_exit, pthread_join, pthread_detach, pthread_self, pthread_equal
  - attributes: pthread_attr_destroy, _getdetachstate, _getguardsize, _getinheritsched, _getschedparam, _getschedpolicy, _getscope, _getstack, _getstacksize
  - once and keys: pthread_once, pthread_key_create, pthread_key_delete, pthread_getspecific
  - cancellation: pthread_cancel, pthread_setcancelstate, pthread_cleanup_pop
  - other: pthread_atfork, pthread_getschedparam, pthread_setschedprio, pthread_getcpuclockid

**Status key:** TESTED (test file:line), UNTESTED, NOT TESTABLE (with the reason), Not applicable. "(new)" marks rows this audit added tests for. Every TESTED row was run and passes on aarch64-linux-gnu and arm-linux-gnueabihf, both for picolibc (`test-X`) and against glibc 2.43 (`test-X-native`), unless the row says picolibc only. "Read" means inferred from the code, not run.

## Requirement table

### Header (basedefs/pthread.h, thread-management part)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | PTHREAD_CANCEL_{ASYNCHRONOUS,ENABLE,DEFERRED,DISABLE}, PTHREAD_CANCELED, PTHREAD_CREATE_{DETACHED,JOINABLE}, PTHREAD_{EXPLICIT,INHERIT}_SCHED, PTHREAD_ONCE_INIT, PTHREAD_SCOPE_{PROCESS,SYSTEM} are defined, and the values of each kind are distinct | TESTED (new) | h_threads.c:41-51 (as macros, picolibc only), 54-58 |
| H2 | PTHREAD_NULL is a constant initializer for pthread_t and equals no thread ID | TESTED (new, picolibc only) | h_threads.c:63-66, 238-242. **Was missing; fixed.** glibc 2.43 doesn't define it |
| H3 | PTHREAD_ONCE_INIT and PTHREAD_CANCELED work as static initializers | TESTED (new) | h_threads.c:61-62 |
| H4 | PTHREAD_CANCELED is a `void *`, not NULL, matching no object | TESTED (new) | h_threads.c:230-235 |
| H5 | pthread_attr_t, pthread_key_t, pthread_once_t and pthread_t are defined | TESTED (new) | h_threads.c:61, 69-71 |
| H6 | The 39 thread-management functions have the specified prototypes | TESTED (new) | h_threads.c:97-138 (checked by initializing function pointers) |
| H7 | pthread_cleanup_push/pop work as a pair of statements in one scope | TESTED (new) | h_threads.c:161-168, 244-247 |
| H8 | <sched.h> and <time.h> symbols are visible | TESTED (new) | h_threads.c:73-87 (types, SCHED_*, CLOCK_REALTIME, and calls to sched_yield and nanosleep with only <pthread.h> included; implicit declarations are errors) |

### pthread_create

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Runs start_routine(arg) and stores the thread ID | TESTED | create.c:173-183 |
| C2 | A NULL attr gives the default attributes | TESTED | create.c:173, and most other tests |
| C3 | Changing attr afterwards doesn't affect the thread | TESTED (new) | attr_setguardsize.c:345-348 |
| C4 | Returning from start_routine is an implicit pthread_exit | TESTED | create.c:173-177; key_create.c:113-121 |
| C5 | Returning from main is exit() with main's value | TESTED | every test's exit status |
| C6 | The signal mask is inherited | TESTED | sigmask.c:78-85; (new) join.c:289-302 |
| C7 | The new thread has no pending signals | TESTED (new) | join.c:293, 301, 304 |
| C8 | The alternate signal stack isn't inherited | TESTED (new) | join.c:295-303 |
| C9 | The thread's current locale isn't inherited | TESTED (new) | join.c:311-320 |
| C10 | The floating-point environment is inherited | TESTED (new) | join.c:322-340 (rounding mode and FE_INEXACT, where the target supports them) |
| C11 | [TCT] The new thread's CPU-time clock starts at zero | TESTED (new) | join.c:342-352. The clock semantics belong to the time unit |
| C12 | If it fails, no thread is created | UNTESTED | read: pthread_create unmaps the memory and restores `_pthread_count` |
| C13 | EAGAIN when resources are lacking | TESTED (new) | attr_setguardsize.c:221-262, 295. Asks for a stack larger than the address space (64-bit) or than RLIMIT_AS (32-bit). Skipped on 32-bit targets under qemu, which ignores RLIMIT_AS |
| C14 | EAGAIN at {PTHREAD_THREADS_MAX} | NOT TESTABLE | the limit isn't defined (indeterminate); the real limit is RLIMIT_NPROC, which every process of this user on the host shares |
| C15 | EPERM without the privilege for the requested scheduling | TESTED | setschedparam.c:106-111 accepts either result; (new) attr_setguardsize.c:409-420 requires exactly EPERM when the caller can't switch itself to SCHED_FIFO |
| C16 | Never returns EINTR | UNTESTED | read: no code path returns EINTR |

### pthread_exit and thread termination

| # | Requirement | Status | Reference |
|---|---|---|---|
| E1 | value_ptr is available to a join | TESTED | create.c:179-183 |
| E2 | Cleanup handlers are popped and run in reverse order | TESTED | cancel.c:302-313 |
| E3 | Thread-specific data destructors run after them | TESTED | cancel.c:305 |
| E4 | Mutexes and file descriptors are not released | TESTED (new) | join.c:278-284 |
| E5 | No atexit handlers run | TESTED (new) | join.c:278-281 |
| E6 | After the last thread ends, the process exits with status 0 as if by exit(0) | TESTED | exit.c; atfork.c child_main |
| E7 | XSH 2.9.5.3: pthread_exit first sets DISABLE and DEFERRED, and the state stays disabled until the thread terminates | TESTED (new) | testcancel.c:870-883. **Was a deviation; fixed** |
| E8 | The initial thread can call pthread_exit and then be joined | TESTED (new) | join.c:266-275 |

### pthread_join

| # | Requirement | Status | Reference |
|---|---|---|---|
| J1 | Waits for the target and stores its value | TESTED | create.c:174-183 |
| J2 | Works when the target has already terminated | TESTED | cancel.c:316-322 |
| J3 | Accepts a NULL value_ptr | TESTED | create.c:189 |
| J4 | When it returns, the target has terminated, destructors included | TESTED | key_create.c:116-121 |
| J5 | A cancelled joiner leaves the target joinable | TESTED (new) | join.c:254-264 |
| J6 | EDEADLK (may fail) | TESTED | create.c:213-217 |
| J7 | Never returns EINTR | TESTED (new) | join.c:241-252 (signals with a non-restarting handler) |
| J8 | Is a cancellation point | TESTED | cancel.c:278-279 |

### pthread_detach, pthread_self, pthread_equal

| # | Requirement | Status | Reference |
|---|---|---|---|
| D1 | Detaches a joinable thread and returns 0 | TESTED | create.c:268-276 |
| D2 | Doesn't terminate the thread | TESTED | create.c:268-276 |
| D3 | Storage is reclaimed and no zombie remains | NOT TESTABLE | nothing observable; read: `_pthread_zombie`/`_pthread_reap` unmap the thread |
| D4 | Never returns EINTR | UNTESTED | read |
| S1 | pthread_self returns the caller's ID | TESTED | create.c:205-210 |
| S2 | pthread_equal is non-zero exactly when the IDs are equal | TESTED | create.c:205, 209-210; PTHREAD_NULL (new) h_threads.c:238-242 |

### Thread attributes

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | init sets the defaults and returns 0; a destroyed object can be initialized again | TESTED | create.c:220-224, 241-247; setschedparam.c:83-88 |
| A2 | The attributes define the created thread | TESTED | create.c:233-258 |
| A3 | One attributes object can be used by simultaneous pthread_create calls | TESTED (new) | attr_setguardsize.c:351-359 |
| A4 | init ENOMEM | NOT TESTABLE | nothing is allocated |
| A5 | detachstate defaults to JOINABLE | TESTED | create.c:221-222 |
| A6 | detachstate round trip for both values | TESTED (new) | attr_setguardsize.c:336-342 |
| A7 | DETACHED makes threads that start detached | TESTED | create.c:263-276 |
| A8 | setdetachstate EINVAL (shall fail) | TESTED | create.c:225 |
| A9 | getguardsize returns the value set, even when it was rounded | TESTED | create.c:235-236; values 1000 and 0 (new) attr_setguardsize.c:298-306 |
| A10 | A guard area of at least guardsize bytes exists, and overflowing into it is an error | TESTED (new) | attr_setguardsize.c:309-313 (in a child, expects SIGSEGV) |
| A11 | guardsize 0 provides no guard area | NOT TESTABLE | absence can't be observed |
| A12 | guardsize is ignored when stackaddr is set | NOT TESTABLE | there's no protection to observe; read |
| A13 | Stack or guard sizes the system can't provide aren't silently reduced | TESTED (new) | attr_setguardsize.c:316-318. **Was a deviation; fixed** |
| A14 | setguardsize EINVAL for an invalid guardsize | Not applicable | no value is invalid when set; sizes that can't be met fail in pthread_create (A13) |
| A15 | inheritsched round trip; the default (implementation-defined) is INHERIT | TESTED | setschedparam.c:84, 102-103 |
| A16 | With INHERIT, the attribute's scheduling is ignored | TESTED (new) | attr_setguardsize.c:370-379 |
| A17 | With EXPLICIT, scheduling comes from the attribute | TESTED | setschedparam.c:106-120; SCHED_FIFO (new) attr_setguardsize.c:409-420 |
| A18 | setinheritsched EINVAL (may fail); ENOTSUP (shall fail) | TESTED / Not applicable | setschedparam.c:90; both values are supported, so ENOTSUP can't arise |
| A19 | schedparam round trip | TESTED | setschedparam.c:97-101 |
| A20 | setschedparam ENOTSUP (shall fail) and EINVAL (may fail) | Not applicable | every priority is stored; out-of-range ones fail in pthread_create with EINVAL |
| A21 | SCHED_FIFO, SCHED_RR and SCHED_OTHER are supported | TESTED | setschedparam.c:95-96; FIFO and OTHER (new) attr_setguardsize.c:362-368 |
| A22 | setschedpolicy ENOTSUP / EINVAL | TESTED | setschedparam.c:91 |
| A23 | SCHED_FIFO/RR threads waiting on a mutex acquire it in priority order | NOT TESTABLE | the chroot user has RLIMIT_RTPRIO 0 and no CAP_SYS_NICE |
| A24 | contentionscope SYSTEM works; PROCESS is ENOTSUP (shall fail) | TESTED | setschedparam.c:86, 92-93 |
| A25 | setscope EINVAL (may fail) | UNTESTED | read |
| A26 | setstack/getstack round trip of base and size | TESTED | create.c:248-253; attr_setstackaddr.c:84-90 |
| A27 | The thread runs on the given stack | TESTED | attr_setstackaddr.c:91-93; create.c:255-258 |
| A28 | setstack EINVAL when stacksize < PTHREAD_STACK_MIN (shall fail) | TESTED (new) | attr_setguardsize.c:328-331 |
| A29 | setstack: EINVAL above a limit (shall fail); EINVAL for misalignment and EACCES (may fail) | Not applicable | nothing is checked when set (allowed); see A13 |
| A30 | stacksize round trip | TESTED | create.c:233-234 |
| A31 | The thread gets at least stacksize bytes of stack | TESTED | create.c:237-240; uses 768 KiB of a 1 MiB stack (new) attr_setguardsize.c:320-325 |
| A32 | setstacksize EINVAL when stacksize < PTHREAD_STACK_MIN | TESTED | create.c:226 |

### pthread_once

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | The first call runs init_routine; later calls don't | TESTED | once.c:86-92 |
| O2 | init_routine has completed when any caller returns | TESTED | once.c:89 |
| O3 | Is not a cancellation point | TESTED (new) | testcancel.c:664 |
| O4 | A cancelled init_routine leaves once_control as if never called, and a waiting caller then runs its routine | TESTED (new) | testcancel.c:914-923. **Was a deviation; fixed** |

### Thread-specific data

| # | Requirement | Status | Reference |
|---|---|---|---|
| K1 | Keys are visible to every thread; values are per thread | TESTED | key_create.c:107-121 |
| K2 | A new key reads NULL in every active thread | TESTED | key_create.c:126-127; in an existing thread after a delete (new) key_delete.c:176-184 |
| K3 | A new thread reads NULL for every key | TESTED | key_create.c use_key |
| K4 | The destructor gets the old value, after the key's value is set to NULL | TESTED | key_create.c:120-121; (new) key_delete.c:188-195 |
| K5 | No destructor call for NULL values | TESTED (new) | key_delete.c:197-201 |
| K6 | Destructor rounds repeat, at least PTHREAD_DESTRUCTOR_ITERATIONS times | TESTED | key_create.c; (new) key_delete.c:204-210 |
| K7 | key_create EAGAIN beyond PTHREAD_KEYS_MAX (shall fail) | TESTED (new) | key_delete.c:230-243 |
| K8 | ENOMEM from key_create and setspecific | NOT TESTABLE | the key table is static and value storage is preallocated |
| K9 | pthread_key_delete runs no destructors, and a deleted key's destructor isn't called at exit | TESTED (new) | key_delete.c:176-185 |
| K10 | pthread_key_delete can be called from a destructor | TESTED (new) | key_delete.c:212-219 |
| K11 | getspecific/setspecific round trip; NULL when no value is set | TESTED | key_create.c:110-119 |
| K12 | getspecific/setspecific can be called from destructors | TESTED | key_create.c destructor_again |
| K13 | The initial thread runs destructors in pthread_exit | TESTED (new) | key_delete.c:221-228 |

### Cancellation

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | pthread_cancel returns 0, including for a thread that has terminated | TESTED | cancel.c:316-322 |
| X2 | Cleanup handlers run, then destructors, then the thread ends with PTHREAD_CANCELED | TESTED | cancel.c:300-305 |
| X3 | Cancellation is processed asynchronously to the caller | TESTED | cancel.c:224-235 |
| X4 | Every thread starts with ENABLE and DEFERRED | TESTED | main thread: cancel.c:265-270; new threads (new) testcancel.c:865-868 |
| X5 | setcancelstate/setcanceltype return the old value; EINVAL (may fail) | TESTED | cancel.c:265-270 |
| X6 | testcancel is a cancellation point and has no effect when cancellation is disabled | TESTED | cancel.c:283-290, 300-305 |
| X7 | With cancellation disabled, requests are held pending | TESTED | cancel.c:283-290 |
| X8 | With cancellation disabled, a request doesn't disturb the thread: no EINTR from a sleep | TESTED (new) | testcancel.c:895-903. **Was a deviation; fixed** |
| X9 | Asynchronous cancellation can act at any time | TESTED | cancel.c:292-297 |
| X10 | Deferred requests are held until a cancellation point | TESTED (new) | testcancel.c:905-912 |
| X11 | The 57 required cancellation points act on a pending request before returning | TESTED (new) for 52 | testcancel.c:602-660, 853-863. accept4, pthread_cond_clockwait, posix_close and ppoll are picolibc only. pthread_join and pthread_testcancel are covered in cancel.c, aio_suspend in X15. cnd_wait and thrd_join are UNTESTED |
| X12 | A thread blocked at a cancellation point is woken and the request acted on | TESTED | cancel.c:272-279; aio_suspend (new, picolibc only) testcancel.c:930-940 |
| X13 | No cancellation points are introduced into other functions | TESTED (new) for a sample | testcancel.c:661-672. if_nameindex is checked for picolibc only, because glibc's is a cancellation point. **The if_* functions were deviations; fixed** |
| X14 | Acting on a request while suspended has only the side effects of an EINTR return | UNTESTED | **Deviation found by reading**, see below |
| X15 | A cancelled aio_suspend leaves asynchronous I/O usable | TESTED (new, picolibc only) | testcancel.c:926-972. **Was a deviation; fixed.** glibc returns at once from aio_suspend with only NULL entries, which the test relies on to block |
| X16 | Cancelled condition wait: the mutex is reacquired before the first cleanup handler | TESTED | cancel.c:87-94, 274 |
| X17 | Cancelled condition wait: no pending condition signal is consumed | NOT TESTABLE | a race with no observable handshake |
| X18 | Acting on a request sets DISABLE and DEFERRED | TESTED (new) | testcancel.c:885-893. Fixed by the pthread_exit change |
| X19 | cleanup_pop with a non-zero argument runs the handler, with 0 it doesn't; handlers run at exit | TESTED | cancel.c:305-313; h_threads.c:244-247 |
| X20 | The cancellation functions are async-cancel-safe, and setcancelstate is async-signal-safe | NOT TESTABLE | a property; read |

### pthread_atfork ([OB])

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | prepare handlers run before fork, in reverse order | TESTED | atfork.c:152-172 |
| F2 | parent and child handlers run in registration order | TESTED | atfork.c:166-167, child_main |
| F3 | NULL handlers are allowed | TESTED | atfork.c:155 |
| F4 | Handlers run in the thread that calls fork | UNTESTED | read |
| F5 | ENOMEM | NOT TESTABLE | would need realloc to fail |

### Scheduling and CPU-time clock

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | getschedparam returns the policy and parameters | TESTED | setschedparam.c:124-125 |
| P2 | setschedparam sets them | TESTED | setschedparam.c:126; SCHED_FIFO when privileged (new) attr_setguardsize.c:392-399 |
| P3 | getschedparam reports the last value set, not temporary ceiling boosts | NOT TESTABLE | needs SCHED_FIFO. **Deviation found by reading**, see below |
| P4 | A failed setschedparam changes nothing | TESTED (new) | attr_setguardsize.c:393-402 |
| P5 | setschedparam ENOTSUP/EINVAL, and EPERM (may fail) | TESTED | setschedparam.c:127; EPERM (new) attr_setguardsize.c:393 |
| P6 | setschedprio sets the priority; EINVAL (may fail) | TESTED | setschedparam.c:128-129 |
| P7 | A failed setschedprio changes nothing | TESTED (new) | attr_setguardsize.c:404-407 |
| P8 | Where setschedprio places the thread in its priority list | NOT TESTABLE | needs real-time threads |
| P9 | getcpuclockid returns the thread's CPU-time clock | TESTED | getcpuclockid.c:101-112 |
| P10 | getcpuclockid: no errors are defined | Not applicable | Issue 8 dropped ESRCH; picolibc still returns it once the thread has exited, which is an allowed extra error |

### Counts

130 requirements (rows):
- TESTED: 108. 66 were tested before and 42 are new. Several rows that were already tested got stronger checks too: C6, C15, A9, A17, A21, A31, K2, K4, K6, X4, X12, P2 and P5.
- Checks under `__PICOLIBC__` only:
  - H1 as macros, and H2 (glibc lacks PTHREAD_NULL)
  - X11 for accept4, pthread_cond_clockwait, posix_close and ppoll (glibc needs _GNU_SOURCE or lacks them)
  - X13 for if_nameindex, which is a cancellation point in glibc
  - X12 and X15, the aio_suspend checks
- UNTESTED: 6 (C12, C16, D4, A25, X14, F4), plus cnd_wait and thrd_join within X11.
- NOT TESTABLE: 12 (C14, D3, A4, A11, A12, A23, K8, X17, X20, F5, P3, P8).
- Not applicable: 4 (A14, A20, A29, P10), plus the ENOTSUP part of A18.

## Deviations found and fixed

Each one was verified by running. I reverted only the library change (`git apply -R` of the fix's diff), rebuilt, and ran the new test: it fails with the fix reverted and passes with it, on aarch64-linux-gnu.

1. **PTHREAD_NULL was missing** (libc/include/pthread.h).
   - Spec: "The <pthread.h> header shall define the following compile-time constant expression, valid as an initializer for pthread_t, representing a value that shall not compare equal to the thread ID of any existing thread: PTHREAD_NULL".
   - Fix: added as `((pthread_t)0)`. Thread IDs are descriptor addresses, so 0 is never one.
   - Before the fix, test-pthread_h_threads fails to compile (`#error`).
2. **Huge stack or guard sizes wrapped around** (libos/linux/pthread_create.c).
   - Rounding `SIZE_MAX` up to a page gave 0, and the sums could wrap too.
   - Spec: "If guardsize is greater than zero, a guard area of at least size guardsize bytes shall be provided", and "The stacksize attribute shall define the minimum stack size".
   - Fix: EAGAIN when the sizes, with room for rounding, overflow.
   - Before: `SIZE_MAX stacksize: child status 0xb00` in 3 of 3 runs.
3. **Cancellation stayed enabled while a thread exited** (libos/linux/pthread_exit.c).
   - Spec (XSH 2.9.5.3): "When a cancellation request is acted upon, or when a thread calls pthread_exit(), the thread first disables cancellation by setting its cancelability state to PTHREAD_CANCEL_DISABLE and its cancelability type to PTHREAD_CANCEL_DEFERRED. The cancelability state shall remain set to PTHREAD_CANCEL_DISABLE until the thread has terminated."
   - Effect: a pending request acted inside a cleanup handler or destructor, cutting it short and replacing the exit value. Neither path reset the type.
   - Fix: pthread_exit sets both first.
   - Before: `pthread_exit value kept: result == (void *)22` in 3 of 3 runs.
4. **A thread with cancellation disabled got EINTR** (libos/linux/pthread_cancel.c).
   - pthread_cancel always sent the cancellation signal. Calls Linux never restarts (nanosleep, poll, select, pause, sigsuspend, semop and others) then failed with EINTR.
   - Spec (XSH 2.9.5.1): "When cancelability is PTHREAD_CANCEL_DISABLE ..., cancellation requests against the target thread are held pending".
   - Fix: signal only threads with cancellation enabled.
   - The cancel state is now written with sequentially consistent atomics in pthread_setcancelstate, `_cancel_disable`/`_cancel_restore` and `_pthread_cancel_act`, and pthread_cancel reads it the same way. The pending flag is stored first, so a thread that enables cancellation at that moment sees the request at its next cancellation point. glibc made the same change in 2.34.
   - Before: `nanosleep returned -1 errno 4` in 3 of 3 runs.
5. **A cancelled pthread_once init routine hung every later caller** (libos/linux/pthread_once.c).
   - Spec: "if init_routine is a cancellation point and is canceled, the effect on once_control shall be as if pthread_once() was never called."
   - Fix: a cleanup handler resets once_control and wakes the waiters.
   - Before: the waiting caller never returned (killed by alarm after 60 s) in 2 of 2 runs.
6. **if_nametoindex, if_indextoname and if_nameindex were cancellation points** (libos/linux/if_*.c).
   - Spec (XSH 2.9.5.2): "An implementation shall not introduce cancellation points into any other functions".
   - They called close() or closedir(), and when cancelled there they also leaked the descriptor.
   - Fix: close with the raw system call; if_nameindex holds cancellation off with `_cancel_disable`.
   - Before: `if_nametoindex is a cancellation point` in 3 of 3 runs.
7. **A cancelled aio_suspend left the aio lock held** (libc/posix/aio/aio_suspend.c).
   - aio_suspend waits on a condition variable under `__aio_lock`. Cancellation reacquires the mutex and the cancelled thread never released it.
   - Every later aio call then blocked, the worker threads included, with every signal blocked.
   - Fix: a cleanup handler unlocks it.
   - lio_listio's LIO_WAIT wait is only a "may be" cancellation point, and cancelling it isn't safe because the list it frees is still in use, so it now runs with cancellation disabled.
   - Before: `asynchronous I/O hung after a cancelled aio_suspend` in 2 of 2 runs.

## Deviations found by reading (not changed)

- **Cancellation can act after a cancellation point's system call has finished** (X14; local-cancel.h and `cancel_handler` in pthread_cancel.c).
  - The handler acts whenever `cancel_point` is non-zero. That includes the moment after read() has returned data and before `_cancel_leave`, so the data is lost.
  - Spec (XSH 2.9.5.2): "The side-effects of acting upon a cancellation request while suspended during a call of a function are the same as the side-effects that may be seen in a single-threaded program when a call to a function is interrupted by a signal and the given function returns [EINTR]."
  - The window is a few instructions wide, so it can't be tested reliably.
  - glibc 2.34 fixed the same race by checking whether the interrupted PC lies inside the system call sequence. That needs a syscall stub for each architecture. Needs a design decision.
- **pthread_getschedparam reflects priority-protection boosts** (P3; `_pthread_tpp_raise.c`, `pthread_getschedparam.c`).
  - PRIO_PROTECT mutexes raise the thread with sched_setscheduler, and pthread_getschedparam asks the kernel.
  - Spec: "It shall not reflect any temporary adjustments to its priority as a result of any priority inheritance or ceiling functions."
  - Also, a pthread_setschedparam made while holding such a mutex is undone at unlock, because `_pthread_tpp_lower` restores `tpp_base`.
  - It can't be tested here (SCHED_OTHER only). A likely fix is to keep the thread's own policy and priority in its descriptor, and report and update those.

## Other observations (not changed)

- pclose is in neither cancellation-point list, but it calls fclose and waitpid, so it is one. The spec itself calls cancellation during pclose undefined, so I left it.
- pthread_attr_setschedparam stores any priority, and pthread_create then fails with EINVAL. glibc rejects out-of-range priorities in the attribute call instead. Both are allowed.
- pthread_once's cleanup handler also resets once_control if init_routine calls pthread_exit. glibc does the same.
- aio_read and aio_write use pread and pwrite, so on a pipe they complete at once with ESPIPE. glibc falls back to read and write. This is for the aio unit.
- pthread_getcpuclockid returns ESRCH once the thread has exited, where glibc returns a clock ID.

## Left undone

- X14 and P3, which need design decisions.
- cnd_wait and thrd_join as cancellation points, F4, A25, and C12/C16/D4, which were only read.
- A broader sweep of library functions that call cancellation points internally; only a sample was checked.
- Everything that needs SCHED_FIFO or SCHED_RR (A23, P3, P8).

## Timing and robustness of the new tests

- test-pthread_testcancel sets alarm(60), so a call that wrongly blocks ends the test instead of hanging it. The aio check runs in a child, which the parent kills after 10 s.
- The disabled, deferred, cancelled-joiner and cancelled-once checks use a semaphore handshake, then sleep 20–50 ms. Under load this only weakens the check; a correct implementation can't fail it.
- The guard check writes 32 KiB into a 64 KiB guard below a 256 KiB stack, in a child, and expects SIGSEGV or SIGBUS.
- The EAGAIN check asks for a SIZE_MAX/4 stack on 64-bit targets. On 32-bit targets a child lowers the RLIMIT_AS soft limit to 256 MiB and asks for 1 GiB. It is skipped, with a message, on 32-bit targets under qemu.
- The CPU-time check spins until the creating thread has used 200 ms of CPU, then requires the new thread's clock to read under 100 ms.

## Test runs (verified by running)

All runs were in the Debian sid chroot on the aarch64 host, at the final commit.

| Target | Command | Tests | Ok | Expected Fail | Fail | Skipped |
|---|---|---|---|---|---|---|
| aarch64-linux-gnu (`-Dwerror=true`) | `meson test -t 4 --num-processes 3`, full suite, without `--no-rebuild` | 1255 | 1208 | 6 | 0 | 41 |
| arm-linux-gnueabihf (`-Dwerror=true`) | same | 1254 | 1205 | 6 | 0 | 43 |

- All 10 new tests pass on both targets, for picolibc and the -native glibc builds.
- CMake thumbv7m (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 495 tests, 0 failed (232 passed, 263 skipped). The 5 new tests are registered and skip on bare metal.
- Bare-metal arm under meson was not run on this host, as the brief specifies.
- `clang-format --dry-run -Werror` is clean on every touched C file and header.

## Commits (audit/pthread-threads, on top of 4af0c34b98)

- ef0142a31b pthread.h: Define PTHREAD_NULL
- 4510b4b95c libos/linux: Fail pthread_create for sizes that overflow
- af7ce91dc4 libos/linux: Disable cancellation while a thread exits
- 7a54a7928b libos/linux: Don't interrupt threads with cancellation disabled
- c6bb146591 libos/linux: Reset pthread_once when its init routine is cancelled
- 8d3e2075f8 libos/linux: Keep cancellation points out of the if_ functions
- 517804b49a aio: Don't leave the lock held when aio_suspend is cancelled
- 608a7aa50a test-posix: Check the thread part of pthread.h
- a2236db5ad test-posix: Check guard areas, stack sizes and scheduling attributes
- 7ee8af47ab test-posix: Check joining, thread exit and the state of new threads
- 96b683e64c test-posix: Check deleting keys and running destructors
- b9b3a0d2e4 test-posix: Check cancellation points, exit cancelability, pthread_once

The header test is named `test-pthread_h_threads`, next to pthread-sync's `test-pthread_h_sync`. Nothing pushed, and no AI attribution in any commit.
