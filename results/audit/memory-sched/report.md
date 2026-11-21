# <sys/mman.h>, <semaphore.h> and <sched.h> audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-memory-sched`, branch `audit/memory-sched`, based on d40e4ae0e4 (`claude/posix-spec-audit`). Host: aarch64 Linux (Graviton2, kernel 5.10), with builds and tests in the Debian sid chroot (gcc 16.2, glibc 2.43).

**Scope:**
- `<sys/mman.h>`: mmap, munmap, mprotect, msync, mlock, munlock, mlockall, munlockall, posix_madvise, shm_open, shm_unlink, and the header's constants and types.
  - posix_mem_offset, posix_typed_mem_get_info and posix_typed_mem_open are still in Issue 8, under the Typed Memory Objects option [TYM].
  - picolibc doesn't support that option (`_POSIX_TYPED_MEMORY_OBJECTS` is undefined), so those rows are Not applicable.
- `<semaphore.h>`: sem_init, sem_destroy, sem_open, sem_close, sem_unlink, sem_wait, sem_trywait, sem_timedwait, sem_clockwait, sem_post, sem_getvalue, SEM_FAILED and sem_t.
- `<sched.h>`: sched_get_priority_max/min, sched_getparam, sched_setparam, sched_getscheduler, sched_setscheduler, sched_rr_get_interval, sched_yield, struct sched_param and the SCHED_* policies.
- System V shared memory (shmat and the rest) belongs to the sysv-ipc unit and is untouched.

**Implementation:**
- `libos/linux/`: mmap.c, munmap.c, mprotect.c, msync.c, mlock.c, mlockall_munlockall.c, madvise.c, shm_open.c, shm_unlink.c, local-shm.h, sem_*.c, `_sem_*.c`, `_futex_wait.c`, sched_*.c and local-sched.h, plus the generated `machine/*/linux/linux-mman.h`.
- Headers: `libc/include/sys/mman.h`, `libc/include/semaphore.h`, `libc/include/sched.h` and `libc/include/sys/sched.h`.

**Tests:** in `test/test-posix/test-sys-mman/`, `test-semaphore/` and `test-sched/`. File names below drop the `test-` prefix, so `munmap.c` is `test-sys-mman/test-munmap.c`. `testcancel.c` and `pthread_cancel.c` are in `test-pthread/`.

**Spec source:** pubs.opengroup.org/onlinepubs/9799919799.
- Base definitions: basedefs/sys_mman.h, semaphore.h and sched.h.
- Memory functions: mmap, munmap, mprotect, msync, mlock (with munlock), mlockall (with munlockall), posix_madvise, shm_open, shm_unlink.
- Typed memory: posix_mem_offset, posix_typed_mem_get_info, posix_typed_mem_open.
- Semaphores: sem_init, sem_destroy, sem_open, sem_close, sem_unlink, sem_trywait (with sem_wait), sem_timedwait (with sem_clockwait), sem_post, sem_getvalue.
- Scheduling: sched_get_priority_max (with _min), sched_getparam, sched_setparam, sched_getscheduler, sched_setscheduler, sched_rr_get_interval, sched_yield.
- XSH 2.9.5.2 for the cancellation points.

**Status key:** TESTED (test file:line), UNTESTED, NOT TESTABLE (with the reason), Not applicable. "(new)" marks rows this audit added tests for. Every TESTED row was run and passes on aarch64-linux-gnu and arm-linux-gnueabihf, both for picolibc (`test-X`) and against glibc 2.43 (`test-X-native`), unless the row says picolibc only. "Read" means inferred from the code, not run.

**Host limits:**
- **Real-time scheduling:** the chroot user has none (RLIMIT_RTPRIO 0, no CAP_SYS_NICE). SCHED_FIFO/SCHED_RR setters therefore fail with EPERM, which the tests require, and every scheduling-order requirement is NOT TESTABLE here.
- **Memory locking:** RLIMIT_MEMLOCK is unlimited for this user, so mlock and mlockall succeed, and the EBUSY and ENOMEM checks that need a locked page ran. Where locking fails, the tests print a note and skip those checks.
- **Page size:** every new test takes it from `sysconf(_SC_PAGESIZE)` (4096 here).
- **Names:** shared memory and semaphore names include `getpid()`, and are unlinked on every failure path.

## Requirement table

### Header (basedefs/sys_mman.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| MH1 | PROT_EXEC, PROT_NONE, PROT_READ, PROT_WRITE are defined | TESTED (new) | sys_mman_h.c:50-56 (distinct bits; PROT_NONE grants nothing) |
| MH2 | MAP_ANONYMOUS, MAP_FIXED, MAP_PRIVATE, MAP_SHARED are defined | TESTED (new) | sys_mman_h.c:57-61 |
| MH3 | MAP_ANON is defined, with the value of MAP_ANONYMOUS | TESTED (new) | sys_mman_h.c:62. **Was only visible with `_GNU_SOURCE`; fixed** |
| MH4 | MS_ASYNC, MS_INVALIDATE, MS_SYNC are defined [XSI\|SIO] | TESTED (new) | sys_mman_h.c:63-65 |
| MH5 | MCL_CURRENT, MCL_FUTURE are defined [ML] | TESTED (new) | sys_mman_h.c:66 |
| MH6 | MAP_FAILED has type `void *` | TESTED (new) | sys_mman_h.c:77 |
| MH7 | POSIX_MADV_DONTNEED, _NORMAL, _RANDOM, _SEQUENTIAL, _WILLNEED are defined and distinct [ADV] | TESTED (new) | sys_mman_h.c:67-76 |
| MH8 | POSIX_TYPED_MEM_* constants and struct posix_typed_mem_info [TYM] | Not applicable | TYM isn't supported |
| MH9 | mode_t, off_t and size_t are defined | TESTED (new) | sys_mman_h.c:86-90 |
| MH10 | O_RDONLY, O_RDWR, O_CREAT, O_EXCL, O_TRUNC are defined [SHM] | TESTED (new, picolibc only) | sys_mman_h.c:79-84. **Were missing; fixed** (the header now includes `<fcntl.h>`, which POSIX allows). glibc 2.43 doesn't define them here either |
| MH11 | O_WRONLY, O_CLOEXEC, O_CLOFORK [TYM] | Not applicable | TYM isn't supported. O_WRONLY and O_CLOEXEC now come in with `<fcntl.h>`; O_CLOFORK doesn't exist (see the files unit's cross-cutting notes) |
| MH12 | The 11 functions have the specified prototypes | TESTED (new) | sys_mman_h.c:92-102 (function pointers of the exact types) |
| MH13 | posix_mem_offset, posix_typed_mem_get_info, posix_typed_mem_open prototypes [TYM] | Not applicable | TYM isn't supported |
| MH14 | Strict mode doesn't declare madvise, mlock2, MADV_*, MLOCK_ONFAULT or MCL_ONFAULT | TESTED (new) | sys_mman_h.c:104-116. MCL_ONFAULT is picolibc only, because glibc 2.43 defines it in strict mode |

### mmap

| # | Requirement | Status | Reference |
|---|---|---|---|
| MM1 | Regular files can be mapped | TESTED | mmap.c:200-226; (new) munmap.c:283-312 |
| MM2 | Anonymous memory can be mapped | TESTED | mmap.c:88-100; (new) munmap.c:131-138 |
| MM3 | Shared memory objects can be mapped [SHM] | TESTED | shm_open.c:86-107 |
| MM4 | Typed memory objects [TYM] | Not applicable | TYM isn't supported |
| MM5 | A successful call returns pa, never MAP_FAILED | TESTED | every mapping in the tests |
| MM6 | A MAP_FIXED mapping replaces earlier mappings of the whole pages it covers | TESTED (new) | munmap.c:164-171 (the middle page is replaced; its neighbours aren't) |
| MM7 | len zero fails and maps nothing | TESTED | mmap.c:176-179; (new) munmap.c:214-217 |
| MM8 | No write succeeds without PROT_WRITE, and no access with PROT_NONE alone | TESTED (new) | mprotect.c:118-120, 146-153 (a PROT_READ file mapping gets SIGSEGV on write) |
| MM9 | PROT_NONE, PROT_READ, PROT_WRITE and PROT_READ\|PROT_WRITE are supported | TESTED (new) | mprotect.c:109-127; munmap.c:244-257 |
| MM10 | fildes must be open for reading whatever the protection, and for writing for a writable MAP_SHARED mapping | TESTED (new) | munmap.c:259-270 (EACCES both ways) |
| MM11 | MAP_SHARED writes change the underlying object | TESTED | mmap.c:200-226; (new) munmap.c:298-312 |
| MM12 | MAP_PRIVATE changes are seen only by the caller and don't change the object | TESTED (new) | munmap.c:283-294 (file), 140-154 (a child's writes) |
| MM13 | Either MAP_SHARED or MAP_PRIVATE, not both | TESTED (new) | munmap.c:200-212 (EINVAL, as "the value of flags is invalid"). **A file was mapped shared; fixed.** The file case is picolibc only: glibc 2.43 passes both flags to Linux, which reads them as MAP_SHARED_VALIDATE |
| MM14 | The mapping type is kept across fork | TESTED (new) | munmap.c:140-153 |
| MM15 | MAP_FIXED is supported [XSI] | TESTED (new) | munmap.c:164-185 |
| MM16 | A MAP_FIXED mapping also removes memory locks on the pages it replaces | UNTESTED | read: the kernel does this (munmap of the old range) |
| MM17 | Without MAP_FIXED, a mapping is never placed at 0 and never replaces one | TESTED (new) | munmap.c:157-160 (a hint at a mapped address gets another address, and the old contents stay) |
| MM18 | A MAP_FIXED addr may have to be a multiple of the page size (EINVAL) | TESTED (new) | munmap.c:230-233 |
| MM19 | A partial page in the range is mapped in full | TESTED (new) | munmap.c:131-138 |
| MM20 | MAP_ANONYMOUS/MAP_ANON with fildes -1 and off 0 maps new memory, initialized to zero | TESTED (new) | munmap.c:131-136 |
| MM21 | The partial page at the end of an object is zero-filled | TESTED (new) | munmap.c:290-292 |
| MM22 | Changes past the end of the object in its last page are never written | TESTED (new) | munmap.c:303-310 (file size unchanged) |
| MM23 | References to whole pages past the end of the object get SIGBUS | TESTED (new) | munmap.c:306 |
| MM24 | mmap adds a reference that close doesn't remove | TESTED (new) | munmap.c:302-305; mprotect.c:149 |
| MM25 | The first reference marks the access time for update | NOT TESTABLE | atime depends on the mount options (relatime, noatime) |
| MM26 | A write through MAP_SHARED\|PROT_WRITE marks mtime and ctime by the next msync | TESTED (new) | msync.c:93-103 |
| MM27 | Returns MAP_FAILED and sets errno on failure | TESTED | every error row |
| MM28 | EAGAIN: the mapping can't be locked as mlockall(MCL_FUTURE) requires [ML] | UNTESTED | needs a lower RLIMIT_MEMLOCK, which only the non-POSIX setrlimit name can set |
| MM29 | EINVAL: neither MAP_PRIVATE nor MAP_SHARED | TESTED (new) | munmap.c:197-199 |
| MM30 | EMFILE: too many mapped regions | UNTESTED | needs vm.max_map_count (65530) mappings |
| MM31 | ENOMEM: a MAP_FIXED range outside the address space | TESTED (new) | munmap.c:224-228 |
| MM32 | ENOMEM: no room for the mapping | TESTED (new) | munmap.c:218-223 |
| MM33 | ENOMEM: the lock mlockall requires can't be made [ML] | UNTESTED | as MM28 |
| MM34 | ENOTSUP: MAP_FIXED, MAP_PRIVATE or the prot combination isn't supported | Not applicable | Linux supports them |
| MM35 | ENXIO: an invalid MAP_FIXED combination of addr, len and off, or [off,off+len) invalid for the object | NOT TESTABLE | Linux maps any range of a regular file and delivers SIGBUS past its end (MM23) |
| MM36 | EACCES (as MM10) | TESTED (new) | munmap.c:259-270 |
| MM37 | EBADF: fildes isn't an open descriptor | TESTED (new) | munmap.c:244-253 (-1 without MAP_ANONYMOUS, and a closed descriptor) |
| MM38 | ENODEV: the file type isn't supported | TESTED (new) | munmap.c:254-257 (a pipe) |
| MM39 | EOVERFLOW: off plus len exceeds the offset maximum of the open file description | NOT TESTABLE | picolibc opens every file with large-file offsets, so the maximum is 2^63-1 |
| MM40 | EINVAL (may): off not a multiple of the page size | TESTED (new) | munmap.c:271-273 |
| MM41 | An offset whose page number doesn't fit the 32-bit mmap2 call isn't wrapped | TESTED (new) | munmap.c:313-323. **arm mapped offset 2^44 at offset 0; fixed** (EINVAL, as glibc). On aarch64 the mapping succeeds and its first page gets SIGBUS |

### munmap

| # | Requirement | Status | Reference |
|---|---|---|---|
| MU1 | Removes the mappings of the whole pages containing any part of the range | TESTED (new) | munmap.c:174-177 (one byte removes its page; the neighbours stay) |
| MU2 | Later references get SIGSEGV | TESTED (new) | munmap.c:175, 181 |
| MU3 | A range without mappings has no effect | TESTED (new) | munmap.c:179-180 |
| MU4 | addr may have to be a multiple of the page size (EINVAL, may fail) | TESTED (new) | munmap.c:238-239 |
| MU5 | Changes to a private mapping are discarded | UNTESTED | read: the kernel frees the pages |
| MU6 | Memory locks on the range are removed [ML\|MLR] | UNTESTED | read: the kernel does this |
| MU7 | Typed memory deallocation [TYM] | Not applicable | TYM isn't supported |
| MU8 | Returns 0, or -1 with errno | TESTED | every munmap call |
| MU9 | EINVAL: the range is outside the address space | TESTED (new) | munmap.c:240-242 |
| MU10 | EINVAL: len is 0 | TESTED (new) | munmap.c:236-237 |

### mprotect

| # | Requirement | Status | Reference |
|---|---|---|---|
| MP1 | Changes the protection of the whole pages containing any part of the range | TESTED (new) | mprotect.c:113-116 (a one-byte range changes its whole page and not the next) |
| MP2 | No write without PROT_WRITE, no access with PROT_NONE | TESTED (new) | mprotect.c:114, 118-120 |
| MP3 | PROT_NONE, PROT_READ, PROT_WRITE and PROT_READ\|PROT_WRITE are supported | TESTED | mmap.c:110-121; (new) mprotect.c:113-127 |
| MP4 | addr may have to be a multiple of the page size (EINVAL) | TESTED (new) | mprotect.c:130-131 |
| MP5 | Contents are kept across changes | TESTED (new) | mprotect.c:126-127 |
| MP6 | EACCES: PROT_WRITE on a MAP_SHARED mapping of a file not open for writing, even after close | TESTED (new) | mprotect.c:139-154 |
| MP7 | PROT_WRITE on a MAP_PRIVATE mapping of a read-only file is allowed | TESTED (new) | mprotect.c:157-165 |
| MP8 | EAGAIN or ENOMEM: no memory to reserve for writable private pages | NOT TESTABLE | Linux overcommits, so the failure can't be provoked reliably |
| MP9 | ENOMEM: the range has unmapped pages | TESTED (new) | mprotect.c:132-136 |
| MP10 | ENOTSUP, or failure, for an unsupported prot combination | Not applicable | Linux supports every combination |

### msync

| # | Requirement | Status | Reference |
|---|---|---|---|
| MS1 | Writes the modified data of MAP_SHARED file mappings | TESTED | mmap.c:232-255; (new) msync.c:98-104 |
| MS2 | The writes complete as synchronized I/O data integrity completion | NOT TESTABLE | needs a crash or power-loss test |
| MS3 | Changes in MAP_PRIVATE mappings are never written | TESTED (new) | msync.c:132-138 |
| MS4 | MS_ASYNC and MS_SYNC are accepted | TESTED (new) | msync.c:100-101 (when the writes complete isn't observable) |
| MS5 | MS_INVALIDATE makes later references see the stored data | TESTED (new) | msync.c:105-107 |
| MS6 | Writes mark mtime and ctime | TESTED (new) | msync.c:103 |
| MS7 | addr may have to be a multiple of the page size (EINVAL) | TESTED (new) | msync.c:116-117 |
| MS8 | EBUSY: MS_INVALIDATE on locked pages | TESTED (new) | msync.c:119-127 (runs when mlock succeeds, which it does here) |
| MS9 | EINVAL: invalid flags, including MS_ASYNC with MS_SYNC | TESTED (new) | msync.c:109-115. **Unknown flags were dropped and accepted; fixed** |
| MS10 | ENOMEM: the range is outside the address space or has unmapped pages | TESTED (new) | msync.c:128-130 |
| MS11 | msync is a cancellation point | TESTED | testcancel.c:622 |

### mlock, munlock

| # | Requirement | Status | Reference |
|---|---|---|---|
| ML1 | mlock makes the pages resident until unlocked, exit or exec | NOT TESTABLE | residency isn't observable through POSIX; the return value is checked at mlock.c:76 |
| ML2 | munlock unlocks however many times the pages were locked | TESTED | mlock.c:77-79 (return values only) |
| ML3 | Locks by other processes or other mappings are unaffected | NOT TESTABLE | not observable |
| ML4 | Returns 0, or -1 with no change to locks | TESTED | mlock.c:76-79 |
| ML5 | ENOMEM: part of the range isn't mapped (mlock and munlock) | TESTED (new) | mlock.c:95-105 |
| ML6 | EAGAIN: some memory couldn't be locked | NOT TESTABLE | can't be provoked |
| ML7 | EINVAL (may): addr not a multiple of {PAGESIZE} | Not applicable | Linux rounds addr down instead |
| ML8 | ENOMEM (may) for the lock limit; EPERM (may) without privilege | TESTED | mlock.c:62-66 and 76 accept either. Not provoked, because RLIMIT_MEMLOCK is unlimited here |

### mlockall, munlockall

| # | Requirement | Status | Reference |
|---|---|---|---|
| MA1 | MCL_CURRENT and MCL_FUTURE lock current and future pages | TESTED | mlock.c:81-82 (return values; residency is NOT TESTABLE, as ML1) |
| MA2 | munlockall unlocks every page and returns 0 | TESTED | mlock.c:83 |
| MA3 | EAGAIN: some memory couldn't be locked | NOT TESTABLE | as ML6 |
| MA4 | EINVAL: flags is zero | TESTED | mlock.c:85-86 |
| MA5 | EINVAL: flags includes unimplemented flags | TESTED (new) | mlock.c:87-91. **Unknown flags were dropped; fixed** |
| MA6 | ENOMEM, EPERM (may) | TESTED | as ML8 |

### posix_madvise

| # | Requirement | Status | Reference |
|---|---|---|---|
| PA1 | Has no effect on the semantics of access to the memory | TESTED (new) | posix_madvise.c:140-151 (every advice, on anonymous and on private file memory). **POSIX_MADV_DONTNEED discarded private data; fixed** |
| PA2 | Each of the five advice values is accepted | TESTED | mmap.c:126-143; (new) posix_madvise.c:146-148 |
| PA3 | Returns 0, or an error number | TESTED | every call checks the returned value |
| PA4 | EINVAL: invalid advice | TESTED | mmap.c:181-187; (new) posix_madvise.c:153-159 |
| PA5 | ENOMEM: the range is partly outside the address space | TESTED (new) | posix_madvise.c:162-177. POSIX_MADV_DONTNEED is picolibc only (glibc 2.43 doesn't look at the range). Under qemu-user, other advice is accepted for a hole and the check is skipped |
| PA6 | EINVAL (may): addr not a multiple of the page size | TESTED (new) | posix_madvise.c:160 |
| PA7 | EINVAL (may): len zero | Not applicable | Linux accepts it, which the "may" allows |

### shm_open

| # | Requirement | Status | Reference |
|---|---|---|---|
| SO1 | Makes a new open file description and the lowest free descriptor | TESTED (new) | shm_unlink.c:97-102 |
| SO2 | Names beginning with / refer to the same object in every process | TESTED | shm_open.c:90-107 |
| SO3 | Names without a leading / | Not applicable | implementation-defined; picolibc treats them as if they had the /, as other Linux C libraries do |
| SO4 | The open file description isn't shared with other opens | TESTED (new) | shm_unlink.c:112-116 (separate offsets) |
| SO5 | FD_CLOEXEC is set | TESTED (new) | shm_unlink.c:103 |
| SO6 | File status flags and access mode follow oflag | TESTED (new) | shm_unlink.c:104, 119-128 (with O_RDONLY, write gets EBADF and a writable MAP_SHARED gets EACCES) |
| SO7 | O_CREAT of an existing object has no effect | TESTED (new) | shm_unlink.c:131-135 |
| SO8 | A new object gets the effective user ID, and the mode less the umask | TESTED (new) | shm_unlink.c:94-107 (the group may be the directory's, so it isn't checked) |
| SO9 | A new object has size zero | TESTED (new) | shm_unlink.c:105, 167 |
| SO10 | O_CREAT\|O_EXCL fails if the object exists (EEXIST) | TESTED | shm_open.c:83 |
| SO11 | ... and the check and creation are atomic between processes | TESTED (new) | shm_unlink.c:173-193 (8 processes, one creator) |
| SO12 | O_TRUNC empties an existing object opened O_RDWR, keeping mode and owner | TESTED (new) | shm_unlink.c:136-141 |
| SO13 | close, ftruncate, mmap, shm_open and shm_unlink are atomic with each other | NOT TESTABLE | there's no window to observe |
| SO14 | The object and its data persist until unlinked and unreferenced | TESTED (new) | shm_unlink.c:156-170 |
| SO15 | Returns a non-negative descriptor, or -1 with errno | TESTED | every call |
| SO16 | EACCES: permission denied for oflag | TESTED (new) | shm_unlink.c:143-155 (skipped as root) |
| SO17 | EACCES: O_TRUNC without write permission | UNTESTED | read: open(2) checks it |
| SO18 | EEXIST (as SO10) | TESTED | shm_open.c:83 |
| SO19 | EINTR | NOT TESTABLE | opening a tmpfs file doesn't block |
| SO20 | EINVAL: shm_open isn't supported for the name | TESTED (new, picolibc only) | shm_unlink.c:209-216. **"/." and "/.." opened /dev/shm and /dev; fixed.** glibc 2.43 opens them too |
| SO21 | EMFILE: no descriptors left | UNTESTED | read: open(2) reports it |
| SO22 | ENFILE, ENOSPC | NOT TESTABLE | system-wide limits |
| SO23 | ENOENT: no O_CREAT and no object | TESTED | shm_open.c:111 |
| SO24 | ENAMETOOLONG (may) | TESTED (new) | shm_unlink.c:198-205 (ENAMETOOLONG or EINVAL) |
| SO25 | shm_open isn't a cancellation point | TESTED (new) | sem_unlink.c:69-81, 250-253 |

### shm_unlink

| # | Requirement | Status | Reference |
|---|---|---|---|
| SU1 | Removes the name | TESTED | shm_open.c:110-111 |
| SU2 | The name goes at once; the contents stay until the last reference goes | TESTED (new) | shm_unlink.c:156-164 |
| SU3 | Reusing the name gives a new object | TESTED (new) | shm_unlink.c:165-168 |
| SU4 | Returns 0, or -1 with the object unchanged | TESTED | shm_open.c:110-112 |
| SU5 | EACCES | NOT TESTABLE | /dev/shm is sticky and world-writable, so only another user's object is protected; needs a second user |
| SU6 | ENOENT | TESTED | shm_open.c:112; (new) shm_unlink.c:195-196 |
| SU7 | ENAMETOOLONG (may) | TESTED (new) | shm_unlink.c:206-208 |
| SU8 | No ENAMETOOLONG for a name shm_open accepted | UNTESTED | read: both use `_shm_path` (local-shm.h); the sem_unlink version is tested (SL9) |
| SU9 | shm_unlink isn't a cancellation point | TESTED (new) | sem_unlink.c:78-79, 250-253 |

### Typed memory (posix_mem_offset, posix_typed_mem_get_info, posix_typed_mem_open)

| # | Requirement | Status | Reference |
|---|---|---|---|
| TY1-TY3 | The three functions [TYM] | Not applicable | TYM isn't supported (3 rows) |

### Header (basedefs/semaphore.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| SH1 | sem_t is defined | TESTED (new) | semaphore_h.c:67 |
| SH2 | struct timespec is defined | TESTED (new) | semaphore_h.c:68-70 |
| SH3 | SEM_FAILED has type `sem_t *` | TESTED (new) | semaphore_h.c:69 |
| SH4 | O_CREAT and O_EXCL are defined | TESTED (new, picolibc only) | semaphore_h.c:72-75. glibc 2.43 doesn't define them here |
| SH5 | The 11 functions have the specified prototypes | TESTED (new) | semaphore_h.c:77-89 |
| SH6 | Application macros with plain names don't break the header (sem_t's members) | TESTED (new) | semaphore_h.c:36-58 (`waiters`, `pshared`, `count`, `lock`). A macro named `value` breaks `<time.h>`; see cross-cutting |
| SH7 | Descriptor-based semaphores allow {OPEN_MAX} files and semaphores | Not applicable | named semaphores close their descriptor once mapped |

### sem_init, sem_destroy

| # | Requirement | Status | Reference |
|---|---|---|---|
| SI1 | Sets the value; the semaphore works for the wait, post and destroy calls | TESTED | sem_init.c:123-145 |
| SI2 | pshared non-zero: shared between processes | TESTED | sem_init.c:161-176 |
| SI3 | pshared zero: shared between threads | TESTED | sem_init.c:147-158 |
| SI4 | Returns 0, or -1 with errno | TESTED | sem_init.c:123-126 |
| SI5 | EINVAL: value above {SEM_VALUE_MAX} | TESTED | sem_init.c:123-125 |
| SI6 | ENOSPC, EPERM; EMFILE, ENFILE (may) | Not applicable | sem_init needs no resource or privilege |
| SI7 | XSH 2.9.9: the same semaphore through another mapping | UNTESTED | read: process-shared futexes are keyed by the page, so it works |
| SD1 | sem_destroy destroys an unnamed semaphore and returns 0 | TESTED | sem_init.c:141, 145, 175 |
| SD2 | EBUSY (may): threads are blocked on it | UNTESTED | read: sem_destroy.c checks `__waiters`. A test needs a thread known to be blocked |
| SD3 | EINVAL (may) | Not applicable | there's no validity marker |

### sem_open, sem_close, sem_unlink

| # | Requirement | Status | Reference |
|---|---|---|---|
| SN1 | Connects to the named semaphore, which stays usable until sem_close, _exit or exec | TESTED | sem_open.c:89-111 |
| SN2 | O_CREAT creates it with value | TESTED | sem_open.c:89-95; (new) sem_unlink.c:150-155 |
| SN3 | O_CREAT of an existing semaphore has no effect | TESTED (new) | sem_unlink.c:161-164 (value and handle kept) |
| SN4 | The creator's effective user ID, and the mode less the umask | TESTED (new) | sem_unlink.c:150-160 (checked where the name is visible in /dev/shm, as with picolibc and glibc on Linux) |
| SN5 | Other processes connect with the same name | TESTED | sem_open.c:98-107 |
| SN6 | O_CREAT\|O_EXCL fails if the name exists, atomically between processes | TESTED | sem_open.c:91; (new) sem_unlink.c:203-221 |
| SN7 | Same or unique handle for repeated opens (implementation-defined) | TESTED | sem_open.c:93-94 (same handle) |
| SN8 | Returns the semaphore's address, never SEM_FAILED on success | TESTED | every sem_open |
| SN9 | EACCES | TESTED (new) | sem_unlink.c:167-180 (skipped as root) |
| SN10 | EEXIST | TESTED | sem_open.c:91 |
| SN11 | EINTR, ENOMEM, ENOSPC | NOT TESTABLE | creating a file on tmpfs doesn't block or run out here |
| SN12 | EINVAL: value above {SEM_VALUE_MAX} with O_CREAT | TESTED (new) | sem_unlink.c:145-148 |
| SN13 | ENOENT | TESTED | sem_open.c:113 |
| SN14 | EMFILE, ENFILE (may) | UNTESTED | read: open(2) reports them |
| SN15 | ENAMETOOLONG (may) | TESTED (new) | sem_unlink.c:226-235 (ENAMETOOLONG or EINVAL) |
| SN16 | sem_open isn't a cancellation point | TESTED (new) | sem_unlink.c:71-73, 250-253 |
| SC1 | sem_close ends the process's use and frees its resources | TESTED | sem_open.c:109-111 |
| SC2 | sem_close doesn't change a semaphore that isn't unlinked | TESTED | sem_open.c:110 |
| SC3 | After sem_unlink, the semaphore goes when every handle is closed | TESTED (new) | sem_unlink.c:181-201 |
| SC4 | EINVAL (may): not a valid semaphore | TESTED (new, picolibc only) | sem_unlink.c:239-246 (an unnamed semaphore). glibc unmaps it instead |
| SC5 | sem_close isn't a cancellation point | TESTED (new) | sem_unlink.c:74-75 |
| SL1 | sem_unlink removes the name | TESTED | sem_open.c:112-113 |
| SL2 | Open semaphores keep working, in other processes too | TESTED (new) | sem_unlink.c:181-194 |
| SL3 | sem_open after sem_unlink gets a new semaphore | TESTED (new) | sem_unlink.c:195-198 |
| SL4 | sem_unlink doesn't wait for references to go | TESTED (new) | sem_unlink.c:183 |
| SL5 | Returns 0, or -1 with the semaphore unchanged | TESTED | sem_open.c:112-114 |
| SL6 | EACCES | NOT TESTABLE | as SU5 |
| SL7 | ENOENT | TESTED | sem_open.c:114 |
| SL8 | ENAMETOOLONG (may) | UNTESTED | read: the same check as sem_open (SN15) |
| SL9 | No ENAMETOOLONG for a name sem_open accepted | TESTED (new) | sem_unlink.c:226-238 (the longest name that works) |
| SL10 | sem_unlink isn't a cancellation point | TESTED (new) | sem_unlink.c:76-77 |

### sem_wait, sem_trywait, sem_timedwait, sem_clockwait

| # | Requirement | Status | Reference |
|---|---|---|---|
| SW1 | sem_trywait locks only a positive semaphore, and fails with EAGAIN otherwise | TESTED | sem_init.c:128-130 |
| SW2 | sem_wait blocks until it locks the semaphore or a signal interrupts it | TESTED | sem_init.c:147-158; (new) sem_timedwait.c:207-222 |
| SW3 | The semaphore stays locked until sem_post | TESTED | sem_init.c:128-130 |
| SW4 | sem_wait can be interrupted by a signal, and an unsuccessful call leaves the value unchanged | TESTED (new) | sem_timedwait.c:207-222 (a handler posts while main waits; on EINTR the posted unit is still there) |
| SW5 | EDEADLK, EINVAL (may) | Not applicable | not detected |
| SW6 | EINTR (may) | TESTED (new) | sem_timedwait.c:216-219 accepts it |
| SW7 | sem_wait is a cancellation point | TESTED | pthread_cancel.c:277; (new) sem_timedwait.c:224-229 (pending request) |
| SW8 | sem_trywait isn't a cancellation point | TESTED | testcancel.c:665 |
| ST1 | The timed waits lock as sem_wait does, or stop when the timeout expires | TESTED | sem_init.c:131-140 |
| ST2 | The timeout expires when the clock reaches abstime | TESTED (new) | sem_timedwait.c:100-113 (on ETIMEDOUT the clock is at or past abstime) |
| ST3 | An abstime already passed times out at once | TESTED (new) | sem_timedwait.c:167-173 |
| ST4 | sem_timedwait uses CLOCK_REALTIME; sem_clockwait takes CLOCK_REALTIME and CLOCK_MONOTONIC | TESTED | sem_init.c:131-135; (new) sem_timedwait.c:163-166 |
| ST5 | Never a timeout when the semaphore can be locked | TESTED (new) | sem_timedwait.c:175-179 |
| ST6 | abstime needn't be checked when the semaphore can be locked | TESTED (new, picolibc only) | sem_timedwait.c:181-186 (picolibc's choice) |
| ST7 | Returns 0, or -1 with the value unchanged | TESTED (new) | sem_timedwait.c:113, 206 |
| ST8 | EINVAL: it would block and tv_nsec < 0 or >= 1000 million | TESTED | sem_init.c:136-137; (new) sem_timedwait.c:188-199 |
| ST9 | EINVAL: sem_clockwait with an invalid or unsupported clock | TESTED (new) | sem_timedwait.c:200-205 (an unknown clock and CLOCK_PROCESS_CPUTIME_ID) |
| ST10 | ETIMEDOUT | TESTED | sem_init.c:132-135 |
| ST11 | EDEADLK, EINVAL for sem, EINTR (may) | Not applicable | as SW5; EINTR as SW6 |
| ST12 | sem_timedwait and sem_clockwait are cancellation points | TESTED (new) | sem_timedwait.c:224-229 |

### sem_post, sem_getvalue

| # | Requirement | Status | Reference |
|---|---|---|---|
| SP1 | Increments the value, or lets one waiter return | TESTED | sem_init.c:147-158 |
| SP2 | [PS] Under SCHED_FIFO/SCHED_RR, wakes the highest-priority thread that has waited longest | NOT TESTABLE | no real-time scheduling privilege on this host. Read: futex wake order follows priority for real-time tasks |
| SP3 | [SS] SCHED_SPORADIC behaves as SCHED_FIFO | Not applicable | no sporadic server |
| SP4 | Async-signal-safe, callable from a signal handler | TESTED (new) | sem_timedwait.c:207-222 |
| SP5 | Returns 0, or -1 with errno | TESTED | sem_init.c:138, 144 |
| SP6 | EOVERFLOW | TESTED | sem_init.c:143-144 |
| SP7 | EINVAL (may) | Not applicable | there's no validity marker |
| SP8 | sem_post isn't a cancellation point | TESTED | testcancel.c:665 |
| SG1 | sem_getvalue stores a value the semaphore had, without changing it | TESTED | sem_init.c:127, 158 |
| SG2 | A locked semaphore reads 0, or minus the number of waiters | TESTED | sem_init.c:158; sem_timedwait.c:113 (0) |
| SG3 | Returns 0; EINVAL (may) | TESTED | sem_init.c:127 (EINVAL is Not applicable) |

### Header (basedefs/sched.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| CH1 | pid_t is defined [PS] | TESTED (new) | sched_h.c:60, 65 |
| CH2 | time_t is defined [SS\|TSP] | TESTED (new) | sched_h.c:61 (defined even though the options aren't supported) |
| CH3 | struct timespec is defined | TESTED (new) | sched_h.c:59, 64 |
| CH4 | struct sched_param has `int sched_priority` | TESTED (new) | sched_h.c:58, 63 |
| CH5 | The sporadic-server members [SS\|TSP] | Not applicable | no sporadic server (`_POSIX_SPORADIC_SERVER` is undefined on Linux) |
| CH6 | SCHED_FIFO, SCHED_RR, SCHED_OTHER are defined and distinct | TESTED (new) | sched_h.c:66 |
| CH7 | SCHED_SPORADIC [SS\|TSP] | Not applicable | as CH5; sched_h.c:68-72 checks it when defined |
| CH8 | The 8 functions have the specified prototypes | TESTED (new) | sched_h.c:81-89 |
| CH9 | Strict mode defines only `<sched.h>` and `<time.h>` names | TESTED (new) | sched_h.c:38-52 (no GNU policies, sched_getcpu, CPU_SETSIZE or `<stdint.h>` limits). **`<sched.h>` included `<sys/types.h>`, and with it all of `<stdint.h>`; fixed** |

### Scheduling functions

| # | Requirement | Status | Reference |
|---|---|---|---|
| CP1 | sched_get_priority_max/min return each policy's range | TESTED | sched_get_priority_max.c, sched_get_priority_min.c (FIFO and RR span at least 32 priorities) |
| CP2 | EINVAL: not a defined policy | TESTED | both files |
| CG1 | sched_getparam returns the parameters of pid, or of the caller for 0 | TESTED | sched_getparam.c |
| CG2 | EPERM | NOT TESTABLE | Linux lets any process read scheduling parameters |
| CG3 | ESRCH | TESTED | sched_getparam.c |
| CS1 | sched_setparam sets the parameters of pid, or of the caller for 0 | TESTED | sched_setparam.c:66-67 (SCHED_OTHER priority 0 only) |
| CS2 | EINVAL: priority outside the current policy's range | TESTED | sched_setparam.c:68-69; (new) 71-73 |
| CS3 | Effects under SCHED_FIFO and SCHED_RR | NOT TESTABLE | no real-time privilege |
| CS4 | [SS] sched_ss_* checks | Not applicable | no sporadic server |
| CS5 | On failure the priority is unchanged | TESTED (new) | sched_setscheduler.c:97-99 |
| CS6 | EPERM: no permission for the target process | TESTED (new) | sched_setparam.c:75-82 (process 1, which this process may not signal; skipped otherwise) |
| CS7 | ESRCH | TESTED | sched_setparam.c:70 |
| CQ1 | sched_getscheduler returns the policy of pid, or of the caller for 0 | TESTED | sched_getscheduler.c |
| CQ2 | EPERM | NOT TESTABLE | as CG2 |
| CQ3 | ESRCH | TESTED | sched_getscheduler.c |
| CR1 | sched_setscheduler sets the policy and parameters | TESTED | sched_setscheduler.c:68-69 (own policy only). Changing to SCHED_FIFO/RR is NOT TESTABLE here |
| CR2 | Returns the former policy | TESTED | sched_setscheduler.c:69 (same policy; a different former policy needs privilege) |
| CR3 | On failure the policy and parameters are unchanged | TESTED (new) | sched_setscheduler.c:83-99 |
| CR4 | EINVAL: invalid policy, or a parameter outside its range | TESTED | sched_setscheduler.c:80; (new) 85-96 |
| CR5 | EPERM | TESTED | sched_setscheduler.c:71-79 (SCHED_FIFO, EPERM on this host); (new) 101-111 (another user's process) |
| CR6 | ESRCH | TESTED (new) | sched_setscheduler.c:113-116 |
| CR7 | [SS] sched_ss_* checks | Not applicable | no sporadic server |
| CI1 | sched_rr_get_interval stores the quantum for pid or the caller | TESTED | sched_rr_get_interval.c |
| CI2 | ESRCH | TESTED | sched_rr_get_interval.c |
| CY1 | sched_yield returns 0 | TESTED | sched_yield.c |
| CY2 | The thread goes to the end of its priority list | NOT TESTABLE | the ordering needs SCHED_FIFO |
| PR1 | The calls act on the process | NOT TESTABLE | Linux applies them to one thread; see "Deviations found by reading" |

### Counts

| Area | Rows | TESTED before | TESTED after | UNTESTED | NOT TESTABLE | Not applicable |
|---|---|---|---|---|---|---|
| `<sys/mman.h>` header | 14 | 0 | 11 | 0 | 0 | 3 |
| mmap | 41 | 7 | 32 | 4 | 3 | 2 |
| munmap | 10 | 1 | 7 | 2 | 0 | 1 |
| mprotect | 10 | 1 | 8 | 0 | 1 | 1 |
| msync | 11 | 2 | 10 | 0 | 1 | 0 |
| mlock, munlock | 8 | 3 | 4 | 0 | 3 | 1 |
| mlockall, munlockall | 6 | 4 | 5 | 0 | 1 | 0 |
| posix_madvise | 7 | 3 | 6 | 0 | 0 | 1 |
| shm_open | 25 | 5 | 19 | 2 | 3 | 1 |
| shm_unlink | 9 | 3 | 7 | 1 | 1 | 0 |
| typed memory | 3 | 0 | 0 | 0 | 0 | 3 |
| `<semaphore.h>` header | 7 | 0 | 6 | 0 | 0 | 1 |
| sem_init, sem_destroy | 10 | 6 | 6 | 2 | 0 | 2 |
| sem_open, sem_close, sem_unlink | 31 | 13 | 27 | 2 | 2 | 0 |
| waits | 20 | 9 | 18 | 0 | 0 | 2 |
| sem_post, sem_getvalue | 11 | 7 | 8 | 0 | 1 | 2 |
| `<sched.h>` header | 9 | 0 | 7 | 0 | 0 | 2 |
| scheduling functions | 27 | 16 | 20 | 0 | 5 | 2 |
| **Total** | **259** | **80** | **201** | **13** | **21** | **24** |

## Deviations found and fixed

Each one was verified by running. I reverted only the library change (`git show <fix> -- libc libos | git apply -R`), rebuilt the test and ran it: it fails with the fix reverted and passes with it. All of them were run on aarch64-linux-gnu, except the mmap2 offset, which only arm has; there I reverted just that hunk.

1. **MAP_ANON was hidden, and `<sys/mman.h>` didn't define the shm_open flags** (libc/include/sys/mman.h).
   - Spec: "MAP_ANON: Synonym for MAP_ANONYMOUS. MAP_ANON shall have the same value as MAP_ANONYMOUS", and "The <sys/mman.h> header shall define the following symbolic constants as described in <fcntl.h>: [SHM] O_RDONLY O_RDWR ... O_CREAT O_EXCL O_TRUNC".
   - Fix: MAP_ANON is now unconditional, and the header includes `<fcntl.h>` ("Inclusion of the <sys/mman.h> header may make visible all symbols from the <fcntl.h> header").
   - Before: test-sys_mman_h doesn't compile (`'MAP_ANON' undeclared`).
2. **`<sched.h>` defined all of `<stdint.h>`** (libc/include/sched.h).
   - It included `<sys/types.h>`, which includes `<stdint.h>`. POSIX lets `<sched.h>` make `<time.h>` symbols visible, but not INT8_MAX or SIZE_MAX.
   - Fix: include `<sys/_types.h>` and declare pid_t and time_t directly, as other headers do.
   - Before: test-sched_h stops at `#error "<sched.h> defines <stdint.h> or <sys/select.h> names"`.
3. **posix_madvise(POSIX_MADV_DONTNEED) discarded data** (libos/linux/madvise.c).
   - It passed Linux MADV_DONTNEED, which drops private pages: anonymous memory read back as zero, and private file pages read back as the file's contents.
   - Spec: "The posix_madvise() function shall have no effect on the semantics of access to memory in the specified range".
   - Fix: for POSIX_MADV_DONTNEED, only check the range, with msync(MS_ASYNC). On Linux that only validates the range (mm/msync.c writes nothing without MS_SYNC), so ENOMEM and EINVAL are still reported. glibc returns 0 without a system call.
   - Before: `anonymous contents kept: all(anon, 4 * page, c)` failed for POSIX_MADV_DONTNEED.
4. **mlockall accepted unimplemented flags** (libos/linux/mlockall_munlockall.c).
   - The flag mapping dropped unknown bits.
   - Spec: "[EINVAL] The flags argument is zero, or includes unimplemented flags."
   - Fix: EINVAL for any bits other than MCL_CURRENT, MCL_FUTURE and MCL_ONFAULT.
   - Before: `unimplemented flag: mlockall(MCL_CURRENT | 0x40000000) == -1 && errno == EINVAL`.
5. **msync accepted unknown flags** (libos/linux/msync.c).
   - The flag mapping had the same problem. Spec: "[EINVAL] The value of flags is invalid."
   - Fix: EINVAL for unknown bits, checked after `_cancel_enter` as the net-sockets lesson says. Linux already rejects MS_ASYNC together with MS_SYNC.
   - Before: `unknown flag: msync(p, page, MS_SYNC | bad) == -1 && errno == EINVAL`.
6. **mmap mapped a file shared for MAP_SHARED | MAP_PRIVATE, and arm wrapped large offsets** (libos/linux/mmap.c).
   - **Both flags:** together they have the value of Linux's MAP_SHARED_VALIDATE, so a file mapping succeeded as a shared mapping. Anonymous memory already got EINVAL from the kernel.
     - Spec: "Either MAP_SHARED or MAP_PRIVATE can be specified, but not both", and "[EINVAL] The value of flags is invalid".
     - Fix: EINVAL.
     - Before (aarch64): `both MAP_SHARED and MAP_PRIVATE, file: p == MAP_FAILED && errno == EINVAL`.
   - **Large offsets:** on mmap2 targets the page number went through `(long)(offset >> 12)`, so offset 2^44 became page 0 and mapped the start of the file.
     - Fix: EINVAL when the page number doesn't fit in unsigned long, as glibc does ("[EINVAL] ... off ... is considered invalid by the implementation").
     - Before (arm): `offset 2^44 maps past the end: touch_signal(p, 0) == SIGBUS` failed, because the mapping showed the file's first page.
7. **shm_open("/.") and shm_open("/..") opened /dev/shm and /dev** (libos/linux/local-shm.h).
   - They returned directory descriptors, not shared memory objects, and shm_unlink("/..") tried to unlink /dev.
   - Spec: "[EINVAL] The shm_open() operation is not supported for the given name."
   - Fix: `_shm_path` rejects "." and ".." when there's no prefix. Semaphore names get the "sem." prefix, so they're unaffected.
   - Before: `name /.: shm_open("/.", O_RDONLY, 0) == -1 && errno == EINVAL`.

## Deviations found by reading (not changed)

- **The scheduling calls act on one thread, not the process** (sched_setparam.c, sched_setscheduler.c, sched_getparam.c, sched_getscheduler.c).
  - Linux takes a thread ID, so pid 0 or getpid() changes or reports only the calling thread (or the main thread).
  - The spec describes the process: "the scheduling policy and scheduling parameters shall be set for the process whose process ID is equal to pid".
  - glibc and musl do the same. A fix would walk /proc/<pid>/task, racing with thread creation.
  - Here SCHED_OTHER priority 0 is the only setting allowed, so the difference can't be seen.
  - Needs a design decision; recorded in cross-cutting.md.
- **On realtime-profile targets, the layout of `struct sched_param` depends on include order** (libc/include/sys/sched.h:59).
  - The sporadic-server members appear under `#if defined(_POSIX_SPORADIC_SERVER)`, and `<sys/unistd.h>` defines that macro from `_POSIX_AEP_REALTIME_DEDICATED`.
  - A translation unit that includes `<unistd.h>` before `<sched.h>` therefore sees a bigger structure than one that doesn't. `defined` is also true when the macro is -1.
  - Linux defines neither macro, so this host can't show it. It matters for OSes that claim a realtime profile.
- **mmap and mprotect drop unknown PROT_ bits, and mmap drops unknown flags.** Linux itself would return EINVAL for unknown prot bits. POSIX has no error for them (only ENOTSUP for unsupported combinations), so I left it.
- **msync with neither MS_ASYNC nor MS_SYNC is accepted,** as Linux and glibc accept it. "Either MS_ASYNC or MS_SYNC shall be specified" reads as a requirement on the application, and making it EINVAL would break Linux programs that pass 0.

## Other observations (not changed)

- sem_open and shm_open already run with cancellation disabled. sem_close, sem_unlink and shm_unlink call nothing that is a cancellation point in picolibc, and the new test (sem_unlink.c:62-81) confirms that none of them acts on a pending request.
- A file created by shm_open("/sem.X") is also the named semaphore "/X", and sem_open maps it whatever its size, so a short file gives SIGBUS on use. glibc uses the same `/dev/shm/sem.` layout and has the same property.
- sem_wait returns EINTR when a signal handler runs while it waits. That holds even with SA_RESTART and a handler that posted the semaphore, because the futex wait isn't restarted. POSIX allows EINTR ("may fail"), and the posted unit is kept (SW4).
- Under qemu-user, posix_madvise with advice other than POSIX_MADV_DONTNEED may return 0 for a range with a hole, because qemu ignores most advice. The test skips that check under qemu only.

## Left undone

- These are UNTESTED:
  - EMFILE for mmap, which needs 65530 mappings;
  - EAGAIN/ENOMEM from MCL_FUTURE with a lowered limit, which needs Linux's RLIMIT_MEMLOCK name;
  - sem_destroy's EBUSY, which needs a thread known to be blocked.
- Everything that depends on SCHED_FIFO/SCHED_RR (sem_post wake order, sched_yield ordering, changing policy) needs a host with real-time privilege.
- EACCES for shm_unlink and sem_unlink needs a second user.

## Timing and robustness of the new tests

- No test sleeps or assumes another thread is blocked. In the signal test the handler may run before or during sem_wait, and both outcomes are accepted. The cancellation tests make the request pending before the call.
- Each timeout is 30 ms, and the deadline check reads the clock after the return, so a slow host only makes the tests take longer.
- Access checks (SIGSEGV, SIGBUS) run in forked children, so a failed access never kills the test itself.
- Files use `TEST_FILE_NAME` plus the PID, and shared memory and semaphore names include the PID. Every failed check unlinks them.
- The host security software didn't interfere; these tests make no socket calls.

## Test runs (verified by running)

All builds and tests ran in the Debian sid chroot. The Linux targets ran with `meson test -t 4 --num-processes 3`, full suite, without `--no-rebuild`.

| Configuration | Ok | Fail | Expected Fail | Skipped | Timeout |
|---|---|---|---|---|---|
| aarch64-linux-gnu | 1328 | 0 | 6 | 42 | 0 |
| arm-linux-gnueabihf | 1325 | 0 | 6 | 44 | 0 |
| native (`scripts/do-native-configure`, `ninja` only) | builds cleanly | | | | |
| CMake thumbv7m (`scripts/do-cmake-thumbv7m-configure`, `ninja test`) | 554 tests, 100% passed; 305 skipped (the new tests compile and skip on bare metal) | | | | |

The 6 expected failures are test-except, test-hosted-exit-fail and test-abort-failure, each in a picolibc and a native build.

After the full suites I rebuilt the semaphore test commit to remove a blank line that clang-format wanted, then re-ran test-semaphore_h and test-semaphore_h-native on both Linux targets (all Ok).

The new and changed tests were also run on their own on both targets, picolibc and `-native`: 30 of 30 Ok on each.

## Commits (audit/memory-sched, on top of d40e4ae0e4)

1. 4cdaa795b5 sys/mman.h: Define MAP_ANON and the shm_open flags in strict mode
2. 821d76dede sched.h: Don't include <sys/types.h>
3. 4d9f08bae3 libos/linux: Keep posix_madvise(POSIX_MADV_DONTNEED) from discarding data
4. d5ee8f083c libos/linux: Reject unimplemented mlockall flags
5. bb35b858a8 libos/linux: Reject unknown msync flags
6. d04a450cfb libos/linux: Reject invalid mmap flags and offsets
7. 0165e582ee libos/linux: Reject the shared memory names . and ..
8. ca4dbde3cf test-posix: Check the rest of <sys/mman.h>
9. ddd9bb9524 test-posix: Check timed waits, named semaphores and <semaphore.h>
10. 46c0dc8000 test-posix: Check <sched.h> and scheduling errors
