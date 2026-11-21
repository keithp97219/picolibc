# <pwd.h>, <grp.h>, <utmpx.h> audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-users`, branch `audit/users`, based on d14119a43a.

**Interfaces audited:**
- getpwnam, getpwnam_r, getpwuid, getpwuid_r, getpwent, setpwent, endpwent (XSI).
- getgrnam, getgrnam_r, getgrgid, getgrgid_r, getgrent, setgrent, endgrent (XSI).
- getutxent, getutxid, getutxline, pututxline, setutxent, endutxent (all XSI).
- The three headers.
- What getlogin/getlogin_r should do given picolibc's utmpx (left over from unistd-process).

All 20 interfaces are in Issue 8.

**Implementation:**
- `libc/posix/pwd/`: the new `__pw_find.c` does all the reading; the new `local-getent.h` is shared with grp.
- `libc/posix/grp/`: the new `__gr_find.c`.
- `libc/posix/utmpx/`: an empty database by design (14ebe3b96b).
- `libc/posix/unistd/getlogin.c`, `libos/fallback/sysconf.c`, `libos/linux/sysconf.c`.
- Headers `libc/include/pwd.h`, `grp.h`, `utmpx.h`.

The lookups read `/etc/passwd` and `/etc/group` directly, and there's no way to point them at another file. So the POSIX tests use only uid/gid 0, the running user and group, and names and ids that can't exist (`no:such:user`, `(uid_t)-2`). The parser the lookups share is also reached through the fgetpwent and fgetgrent extensions, which the tests feed from fmemopen buffers. No host account data appears in tests or here.

Tests are in `test/test-posix/test-pwd/`, `test-grp/` and `test-utmpx/`, plus `test-unistd/test-getlogin.c`. File names below drop the `test-` prefix, and line numbers are for the branch tip.

**Spec pages** (pubs.opengroup.org/onlinepubs/9799919799):
- basedefs/pwd.h, grp.h, utmpx.h.
- functions/getpwnam, getpwuid, getpwent, getgrnam, getgrgid, getgrent, endutxent, getlogin.

**Status key:** TESTED (file:line), UNTESTED, NOT TESTABLE (reason).
- Every TESTED row was run on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and, unless noted, for the `-native` glibc build.
- "(new)" marks checks this unit added; "(existing)" marks checks that were already there.

## Requirement table

### Headers

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | struct passwd: char *pw_name, uid_t pw_uid, gid_t pw_gid, char *pw_dir, char *pw_shell | TESTED (new) | pwd_h.c:70-74 (`_Generic`) |
| H2 | <pwd.h> defines gid_t, uid_t, size_t | TESTED (new) | pwd_h.c:64-66 (only <pwd.h> included) |
| H3 | getpwnam, getpwnam_r, getpwuid, getpwuid_r prototypes | TESTED (new) | pwd_h.c:76-81 (function pointers) |
| H4 | XSI endpwent, getpwent, setpwent prototypes | TESTED (new) | setpwent.c:61-63 |
| H5 | struct group: char *gr_name, gid_t gr_gid, char **gr_mem | TESTED (new) | grp_h.c:69-71 |
| H6 | <grp.h> defines gid_t, size_t | TESTED (new) | grp_h.c:64-65 |
| H7 | getgrgid, getgrgid_r, getgrnam, getgrnam_r prototypes | TESTED (new) | grp_h.c:73-78 |
| H8 | XSI endgrent, getgrent, setgrent prototypes | TESTED (new) | setgrent.c:61-63 |
| H9 | struct utmpx: char arrays ut_user, ut_id, ut_line; pid_t ut_pid; short ut_type; struct timeval ut_tv | TESTED (new) | utmpx_h.c:67-74. The ut_tv check is not run against glibc: on 32-bit targets glibc's ut_tv is a pair of int32_t |
| H10 | <utmpx.h> defines pid_t and struct timeval | TESTED (new) | utmpx_h.c:54-63. The tv_usec type check is not run against glibc |
| H11 | The eight ut_type constants are distinct and fit in short | TESTED (new) | utmpx_h.c:78-80, 96-101 |
| H12 | Prototypes for the six utmpx functions | TESTED (new) | utmpx_h.c:84-89 |
| H13 | Strict POSIX without XSI: no XSI functions or extensions are declared (fgetpwent, getpwent_r, putpwent, getpw, fgetgrent, getgrent_r, initgroups, setgroups, putgrent, _PATH_PASSWD, _PATH_GROUP) | TESTED (new) | pwd_h.c:84-97, grp_h.c:81-95 |
| H14 | Prototypes don't use names an application may define as macros | TESTED (new) | pwd_h.c:36-52, grp_h.c:36-52, utmpx_h.c:36-41. **Deviation 1, fixed** |

### getpwnam, getpwuid

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | getpwnam finds the matching name | TESTED | getpwnam.c:74-82 (new); getpwent.c (existing) |
| P2 | getpwuid finds the matching uid | TESTED | getpwnam.c:131-149 (new); getpwent.c (existing) |
| P3 | Not found: null pointer, errno unchanged | TESTED (new) | getpwnam.c:133, 155, 160-167. **Deviation 2, fixed** |
| P4 | On error, errno is set (EMFILE may fail) | TESTED for EMFILE (new) | getpwnam.c:111. EIO, EINTR, ENFILE: NOT TESTABLE (no failing device behind /etc/passwd; regular-file reads aren't interrupted; ENFILE needs the system-wide limit) |
| P5 | A lookup after a failed one still works | TESTED (new) | getpwnam.c:170 |
| P6 | No internal limit on entry size | TESTED (new, through fgetpwent) | fgetpwent.c:153-159 (a 2000-byte gecos field). **Deviation 6, fixed** |
| P7 | Need not be thread-safe; the result may be overwritten | NOT TESTABLE | These are permissions, not requirements |

### getpwnam_r, getpwuid_r

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | Stores the entry in *pwd and pwd in *result | TESTED | getpwnam_r.c / getpwuid_r.c:81-86 |
| R2 | Strings are allocated from the caller's buffer | TESTED (new) | :110-112 in both |
| R3 | Not found: returns 0 with *result NULL | TESTED (existing) | :89-90 in both |
| R4 | Not found is still 0 with a small buffer (ERANGE only for the entry found) | TESTED (new; not run against glibc, which gives ERANGE) | :120-122 in both. **Deviation 3, fixed** |
| R5 | ERANGE shall fail, with *result NULL | TESTED | :93-95 (existing); :100-108 (new: every size below the one needed) |
| R6 | sysconf(_SC_GETPW_R_SIZE_MAX) returns -1 without changing errno, or a suggested size | TESTED (new) | :126-128 in both. **Deviation 7, fixed** |
| R7 | EMFILE (may fail) gives the error and a null result | TESTED (new) | getpwnam.c:113-115 |
| R8 | EIO, EINTR, ENFILE | NOT TESTABLE | As P4 |

### getpwent, setpwent, endpwent (XSI)

| # | Requirement | Status | Reference |
|---|---|---|---|
| E1 | The first getpwent opens the database and returns the first entry | TESTED (new) | setpwent.c:88-92, 135 |
| E2 | Successive calls cover the database | TESTED | setpwent.c:79-103, 121 (new); getpwent.c (existing) |
| E3 | End of file: null pointer, errno unchanged | TESTED (new) | setpwent.c:105. **Deviation 2, fixed** |
| E4 | On error: null pointer, errno set | UNTESTED | The only error path is fopen failing; by reading, getpwent passes fopen's errno through |
| E5 | setpwent rewinds | TESTED | setpwent.c:124-128, 146-148 |
| E6 | endpwent closes, and the next getpwent starts over | TESTED (new) | setpwent.c:131-135, 150-152 |
| E7 | setpwent and endpwent leave errno alone | TESTED (new) | setpwent.c:126, 133, 152 |
| E8 | Lookups between getpwent calls don't move its position | TESTED (new) | setpwent.c:137-143 |
| E9 | Lines that aren't entries don't end the walk | TESTED (new, through fgetpwent_r) | fgetpwent.c:127-135. **Deviation 4, fixed** |
| E10 | EINTR, EIO, EMFILE, ENFILE (may fail) | NOT TESTABLE / UNTESTED | As P4; getpwent's EMFILE is untested |

### getgrnam, getgrgid

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | getgrnam finds the matching name | TESTED | getgrnam.c:80-88 (new); getgrent.c (existing) |
| G2 | getgrgid finds the matching gid | TESTED | getgrnam.c:137-155 (new); getgrent.c (existing) |
| G3 | gr_mem is a null-terminated array of names; an empty field means no members | TESTED (new) | getgrnam.c:66-75; fgetgrent.c:97-116, 150-158 |
| G4 | Not found: null pointer, errno unchanged | TESTED (new) | getgrnam.c:139, 161, 166-173. **Deviation 2, fixed** |
| G5 | On error, errno is set (EMFILE) | TESTED (new) | getgrnam.c:117. EIO, EINTR, ENFILE: NOT TESTABLE |
| G6 | A long entry (many members) is found | TESTED (new, through fgetgrent) | fgetgrent.c:177-184 (300 members). **Deviation 6, fixed** |

### getgrnam_r, getgrgid_r

| # | Requirement | Status | Reference |
|---|---|---|---|
| Q1 | Stores the entry in *grp and grp in *result | TESTED | :81-86 in both |
| Q2 | The name, member array and member names lie in the buffer | TESTED (new) | :110-119 in both. **Deviation 5, fixed** |
| Q3 | Not found: 0 with *result NULL | TESTED | :89-90 in both |
| Q4 | Not found is still 0 with a small buffer | TESTED (new; not run against glibc) | :126-128 in both. **Deviation 3, fixed** |
| Q5 | ERANGE shall fail, with *result NULL | TESTED | :93-94 (existing); :100-108 (new) |
| Q6 | sysconf(_SC_GETGR_R_SIZE_MAX) | TESTED (new) | :132-134 in both. **Deviation 7, fixed** |
| Q7 | EMFILE | TESTED (new) | getgrnam.c:119-121 |
| Q8 | EIO, EINTR, ENFILE | NOT TESTABLE | As P4 |

### getgrent, setgrent, endgrent (XSI)

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | The first call opens the database and returns the first entry | TESTED (new) | setgrent.c:87-91, 134 |
| F2 | Successive calls cover the database | TESTED | setgrent.c:78-102, 120 |
| F3 | End of file: errno unchanged | TESTED (new) | setgrent.c:104. **Deviation 2, fixed** |
| F4 | On error, errno is set | UNTESTED | As E4 |
| F5 | setgrent rewinds | TESTED | setgrent.c:123-127, 145-147 |
| F6 | endgrent closes, and the next getgrent starts over | TESTED (new) | setgrent.c:130-134, 149-151 |
| F7 | setgrent and endgrent leave errno alone | TESTED (new) | setgrent.c:125, 132, 151 |
| F8 | Lookups between getgrent calls don't move it | TESTED (new) | setgrent.c:136-142 |
| F9 | Malformed lines and bad gids are skipped | TESTED (new, through fgetgrent_r) | fgetgrent.c:151-159. **Deviation 4, fixed** |
| F10 | EINTR, EIO, EMFILE, ENFILE | NOT TESTABLE / UNTESTED | As E10 |

### utmpx functions (XSI): the database is empty by design

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | getutxent returns the next entry, and NULL at the end | TESTED (existing) | getutxent.c:59-61, 76 |
| X2 | getutxid matching rules (by ut_type, or by ut_id for the four process types) | TESTED only for "no match" (existing) | getutxent.c:69-71. Matching is NOT TESTABLE: there are no entries |
| X3 | getutxline matches LOGIN_PROCESS or USER_PROCESS entries by ut_line | TESTED only for "no match" | getutxent.c:65-67 |
| X4 | pututxline writes or replaces an entry, or returns NULL | NOT TESTABLE | There is no database, and tests must never write the system's utmp. picolibc returns NULL with ENOSYS (getutxent.c:77-78) |
| X5 | pututxline's implicit read leaves the static entry alone | NOT TESTABLE | As X4 |
| X6 | endutxent and setutxent | TESTED (existing) | getutxent.c:58, 64, 72 |
| X7 | Which members are meaningful for each ut_type | NOT TESTABLE | There are no entries |
| X8 | No errors are defined; pututxline may fail with EPERM | UNTESTED | By reading: ENOSYS is an additional error |

### getlogin (this unit's side)

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | Only a later getlogin call may overwrite its string | TESTED (new) | getlogin.c:75-80. **Deviation 8, fixed** |
| L2 | Login names fit {LOGIN_NAME_MAX} | TESTED (new) | setpwent.c:75, 84-86. **Deviation 9, fixed** |
| L3 | Returns the name used at login, associated with the controlling terminal | NOT TESTABLE | There is no login record (empty utmpx) and no controlling terminal under meson; see "Deviations not fixed" |

**Totals:** 77 requirements. Before this unit 18 were tested. After it, 58 are TESTED (some only for picolibc, as marked), 5 UNTESTED and 14 NOT TESTABLE.

## Deviations found and fixed

1. **<pwd.h> prototypes used plain parameter names** (`uid`, `pwd`, `buffer`, `bufsize`, `result`, `name`). An application macro with one of those names, defined before the include, broke the header.
   - Fix: the prototypes now use `__` names.
   - Failing before (compile): `test-pwd_h.c:38:17: error: expected ';', ',' or ')' before '+' token`.
2. **errno was set to ENOENT when nothing matched** in getpwnam, getpwuid, getgrnam and getgrgid, and at end of file in getpwent and getgrent. POSIX says errno shall not change in those cases.
   - Failing before: `getpwnam of a missing name: getpwnam(NO_NAME) == NULL && errno == 0`, the same for getgrnam, and `errno at the end: errno == 0`.
3. **ERANGE for entries that weren't asked for.** Every line was read into the caller's buffer. So an earlier line that didn't fit, or any line when the entry was missing, gave ERANGE instead of "found" or "not found".
   - Failing before: `missing, small buffer: getpwnam_r(NO_NAME, ...) == 0 && result == NULL`, and the same for getpwuid_r.
   - glibc behaves the same way (confirmed with a probe program), so these checks are `#ifndef __GLIBC__`.
4. **A malformed line ended every lookup and walk with EINVAL.** The old parser also mis-read ids:
   - The gid was never validated: the error `break` left the switch, not the loop.
   - strtol was used, so on 32-bit targets uid 4294967294 became LONG_MAX.
   - An empty uid field read as 0 (root).
   - picolibc's strsep drops an empty last field, so a valid entry with an empty shell (`name:x:6:6::/:`) counted as malformed.
   - Failing before: `fgetpwent_r: ret == 0 && pw == &ent` and `fgetgrent_r: ret == 0 && gr == &ent`.
5. **getgrnam_r and friends could write past the caller's buffer.** The room for the member array was computed as `((buf + size) - (char *)gr_memp) / sizeof(*gr_memp)`. When alignment pushed `gr_memp` past the end, the negative difference became a huge size_t, and the array's NULL terminator went beyond `buf + size`.
   - Failing before: `member array in the buffer: (const char *)&ent.gr_mem[m] >= buf && (const char *)(&ent.gr_mem[m] + 1) <= buf + need`.

   **Fix for 2–5** (one commit, a rewrite of the search):
   - Each line is read with getline and split in place with a strchr field splitter.
   - Lines that aren't entries are skipped: fewer than 7 passwd or 3 group fields, an empty name, or an id that isn't unsigned decimal within uid_t/gid_t.
   - Only the entry found is copied into the caller's buffer; ERANGE comes only when that entry doesn't fit.
   - On ERANGE the stream position is restored when the file is seekable, so the `_r` enumerators can retry with a bigger buffer (glibc does the same).
   - errno is saved and restored, and the files are now opened close-on-exec ("re").
6. **The non-reentrant functions used a BUFSIZ (512-byte) buffer.** An entry longer than that, such as a group with many members, couldn't be found: the function returned NULL with ERANGE, which POSIX doesn't list for them.
   - Fix: they keep the line getline read, grown to hold the member array for groups.
   - Failing before: `fgetpwent: pw != NULL` and `fgetgrent: gr != NULL`.
7. **sysconf(_SC_GETPW_R_SIZE_MAX) and sysconf(_SC_GETGR_R_SIZE_MAX) failed with EINVAL.** POSIX says they return -1 without changing errno, or a suggested size.
   - Fix: libos/fallback now returns 1024 (glibc's value).
   - Failing before: the sysconf checks in the four `_r` tests.
8. **getlogin returned getpwuid's static pw_name**, so a later getpwuid, getpwnam or getpwent call overwrote it. POSIX says only a later getlogin call may overwrite it. (With fix 6 the string would have been freed, so this commit comes before fix 6.)
   - Fix: getlogin fills its own 256-byte buffer through getlogin_r, and sets errno when that fails.
   - Failing before (library change reversed on the tip): `name after getpwuid and getpwnam: getlogin_r(name, sizeof(name)) == 0 && strcmp(p, name) == 0`.
9. **sysconf(_SC_LOGIN_NAME_MAX) was 9** on Linux, where user names run to 32 characters or more.
   - Fix: libos/linux now returns 256, as glibc does.
   - Failing before: `name fits {LOGIN_NAME_MAX}: strlen(pw->pw_name) < (size_t)login_max` (this host's database has longer names).

Every fix's test failed before the fix and passes after. The failing output above comes from aarch64 runs, in order:
- the base plus fix 1 (fixes 2–5);
- after the rewrite (fixes 6, 7, 9);
- with the library change reversed on the tip (fix 8).

## Deviations not fixed

- **getlogin and getlogin_r report the user database name for getuid()**, not "the name the user logged in under". The unistd-process unit left this for this unit, and I decided to keep it:
  - picolibc's utmpx is an empty database by the maintainer's design, so there is no login record to look up.
  - Always returning ENXIO would break the common `getpwnam(getlogin())`.
  - The answer is wrong after su, for a setuid program, or when several names share a uid (the first passwd entry wins).
  - If a better answer is wanted on Linux, glibc's first step is to look up the uid in `/proc/self/loginuid`; that would belong in libos/linux. Recorded in cross-cutting.md.
- **getlogin_r reads the entry into a 1024-byte stack buffer.** A passwd line longer than that gives ERANGE, which for getlogin_r means "namesize too small". By reading, not run.
- **pututxline fails with ENOSYS** rather than the listed EPERM. POSIX allows additional errors; left as the maintainer wrote it.
- **The utmpx database ignores the system's /var/run/utmp.** This is allowed, but tools like `who` see nobody.
- **A member list item that is empty (`a,,b`) gives an empty member name**, as in glibc. Only a wholly empty member field means "no members". Not tested.
- **On an unseekable stream, fgetpwent and fgetgrent (extensions) lose an entry that gives ERANGE**, because the position can't be restored.

## Other observations

- glibc 2.43 differences, which the tests guard:
  - Its `_r` lookups give ERANGE for any line that doesn't fit.
  - Its fgetpwent accepts a 4-field line.
  - Its fgetpwent and fgetgrent set errno at end of file.
  - On 32-bit targets its ut_tv isn't a struct timeval, and tv_usec isn't suseconds_t.
- picolibc already hid the XSI and extension names in strict mode; only the parameter names leaked.

## Left undone

- E4/F4 (the error path of getpwent and getgrent) and getpwent's EMFILE are untested.
- EIO, EINTR and ENFILE are not testable here.
- utmpx X2–X5 and X7 need a database with entries.
- L3 needs a decision on where a login record would come from.

## Test runs (verified by running, at the branch tip)

- **aarch64-linux-gnu** (`-Dwerror=true`), full `meson test -t 4`: 1539 Ok, 0 Fail, 6 Expected Fail, 43 Skipped.
- **arm-linux-gnueabihf** (`-Dwerror=true`), full `meson test -t 4`: 1538 Ok, 0 Fail, 6 Expected Fail, 45 Skipped.
- Every new and changed test passes on both targets, for picolibc and for `-native`.
- **Native** (`scripts/do-native-configure`, which sets `-Dwerror=true`; `ninja` only): builds cleanly.
- **CMake** (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`): 598 tests, 100% passed, 334 of them skipped.
- **clang:** `/tmp/arm64run/clang-check builds/build-aarch64-linux-gnu` on all 23 changed library files and 16 new or changed tests reported 55 OK and 0 FAIL. It covers both the picolibc and `-native` entries. A full clang build was not run.
- `clang-format --dry-run --Werror` is clean on every touched C file and header.
- Bare-metal meson targets were not run; the x86_64 host does them.

**Incidents:**
- The first armhf build failed on test-utmpx_h-native (glibc's 32-bit ut_tv). It is now guarded.
- My first EMFILE test edit also inserted a helper into the `#else` stub, which would have broken bare-metal builds. Both fixes were folded into their commits before the final runs above.
- There were no kills by host security software.

## Commits (audit/users, on top of d14119a43a)

- 7fbb66bbe9 pwd.h: Use reserved names for the parameters of the POSIX prototypes
- 65f28fe467 pwd, grp: Search the databases a line at a time, and report only real errors
- 32d2e8fed9 unistd: Keep getlogin's name apart from the user database's static data
- 41cc899109 pwd, grp: Size the non-reentrant functions' buffer to fit the entry
- 19ee1bab8a sysconf: Suggest a buffer size for the getpw*_r and getgr*_r functions
- fe715223a5 libos/linux: Report a login name limit that fits Linux user names
- 97e2e13143 test-posix: Check the types, members and prototypes in <pwd.h>, <grp.h> and <utmpx.h>
- 6fab10facc test-posix: Check getpwnam, getpwuid and their reentrant forms
- 97d90f4266 test-posix: Check getpwent, setpwent and endpwent, and the shared parser
- d85916990e test-posix: Check getgrnam, getgrgid and their reentrant forms
- b17134865b test-posix: Check getgrent, setgrent and endgrent, and the shared parser
- 6b2f3def74 test-posix: Check that the user database functions leave getlogin's name alone

Nothing was pushed, and no commit carries AI attribution.
