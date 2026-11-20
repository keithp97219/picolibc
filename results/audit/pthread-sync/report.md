# <pthread.h> synchronization audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-pthread-sync`, branch `audit/pthread-sync`,
based on 2d4e737cfc (`claude/posix-spec-audit`). Host: aarch64 Linux (Graviton2),
builds and tests in the Debian sid chroot.

Scope: mutex, mutexattr, cond, condattr, rwlock, rwlockattr, spin, barrier and
barrierattr interfaces (58 functions), plus the matching parts of the header
(constants, initializers, types and prototypes). Threads, attributes, once,
keys, cancellation and scheduling belong to the pthread-threads unit.

Implementation: `libos/linux/pthread_{mutex,cond,rwlock,spin,barrier}*.c`,
`_pthread_mutex_lock_timed.c`, `_pthread_mutex_unlock_ceiling.c`,
`_pthread_cond_wait_timed.c`, `_pthread_rwlock_lock_timed.c`, `_pthread_robust.c`,
`_pthread_tpp_*.c`, `_futex_*.c`, `local-pthread.h`; headers
`libc/include/pthread.h`, `libc/include/sys/_pthreadtypes.h`.
Tests: `test/test-posix/test-pthread/` (paths below are relative to it).

Spec source: pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/pthread.h and
functions/pthread_mutex_lock, pthread_mutex_timedlock, pthread_mutex_destroy,
pthread_mutex_consistent, pthread_mutex_getprioceiling, pthread_mutexattr_destroy,
pthread_mutexattr_{getprioceiling,getprotocol,getpshared,getrobust,gettype},
pthread_cond_timedwait, pthread_cond_broadcast, pthread_cond_destroy,
pthread_condattr_{destroy,getclock,getpshared}, pthread_rwlock_{destroy,rdlock,
timedrdlock,timedwrlock,trywrlock,unlock}, pthread_rwlockattr_{destroy,getpshared},
pthread_spin_{destroy,lock,unlock}, pthread_barrier_{destroy,wait},
pthread_barrierattr_{destroy,getpshared}.

Status key: TESTED (test file:line), UNTESTED, NOT TESTABLE (reason). "(new)"
marks rows this audit added tests for; the others were already tested.
Every TESTED row was run and passes for aarch64-linux-gnu and
arm-linux-gnueabihf picolibc (`test-X`) and against glibc 2.43 (`test-X-native`),
unless it says picolibc-only.

## Requirement table

### Header (basedefs/pthread.h, synchronization part)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | PTHREAD_BARRIER_SERIAL_THREAD, PTHREAD_MUTEX_{DEFAULT,ERRORCHECK,NORMAL,RECURSIVE,ROBUST,STALLED}, PTHREAD_PRIO_{INHERIT,NONE,PROTECT}, PTHREAD_PROCESS_{SHARED,PRIVATE} defined | TESTED (new) | test-pthread_h_sync.c:45-56 (as macros, picolibc only: glibc makes most of them enumerators), 59-69 (both) |
| H2 | Constants of each kind distinct | TESTED (new) | test-pthread_h_sync.c:59-69 |
| H3 | PTHREAD_BARRIER_SERIAL_THREAD distinct from the 0 other threads get | TESTED | test-pthread_h_sync.c:71; test-pthread_barrier_wait.c:195 |
| H4 | PTHREAD_{COND,MUTEX,RWLOCK}_INITIALIZER are constant expressions valid as initializers | TESTED (new) | test-pthread_h_sync.c:74-76 (static objects) |
| H5 | The nine sync types defined | TESTED (new) | test-pthread_h_sync.c:74-84 |
| H6 | Prototypes of the 58 functions as specified | TESTED (new) | test-pthread_h_sync.c:94-166 (function-pointer initialisation; the four clock_id variants picolibc only, since glibc 2.43 declares them only for _GNU_SOURCE. restrict on parameters doesn't affect compatibility; checked by reading, matches) |

### Mutex attributes

| # | Requirement | Status | Reference |
|---|---|---|---|
| MA1 | init: type DEFAULT, pshared PRIVATE, protocol NONE, robust STALLED | TESTED | test-pthread_mutex_timedlock.c:147-149, test-pthread_mutexattr_setprotocol.c:154, test-pthread_mutex_consistent.c:151 |
| MA2 | settype/gettype round trip; invalid type EINVAL (shall) | TESTED | test-pthread_mutex_timedlock.c:153, 131 |
| MA3 | setpshared/getpshared; invalid EINVAL (may) | TESTED | test-pthread_mutex_timedlock.c:150-154 |
| MA4 | setrobust/getrobust; invalid EINVAL (shall) | TESTED | test-pthread_mutex_consistent.c:151-156 |
| MA5 | setprotocol/getprotocol; invalid EINVAL (may); ENOTSUP for unsupported protocols | TESTED | test-pthread_mutexattr_setprotocol.c:154-158; all three protocols are supported, so ENOTSUP doesn't arise |
| MA6 | prioceiling within the SCHED_FIFO range; out of range EINVAL (may) | TESTED (new) | test-pthread_mutex_lock.c:294-304; test-pthread_mutexattr_setprotocol.c:160-162 |
| MA7 | Changing or destroying attributes doesn't affect mutexes already initialized | TESTED (new) | test-pthread_mutex_lock.c:266-274 |
| MA8 | init/destroy return 0; init ENOMEM | TESTED / NOT TESTABLE | every test; nothing is allocated, so ENOMEM can't happen |

### pthread_mutex_init / pthread_mutex_destroy

| # | Requirement | Status | Reference |
|---|---|---|---|
| MI1 | NULL attr gives the defaults; PTHREAD_MUTEX_INITIALIZER equivalent | TESTED | test-pthread_mutex_timedlock.c:157-166, test-pthread_mutex_lock.c:229-231 |
| MI2 | Initialized mutex is unlocked | TESTED | test-pthread_mutex_timedlock.c:166 |
| MI3 | Destroyed mutex can be initialized again | TESTED (new) | test-pthread_mutex_lock.c:228-231 |
| MI4 | init EAGAIN, ENOMEM, EPERM (shall) | NOT TESTABLE | init allocates nothing and needs no privilege |
| MI5 | init EINVAL for robust without pshared (may) | Not applicable | picolibc supports private robust mutexes |

### pthread_mutex_lock / trylock / unlock

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | Lock blocks while another thread holds it; returns owning it | TESTED | test-pthread_mutexattr_setprotocol.c:173, test-pthread_mutex_lock.c:246-262 |
| L2 | NORMAL relock deadlocks (or EDEADLK, may fail) | TESTED (new) | test-pthread_mutex_lock.c:207-215 (timed relock: ETIMEDOUT at the deadline, or EDEADLK) |
| L3 | NORMAL robust relock deadlocks | TESTED (new) | test-pthread_mutex_lock.c:276-288 (skipped if robust mutexes are unavailable) |
| L4 | ERRORCHECK relock EDEADLK (shall) | TESTED | test-pthread_mutex_timedlock.c:186; timed variants (new) test-pthread_mutex_lock.c:222-225 |
| L5 | RECURSIVE count: each lock +1, available at zero | TESTED | test-pthread_mutex_timedlock.c:168-180 |
| L6 | EAGAIN when the recursion count would overflow (shall) | NOT TESTABLE | needs UINT_MAX relocks; code returns EAGAIN at UINT_MAX (read, local-pthread.h `_pthread_mutex_relock`) |
| L7 | unlock EPERM: ERRORCHECK or RECURSIVE not owner (shall) | TESTED | test-pthread_mutex_timedlock.c:174, 179, 184, 188 |
| L8 | unlock EPERM: robust not owner (shall) | TESTED | test-pthread_mutex_consistent.c:233-234 |
| L9 | trylock EBUSY when held by another thread | TESTED | test-pthread_mutex_timedlock.c:163, 173 |
| L10 | trylock EBUSY when held by the caller (normal, errorcheck) | TESTED (new) | test-pthread_mutex_lock.c:202-205, 221 |
| L11 | trylock on own RECURSIVE mutex increments the count | TESTED | test-pthread_mutex_timedlock.c:172 |
| L12 | A signal doesn't end the wait; never EINTR | TESTED (new) | test-pthread_mutex_lock.c:246-262 (see "timing" below) |
| L13 | Robust: EOWNERDEAD when the owning process died (shall) | TESTED | test-pthread_mutex_consistent.c:276-290 |
| L14 | Robust: EOWNERDEAD when the owning thread died (may); with a waiter | TESTED | test-pthread_mutex_consistent.c:217-227 |
| L15 | Robust: unlocked without consistent, then ENOTRECOVERABLE (lock, trylock) | TESTED | test-pthread_mutex_consistent.c:244-247 |
| L16 | Robust: EAGAIN when robust-mutex resources exhausted (shall) | NOT TESTABLE | no such limit (the robust list is intrusive) |
| L17 | PRIO_PROTECT: EINVAL when caller's priority exceeds the ceiling (shall) | NOT TESTABLE | needs SCHED_FIFO; the chroot user has RLIMIT_RTPRIO 0 and no CAP_SYS_NICE (tests accept 0 or EINVAL) |
| L18 | PRIO_INHERIT / PRIO_PROTECT priority effects | NOT TESTABLE | same privilege; counters only, test-pthread_mutexattr_setprotocol.c:173, 223 |
| L19 | Scheduling policy picks which waiter acquires | NOT TESTABLE | kernel futex ordering, needs real-time threads |

### pthread_mutex_timedlock / pthread_mutex_clocklock

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | ETIMEDOUT while another thread holds it | TESTED | test-pthread_mutex_timedlock.c:192-197 |
| T2 | Expires when the clock reaches abstime, not before | TESTED (new) | test-pthread_mutex_lock.c:122-133 (both functions, both clocks) |
| T3 | abstime already passed, or before the epoch, times out | TESTED (new) | test-pthread_mutex_lock.c:110-120 |
| T4 | No timeout if the mutex is free; abstime validity need not be checked | TESTED | test-pthread_mutex_timedlock.c:205; passed deadline (new) test-pthread_mutex_lock.c:237-244 |
| T5 | EINVAL for tv_nsec < 0 or >= 1e9 when it would block | TESTED | test-pthread_mutex_timedlock.c:206; negative (new) test-pthread_mutex_lock.c:140-143 |
| T6 | clocklock supports CLOCK_REALTIME and CLOCK_MONOTONIC | TESTED | test-pthread_mutex_timedlock.c:196; REALTIME (new) test-pthread_mutex_lock.c:130-133 |
| T7 | clocklock EINVAL for an unsupported clock when it would block | TESTED (new) | test-pthread_mutex_lock.c:135-138 (CLOCK_PROCESS_CPUTIME_ID) |
| T8 | EDEADLK for ERRORCHECK relock (shall) | TESTED (new) | test-pthread_mutex_lock.c:222-225 |
| T9 | Robust EOWNERDEAD / ENOTRECOVERABLE through the timed functions | UNTESTED | same code path as pthread_mutex_lock (`_pthread_mutex_lock_timed`), read only |
| T10 | PI: priority readjusted when a timed wait expires | NOT TESTABLE | privilege, kernel behaviour |

### pthread_mutex_consistent

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Marks the state consistent; the mutex is usable after unlock | TESTED | test-pthread_mutex_consistent.c:226-230 |
| C2 | EINVAL: not robust, or not inconsistent (shall) | TESTED | test-pthread_mutex_consistent.c:228, 293 |
| C3 | Owner dies after EOWNERDEAD without consistent/unlock: next locker EOWNERDEAD | TESTED | test-pthread_mutex_consistent.c:243-244 |

### pthread_mutex_getprioceiling / setprioceiling

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | get returns the current ceiling | TESTED | test-pthread_mutexattr_setprotocol.c:206 |
| P2 | set changes it and returns the old value | TESTED | test-pthread_mutexattr_setprotocol.c:209-211 |
| P3 | EINVAL for a PRIO_NONE mutex (shall), get and set | TESTED | get test-pthread_mutexattr_setprotocol.c:167; set (new) test-pthread_mutex_lock.c:321-324 |
| P4 | set: EDEADLK for an ERRORCHECK mutex the caller holds (shall); ceiling unchanged on failure | TESTED (new) | test-pthread_mutex_lock.c:309-317 (runs when the PROTECT lock succeeds, which it does for SCHED_OTHER) |
| P5 | set: EINVAL for an out-of-range ceiling (may) | TESTED | test-pthread_mutexattr_setprotocol.c:208 |
| P6 | set: EAGAIN, EOWNERDEAD, ENOTRECOVERABLE as for lock | UNTESTED | uses `_pthread_mutex_lock_timed`; read only |
| P7 | EPERM for lack of privilege | Not applicable | no privilege needed |

### Condition variable attributes

| # | Requirement | Status | Reference |
|---|---|---|---|
| CA1 | init: clock CLOCK_REALTIME, pshared PRIVATE | TESTED | test-pthread_cond_timedwait.c:172-175 |
| CA2 | setclock/getclock; CPU-time clock EINVAL (may) and the clock kept | TESTED | test-pthread_cond_timedwait.c:177-178; CPU-time (new) test-pthread_cond_wait.c:266-272 |
| CA3 | setpshared/getpshared; invalid EINVAL (may) | TESTED | test-pthread_cond_timedwait.c:176, test-pthread_condattr_setpshared.c:191-192 |
| CA4 | Destroying the attributes doesn't affect condition variables | TESTED (new) | test-pthread_cond_wait.c:277-282 |

### pthread_cond_init / destroy / signal / broadcast

| # | Requirement | Status | Reference |
|---|---|---|---|
| CI1 | NULL attr gives defaults; PTHREAD_COND_INITIALIZER equivalent | TESTED | test-pthread_cond_timedwait.c:194-197; initializer (new) test-pthread_cond_wait.c:252-256 |
| CI2 | Destroy returns 0; can be initialized again | TESTED | test-pthread_cond_timedwait.c:192-194 |
| CI3 | init EAGAIN / ENOMEM | NOT TESTABLE | nothing allocated |
| CS1 | broadcast unblocks every waiter | TESTED | test-pthread_cond_timedwait.c:158-168 |
| CS2 | signal unblocks at least one waiter | TESTED | test-pthread_cond_timedwait.c:147-156 (producer and consumers) |
| CS3 | No effect, and 0, without waiters | TESTED | test-pthread_cond_timedwait.c:215-217 ("no effect" itself can't be observed: spurious wakeups are allowed) |
| CS4 | Unblocked threads contend for the mutex as if by pthread_mutex_lock | TESTED | covered by CS1/CS2, each thread returns owning the mutex |
| CS5 | Scheduling policy orders unblocking | NOT TESTABLE | privilege |

### pthread_cond_wait / timedwait / clockwait

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | Atomically releases the mutex; a signal after acquiring the mutex isn't lost | TESTED | test-pthread_cond_timedwait.c:147-156, test-pthread_cond_wait.c:287-307 |
| W2 | Returns owning the mutex, also after ETIMEDOUT | TESTED | test-pthread_cond_timedwait.c:210-212; errorcheck (new) test-pthread_cond_wait.c:241-250 |
| W3 | EPERM: ERRORCHECK mutex not owned (free, or held by another thread) | TESTED (new) | test-pthread_cond_wait.c:218-226, 113-126 |
| W4 | EPERM: robust mutex not owned | TESTED (new, picolibc only) | test-pthread_cond_wait.c:329-330, 334-344 |
| W5 | Errors other than ETIMEDOUT/EOWNERDEAD/ENOTRECOVERABLE are detected before the mutex or cond is modified | TESTED (new) | EINVAL: test-pthread_cond_wait.c:228-239; EPERM on a robust recursive mutex another thread is relocking: 334-344 (picolibc only). **Was a deviation; fixed.** |
| W6 | Robust mutex whose owner died while waiting: acquired with EOWNERDEAD | TESTED (new, picolibc only) | test-pthread_cond_wait.c:310-327. glibc would wait forever where the kernel has no robust list (QEMU user mode), hence the guard |
| W7 | ENOTRECOVERABLE | UNTESTED | relock goes through `_pthread_mutex_lock_timed` (read) |
| W8 | EAGAIN robust resources | NOT TESTABLE | as L16 |
| W9 | ETIMEDOUT at the deadline; abstime already passed or before the epoch | TESTED | test-pthread_cond_timedwait.c:185, 197; passed/epoch (new) test-pthread_cond_wait.c:241-250 |
| W10 | Timeout expires when the clock reaches abstime, not before | TESTED (new) | test-pthread_cond_wait.c:252-264, 278-282 |
| W11 | timedwait uses the condition's clock attribute, default CLOCK_REALTIME | TESTED | test-pthread_cond_timedwait.c:181-197; test-pthread_cond_wait.c:252-256, 278-282 |
| W12 | clockwait supports CLOCK_REALTIME and CLOCK_MONOTONIC | TESTED | test-pthread_cond_timedwait.c:187, 199; test-pthread_cond_wait.c:258-264 |
| W13 | EINVAL: tv_nsec out of range, or unsupported clock_id (shall) | TESTED | test-pthread_cond_timedwait.c:190; negative, clockwait, CPU-time clock (new) test-pthread_cond_wait.c:228-239 |
| W14 | A signal: resume waiting or return 0, never EINTR | TESTED (new) | test-pthread_cond_wait.c:287-307 |
| W15 | Cancellation point; mutex reacquired before cleanup handlers | TESTED | test-pthread_cancel.c:87-94, 274 (pthread-threads unit's area) |
| W16 | A thread cancelled in a wait doesn't consume a concurrent signal | NOT TESTABLE | race with no observable handshake |
| W17 | Dynamic binding of mutex and cond | NOT TESTABLE | defines undefined behaviour only |

### Reader-writer lock attributes

| # | Requirement | Status | Reference |
|---|---|---|---|
| RA1 | init: pshared PRIVATE; set/get; invalid EINVAL (may) | TESTED | test-pthread_rwlock_timedwrlock.c:164-172 |

### pthread_rwlock_init / destroy / rdlock / tryrdlock / wrlock / trywrlock / unlock

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | Initialized (including PTHREAD_RWLOCK_INITIALIZER) to unlocked | TESTED | test-pthread_rwlock_timedwrlock.c:65; (new) test-pthread_rwlock_rdlock.c:246-247 |
| R2 | Destroyed lock can be initialized again | TESTED (new) | test-pthread_rwlock_rdlock.c:286-290 |
| R3 | init EAGAIN, ENOMEM, EPERM | NOT TESTABLE | nothing allocated |
| R4 | Readers share; writers exclude readers and writers | TESTED | test-pthread_rwlock_timedwrlock.c:177-185 |
| R5 | rdlock doesn't acquire while a writer holds it | TESTED | test-pthread_rwlock_timedwrlock.c:196 (try and timed) |
| R6 | A thread may hold several read locks with matching unlocks | TESTED | test-pthread_rwlock_timedwrlock.c:187-193 |
| R7 | Releasing one of several read locks leaves it read locked; the last unlocks it | TESTED (new) | test-pthread_rwlock_rdlock.c:249-256 |
| R8 | Releasing the write lock unlocks it | TESTED (new) | test-pthread_rwlock_rdlock.c:258-274 |
| R9 | tryrdlock EBUSY while a writer holds it (shall) | TESTED | test-pthread_rwlock_timedwrlock.c:196 |
| R10 | trywrlock EBUSY while read or write locked (shall) | TESTED | test-pthread_rwlock_timedwrlock.c:191, 196 |
| R11 | rdlock by the write holder: EDEADLK (may) or deadlock | TESTED (new) | test-pthread_rwlock_rdlock.c:262-269 |
| R12 | wrlock by a holder: EDEADLK or deadlock | TESTED (new) for a write holder | test-pthread_rwlock_rdlock.c:262-267; a read holder deadlocks in picolibc (allowed), not asserted |
| R13 | EAGAIN when the reader count would overflow (may) | NOT TESTABLE | needs UINT_MAX - 1 read locks. picolibc's tryrdlock reports it as EBUSY (see observations) |
| R14 | [TPS] SCHED_FIFO/RR readers don't acquire past blocked writers of equal or higher priority; unlock favours writers of equal priority | NOT TESTABLE | needs real-time threads. **Deviation by reading**, see below |
| R15 | Without TPS policies, writer preference is implementation-defined | TESTED | readers are preferred (read, local-pthread.h); test-pthread_rwlock_timedwrlock.c:187-191 |
| R16 | A signal: resume waiting (rdlock and wrlock) | TESTED (new) | test-pthread_rwlock_rdlock.c:200-237, 279-284 |

### pthread_rwlock_timedrdlock / clockrdlock / timedwrlock / clockwrlock

| # | Requirement | Status | Reference |
|---|---|---|---|
| RT1 | ETIMEDOUT while busy | TESTED | test-pthread_rwlock_timedwrlock.c:108-140 |
| RT2 | Expires when the clock reaches abstime, not before | TESTED (new) | test-pthread_rwlock_rdlock.c:147-161 |
| RT3 | abstime already passed or before the epoch times out | TESTED (new) | test-pthread_rwlock_rdlock.c:130-145 |
| RT4 | No timeout if acquirable; abstime validity need not be checked | TESTED (new) | test-pthread_rwlock_rdlock.c:167-186 |
| RT5 | CLOCK_REALTIME and CLOCK_MONOTONIC supported | TESTED | MONOTONIC test-pthread_rwlock_timedwrlock.c:118, 134; REALTIME (new) test-pthread_rwlock_rdlock.c:159-161 |
| RT6 | EINVAL for bad tv_nsec or unsupported clock (may) | UNTESTED | optional; picolibc returns EINVAL when it would block (read: `_futex_wait`) and for any clock other than the two up front |
| RT7 | EDEADLK (may) | TESTED (new) | as R11/R12 |

### Spin locks

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | init unlocked; lock gives mutual exclusion; unlock releases | TESTED | test-pthread_spin_lock.c:91-108 |
| S2 | trylock EBUSY while held (shall) | TESTED | test-pthread_spin_lock.c:103 |
| S3 | [TSH] PTHREAD_PROCESS_SHARED works across processes | TESTED (new) | test-pthread_condattr_setpshared.c:92-116, 145-156, 225 |
| S4 | lock EDEADLK (may) | Not applicable | picolibc spins (allowed: undefined behaviour) |
| S5 | init EAGAIN / ENOMEM | NOT TESTABLE | nothing allocated |

### Barrier attributes and barriers

| # | Requirement | Status | Reference |
|---|---|---|---|
| BA1 | init: pshared PRIVATE; set/get; invalid EINVAL (may) | TESTED | test-pthread_barrier_wait.c:172-178 |
| BA2 | Changing or destroying the attributes doesn't affect barriers | TESTED | test-pthread_condattr_setpshared.c:202-205 (destroyed before the barrier is used) |
| B1 | init EINVAL for count 0 (shall) | TESTED | test-pthread_barrier_wait.c:180 |
| B2 | init EAGAIN / ENOMEM | NOT TESTABLE | nothing allocated |
| B3 | No thread returns before count threads have arrived | TESTED | test-pthread_barrier_wait.c:75-90 (count = threads); with more threads than the count (new) 93-128, 198-205. **Was a deviation; fixed** |
| B4 | One thread gets PTHREAD_BARRIER_SERIAL_THREAD, the others 0 | TESTED | test-pthread_barrier_wait.c:182, 186-195 |
| B5 | Reset to the initial state after each round | TESTED | test-pthread_barrier_wait.c:186-195; (new) 198-205 |
| B6 | A signal: resume waiting until the barrier completes; never EINTR | TESTED (new) | test-pthread_barrier_wait.c:208-226 |
| B7 | [TSH] process-shared barrier across processes | TESTED (new) | test-pthread_condattr_setpshared.c:85-91, 117-118 and each later step |
| B8 | Blocked threads don't prevent others' progress | NOT TESTABLE | scheduler property |

### Process-shared objects (TSH, used by two processes)

| # | Requirement | Status | Reference |
|---|---|---|---|
| PS1 | Mutex | TESTED (new) | test-pthread_condattr_setpshared.c:92-99, 224 |
| PS2 | Error checking mutex: owner seen across processes (EBUSY, EPERM, then free) | TESTED (new) | test-pthread_condattr_setpshared.c:135-164 |
| PS3 | Condition variable (each process waits for the other's broadcast) | TESTED (new) | test-pthread_condattr_setpshared.c:120-133 |
| PS4 | Reader-writer lock | TESTED (new) | test-pthread_condattr_setpshared.c:104-114, 226 |

### Counts

130 requirements (rows):
- TESTED: 105, of which 64 were tested before and 41 are new (rows marked "(new)").
  Another 14 rows that were already tested got extra checks (the "(new)" parts of
  their references). Checks only under `__PICOLIBC__`: H1 as macros, H6 for the
  clock variants, W4, the EPERM half of W5, and W6
- UNTESTED: 4 (T9, P6, W7, RT6), plus R12's read-holder case, which isn't asserted
- NOT TESTABLE: 18, with the reason in each row (MA8's ENOMEM half too)
- Not applicable: 3 (MI5, P7, S4), plus the ENOTSUP part of MA5

## Deviations found and fixed

Both were verified by running: each new test fails with only the library change
reverted, and passes with it.

1. **Barrier arrivals could join the wrong round** (libos/linux/pthread_barrier_wait.c).
   pthread_barrier_wait read `__seq`, then counted itself in with a separate
   atomic add, and the last thread reset `__waiting` to 0 before advancing `__seq`.
   With more threads than the count, a thread arriving as a round completed could
   (a) count itself into the next round yet leave with the previous one, or (b) have
   its increment overwritten by the reset, so a round never fills. POSIX: "The
   calling thread shall block until the required number of threads have called
   pthread_barrier_wait()" and "the barrier shall be reset to the state it had as a
   result of the most recent pthread_barrier_init()". Fix: count arrivals and read
   `__seq` under a small futex lock (new `__lock` member in `pthread_barrier_t`;
   there is no static initializer, so only pthread_barrier_init sets it).
   Failing before (8 threads, count 2, 100000 arrivals): 3 of 6 runs failed, two
   hanging (lost arrival) and one with `pthread_barrier_destroy crowd:
   pthread_barrier_destroy(&barrier) == 0` (a phantom arrival left behind). After:
   6 of 6 passed at 200000 arrivals, and every full-suite run since. glibc passes
   every time.
2. **Condition wait modified a mutex it then rejected with EPERM**
   (libos/linux/_pthread_cond_wait_timed.c). It zeroed `mutex->__count` (to release
   a recursive mutex completely), called pthread_mutex_unlock, and on EPERM wrote the
   old count back. For a recursive robust mutex owned by another thread, that thread's
   own relocks and unlocks were lost, so its later unlocks released the mutex early
   or failed. POSIX: "Except for [ETIMEDOUT], [ENOTRECOVERABLE], and [EOWNERDEAD], all
   these error checks shall act as if they were performed immediately at the
   beginning of processing for the function and shall cause an error return, in
   effect, prior to modifying the state of the mutex"; "[EPERM] ... the mutex is a
   robust mutex, and the current thread does not own the mutex". Fix: check
   ownership first for every mutex that tracks an owner (all but non-robust NORMAL).
   Failing before: `lock count of the owner kept: result == NULL` in 9 of 10 runs;
   after: 10 of 10 and 20 of 20 passed.

## Deviations found by reading (not changed)

- **Reader-writer locks ignore writer priority under SCHED_FIFO/SCHED_RR** (R14).
  picolibc defines `_POSIX_THREAD_PRIORITY_SCHEDULING`, and the rwlock always
  prefers readers (`_pthread_rwlock_try` takes a read lock whenever no writer holds
  it, and unlock wakes every waiter). XSH pthread_rwlock_rdlock [TPS]: "the calling
  thread shall not acquire the lock if a writer holds the lock or if the calling
  thread does not already hold a read lock and writers of higher or equal priority
  are blocked on the lock"; pthread_rwlock_unlock [TPS]: "For equal priority threads,
  write locks shall take precedence over read locks." Fixing it means tracking
  blocked writers and each reader's priority, so whether a reader already holds the
  lock becomes per-thread state. Can't be tested here: the chroot user has
  RLIMIT_RTPRIO 0 and no CAP_SYS_NICE. Needs a design decision.
- **pthread_mutex_setprioceiling on a recursive PRIO_PROTECT mutex the caller holds**
  (pthread_mutex_setprioceiling.c). The relock only bumps the count, the ceiling then
  changes, and the final pthread_mutex_unlock passes the *new* ceiling to
  `_pthread_tpp_lower`, while `_pthread_tpp_raise` counted the old one. The
  `tpp_count` entries get out of step (one stays raised, one wraps below zero), so a
  SCHED_FIFO thread can keep running at the old ceiling. Invisible under SCHED_OTHER,
  so it isn't testable here. A likely fix: record the ceiling a thread was raised to
  when it first takes the mutex, and lower by that.

## Other observations (not changed)

- pthread_rwlock_tryrdlock and trywrlock map EAGAIN (too many readers) to EBUSY.
  EAGAIN is a "may fail" error, so this conforms, but the EBUSY condition in the
  spec is "a writer holds the lock". glibc returns EAGAIN. Only reachable with
  UINT_MAX - 1 readers.
- pthread_barrier_destroy returns 0 once the last thread has arrived, while
  released threads may still be about to re-read `__seq`. If the serial thread
  destroys and frees the barrier right away (a common pattern, and allowed since no
  thread is blocked any more), a slow waiter reads freed memory, and could sleep on
  it if the memory is reused with the same value. glibc's destroy waits for the
  waiters to leave. Not reproduced.
- After a condition wait's relock fails with ENOTRECOVERABLE, `mutex->__count` is
  written on a mutex the caller doesn't own. Harmless, since the mutex can only be
  destroyed then.
- `PTHREAD_MUTEX_INITIALIZER` sets the priority ceiling to 0 while
  pthread_mutex_init(NULL) uses 1. It isn't observable (getprioceiling fails for
  PRIO_NONE mutexes).
- pthread_mutex_destroy, pthread_rwlock_destroy, pthread_spin_destroy and
  pthread_barrier_destroy return EBUSY on objects in use (undefined behaviour;
  allowed).
- pthread_mutex_init returns ENOTSUP for PRIO_INHERIT without kernel PI futexes, and
  for robust mutexes without a robust list. These aren't listed errors, but XSH 2.3
  allows extra errors.
- clocklock / clockwait / clockrdlock reject unsupported clocks even when they
  wouldn't block. This is conservative, and glibc does the same.

## Left undone

- T9, P6, W7: robust-mutex errors through the timed lock functions, setprioceiling
  and the condition-wait relock. They share code with the tested pthread_mutex_lock
  paths; only read, not run.
- RT6: the optional EINVAL for rwlock deadlines isn't asserted.
- Everything that needs SCHED_FIFO/SCHED_RR (L17-L19, T10, CS5, R14) could run on a
  host where the tests have CAP_SYS_NICE or a non-zero RLIMIT_RTPRIO.

## Timing and robustness of the new tests

- The signal checks (mutex, cond, rwlock, barrier) wait for the thread to say it's
  about to block, then sleep 20 ms before sending the signal and 20 ms after the
  handler runs, then check that the thread hasn't returned. A correct
  implementation can't fail them. Under heavy load the signal may arrive before the
  thread blocks, which only weakens the check.
- The barrier crowd check is statistical (it caught the bug in about half the runs
  before the fix). When it fails it may hang until meson's timeout rather than report.
- The cond EPERM check runs 1,000,000 relocks (about 0.15 s).
- Process-shared test: if one process fails, the parent kills the child so the test
  doesn't hang.

## Test runs (verified by running)

All in the Debian sid chroot on the aarch64 host, at the final commit:

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`),
  `meson test -t 4 --num-processes 3`: 1228 tests; **1181 Ok, 6 Expected Fail,
  0 Fail, 41 Skipped**. (A first run with `--no-rebuild` reported the three ctype-diff
  tests as failing, because their custom-target inputs weren't built. They pass when
  rerun without `--no-rebuild`.)
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`), full
  suite: 1227 tests; **1178 Ok, 6 Expected Fail, 0 Fail, 43 Skipped**.
- bare-metal arm (`scripts/do-arm-configure -Dwerror=true -Dmultilib=false`, in
  `builds/build-arm-single`), `meson test -t 20 --suite test-pthread --suite test-posix`:
  **28 Ok, 0 Fail, 249 Skipped** (the thread tests skip without OS support). Built
  with `-Dmultilib=false` because the full multilib build is about 210k targets,
  roughly 90 minutes at -j3 on the shared host. The only change bare metal
  compiles is the `pthread_barrier_t` layout in `sys/_pthreadtypes.h`.
- CMake thumbv7m (`scripts/do-cmake-thumbv7m-configure`, `ninja test`): 481 tests,
  **0 failed** (229 passed, 252 skipped). The five new tests are registered and skip
  on bare metal.
- Suite test-pthread on aarch64: 45 Ok (23 tests x picolibc and glibc, less
  test-pthread_mutex_consistent-native, which is skipped on non-x86 hosts).
- `clang-format --dry-run -Werror` is clean on every touched C file.

## Commits (audit/pthread-sync, on top of 2d4e737cfc)

- cb9ffc951b libos/linux: Count barrier arrivals under a lock
- c263fb8532 test-posix: Check overlapping rounds and signals in pthread_barrier_wait
- bb7474033d libos/linux: Check the mutex owner before a condition wait touches it
- f2757798fd test-posix: Check errors, deadlines and robust mutexes in pthread_cond_wait
- a3d7b93b3b test-posix: Check relocking, deadlines and signals in pthread_mutex_lock
- 14aa4ee653 test-posix: Check lock states, deadlines and signals in pthread_rwlock_rdlock
- 456066b29a test-posix: Check process-shared synchronization objects between processes
- 4f95e0543e test-posix: Check the synchronization part of pthread.h

The header test is named `test-pthread_h_sync` so it won't collide with a
`test-pthread_h` from the pthread-threads unit.

Nothing pushed. No AI attribution in any commit.
