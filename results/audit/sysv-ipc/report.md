# `<sys/ipc.h>`, `<sys/msg.h>`, `<sys/sem.h>`, `<sys/shm.h>` audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-sysv-ipc`, branch `audit/sysv-ipc`, based on
d40e4ae0e4 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10, 4 KiB
pages; builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43) as an
unprivileged user. armhf ran natively on the AArch32 compat layer, so the 32-bit
structure conversions were checked against a real kernel, not qemu-user.

Implementation: `libos/linux/` (msgget.c, msgctl.c, msgsnd.c, msgrcv.c, semget.c,
semctl.c, semop.c, shmget.c, shmctl.c, shmat.c, shmdt.c and local-ipc.h, which holds the
kernel's 64-bit structures, the `LINUX_IPC_64` command flag for 32-bit ARM and the
conversions) and `libc/posix/sys-ipc/ftok.c`. Headers: `libc/include/sys/ipc.h`, `msg.h`,
`sem.h`, `shm.h`; key_t comes from `<sys/types.h>`.

Tests: `test/test-posix/test-sys-ipc/`, `test-sys-msg/`, `test-sys-sem/`, `test-sys-shm/`.
File names drop the `test-` prefix (`msgctl.c` = `test-sys-msg/test-msgctl.c`). Line
numbers are for the branch tip.

Spec source: the pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/sys_ipc.h,
sys_msg.h, sys_sem.h, sys_shm.h; functions/ftok, msgget, msgctl, msgsnd, msgrcv, semget,
semctl, semop, shmget, shmctl, shmat, shmdt; XSH 2.7 (XSI IPC, permissions and
timestamps), 2.2.2 (reserved names) and 2.9.5.2 (cancellation points). Issue 8 changes:
SHM_FAILED and intptr_t in `<sys/shm.h>` (Austin Group 1239); semget must zero semval,
sempid, semncnt and semzcnt (377); semop's handling of a failing or blocking element in
an array is spelled out (628); shmctl's IPC_RMID wording (1240). There is no semtimedop
in Issue 8 (functions/semtimedop.html doesn't exist), so it is out of scope.

Status key: TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row was
run and passes for picolibc (`test-X`) and against glibc (`test-X-native`) on
aarch64-linux-gnu and arm-linux-gnueabihf, unless it says picolibc only. Every test
creates its objects with IPC_PRIVATE or with an ftok key from a fresh `mkdtemp()` file,
and removes them from an atexit handler and from a 30-60 s SIGALRM watchdog, so failures
don't leak objects. Blocking checks use two processes: semaphores wait on GETNCNT and
GETZCNT, message queues on a pipe byte followed by the child's `/proc/PID/stat` state
`S`. Permission checks run only when the effective user ID isn't 0. "(existing)" marks
rows that were tested before this audit.

## Requirement table

### Headers (basedefs/sys_ipc.h, sys_msg.h, sys_sem.h, sys_shm.h; XSH 2.2.2)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | struct ipc_perm has uid (uid_t), gid (gid_t), cuid (uid_t), cgid (gid_t), mode (mode_t) | TESTED | sys_ipc_h.c:53 |
| H2 | `<sys/ipc.h>` defines uid_t, gid_t, mode_t and key_t | TESTED | sys_ipc_h.c:47-50 |
| H3 | IPC_CREAT, IPC_EXCL, IPC_NOWAIT; IPC_PRIVATE; IPC_RMID, IPC_SET, IPC_STAT (distinct, outside the permission bits, usable in #if) | TESTED | sys_ipc_h.c:60-73 |
| H4 | ftok prototype | TESTED | sys_ipc_h.c:76 |
| H5 | msgqnum_t and msglen_t are unsigned and hold at least an unsigned short | TESTED | sys_msg_h.c:47-48 |
| H6 | MSG_NOERROR | TESTED | sys_msg_h.c:56-59 |
| H7 | struct msqid_ds has msg_perm, msg_qnum, msg_qbytes, msg_lspid, msg_lrpid, msg_stime, msg_rtime, msg_ctime with the listed types | TESTED | sys_msg_h.c:62-70 |
| H8 | `<sys/msg.h>` defines pid_t, size_t, ssize_t, time_t | TESTED | sys_msg_h.c:51-53 |
| H9 | Prototypes of msgctl, msgget, msgrcv, msgsnd | TESTED | sys_msg_h.c:77-80 |
| H10 | `<sys/msg.h>` includes `<sys/ipc.h>` | TESTED | sys_msg_h.c:73-74 |
| H11 | SEM_UNDO; GETNCNT, GETPID, GETVAL, GETALL, GETZCNT, SETVAL, SETALL (distinct from each other and the IPC_ commands) | TESTED | sys_sem_h.c:52-62, 98-104 |
| H12 | struct semid_ds has sem_perm, sem_nsems (unsigned short), sem_otime, sem_ctime | TESTED (sem_nsems's type picolibc only; glibc's is unsigned long) | sys_sem_h.c:64-71 |
| H13 | `<sys/sem.h>` defines pid_t, size_t, time_t | TESTED | sys_sem_h.c:47-49 |
| H14 | struct sembuf has sem_num (unsigned short), sem_op (short), sem_flg (short) | TESTED | sys_sem_h.c:73-77 |
| H15 | Prototypes of semctl (variadic), semget, semop | TESTED | sys_sem_h.c:83-85 |
| H16 | `<sys/sem.h>` includes `<sys/ipc.h>` | TESTED | sys_sem_h.c:79-80 |
| H17 | union semun is the application's to declare, so the header must not | TESTED | sys_sem_h.c:88-92, and every semctl test |
| H18 | SHM_RDONLY, SHM_RND, SHMLBA | TESTED | sys_shm_h.c:59-61, 106-110. SHMLBA's value: see "Deviations not fixed" |
| H19 | SHM_FAILED, the same value as ((void *)(intptr_t)-1) (Issue 8) | TESTED (picolibc only; glibc lacks it) | sys_shm_h.c:63-66, 94-103. **Missing; fixed** |
| H20 | `<sys/shm.h>` defines intptr_t (Issue 8) | TESTED (picolibc only; glibc lacks it) | sys_shm_h.c:50-53 |
| H21 | shmatt_t is unsigned and holds at least an unsigned short | TESTED | sys_shm_h.c:56 |
| H22 | struct shmid_ds has shm_perm, shm_segsz, shm_lpid, shm_cpid, shm_nattch, shm_atime, shm_dtime, shm_ctime with the listed types | TESTED | sys_shm_h.c:69-77 |
| H23 | `<sys/shm.h>` defines pid_t, size_t, time_t | TESTED | sys_shm_h.c:47-49 |
| H24 | Prototypes of shmat, shmctl, shmdt, shmget | TESTED | sys_shm_h.c:84-87 |
| H25 | `<sys/shm.h>` includes `<sys/ipc.h>` | TESTED | sys_shm_h.c:80-81 |
| H26 | Strict XSI mode declares nothing outside the reserved names (ipc_/IPC_, msg/MSG_, sem/SEM_, shm/SHM[A-Z]/SHM_, *_t) | UNTESTED by the suite; checked with `-E -dM` | The four headers add only reserved names. Everything else comes from `<sys/types.h>`, which pulls in all of `<stdint.h>`; recorded as cross-cutting |
| H27 | Each semaphore has semval, sempid, semncnt, semzcnt | NOT TESTABLE directly | an internal structure; read through GETVAL, GETPID, GETNCNT and GETZCNT (S rows) |

### XSH 2.7: common rules

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Each object has a unique identifier | TESTED | msgget.c:157, semget.c:130, shmget.c:140 (distinct, non-negative; Linux can return 0, which the "positive" in 2.7.1 doesn't allow but the get functions' "non-negative" does) |
| C2 | Read and write/alter permission come from the owner, group or other bits of mode, and the owner is matched by uid or cuid | TESTED for the owner bits | the EACCES rows below; group and other bits NOT TESTABLE without a second user |
| C3 | A process with appropriate privileges is always granted permission | NOT TESTABLE here | not run as root |
| C4 | A timestamp is set to the largest time_t not greater than the current time | TESTED | every time member is checked against time() before and after (msgctl.c, semctl.c, shmctl.c) |

### ftok

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | Returns a key usable with msgget, semget and shmget | TESTED | msgget.c:164-179, semget.c, shmget.c |
| F2 | The same key for every path naming the same file, with the same id | TESTED | ftok.c:121, 135-138 (hard link, symbolic link, `dir/./../dir/file`) |
| F3 | Should give different keys for different ids or files | TESTED | ftok.c:122, 145-146 |
| F4 | Only the low 8 bits of id matter | TESTED | ftok.c:141-142 |
| F5 | Succeeds where stat would fail with EOVERFLOW for the file's size | TESTED | ftok.c:148-155 (a 3 GiB sparse file; picolibc's off_t is 64-bit on armhf) |
| F6 | Returns (key_t)-1 and sets errno on failure | TESTED | ftok.c:123 (existing), 159-173 |
| F7 | EACCES for search permission on a prefix | TESTED | ftok.c:169-173 |
| F8 | EIO | NOT TESTABLE | needs a failing file system |
| F9 | ELOOP for a symbolic link loop | TESTED | ftok.c:163-165 |
| F10 | ENAMETOOLONG for a component over NAME_MAX | TESTED | ftok.c:166-168 |
| F11 | ENOENT for a missing file or an empty path | TESTED | ftok.c:123 (existing), 159-160 |
| F12 | ENOTDIR for a non-directory prefix, or a trailing slash after a non-directory | TESTED | ftok.c:161-162 |
| F13 | May fail: ELOOP past SYMLOOP_MAX, ENAMETOOLONG past PATH_MAX | UNTESTED | the path comes from stat; PATH_MAX is the known cross-cutting 1024 |

### msgget

| # | Requirement | Status | Reference |
|---|---|---|---|
| MG1 | Returns the identifier for an existing key, with or without IPC_CREAT | TESTED | msgget.c:180-182 |
| MG2 | IPC_PRIVATE always creates, with or without IPC_CREAT | TESTED | msgget.c:150-157 (existing in part) |
| MG3 | IPC_CREAT creates for a key with no queue | TESTED | msgget.c:178-179 |
| MG4 | cuid, uid, cgid and gid are the effective IDs | TESTED | msgget.c:194-195 |
| MG5 | The low 9 bits of mode come from msgflg | TESTED | msgget.c:196 |
| MG6 | msg_qnum, msg_lspid, msg_lrpid, msg_stime, msg_rtime are 0 | TESTED | msgget.c:197-199 |
| MG7 | msg_ctime is the current time | TESTED | msgget.c:201 |
| MG8 | msg_qbytes is the system limit | TESTED | msgget.c:202-206 (`/proc/sys/kernel/msgmnb`) |
| MG9 | Returns a non-negative identifier, or -1 and errno | TESTED | throughout |
| MG10 | EACCES when the queue doesn't grant what the low 9 bits of msgflg ask | TESTED | msgget.c:208-215 |
| MG11 | EEXIST with IPC_CREAT and IPC_EXCL | TESTED | msgget.c:183 |
| MG12 | ENOENT without IPC_CREAT | TESTED | msgget.c:174 |
| MG13 | ENOSPC at the system-wide limit | NOT TESTABLE | msgmni is host-wide; filling it would disturb other users |

### msgctl

| # | Requirement | Status | Reference |
|---|---|---|---|
| MC1 | IPC_STAT copies every member of msqid_ds | TESTED (each member holds a value only it can have; the times are distinct seconds) | msgctl.c:189-202 |
| MC2 | IPC_SET sets msg_perm.uid, msg_perm.gid, msg_perm.mode and msg_qbytes, and nothing else | TESTED | msgctl.c:204-240 |
| MC3 | IPC_SET sets msg_ctime | TESTED | msgctl.c:225 |
| MC4 | IPC_SET by the owner or creator (or privileged) | TESTED for the creator after giving the queue away | msgctl.c:227-246 |
| MC5 | Only a privileged process can raise msg_qbytes | TESTED past msgmnb | msgctl.c:249-254. **Deviation (kernel), not fixed:** Linux lets the owner raise it up to msgmnb (glibc probe) |
| MC6 | IPC_RMID destroys the queue and its messages | TESTED | msgctl.c:272-279 (existing in part) |
| MC7 | IPC_RMID by the owner or creator (or privileged) | TESTED for the owner | msgctl.c:273 |
| MC8 | Returns 0, or -1 and errno | TESTED | throughout |
| MC9 | EACCES for IPC_STAT without read permission | TESTED | msgctl.c:256-263 |
| MC10 | EINVAL for a bad identifier or command | TESTED | msgctl.c:268-270 |
| MC11 | EPERM for IPC_RMID or IPC_SET by a process that isn't the owner or creator | NOT TESTABLE here | needs a second user |
| MC12 | EPERM when raising msg_qbytes without privilege | TESTED | msgctl.c:253 |

### msgsnd

| # | Requirement | Status | Reference |
|---|---|---|---|
| MS1 | Sends msgsz bytes, from 0 up to the system maximum | TESTED | msgsnd.c:284-294, 302-315 |
| MS2 | mtype must be positive | TESTED | msgsnd.c:296-300 |
| MS3 | A full queue with IPC_NOWAIT: nothing sent, return at once | TESTED | msgsnd.c:321-327 |
| MS4 | A full queue without IPC_NOWAIT: wait until there's room, then send | TESTED | msgsnd.c:329-337 |
| MS5 | The system-wide message limit | NOT TESTABLE | Linux has no system-wide message count |
| MS6 | EIDRM when the queue is removed during the wait | TESTED | msgsnd.c:348-355 |
| MS7 | A caught signal ends the wait without sending (EINTR) | TESTED | msgsnd.c:339-346 |
| MS8 | Success: msg_qnum + 1, msg_lspid, msg_stime | TESTED | msgsnd.c:288-291, 336-337 |
| MS9 | Returns 0; on error nothing is sent | TESTED | msgsnd.c:318-319, 326-327, 346 |
| MS10 | EACCES without write permission | TESTED | msgsnd.c:357-361 |
| MS11 | EINVAL for a bad identifier, mtype < 1 or msgsz over the limit | TESTED | msgsnd.c:296-317 |
| MS12 | msgsnd is a cancellation point | TESTED (existing) | test-pthread/test-pthread_testcancel.c:202-205, 621 |

### msgrcv

| # | Requirement | Status | Reference |
|---|---|---|---|
| MR1 | Places the message's type and text in the buffer | TESTED | msgrcv.c:269-270 (existing in part) |
| MR2 | MSG_NOERROR truncates to msgsz and the rest is lost | TESTED | msgrcv.c:300-305 (existing in part) |
| MR3 | msgsz over SSIZE_MAX is implementation-defined | NOT TESTABLE | implementation-defined (Linux: EINVAL) |
| MR4 | msgtyp 0 is the first message, > 0 the first of that type, < 0 the first of the lowest type up to its magnitude | TESTED | msgrcv.c:266-287 |
| MR5 | IPC_NOWAIT and no message: ENOMSG | TESTED | msgrcv.c:271, 279, 285 |
| MR6 | Otherwise it waits for a message of the type asked for | TESTED | msgrcv.c:322-330 (a message of another type comes first and is left) |
| MR7 | EIDRM when the queue is removed during the wait | TESTED | msgrcv.c:339-345 |
| MR8 | A caught signal ends the wait without receiving (EINTR) | TESTED | msgrcv.c:332-337 |
| MR9 | Success: msg_qnum - 1, msg_lrpid, msg_rtime | TESTED | msgrcv.c:307-320 |
| MR10 | Returns the number of bytes placed in mtext | TESTED | msgrcv.c:289-291, 301 |
| MR11 | E2BIG when the text is longer than msgsz without MSG_NOERROR, and nothing received | TESTED | msgrcv.c:294-299 |
| MR12 | EACCES without read permission | TESTED | msgrcv.c:351-356 |
| MR13 | EINVAL for a bad identifier | TESTED | msgrcv.c:346-349 |
| MR14 | msgrcv is a cancellation point | TESTED (existing) | test-pthread/test-pthread_testcancel.c:196-199, 620 |

### semget

| # | Requirement | Status | Reference |
|---|---|---|---|
| SG1 | Returns the identifier for an existing key; nsems 0 or smaller than the set's is accepted | TESTED | semget.c:158-160 |
| SG2 | IPC_PRIVATE always creates | TESTED | semget.c:126-130 |
| SG3 | IPC_CREAT creates for a key with no set | TESTED | semget.c:156-157 |
| SG4 | cuid, uid, cgid, gid and the low 9 bits of mode | TESTED | semget.c:167-169 |
| SG5 | sem_nsems is nsems | TESTED | semget.c:170 |
| SG6 | sem_otime is 0, sem_ctime the current time | TESTED | semget.c:171-173 |
| SG7 | semval, sempid, semncnt and semzcnt start at zero (Issue 8) | TESTED | semget.c:175-185 |
| SG8 | Returns a non-negative identifier, or -1 and errno | TESTED | throughout |
| SG9 | EACCES when the set doesn't grant what semflg asks | TESTED | semget.c:188-193 |
| SG10 | EEXIST with IPC_CREAT and IPC_EXCL | TESTED | semget.c:162 |
| SG11 | EINVAL for nsems <= 0 or over the limit when creating, or more than the existing set has | TESTED | semget.c:133-136, 161 |
| SG12 | ENOENT without IPC_CREAT | TESTED | semget.c:153 |
| SG13 | ENOSPC at the system-wide limit | NOT TESTABLE | host-wide limit |

### semctl

| # | Requirement | Status | Reference |
|---|---|---|---|
| SC1 | The fourth argument is the application's union semun, and only for the commands that use it | TESTED | every semctl call; GETVAL, GETPID, GETNCNT, GETZCNT and IPC_RMID pass none |
| SC2 | GETVAL returns semval; needs read permission | TESTED | semctl.c:215, 396 (existing in part) |
| SC3 | SETVAL sets semval, clears semadj in every process, sets sem_ctime; needs alter permission | TESTED | semctl.c:212-219, 291-306, 406 (existing in part) |
| SC4 | GETPID returns sempid | TESTED | semctl.c:238-248, 397 (existing in part) |
| SC5 | GETNCNT returns semncnt | TESTED | semctl.c:263-289, 398 |
| SC6 | GETZCNT returns semzcnt | TESTED | semctl.c:263-289, 399 |
| SC7 | GETALL fills arg.array | TESTED | semctl.c:231, 401 (existing in part) |
| SC8 | SETALL sets every semval, clears semadj, sets sem_ctime; needs alter permission | TESTED | semctl.c:221-235, 308-317, 408 (existing in part) |
| SC9 | IPC_STAT copies every member of semid_ds; needs read permission | TESTED | semctl.c:250-259, 395 |
| SC10 | IPC_SET sets sem_perm.uid, gid and the mode bits, and sem_ctime | TESTED | semctl.c:346-387 |
| SC11 | IPC_SET and IPC_RMID by the owner or creator (or privileged) | TESTED for the creator | semctl.c:369-387 |
| SC12 | IPC_RMID removes the set | TESTED | semctl.c:413-418 (existing in part) |
| SC13 | Returns semval, sempid, semncnt or semzcnt, otherwise 0 | TESTED | semctl.c:214-215, 228, 247, 281-282, 414 |
| SC14 | EACCES | TESTED | semctl.c:390-408 |
| SC15 | EINVAL for a bad identifier, semnum < 0 or >= sem_nsems, or a bad command | TESTED | semctl.c:334-344 |
| SC16 | EPERM for IPC_SET or IPC_RMID by someone else | NOT TESTABLE here | needs a second user |
| SC17 | ERANGE for SETVAL or SETALL past the system maximum | TESTED | semctl.c:319-332 |
| SC18 | Each operation is atomic | NOT TESTABLE | inferred: the kernel does each command under the set's lock |

### semop

| # | Requirement | Status | Reference |
|---|---|---|---|
| SO1 | The array is done atomically, in array order | TESTED | semop.c:190-196; semop-block.c:236-249 |
| SO2 | The array isn't modified | TESTED | semop.c:198-207 |
| SO3 | A failing or blocking element undoes the earlier ones; the blocked count stays | TESTED | semop.c:198-206; semop-block.c:240-245 |
| SO4 | Negative sem_op subtracts when semval is large enough; SEM_UNDO adds to semadj | TESTED | semop.c:175-177; semop-block.c:270-295 |
| SO5 | ... otherwise EAGAIN with IPC_NOWAIT | TESTED | semop.c:185-186 |
| SO6 | ... otherwise waits, counted in semncnt, until semval changes, the set goes (EIDRM) or a signal comes (semncnt decremented) | TESTED | semop-block.c:210-222, 251-259, 297-310 (existing in part) |
| SO7 | Positive sem_op adds; SEM_UNDO subtracts from semadj | TESTED | semop.c:178-179; semop-block.c:270-295 |
| SO8 | Zero sem_op succeeds at 0, else EAGAIN with IPC_NOWAIT, else waits in semzcnt (with EIDRM and signals as above) | TESTED | semop.c:180-188; semop-block.c:224-234, 261-268, 303-310 |
| SO9 | Success sets sempid for each semaphore in the array, and sem_otime | TESTED | semop.c:170-172, 209-233 |
| SO10 | E2BIG when nsops is over the system maximum | TESTED | semop.c:235-255. **A count past 32 bits wrapped on 64-bit; fixed** |
| SO11 | EACCES | TESTED | semop.c:279-296 |
| SO12 | EAGAIN | TESTED | semop.c:185-188, 205 |
| SO13 | EFBIG for sem_num >= the number of semaphores | TESTED | semop.c:236-237 |
| SO14 | EIDRM | TESTED | semop-block.c:297-310 |
| SO15 | EINTR | TESTED | semop-block.c:251-268 |
| SO16 | EINVAL for a bad identifier | TESTED | semop.c:275-277; the SEM_UNDO-limit half is NOT TESTABLE (Linux has no limit) |
| SO17 | ENOSPC for too many processes using SEM_UNDO | NOT TESTABLE | Linux has no such limit |
| SO18 | ERANGE when semval or semadj would overflow | TESTED | semop.c:257-273 |

### shmget

| # | Requirement | Status | Reference |
|---|---|---|---|
| HG1 | Returns the identifier for a key; IPC_PRIVATE and IPC_CREAT create | TESTED | shmget.c:135-140, 168-173 (existing in part) |
| HG2 | The segment holds at least size bytes, all zero | TESTED | shmget.c:189-194 |
| HG3 | cuid, uid, cgid, gid and the low 9 bits of mode | TESTED | shmget.c:179-181 |
| HG4 | shm_segsz is size | TESTED | shmget.c:182 |
| HG5 | shm_lpid, shm_nattch, shm_atime, shm_dtime are 0 | TESTED | shmget.c:183-184 |
| HG6 | shm_ctime is the current time | TESTED | shmget.c:187 |
| HG7 | EACCES when the segment doesn't grant what shmflg asks | TESTED | shmget.c:196-201 |
| HG8 | EEXIST with IPC_CREAT and IPC_EXCL | TESTED | shmget.c:175 |
| HG9 | EINVAL for a size below the minimum or over the maximum when creating | TESTED for size 0; past shmmax on aarch64 only | shmget.c:142-149 (armhf's size_t can't exceed shmmax) |
| HG10 | EINVAL when the existing segment is smaller than size | TESTED | shmget.c:174 |
| HG11 | ENOENT without IPC_CREAT | TESTED | shmget.c:166 |
| HG12 | ENOMEM, ENOSPC | NOT TESTABLE | host memory and host-wide limits |

### shmctl

| # | Requirement | Status | Reference |
|---|---|---|---|
| HC1 | IPC_STAT copies every member of shmid_ds | TESTED (each member holds a value only it can have) | shmctl.c:182-195 |
| HC2 | IPC_SET sets shm_perm.uid, gid and the low nine mode bits, and nothing else | TESTED | shmctl.c:197-235 |
| HC3 | IPC_SET sets shm_ctime | TESTED | shmctl.c:219 |
| HC4 | IPC_SET and IPC_RMID by the owner or creator (or privileged) | TESTED for the creator | shmctl.c:221-236 |
| HC5 | IPC_RMID with attachments: destroyed after the last detach | TESTED | shmctl.c:254-260 |
| HC6 | IPC_RMID without attachments: destroyed at once | TESTED | shmctl.c:262-268 (existing in part) |
| HC7 | IPC_RMID removes the identifier at once | **Deviation (kernel), not fixed** | Linux keeps a removed but attached id usable for shmat and shmctl until the last detach (glibc probe) |
| HC8 | Returns 0, or -1 and errno | TESTED | throughout |
| HC9 | EACCES for IPC_STAT without read permission | TESTED | shmctl.c:239-245 |
| HC10 | EINVAL for a bad identifier or command | TESTED | shmctl.c:250-252 |
| HC11 | EPERM for IPC_SET or IPC_RMID by someone else | NOT TESTABLE here | needs a second user |
| HC12 | May fail: EOVERFLOW for a uid or gid too large | NOT TESTABLE | uid_t and gid_t are 32-bit, like the kernel's |

### shmat

| # | Requirement | Status | Reference |
|---|---|---|---|
| HA1 | A null address: the system chooses | TESTED (existing) | shmat.c:158-161 |
| HA2 | SHM_RND: attached at shmaddr - shmaddr % SHMLBA | TESTED | shmat.c:220-223 |
| HA3 | No SHM_RND: attached at shmaddr | TESTED | shmat.c:204-218 |
| HA4 | SHM_RDONLY attaches read-only | TESTED | shmat.c:190-203 (a write faults) |
| HA5 | Otherwise read-write, shared with other attachments and processes | TESTED | shmat.c:169-188 (existing in part) |
| HA6 | Success: shm_nattch + 1, shm_atime, returns the start address | TESTED | shmat.c:162-167, 185 (existing in part) |
| HA7 | Failure returns SHM_FAILED and attaches nothing | TESTED | shmat.c:225-231 |
| HA8 | EACCES | TESTED | shmat.c:254-270 |
| HA9 | EINVAL for a bad identifier or an illegal address | TESTED | shmat.c:225-230 |
| HA10 | EMFILE past the per-process limit | NOT TESTABLE | Linux doesn't enforce SHMSEG |
| HA11 | ENOMEM when the address space has no room | TESTED | shmat.c:233-252 (a child under RLIMIT_AS) |

### shmdt

| # | Requirement | Status | Reference |
|---|---|---|---|
| HD1 | Detaches the segment at shmaddr | TESTED | shmdt.c:137-159 (the memory faults afterwards) (existing in part) |
| HD2 | Success: shm_nattch - 1, shm_dtime, returns 0 | TESTED | shmdt.c:130-144 |
| HD3 | EINVAL for an address that isn't a segment's start, detaching nothing | TESTED | shmdt.c:117-128, 147 |

### Counts

182 requirements:
- TESTED: 164, in full or in part (H1-H25, C1, C2, C4, F1-F7, F9-F12, MG1-MG12, MC1-MC10,
  MC12, MS1-MS4, MS6-MS12, MR1, MR2, MR4-MR14, SG1-SG12, SC1-SC15, SC17, SO1-SO16, SO18,
  HG1-HG11, HC1-HC6, HC8-HC10, HA1-HA9, HA11, HD1-HD3). Three are picolibc only in whole
  or part (H19 and H20, which glibc lacks, and the sem_nsems type in H12), and one check
  in SO10 (the 32-bit wrap) is picolibc only because glibc truncates the count.
- UNTESTED: 2 (H26, checked by hand with a macro dump; F13).
- NOT TESTABLE: 15 rows (H27, C3, F8, MG13, MC11, MS5, MR3, SG13, SC16, SC18, SO17, HG12,
  HC11, HC12, HA10), plus the parts of C2, MC4, SC11, HC4, SO16 and HG9 that need a
  second user, root or a wider size_t.
- Deviations in the kernel, recorded and not fixed: 2 (MC5, which is also TESTED for the
  part Linux gets right, and HC7).

Before this audit about 40 rows were tested, by the four existing tests (test-ftok,
test-msgrcv, test-semop, test-shmat) and test-pthread_testcancel's msgsnd and msgrcv
probes. None checked the headers, IPC_SET beyond the mode, the structure members one by
one, blocking with EIDRM or EINTR, semncnt and semzcnt, SEM_UNDO, the address arguments
of shmat and shmdt, or the permission errors. Now 155 are tested. The audit added 13
test programs (sys_ipc_h, sys_msg_h, sys_sem_h, sys_shm_h, msgget, msgctl, msgsnd,
semget, semctl, semop-block, shmget, shmctl, shmdt) and extended 4 (ftok, msgrcv, semop,
shmat).

### The 32-bit ARM structures

The brief asked for every member to round-trip through IPC_STAT and IPC_SET on armhf.
On real AArch32 (compat syscalls on this arm64 kernel) test-msgctl, test-semctl and
test-shmctl check each member of msqid_ds, semid_ds and shmid_ds after setting up state
in which every member has a different value: two process IDs (parent and child), three
or four timestamps a second or more apart, counts unlike the sizes. They also fill the
structure with 0x55 bytes before each IPC_STAT. IPC_SET is checked by setting the
mode, msg_qbytes and a different uid and gid (the creator can set them back), and by
setting every other member to junk that must be ignored. All pass for picolibc and for
glibc on armhf, and on aarch64. So `LINUX_IPC_64` (0x100 on arm) and the
`__kernel_*64_ds` layouts in local-ipc.h match the kernel. For the 64-bit times: every
timestamp is checked to lie between time() taken before and after, which a value built
from the wrong word for the high half (a count, a size or 0x55555555) can't do. A
timestamp past 2038, with a non-zero high half, can't be made without setting the clock,
so the high half is only shown to be read from a word that the kernel sets to 0 now. The
code (`_ipc_time` in local-ipc.h) combines `name_high << 32 | name`, which matches the
kernel's `compat_*64_ds` (inferred by reading).

Under qemu-user (x86_64 host only) the msqid_ds fields come back misplaced, so the msg
tests skip their msqid_ds checks there; they detect qemu at run time with the
BogoMIPS test (`is_qemu()`). That replaces the meson rule that kept test-msgrcv-native
from being built at all on arm, so the native test now runs on real ARM hardware. The
semid_ds and shmid_ds checks still run under qemu, as test-semop and test-shmat already
did; if qemu gets those wrong too, the x86_64 host will see it.

## Deviations found and fixed

Each fix was verified on the targets named. With only the library change reversed
(`git show <fix> -- libc libos | git apply -R`, rebuild, run, reapply) the new check
fails; with the fix it passes. The failing check is quoted for each.

1. **SHM_FAILED was missing** (libc/include/sys/shm.h; H19, HA7). Issue 8, Austin Group
   Defect 1239: "The `<sys/shm.h>` header shall define the symbolic constant SHM_FAILED
   which shall evaluate to the same value as ((void *)(intptr_t)-1)", and shmat "shall
   return SHM_FAILED" on failure. It is now defined, `<sys/shm.h>` includes
   `<sys/_stdint.h>` for intptr_t explicitly (it already came through `<sys/types.h>`),
   and libos/linux/shmat.c returns it. Failing-before, aarch64 and armhf:
   test-sys_shm_h prints `no SHM_FAILED`.
2. **semop let a count past 32 bits wrap** (libos/linux/semop.c; SO10). The kernel's
   semop and semtimedop take nsops as `unsigned int` (ipc/sem.c, `SYSCALL_DEFINE3(semop,
   ..., unsigned, nsops)` in v5.10), so on 64-bit targets `nsops = 2^32 + 1` reached the
   kernel as 1 and semop did one operation. POSIX: "[E2BIG] The value of nsops is greater
   than the system-imposed maximum." semop now fails with E2BIG for counts over UINT_MAX,
   after entering the cancellation point (as the net-sockets lesson asks); the check is
   compiled only where size_t is wider than int. glibc 2.43 has the same wrap (probe:
   the call returns 0 and adds 1), so the check is skipped for glibc. Failing-before,
   aarch64: `E2BIG past 32 bits: semop(id, ops, ((size_t)1 << 32) | 1) == -1 && errno == E2BIG`.
   armhf can't express such a count.

## Deviations not fixed

- **An unprivileged owner can raise msg_qbytes** (MC5). POSIX: "Only a process with
  appropriate privileges can raise the value of msg_qbytes", with EPERM. Linux checks
  privilege only past msgmnb (ipc/msg.c `msgctl_down`: `msg_qbytes > ns->msg_ctlmnb &&
  !capable(CAP_SYS_RESOURCE)`), so lowering msg_qbytes to 100 and raising it to 200
  succeeds (glibc probe). The library could compare with an IPC_STAT first, but that
  races with other processes and would refuse something the kernel allows, so it is
  left as kernel behaviour. The test checks the part Linux gets right (past msgmnb).
- **A removed shared memory id stays usable while attached** (HC7). POSIX: IPC_RMID
  "Remove[s] the shared memory identifier specified by shmid from the system"; only the
  segment and its shmid_ds outlive it. Linux marks the segment SHM_DEST (mode 01000 in
  IPC_STAT), takes it off the key, but still accepts the id in shmat and shmctl until the
  last detach (glibc probe: shmat after IPC_RMID attaches again). picolibc could refuse
  ids whose IPC_STAT shows SHM_DEST, at the cost of an extra system call in every shmat
  and a race; left as kernel behaviour. The test checks that the memory stays and that
  the id is gone after the last detach.
- **SHMLBA is fixed at compile time** (H18). It is 4096 on aarch64 and 16384 on 32-bit
  ARM. Linux uses PAGE_SIZE for arm64 tasks and 4 * PAGE_SIZE for compat tasks, so on an
  arm64 kernel with 16 KiB or 64 KiB pages, shmat(SHM_RND) would round to a multiple the
  application doesn't know, and the attach address could differ from
  `addr - addr % SHMLBA`. Not visible on this 4 KiB-page host (test-shmat's SHM_RND check
  passes). glibc defines SHMLBA as `__getpagesize()` on aarch64. A fix needs a
  page-size function with a reserved name that the header can call; left for a decision
  (also in cross-cutting.md). x86_64 and i686 always have 4 KiB pages, so they are
  right.

## Other observations (not changed)

- Linux behaviours the tests accept, all allowed by POSIX: identifiers can be 0
  ("non-negative" in the get functions); SETVAL and SETALL also set sempid; fork
  counts as an attach for each inherited segment (shm_nattch, shm_atime and shm_lpid
  change); the mode from IPC_STAT can include SHM_DEST and SHM_LOCKED above the
  permission bits; semctl ignores semnum for the commands that act on the whole set;
  an address that is page aligned but not SHMLBA aligned is accepted without SHM_RND;
  msgsz > SSIZE_MAX gives EINVAL (implementation-defined); semop with nsops 0 gives
  EINVAL (unspecified).
- semop is a cancellation point in picolibc (`_cancel_enter`). It is on POSIX's "may"
  list, so that is allowed. msgsnd and msgrcv must be cancellation points and are
  (test-pthread_testcancel).
- The ctl wrappers reject commands other than IPC_STAT, IPC_SET, IPC_RMID (and the
  semctl GET/SET commands) with EINVAL before calling the kernel, so Linux extensions
  such as IPC_INFO, MSG_STAT and SHM_LOCK aren't reachable. POSIX requires only the
  listed commands, so that is conforming (inferred by reading).
- ftok matches glibc's formula (`id << 24 | (st_dev & 0xff) << 16 | (st_ino & 0xffff)`).
  picolibc's st_dev is `major << 8 | minor` (the files unit's cross-cutting note), and
  its low byte is the minor's low byte, as in the kernel's encoding, so picolibc and
  glibc programs get the same keys (inferred by reading).
- glibc 2.43 differences met along the way, all guarded with `__GLIBC__`: no SHM_FAILED
  or intptr_t in `<sys/shm.h>`; sem_nsems is `unsigned long`; semop wraps the count (see
  fix 2).
- The kernel's IPC timestamps come from `ktime_get_real_seconds()`, which can lag
  CLOCK_REALTIME by up to a tick, so the tests allow a second of slack against a
  `time()` taken just before, and wait two seconds when two timestamps must differ.
  This makes test-msgctl, test-semctl and test-shmctl take 3-6 s each.

## Left undone

- H26 (namespace) has no suite test; the only leak found comes from `<sys/types.h>`
  (cross-cutting). F13 (the "may fail" path-length errors).
- The checks that need a second user or root: EPERM for IPC_SET and IPC_RMID (MC11,
  SC16, HC11), the group and other permission bits (C2), and privileged overrides (C3).
  A root run could cover them with `setuid` in a child; the tests skip EACCES when euid
  is 0 rather than doing that.
- The kernel deviations (MC5, HC7) and SHMLBA need decisions (see above).

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`), full
  `meson test -t 4` at the branch tip: 1382 tests; 1334 Ok, 0 Fail, 6 Expected Fail,
  42 Skipped.
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`), full
  `meson test -t 4` at the branch tip: 1382 tests; 1332 Ok, 0 Fail, 6 Expected Fail,
  44 Skipped.
- In both runs every new and changed test passes, for picolibc and as `-native`
  against glibc, with nothing skipped. test-msgrcv-native is now built and runs on arm.
- The native configuration (`scripts/do-native-configure -Dwerror=true`, `ninja` only)
  builds cleanly.
- CMake (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 557 tests, 100%
  passed (253 ran, 304 skipped). The four header tests run there; the others skip, as
  intended, for lack of OS support.
- Bare-metal meson targets weren't run here; the x86_64 host does them.
- `clang-format --dry-run --Werror` is clean on every C file and header the branch
  touches.
- After every run, `/proc/sysvipc/msg`, `sem` and `shm` held no objects at all, and
  no `/tmp/{ftok,msgget,semget,shmget}-*` directories were left.
- Incident: in an earlier full aarch64 run, test-accept4-native (net-sockets, not this
  unit) was "killed by signal 9 SIGKILL" with no output, the host security software
  pattern. It passed 3 of 3 runs on its own and in the final full run. Nothing changed.
- The kernel deviations (MC5, HC7) and glibc's semop wrap were confirmed with a small
  glibc probe in /tmp/sysv-ipc-probe, not with suite tests.
- Not run as root.

## Commits (audit/sysv-ipc, on top of d40e4ae0e4)

- d0c5e220fa sys/shm: Define SHM_FAILED
- 198fbc4823 libos/linux: Return E2BIG from semop for counts past 32 bits
- a8d47fc647 test-posix: Check the System V IPC headers
- 255c8e361b test-posix: Check ftok's keys and errors
- 7ef8096100 test-posix: Check msgget, msgctl, msgsnd and msgrcv
- 5bef92cde2 test-posix: Check semget, semctl and semop
- 7a81c40f8d test-posix: Check shmget, shmctl, shmat and shmdt

Nothing pushed. No AI attribution in any commit.
