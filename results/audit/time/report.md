# `<time.h>`, `<sys/time.h>`, `<sys/times.h>` audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-time`, branch `audit/time`, based on 4af0c34b98
(`claude/posix-spec-audit`). Host: aarch64 Linux (AWS Graviton2, kernel 5.10). Builds and
tests ran in the Debian sid chroot (gcc 16, glibc 2.43).

Implementation: `libc/time/` (asctime, ctime, difftime, gmtime_r, lcltime_r, mktime,
tzset, tzcalc_limits, strftime, strptime_l, getdate_r, timespec_get, time, clock),
`libos/linux/` (clock_*, nanosleep, timer_*, _timer_thread, times, utimes, sysconf and the
new clock.c), and the headers `libc/include/time.h`, `sys/time.h`, `sys/times.h`.
Tests: `test/test-posix/test-time/`, `test-sys-time/` and the new `test-sys-times/`.
Paths below are relative to `test/test-posix/`. Line numbers are for the branch tip.

Spec source: pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/time.h, sys_time.h,
sys_times.h, V1_chap08 (8.3 TZ) and functions/asctime, clock, clock_getcpuclockid,
clock_getres (covers gettime/settime), clock_nanosleep, ctime, difftime, getdate, gmtime,
localtime, mktime, nanosleep, strftime (covers strftime_l), strptime, time, timer_create,
timer_delete, timer_getoverrun (covers gettime/settime), timespec_get, tzset (covers
daylight/timezone/tzname), times and utimes.

**Scope.** These are no longer in Issue 8 and were skipped: asctime_r and ctime_r
(Austin Group defect 1410), gettimeofday, getitimer/setitimer and struct itimerval
(the pages are gone), and timegm (C23, not in Issue 8). `select` belongs to net-sockets;
only its visibility from `<sys/time.h>` is checked here. `utimes` is in `<sys/time.h>`
and is covered here; futimens and utimensat, on the same page, belong to files.
pthread_getcpuclockid's own errors belong to pthread-threads; using its clock id is
checked here.

Status key: TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row was
run and passes for picolibc on aarch64-linux-gnu and arm-linux-gnueabihf, and against
glibc on both (`-native`), unless the row says picolibc-only.

## Requirement table

### Header `<time.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | clock_t, size_t, time_t, clockid_t, timer_t, locale_t, pid_t; struct sigevent declared | TESTED | test-time/test-time_h.c:46-55 |
| H2 | struct tm members tm_sec..tm_isdst, all int | TESTED | test-time_h.c:59-67 |
| H3 | tm_gmtoff (long) and tm_zone (const char *) | TESTED | test-time_h.c:79-82. **Was a deviation (missing); fixed** |
| H4 | struct timespec (tv_sec time_t, tv_nsec long), struct itimerspec | TESTED | test-time_h.c:84-87 |
| H5 | NULL; CLOCKS_PER_SEC is a clock_t expression, one million (XSI) | TESTED | test-time_h.c:90, 197. **Was a deviation (int); fixed** |
| H6 | TIME_UTC positive and usable in #if | TESTED | test-time_h.c:91-93 |
| H7 | CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID, CLOCK_REALTIME, CLOCK_THREAD_CPUTIME_ID assignable to clockid_t and distinct; TIMER_ABSTIME | TESTED | test-time_h.c:94-96, 200-203 |
| H8 | getdate_err is an int expression | TESTED | test-time_h.c:207 |
| H9 | daylight, timezone, tzname declared | TESTED | test-time_h.c:99-101, 205 |
| H10 | Prototypes for the 27 functions | TESTED | test-time_h.c:104-145 (the OS ones only where `_POSIX_TIMERS` is set) |
| H11 | tm_isdst never set negative by any interface | TESTED | test-localtime.c:197-199, test-mktime.c:93, test-strptime.c:76. **Was a deviation (localtime before 1970, mktime, strptime); fixed** |

### Header `<sys/time.h>` and `<sys/times.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| ST1 | fd_set, struct timeval (time_t tv_sec, suseconds_t tv_usec), time_t, suseconds_t | TESTED | test-sys-time/test-sys_time_h.c:50-62 (the tv_usec type is picolibc-only: 32-bit glibc with 64-bit time uses a wider type) |
| ST2 | FD_CLR, FD_ISSET, FD_SET, FD_ZERO, FD_SETSIZE | TESTED | test-sys_time_h.c:63, 83-89 |
| ST3 | select and utimes prototypes | TESTED | test-sys_time_h.c:69-73 |
| SX1 | struct tms with four clock_t members; clock_t; times prototype | TESTED | test-sys-times/test-times.c:40-43, 109 |

### clock, time, difftime, timespec_get

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | clock() is the process's processor time, in CLOCKS_PER_SEC units | TESTED | test-time/test-clock_gettime.c:189-197. **Was a deviation (Linux 100 Hz ticks, 10000x too small); fixed** |
| C2 | ... of this process only (not waited-for children) | TESTED | test-clock_gettime.c:206-219. **Was a deviation; fixed** (same fix) |
| C3 | (clock_t)-1 / EOVERFLOW when it can't be represented | NOT TESTABLE | 64-bit clock_t of microseconds doesn't overflow |
| T1 | time() returns seconds since the Epoch and stores through tloc | TESTED | test-time/test-time.c:71-84 (existing) |
| T2 | EOVERFLOW (may fail) | NOT TESTABLE | 64-bit time_t |
| D1 | difftime returns time1 - time0 as double | TESTED | test-localtime.c:259 |
| TS1 | timespec_get(TIME_UTC) is clock_gettime(CLOCK_REALTIME) and returns base | TESTED | test-clock_gettime.c:174-177 |
| TS2 | Returns 0 for an unsupported base | TESTED | test-clock_gettime.c:178 |

### clock_getres, clock_gettime, clock_settime, CPU-time clocks

| # | Requirement | Status | Reference |
|---|---|---|---|
| K1 | CLOCK_REALTIME is seconds since the Epoch | TESTED | test-clock_gettime.c:123-124 |
| K2 | CLOCK_MONOTONIC never goes backwards | TESTED | test-clock_gettime.c:127-132 |
| K3 | clock_getres for every clock; a null res is allowed | TESTED | test-clock_gettime.c:136-144 |
| K4 | EINVAL for an unknown clock (getres, gettime, settime) | TESTED | test-clock_gettime.c:147-152 |
| K5 | clock_settime: EINVAL for CLOCK_MONOTONIC and for tv_nsec outside [0, 1e9) | TESTED | test-clock_gettime.c:155-165 |
| K6 | clock_settime EPERM without privilege (may fail) | TESTED | test-clock_gettime.c:166-171 (skipped as root) |
| K7 | Setting CLOCK_REALTIME moves absolute timers and sleeps but not relative ones | NOT TESTABLE | needs CAP_SYS_TIME; the kernel implements it |
| K8 | clock_settime truncates to the resolution | NOT TESTABLE | needs CAP_SYS_TIME |
| K9 | EOVERFLOW from clock_gettime | NOT TESTABLE | 64-bit time_t everywhere |
| K10 | CLOCK_PROCESS_CPUTIME_ID and CLOCK_THREAD_CPUTIME_ID measure execution time | TESTED | test-clock_gettime.c:183-188, 199-203 |
| K11 | clock_getcpuclockid(0) and of another process; ESRCH (may fail) | TESTED | test-time/test-clock_getcpuclockid.c:89-104 (existing) |
| K12 | clock_getcpuclockid EPERM | NOT TESTABLE | Linux lets any process read another's CPU clock (implementation-defined) |
| K13 | pthread_getcpuclockid ids work with clock_gettime, for another thread and for self | TESTED | test-clock_gettime.c:223-233 |
| K14 | Sporadic-server interaction | NOT TESTABLE | no SCHED_SPORADIC; no RT privilege on this host |

### nanosleep and clock_nanosleep

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | nanosleep suspends at least rqtp, measured on CLOCK_REALTIME; returns 0 | TESTED | test-clock_gettime.c:240-245 |
| N2 | EINTR with the time remaining; rqtp and rmtp may be the same object | TESTED | test-clock_gettime.c:255-265 |
| N3 | EINVAL for tv_nsec < 0 or >= 1e9 | TESTED | test-clock_gettime.c:247-253 |
| N4 | No effect on signal actions or the mask | TESTED | test-clock_gettime.c:268-279 |
| CN1 | clock_nanosleep relative and absolute, errors returned not in errno, rmtp on EINTR | TESTED | test-time/test-clock_nanosleep.c:79-100 (existing) |
| CN2 | EINVAL for the calling thread's CPU-time clock | TESTED | test-clock_gettime.c:234-238. **Was a deviation (EOPNOTSUPP); fixed** |
| CN3 | An absolute time already passed returns at once | UNTESTED | inferred: the kernel does it; the timer analogue is TESTED (TM6) |

### timer_create, timer_delete, timer_settime, timer_gettime, timer_getoverrun

| # | Requirement | Status | Reference |
|---|---|---|---|
| TM1 | timer_create on CLOCK_REALTIME and CLOCK_MONOTONIC; unique ids | TESTED | test-timer_create.c:104, 134; test-time/test-timer_settime.c:120-122 |
| TM2 | A new timer is disarmed | TESTED | test-timer_settime.c:124-129 |
| TM3 | A null evp gives SIGEV_SIGNAL, a default signal and the timer id as value | TESTED | test-timer_settime.c:226-236 (the value check is picolibc-only: glibc's timer_t is a pointer) |
| TM4 | SIGEV_SIGNAL value and SI_TIMER; SIGEV_NONE; EINVAL for a bad signal | TESTED | test-timer_create.c:104-111, 160-169 (existing) |
| TM5 | timer_gettime: time left (relative even for an absolute timer) and reload | TESTED | test-timer_settime.c:131-140 |
| TM6 | Absolute time already passed: success and immediate notification | TESTED | test-timer_settime.c:170-174 |
| TM7 | Resetting an armed timer; ovalue gets the time left and reload | TESTED | test-timer_settime.c:142-149 |
| TM8 | A zero it_value disarms; ovalue on disarm | TESTED | test-timer_settime.c:151-156 |
| TM9 | EINVAL for a bad tv_nsec in it_value or it_interval | TESTED | test-timer_settime.c:158-167 |
| TM10 | Timers don't expire early | TESTED | test-timer_settime.c:176-182 |
| TM11 | One signal queued; extra expirations counted by timer_getoverrun | TESTED | test-timer_settime.c:184-195 |
| TM12 | The overrun count stops at {DELAYTIMER_MAX} | TESTED | test-timer_settime.c:196-199. **Was a deviation (sysconf said 32, the kernel counts to INT_MAX); fixed** |
| TM13 | Deleting an armed timer disarms it | TESTED | test-timer_settime.c:211-222 |
| TM14 | CPU-time clock timers | TESTED | test-timer_settime.c:238-249 |
| TM15 | EINVAL for an unknown clock | TESTED | test-timer_settime.c:252-253 |
| TM16 | EAGAIN when out of timers or signal resources | UNTESTED | needs exhausting RLIMIT_SIGPENDING |
| TM17 | Not inherited across fork; deleted by exec | UNTESTED | timer ids aren't valid in the child, so checking is undefined behaviour; kernel behaviour |
| TM18 | ENOTSUP for another process's CPU-time clock | NOT TESTABLE | implementation-defined; Linux allows it |
| TM19 | SIGEV_THREAD timers | UNTESTED | not in this unit's tests (pthread-threads covers the helper threads); the may-fail EINVAL for a fixed stack is not asserted |

### gmtime, localtime, tzset and TZ (XBD 8.3)

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | gmtime/gmtime_r follow XBD 4.19, including before 1970 | TESTED | test-time/test-localtime.c:124-136 |
| G2 | gmtime_r returns result and sets tm_zone to "UTC" (static) | TESTED | test-localtime.c:133, 137-140. **Was a deviation; fixed** (H3) |
| G3 | EOVERFLOW from gmtime, gmtime_r, localtime and localtime_r | TESTED | test-localtime.c:143-154 (64-bit time_t only). **Was a deviation (errno not set); fixed** |
| L1 | localtime corrects for the zone and DST; calls tzset (sets tzname) | TESTED | test-localtime.c:175-187 |
| L2 | localtime_r returns result; tm_gmtoff and tm_zone set | TESTED | test-localtime.c:79, 188-193 |
| L3 | localtime_r sets tzname only together with daylight and timezone | TESTED | inferred plus test-localtime.c:187: picolibc's localtime_r calls tzset, which sets all three |
| Z1 | tzset sets tzname[0], tzname[1], timezone (UTC - local std) and daylight | TESTED | test-localtime.c:157-178 |
| Z2 | Offsets hh[:mm[:ss]], optional +, '-' meaning east | TESTED | test-localtime.c:157-166 |
| Z3 | Quoted <...> names with digits and signs | TESTED | test-localtime.c:169-173 |
| Z4 | Mm.n.d rules, week 5 meaning the last such day, default time 02:00 | TESTED | test-localtime.c:176-183, 201-208 |
| Z5 | Rule times with a sign and up to 167 hours | TESTED | test-localtime.c:201-208 (/-1 and /26) |
| Z6 | DST one hour ahead when dst has no offset; an explicit dst offset | TESTED | test-localtime.c:180-183, 211-212 |
| Z7 | Jn ignores February 29; n counts it | TESTED | test-localtime.c:214-222 |
| Z8 | A second date earlier than the first: the year starts and ends in DST | TESTED | test-localtime.c:224-228 |
| Z9 | XBD 8.3's all-year-DST example EST5EDT,0/0,J365/25 | TESTED (picolibc-only for the first hours of the year) | test-localtime.c:230-237. **Was a deviation (standard time for the first hours of each year); fixed**; glibc has the same bug |
| Z10 | The rule applies to every year (before 1970 too) | TESTED (picolibc-only) | test-localtime.c:195-200. **Was a deviation (tm_isdst -1); fixed**; glibc doesn't apply the rule before 1970 |
| Z11 | TZ beginning with ':' is implementation-defined | NOT TESTABLE | implementation-defined (picolibc parses the rest) |
| Z12 | Geographical (Area/Location) timezones from a database | UNTESTED | **Deviation, not fixed:** picolibc has no timezone database and falls back to GMT |
| Z13 | Default timezone when TZ is unset | NOT TESTABLE | implementation-defined (GMT) |

### mktime

| # | Requirement | Status | Reference |
|---|---|---|---|
| M1 | Uses only year, mon, mday, hour, min, sec, isdst; computes wday and yday | TESTED | test-time/test-mktime.c:105-107 |
| M2 | Out-of-range members are normalized (months, days, negative seconds, minutes) | TESTED | test-mktime.c:109-126 |
| M3 | tm_isdst 0: no DST adjustment | TESTED | test-mktime.c:145-150 |
| M4 | tm_isdst positive: DST adjustment | TESTED | test-mktime.c:152-156 |
| M5 | tm_isdst negative: mktime decides | TESTED | test-mktime.c:158-164 |
| M6 | Skipped or repeated hour: either reading, members as localtime gives | TESTED | test-mktime.c:166-174. **Was a deviation (tm_isdst left at -1); fixed** |
| M7 | Members are set as localtime would set them | TESTED | test-mktime.c:80-94 and its uses |
| M8 | Rules before 1970 | TESTED (picolibc-only) | test-mktime.c:176-182 (same fix as Z10) |
| M9 | (time_t)-1 is a valid result, told apart by tm_wday | TESTED | test-mktime.c:128-135. **Was a deviation (local 1969-12-31 23:59:59 taken as an overflow); fixed** |
| M10 | EOVERFLOW, -1 and tm_wday unchanged when tm_year can't be represented | TESTED | test-mktime.c:137-142 |
| M11 | Calls tzset (sees a TZ change; sets tzname and timezone); sets tm_gmtoff, tm_zone | TESTED | test-mktime.c:184-192 |
| M12 | Geographical-zone offset changes | NOT TESTABLE | no timezone database (Z12) |

### strftime, strftime_l

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | Every conversion specifier in the POSIX locale (%a..%%, %c, %x, %X, %r) | TESTED | test-time/test-strftime_l.c:126-138; also test/test-strftime.c (existing) |
| F2 | %g, %G, %V week-based year (the spec's 1999-01-02 and 1997-12-30 examples) | TESTED | test-strftime_l.c:129, 136 |
| F3 | %s seconds since the Epoch as mktime computes | TESTED | test-strftime_l.c:144 |
| F4 | %z from tm_isdst (no characters for negative), %Z | TESTED | test-strftime_l.c:141-152 (tm_gmtoff and tm_zone are set too, because glibc reads them) |
| F5 | E and O modifiers fall back in the POSIX locale | TESTED | test-strftime_l.c:155-158 |
| F6 | + flag: a sign when the field is wider than 4 bytes (%Y, %G, %F) or 2 (%C); the spec's table | TESTED (picolibc-only) | test-strftime_l.c:161-173. **Was a deviation; fixed**; glibc ignores + |
| F7 | %C is two digits for years 0..9999 | TESTED (picolibc-only) | test-strftime_l.c:175-176, test/test-strftime.c. **Was a deviation (one digit); fixed** |
| F8 | %F with width x formats the year with width x-6 and the flag | TESTED | test-strftime_l.c:172 (the + cases picolibc-only). **Was a deviation (width ignored); fixed** |
| F9 | Returns the byte count; errno unchanged on success | TESTED | test-strftime_l.c:104-113, 190-192 |
| F10 | ERANGE and 0 when the result and NUL don't fit; nothing more than maxsize written | TESTED (errno picolibc-only) | test-strftime_l.c:180-189. **Was a deviation (no errno; maxsize 0 wrote past the buffer); fixed** |
| F11 | strftime_l uses the given locale | TESTED | test-strftime_l.c:194-202 |
| F12 | Calls tzset | TESTED | test-strftime_l.c:204-208 |
| F13 | %s EINVAL / EOVERFLOW (may fail) | Not asserted | optional |
| F14 | Alternative (era, alt digits) forms in other locales | UNTESTED | picolibc ships no locale with ERA or alt_digits data |

### strptime

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | Returns a pointer past the last character parsed; NULL on failure | TESTED | test-time/test-strptime.c:86, 121, 187 |
| P2 | Fixed-width conversions need no separators; maximum widths ({4} for %Y, [x,y] for the rest) | TESTED | test-strptime.c:90-95. **Was a deviation (%Y%m%d failed); fixed** |
| P3 | Field widths and the ignored 0 and + flags | TESTED (picolibc-only) | test-strptime.c:97-107. **Was a deviation; fixed** |
| P4 | %a/%A and %b/%B/%h take full or abbreviated names, any case | TESTED | test-strptime.c:110-121. **Was a deviation (%A only full, %a only abbreviated); fixed** |
| P5 | %y: 69-99 are 1969-1999, 00-68 are 2000-2068 | TESTED | test-strptime.c:123-129. **Was a deviation (69 gave 2069); fixed** |
| P6 | %C combines with %y; %C alone gives year 00 | TESTED | test-strptime.c:130-135. **Was a deviation (last one wins); fixed** |
| P7 | Ranges and optional leading zeros for d, m, H, M, S, I, u, w, j | TESTED | test-strptime.c:138-157 (%M 60 and %S 61 picolibc-only) |
| P8 | %j sets tm_yday - 1; %u modulo 7 | TESTED | test-strptime.c:152-155 |
| P9 | %I with %p; %r | TESTED | test-strptime.c:160-165 |
| P10 | %D, %R, %T, %c, %x, %X | TESTED | test-strptime.c:168-179 |
| P11 | White space, %n, %t, ordinary characters, %% | TESTED | test-strptime.c:182-187 |
| P12 | %F with a year of any width, limited by a field width | TESTED (picolibc-only) | test-strptime.c:105-107. **Was a deviation; fixed** |
| P13 | %G, %g, %V, %U, %W, %z, %s parse | TESTED | test-strptime.c:190-194. **Was a deviation (%G, %g, %z not recognized); fixed** |
| P14 | %Z sets tm_isdst from tzname | TESTED (dst name picolibc-only) | test-strptime.c:197-202. **Was a deviation (consumed nothing); fixed** |
| P15 | E and O modifiers fall back | TESTED | test-strptime.c:205-206 |
| P16 | Alternative month names (%OB) | UNTESTED | no locale has them |

### getdate

| # | Requirement | Status | Reference |
|---|---|---|---|
| GD1 | Error 1: DATEMSK unset or null | TESTED | test-time/test-getdate_err.c:96-100 |
| GD2 | Error 2: the template file can't be opened | TESTED (picolibc-only) | test-getdate_err.c:102-107 (glibc reports 3) |
| GD3 | Error 3: file status | NOT TESTABLE | can't make stat fail on a file that opened |
| GD4 | Error 4: not a regular file | TESTED | test-getdate_err.c:109-114 (a directory). **Was a deviation; fixed.** FIFOs and devices aren't detected (no portable fstat; see below) |
| GD5 | Error 5: I/O error reading | UNTESTED | needs a failing read; the code maps ferror to 5 |
| GD6 | Error 6: memory | NOT TESTABLE | needs allocation failure |
| GD7 | Error 7: no template matches all of the input | TESTED | test-getdate_err.c:139-141. **Was a deviation (a prefix match counted); fixed** |
| GD8 | Error 8: invalid input (February 31, April 31) | TESTED | test-getdate_err.c:143-145. **Was a deviation; fixed** |
| GD9 | First matching template used; extra white space ignored; case-insensitive | TESTED | test-getdate_err.c:121-137, 148 |
| GD10 | No time given: the current time | TESTED | test-getdate_err.c:127-128. **Was a deviation (stale fields); fixed** |
| GD11 | Only a weekday: today or the next such day | TESTED | test-getdate_err.c:147-154 |
| GD12 | Only an hour: this hour or the next such hour | TESTED | test-getdate_err.c:156-167 |
| GD13 | Only a month: this month or the next, first day | TESTED (picolibc-only) | test-getdate_err.c:169-174 (glibc keeps the day) |
| GD14 | %Z initializes to the current time in that zone | UNTESTED | **Deviation, not fixed:** getdate doesn't use the %Z zone |

### asctime, ctime

| # | Requirement | Status | Reference |
|---|---|---|---|
| A1 | asctime's format | TESTED | test-localtime.c:240-251 |
| A2 | ctime is asctime(localtime(clock)) | TESTED | test-localtime.c:252-256 |
| A3 | Static objects shared between the four functions; no data races with other functions | NOT TESTABLE | inferred: thread-local buffers |

### times

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | Returns elapsed real time in clock ticks from a fixed point | TESTED | test-sys-times/test-times.c:111-127 |
| X2 | tms_utime + tms_stime count this process's CPU time | TESTED | test-times.c:128-132 |
| X3 | Children's times appear once waited for, not before; a child's unwaited children don't count | TESTED | test-times.c:133-168 |
| X4 | EOVERFLOW | NOT TESTABLE | 64-bit clock_t |

### utimes

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | Sets atime and mtime to the microsecond; a null times uses the current time | TESTED | test-sys-time/test-utimes.c:77-82 (existing) |
| U2 | ENOENT for a missing file and for an empty path | TESTED | test-utimes.c:83-84 |
| U3 | ENOTDIR for a trailing slash on a file | TESTED | test-utimes.c:85 |
| U4 | EINVAL for tv_usec out of range, with the times unchanged | TESTED | test-utimes.c:87-110 (the wrapping value 4294968 picolibc-only). **Was a deviation on 32-bit targets; fixed** |
| U5 | EACCES, EPERM, EROFS, ELOOP, ENAMETOOLONG | UNTESTED | path and permission errors come from utimensat (files unit), which utimes calls |

### Counts

150 requirements:
- TESTED: 124. Of these, 16 are wholly or partly picolibc-only, because glibc behaves
  differently (ST1, TM3, Z9, Z10, M8, F6, F7, F8, F10, P3, P7, P12, P14, GD2, GD13, U4).
- UNTESTED: 10 (CN3, TM16, TM17, TM19, Z12, F14, P16, GD5, GD14, U5; Z12 and GD14 are
  deviations left unfixed).
- NOT TESTABLE: 15 (C3, T2, K7, K8, K9, K12, K14, TM18, Z11, Z13, M12, GD3, GD6, A3, X4).
- Optional errors deliberately not asserted: 1 (F13).

Before this audit, roughly 25 of these rows were tested, by test-time.c,
test-clock_nanosleep, test-clock_getcpuclockid, test-timer_create, test-utimes,
test/test-strftime.c and test/test-getdate.c (an estimate from reading those tests).
Now 124 are.

## Deviations found and fixed

Each fix was verified by running: on aarch64-linux-gnu (utimes on arm-linux-gnueabihf,
where it applies) the new test fails with only the library change reverted, with the
message quoted, and passes with it.

1. **clock() was 10000 times too small on Linux** (libc/time/clock.c, new
   libos/linux/clock.c). The generic version summed the times() fields, which count
   100 Hz ticks, and returned them as CLOCKS_PER_SEC (10^6) units; it also added the
   CPU time of waited-for children. Now it reads CLOCK_PROCESS_CPUTIME_ID.
   Failing-before: `clock() measured 2e-05 s, CPU-time clock 0.200007 s`.
2. **gmtime_r and localtime_r didn't set errno to EOVERFLOW** (gmtime_r.c).
   Failing-before: `gmtime_r EOVERFLOW: gmtime_r(&t, &tm) == NULL && errno == EOVERFLOW`.
3. **struct tm had no tm_gmtoff or tm_zone** (time.h). Added at the end (named
   `__tm_gmtoff`/`__tm_zone` for strict ISO C, as glibc does); gmtime_r/timegm set 0 and
   "UTC", localtime_r/mktime set them from the rule in effect. Failing-before (with
   just the gmtime_r assignment reverted): `gmtime_r sets tm_zone to "UTC"`; with the
   header reverted the tests don't compile.
4. **The TZ rule was ignored before 1970 and at year ends** (tzcalc_limits.c,
   lcltime_r.c). `__tzcalc_limits` gave up before 1970, so localtime returned
   tm_isdst -1; and only the UTC year's transitions were used, so a rule whose
   transition falls in the next year (XBD 8.3's `EST5EDT,0/0,J365/25`) gave standard
   time for the first hours of every year. New `__tz_isdst` checks the neighbouring
   years near the ends. Failing-before:
   `TZ=EST5EDT,0/0,J365/25, 1735689600: got "2024-12-31 19:00:00 0", want "2024-12-31 20:00:00 1"`
   and `TZ=EST5EDT,M3.2.0,M11.1.0, -299851200: got "1960-07-01 07:00:00 0", want "1960-07-01 08:00:00 1"`.
5. **mktime left tm_isdst -1 and rejected a result of -1** (mktime.c). In the repeated
   hour, or before 1970, a negative tm_isdst stayed negative. And mktime_utc used -1 as
   its overflow value, so a local time of 1969-12-31 23:59:59 returned -1 without
   setting the members. mktime now picks the DST state, sets the members through
   localtime_r, and reports overflow separately. Failing-before:
   `a result of -1: mktime(&tm) == (time_t)-1 && tm.tm_wday == 3 && errno == 0`.
6. **strftime didn't set ERANGE, and wrote past a zero-size buffer** (strftime.c).
   `count < maxsize - 1` wrapped for maxsize 0. Failing-before:
   `ERANGE: strftime(buf, 10, "%Y-%m-%d", &tm) == 0 && IS_ERANGE(errno)`.
7. **strftime's + flag, %C and %F widths** (strftime.c). The + flag only added a sign
   for years over 9999; POSIX adds it when the field is wider than four bytes (two
   for %C), as the spec's table shows. %C gave one digit for years under 1000. %F
   ignored its flag and width. Failing-before:
   `strftime("%+4Y|%C%y|%+5Y|%+3C%y"): got "0270|270|00270|00270", want "0270|0270|+0270|+0270"`.
   test/test-strftime.c expected one digit for %C of year 7; updated (glibc keeps one).
8. **strptime** (strptime_l.c), rewritten parsing state: maximum widths and field
   widths (so `%Y%m%d` works), ignored 0/+ flags, %C with %y, %y 69 meaning 1969,
   full or abbreviated names for %a/%A/%b/%B/%h, %F with a wide year, %Z (sets
   tm_isdst), %z, %G and %g, and no more tm_isdst = -1. Failing-before (with the getdate
   commit also reverted, since it uses the new interface): `%Y-%m-%d: tm.tm_isdst >= 0`;
   the other checks follow.
9. **getdate** (getdate_r.c) accepted a template matching a prefix, returned stale
   fields for what the template didn't give, didn't reject February 31, and reported a
   directory as no match. It now follows the spec's rules for missing fields, uses
   mktime to validate, and reports 4 and 5. Failing-before:
   `template is a directory: getdate("Monday") == NULL && getdate_err == 4`.
10. **utimes accepted out-of-range microseconds on 32-bit targets**
    (libos/linux/utimes.c): tv_usec * 1000 in a 32-bit long wrapped 4294968 to 704 ns.
    Failing-before on armhf: `EINVAL for tv_usec: utimes("file", btv) == -1 && errno == EINVAL`.
11. **sysconf(_SC_DELAYTIMER_MAX) was 32** (libos/linux/sysconf.c) while the kernel
    counts overruns to INT_MAX. Failing-before:
    `overrun within DELAYTIMER_MAX: delaytimer_max == -1 || overrun <= delaytimer_max`.
12. **CLOCKS_PER_SEC was an int** (time.h). Failing-before:
    `static assertion failed: "CLOCKS_PER_SEC is a clock_t"`.
13. **clock_nanosleep(CLOCK_THREAD_CPUTIME_ID) returned EOPNOTSUPP** (libos/linux/
    clock_nanosleep.c); POSIX requires EINVAL. Failing-before:
    `clock_nanosleep on the thread CPU-time clock: ... == EINVAL`.
14. **setenv freed under the libc lock** (libc/stdlib/setenv.c, cross-cutting): the
    second environment growth asserted in lock-valid and overflowed the stack.
    Failing-before: test-getdate_err killed by SIGSEGV.

## Deviations not fixed

- **No timezone database** (Z12, M12). TZ values like `America/New_York` fall back to
  GMT. POSIX expects an implementation-defined database; that's a design decision for
  an embedded libc.
- **getdate ignores the %Z zone** (GD14): the broken-down time isn't initialized to the
  current time in the scanned zone.
- **getdate can't detect FIFOs and devices** (GD4): libos/semihost has no fstat, so
  generic code detects only directories (through EISDIR).
- **strftime %z/%Z use TZ and tm_isdst, not tm_gmtoff/tm_zone.** The spec lists
  tm_gmtoff and tm_zone among the inputs; glibc reads them. picolibc keeps its old
  behaviour, so hand-built struct tm values still format as before. Not a "shall"
  violation as written, so left for a decision.

## Other observations (not changed)

- timer_settime fails with EINVAL for a bad it_interval even when it_value is zero.
  POSIX requires the error only for a non-zero it_value, but allows it otherwise
  (XSH 2.3), and Linux does the same.
- localtime_r can overflow tm_year by one when the local adjustment crosses the end
  of year INT_MAX (inferred; unreachable with realistic times).
- `<sys/time.h>` still declares gettimeofday, getitimer and setitimer, which Issue 8
  removed; keeping them is allowed.
- The `<sys/_select.h>` select prototype has no `restrict`; compatible, but not the
  spec's text (net-sockets).
- I folded the strptime fixes into one commit because they share a rewritten parser;
  the commit message lists each.

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`), full
  `meson test -t 4`: 1265 tests; 1218 Ok, 0 Fail, 6 Expected Fail, 41 Skipped.
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`), full
  `meson test -t 4`: 1264 tests; 1215 Ok, 0 Fail, 6 Expected Fail, 43 Skipped.
- CMake (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 500 tests, 100% passed (262 skipped), including test-time_h, test-sys_time_h, test-localtime, test-mktime, test-strftime_l and test-strptime, which run on bare metal; the OS-dependent new tests skip.
- Timing: tests use sigtimedwait with 10 s timeouts, minimum-duration checks and
  2x/0.5x bounds for CPU-time comparisons; the whole test-clock_gettime takes about
  2 s, test-times about 1 s.
- `clang-format --dry-run` is clean on every C file and header I touched.

## Commits (audit/time, on top of 4af0c34b98)

- 51b67a556e libos/linux: Measure clock() with the process CPU-time clock
- 6e1b278c47 time: Set errno to EOVERFLOW when gmtime_r can't represent the year
- 017d2e7c3d time: Add tm_gmtoff and tm_zone to struct tm
- 5c120810f2 time: Apply the TZ rule to every year, including across year ends
- 183d96ed0d time: Never leave tm_isdst negative after mktime, and allow a result of -1
- 68d220d55f time: Set errno to ERANGE when the strftime result doesn't fit
- d0802f2d0d time: Follow POSIX.1-2024 for the strftime + flag, %C and %F widths
- 211f01ea28 time: Follow POSIX.1-2024 in strptime
- eab5ceac23 time: Complete and check the getdate result as POSIX describes
- d3fbab1b9c libos/linux: Reject out-of-range microseconds in utimes
- 262d8097eb libos/linux: Report the kernel's DELAYTIMER_MAX from sysconf
- 4477876670 time: Give CLOCKS_PER_SEC the type clock_t
- 6b9b0dadfe libos/linux: Fail with EINVAL in clock_nanosleep on the thread's CPU-time clock
- 8c21a4ddc4 stdlib: Free the old environment array after releasing the lock in setenv
- e5bdf273cc test-posix: Check the types, macros, variables and prototypes in time.h
- 8f5010868a test-posix: Check the clocks, CPU-time clocks, clock and nanosleep
- f96560212b test-posix: Check gmtime, localtime and the XBD 8.3 TZ rules
- 6ebd676e28 test-posix: Check normalization, tm_isdst and errors in mktime
- c2d0f0cd16 test-posix: Check conversions, flags, widths and ERANGE in strftime
- a6c799fbe2 test-posix: Check conversions, widths and centuries in strptime
- 1cd57f7ba2 test-posix: Check the defaults and getdate_err values of getdate
- ced6934cf3 test-posix: Check disarming, readings, overruns and deletion of timers
- cbba223cd5 test-posix: Check the types, macros and prototypes in sys/time.h
- 20e9cba76b test-posix: Check errors for bad microseconds and paths in utimes
- be26ee72f8 test-posix: Check times and the sys/times.h structure

Nothing pushed. No AI attribution in any commit.
