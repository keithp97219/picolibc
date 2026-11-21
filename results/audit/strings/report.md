# `<string.h>`, `<strings.h>`: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-strings`, branch `audit/strings`, based on 6edd04ce22 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot as an unprivileged user, with gcc 16.2, clang 21 and glibc 2.43.

**Interfaces (44).**
- `<string.h>`: memccpy (XSI), memchr, memcmp, memcpy, memmem, memmove, memset, stpcpy, stpncpy, strcat, strchr, strcmp, strcoll, strcoll_l, strcpy, strcspn, strdup, strerror, strerror_l, strerror_r, strlcat, strlcpy, strlen, strncat, strncmp, strncpy, strndup, strnlen, strpbrk, strrchr, strsignal, strspn, strstr, strtok, strtok_r, strxfrm, strxfrm_l.
- `<strings.h>`: ffs, ffsl, ffsll (XSI), strcasecmp, strcasecmp_l, strncasecmp, strncasecmp_l.
- The POSIX additions came first (stpcpy, stpncpy, strdup, strndup, strnlen, strtok_r, strerror_r/strerror_l, strsignal, memccpy, the `_l` functions, and Issue 8's memmem, strlcpy, strlcat, ffsl and ffsll). Then the CX requirements on the ISO C functions: errno, locale dependence, strerror's messages and thread rules.
- strsep, a BSD extension, is fixed and tested too: cross-cutting.md (users unit) recorded that it dropped an empty last field.

**Out of scope.**
- bcmp, bcopy, bzero, index and rindex were removed in Issue 7. They are checked only as namespace items.
- The locale objects themselves (newlocale, uselocale and so on) belong to the locale unit. Here only the `_l` string functions' use of a locale is checked.
- psignal and psiginfo are `<signal.h>` interfaces. psignal is only checked as the user of strsignal's messages.

**Implementation.**
- Generic C: `libc/string/` (plus `libc/signal/psignal.c`).
- Assembly: `libc/machine/aarch64/*.S` (memchr, memcmp, memcpy and memmove, memset, stpcpy, strchr, strcmp, strcpy, strlen, strncmp, strnlen, strrchr, with `-stub.c` fallbacks that include the generic C when `__OPTIMIZE_SIZE__` is set or NEON is missing). `libc/machine/arm/`: strcmp.S, strcpy.S and memchr.S in every build; strlen.S, memcpy.S and memset.S depending on the optimization level.
- Headers: `libc/include/string.h`, `strings.h`, `ssp/string.h`, `ssp/strings.h`, `ssp/ssp.h`; the `__malloc_like` attribute in `sys/cdefs.h` (changed here: one line, which every header using `__malloc_like` sees).

**Which code was run.** The default build type is minsize. There the aarch64 assembly isn't used at all, and on armhf only strcmp, strcpy, memchr and strlen are. So I also built both targets with `--buildtype=release` (`builds/build-aarch64-rel`, `builds/build-arm-rel`), checked with `nm` that the assembly versions were the ones linked, and ran the full suite there as well.

**Tests.** File names below drop the `test-` prefix, and line numbers are for the branch tip.
- `test/test-posix/test-string/`: string_h.c, memchr-pages.c, strnlen-pages.c (both share string-pages.h), strerror_r.c, strsignal.c, strtok_r.c, strndup.c, strcoll_l.c (all new).
- `test/test-posix/test-strings/`: strings_h.c, ffsll.c, strcasecmp_l.c (all new).
- `test/test-string/test-strsep.c` (new; picolibc's own extension test).
- The page tests map three arenas of one page each, with a PROT_NONE page on each side, so an access past either end of an operand faults. A SIGSEGV/SIGBUS handler names the call that faulted. Without mmap (bare metal) they use static buffers and check only the contents. All library calls go through volatile function pointers, so the compiler can't inline or fold them, and the reference loops use volatile accesses so they can't be turned back into library calls.
- In the alignment loops the c or the difference is put at positions near the start, middle and end; at the page edges, at every position up to 64.
- Existing tests relied on: `test/test-string/` (memchr, memmem, memmove, memset, strchr, strncpy, ffs, strerror), `test/libc-testsuite/string.c`, `test/testsuite/newlib.string/` (tstring.c, memcpy-1.c, memmove1.c, strcmp-1.c) and test-signal/test-psiginfo.c.

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799: basedefs/string.h and strings.h; functions/ memccpy, memchr, memcmp, memcpy, memmem, memmove, memset, strcat, strchr, strcmp, strcoll, strcpy, strcspn, strdup, strerror, strlcat, strlen, strncat, strncmp, strncpy, strpbrk, strrchr, strsignal, strspn, strstr, strtok, strxfrm, ffs and strcasecmp.

**Status key.**
- TESTED (file:line), UNTESTED, or NOT TESTABLE (with the reason).
- TESTED rows were run on aarch64-linux-gnu and arm-linux-gnueabihf, minsize and release, for picolibc and, unless noted, for the `-native` glibc build.
- "(new)" means added by this unit; "(existing)" means an earlier test already covered it.

## Requirement table

### `<string.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | NULL and size_t; locale_t (CX) | TESTED (new) | string_h.c:139-145 |
| H2 | Every function declared with the synopsis prototype | TESTED (new) | string_h.c:154-195 (`_Generic` on `&f`). memmem, strlcat and strlcpy are skipped for glibc, which hides them without `_DEFAULT_SOURCE` |
| H3 | Each is a function, even where a macro exists | TESTED (new) | string_h.c:214-252 (`(f)(...)` calls) |
| H4 | strerror_r is the XSI `int` form without `_GNU_SOURCE` | TESTED (new) | string_h.c:173, 235 |
| H5 | memccpy is declared with `_XOPEN_SOURCE` | TESTED (new) | memchr-pages.c:36-63 |
| H6 | Strict POSIX declares no BSD or GNU extensions, no `<strings.h>` names, no removed bcmp family, not the XSI memccpy, and not free | TESTED (new) | string_h.c:104-135. **Deviations 5 and 6, fixed** (free, memccpy) |
| H7 | Parameter names don't collide with application macros (also with `_FORTIFY_SOURCE`) | TESTED (new) | string_h.c:46-100. **Deviations 4 and 7, fixed** |
| H8 | The header may make `<stddef.h>` symbols visible | n/a | a permission |

### `<strings.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| SH1 | size_t and locale_t | TESTED (new) | strings_h.c:86-87 |
| SH2 | strcasecmp, strcasecmp_l, strncasecmp, strncasecmp_l prototypes; they are functions | TESTED (new) | strings_h.c:91-94, 104-111 |
| SH3 | ffs, ffsl, ffsll prototypes with `_XOPEN_SOURCE` | TESTED (new) | ffsll.c:51-53. glibc needs `_DEFAULT_SOURCE` for ffsl and ffsll |
| SH4 | Strict non-XSI mode declares none of ffs, ffsl, ffsll, the removed bcmp family, the BSD extensions or `<string.h>` names | TESTED (new) | strings_h.c:67-81. **Deviation 6, fixed** (ffsl, ffsll) |
| SH5 | Parameter names don't collide with application macros | TESTED (new) | strings_h.c:43-59 |

### Every function

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | "shall not change the setting of errno on valid input" (memccpy, memchr, memcmp, memcpy, memmove, memset, stpcpy, strcpy, stpncpy, strncpy, strcat, strncat, strchr, strrchr, strcmp, strncmp, strcspn, strspn, strpbrk, strstr, strlcpy, strlcat, strlen, strnlen, strtok, strtok_r, strcasecmp family in the POSIX locale; strcoll, strxfrm, strerror, strerror_l when successful) | TESTED (new) | `check_errno` after every call in memchr-pages.c and strnlen-pages.c; strtok_r.c:76-77; strcasecmp_l.c:113-114; strcoll_l.c:95-96, 132-133; strerror_r.c:276-279 |
| C2 | Every alignment of source and destination (0-15 each) for the copying and comparing functions, 0-31 for single operands | TESTED (new) | memchr-pages.c:160-180, 220-241, 260-283, 322-366, 386-410, 458-476; strnlen-pages.c:165-207, 245-267, 326-354, 383-412, 437-468 |
| C3 | Lengths 0-64 and 65-1000 around word, vector and cache-line sizes | TESTED (new) | string-pages.h:240-256 (`length()`), used throughout |
| C4 | Operands ending at, or starting at, an inaccessible page | TESTED (new) | every `at_end`/`at_start`/`str_end`/`arr_end` call in both page tests |
| C5 | Bytes with the high bit set (compare as unsigned char; c converted) | TESTED (new) | string-pages.h:177-185 (`pattern()`); memcmp and strcmp sign checks with 0x80, 0x01 and 0xff flips (memchr-pages.c:193-214, strnlen-pages.c:284-311) |

### Searching and comparing

| # | Requirement | Status | Reference |
|---|---|---|---|
| M1 | memchr finds the first c, converted to unsigned char, in n bytes; NULL if absent | TESTED | memchr-pages.c:110-158 (new); test-string/test-memchr.c (existing) |
| M2 | memchr behaves as if it reads sequentially and stops at the match: n past the guard page, and n = SIZE_MAX, with the match before it | TESTED (new) | memchr-pages.c:145-152 |
| M3 | memcmp's sign is that of the first differing pair as unsigned char; n bytes only | TESTED | memchr-pages.c:183-241 (new); tstring.c (existing) |
| M4 | memmem finds the first occurrence; NULL if not found | TESTED | memchr-pages.c:479-574 (new; two- and four-byte alphabets against a naive search, needles of 1-300 bytes, both page ends); test-string/test-memmem.c (existing) |
| M5 | memmem: needlelen 0 returns haystack; haystacklen < needlelen returns NULL | TESTED (new) | memchr-pages.c:525-540 |
| S1 | strchr and strrchr find the first and last c converted to char; the null is part of the string | TESTED | strnlen-pages.c:209-267 (new); test-string/test-strchr.c (existing) |
| S2 | strcmp's sign uses unsigned char; a prefix compares less | TESTED | strnlen-pages.c:269-354 (new); strcmp-1.c (existing) |
| S3 | strncmp compares at most n bytes, none after a null, of possibly unterminated arrays | TESTED (new) | strnlen-pages.c:276-310, 336-337 (arrays ending at the guard) |
| S4 | strspn, strcspn and strpbrk, sets with high-bit bytes and the empty set | TESTED | strnlen-pages.c:612-669 (new); tstring.c (existing) |
| S5 | strstr finds the first occurrence; an empty s2 returns s1; NULL if not found | TESTED | strnlen-pages.c:671-731 (new); libc-testsuite string.c (existing) |
| S6 | strlen; strnlen returns the smaller of the length and maxlen, and never examines more than maxlen bytes | TESTED | strnlen-pages.c:165-207 (new: arrays without a null ending at the guard, maxlen = SIZE_MAX); tstring.c (existing strlen) |

### Copying and concatenating

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | memcpy copies n bytes and returns s1 | TESTED | memchr-pages.c:243-283 (new); memcpy-1.c (existing) |
| P2 | memmove copies as if through a temporary array; returns s1 | TESTED | memchr-pages.c:285-366 (new: offsets -40..40, at 8 source alignments for n <= 64, moves by exactly n and n-1, overlapping moves at both page edges); test-string/test-memmove.c, memmove1.c (existing) |
| P3 | memset stores c converted to unsigned char in n bytes; returns s | TESTED | memchr-pages.c:368-410 (new: c = 0x1a5, -1, -128 too); test-string/test-memset.c (existing) |
| P4 | memccpy stops after the first c (converted to unsigned char) or n bytes; returns the byte after c, or NULL | TESTED (new) | memchr-pages.c:412-476 |
| P5 | strcpy and stpcpy copy the string and its null; strcpy returns s1, stpcpy the null | TESTED | strnlen-pages.c:356-412 (new); libc-testsuite string.c (existing strcpy) |
| P6 | strncpy and stpncpy copy at most n bytes of a possibly unterminated array and pad with nulls; stpncpy returns the first null written or &s1[n] | TESTED | strnlen-pages.c:414-468 (new); test-string/test-strncpy.c (existing) |
| P7 | strcat appends; strncat appends at most n bytes of a possibly unterminated array and always terminates; both return s1 | TESTED | strnlen-pages.c:470-524 (new: n = SIZE_MAX too); tstring.c (existing) |
| P8 | strlcpy copies at most dstsize-1 bytes, terminates unless dstsize is 0, returns strlen(src) | TESTED | strnlen-pages.c:526-542, 575-594 (new); libc-testsuite string.c (existing) |
| P9 | strlcat appends at most dstsize-strlen(dst)-1 bytes, terminates unless the null would be at or past dst+dstsize, returns strnlen(dst, dstsize)+strlen(src); a dst without a null in dstsize bytes is left alone | TESTED | strnlen-pages.c:544-610 (new); libc-testsuite string.c (existing) |
| P10 | Overlapping copies (all but memmove) are undefined | NOT TESTABLE | undefined behaviour |
| D1 | strdup returns a new copy that free accepts | TESTED (new) | strndup.c:118-127; strnlen-pages.c:733-754 |
| D2 | strndup copies at most size bytes, up to and including a null, and always terminates | TESTED (new) | strndup.c:129-147 |
| D3 | strndup examines no more than size bytes | TESTED (new) | strnlen-pages.c:755-766 (arrays without a null ending at the guard) |
| D4 | Both fail with ENOMEM when there is no storage | TESTED (new) | strndup.c:68-110 and 149-166: a child with RLIMIT_AS takes all memory first. Skipped under qemu-user (which ignores RLIMIT_AS) and with ASan |

### strtok, strtok_r

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | Tokens are found after leading separators; no token gives NULL | TESTED | strtok_r.c:142-156 (new, including both examples on the page); libc-testsuite string.c (existing) |
| T2 | The separator after a token is overwritten with a null; the search continues after it | TESTED (new) | strtok_r.c:174-180 |
| T3 | After the last token, later calls return NULL | TESTED (new) | strtok_r.c:91-93 |
| T4 | The separator string may change from call to call | TESTED (new) | strtok_r.c:158-172 |
| T5 | strtok_r keeps its state in *state; the state needn't be initialized | TESTED (new) | strtok_r.c:70-73 |
| T6 | The state never points outside s | TESTED (new) | strtok_r.c:78-79 |
| T7 | strtok_r is thread-safe; strtok's state and strtok_r's are separate | TESTED (new) | strtok_r.c:106-134 and 207-221 (four threads); 182-205 |
| T8 | strtok "shall avoid data races with all other functions"; no library function calls strtok | NOT TESTABLE | needs a race detector. By reading: the state is thread-local and nothing in libc calls strtok |

### strerror, strerror_l, strerror_r

| # | Requirement | Status | Reference |
|---|---|---|---|
| E1 | A valid error number's message says what error occurred: every POSIX error number, and every other one Linux can return that `<errno.h>` names | TESTED (new) | strerror_r.c:337-357. **Deviation 1, fixed** (30 numbers gave "Unknown error"; EDQUOT and ESTALE came from logging-misc) |
| E2 | Distinct error numbers have distinct messages | TESTED (new) | strerror_r.c:359-364 |
| E3 | Zero gives an empty string or one saying there was no error | TESTED (new) | strerror_r.c:366-377 |
| E4 | Any other int gets a message (an unknown error) | TESTED | strerror_r.c:379-407 (new: -1, -2, INT_MIN, INT_MAX, 4096, 100000); test-string/test-strerror.c (existing, -2) |
| E5 | strerror_l gives the message in the given locale | TESTED (new) | strerror_r.c:350-355 (the POSIX locale). picolibc has one set of messages |
| E6 | strerror_r returns 0 with the message in the buffer | TESTED (new) | strerror_r.c:288-296 |
| E7 | strerror_r fails with ERANGE when buflen is too small, for every buflen from 0 to the length | TESTED (new) | strerror_r.c:298-309. Nothing is written past buflen |
| E8 | strerror_r may fail with EINVAL for an unknown number | TESTED (new) | strerror_r.c:401-406: 0 and EINVAL are both accepted. picolibc returns 0 with "Unknown error" |
| E9 | errno is unchanged when they succeed; strerror may set EINVAL for unknown numbers | TESTED (new) | strerror_r.c:273-279, 383-399 |
| E10 | No other function calls strerror: its string survives perror, strerror_r, strerror_l, strsignal and snprintf | TESTED (new) | strerror_r.c:409-426 |
| E11 | strerror "shall avoid data races with all other functions" | NOT TESTABLE | needs a race detector. By reading: every message is a constant string |
| E12 | The messages "should" follow LC_MESSAGES | n/a | a recommendation; picolibc has English messages only |

### strsignal

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | A string for every valid signal number | TESTED (new) | strsignal.c:126-147, every number up to SIGRTMAX. **Deviation 2, fixed** (SIGTTOU gave NULL) |
| G2 | Distinct signals, including each real-time signal, have distinct messages | TESTED (new) | strsignal.c:135-163 |
| G3 | The same set of messages as psignal | TESTED (new) | strsignal.c:178-187 and 213-224. The real-time part is skipped for glibc, whose psignal says "Unknown signal" for them |
| G4 | No function calls strsignal (as if) | TESTED (new) | strsignal.c:188-211. **Deviation 3, fixed** (psignal overwrote the buffer) |
| G5 | The result for an invalid number is unspecified | TESTED (new, picolibc) | strsignal.c:165-175: picolibc gives "Unknown signal N" |
| G6 | strsignal need not be thread-safe | n/a | picolibc's buffer is per thread anyway |
| G7 | The messages "should" follow LC_MESSAGES | n/a | a recommendation |

### Collation

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | strcoll compares by LC_COLLATE of the current locale; in the POSIX locale that is byte order | TESTED (new) | strcoll_l.c:157 |
| L2 | strcoll_l uses the locale argument, whatever the current locale | TESTED (new) | strcoll_l.c:158, 163-170. The part with another current locale needs a C.UTF-8 locale, which only `-Dmb-capable=true` picolibc builds and glibc have, so on these builds it ran for glibc only. By reading, picolibc's strcoll_l ignores its argument (all its locales collate by byte) |
| L3 | strcmp of strxfrm results agrees with strcoll, and strxfrm_l with strcoll_l in the same locale | TESTED (new) | strcoll_l.c:118-148 |
| L4 | strxfrm writes no more than n bytes; n = 0 allows a null s1; it returns the transformed length | TESTED (new) | strcoll_l.c:88-114 |
| L5 | A locale whose collation differs from byte order | NOT TESTABLE | picolibc has none: strcoll is strcmp in every locale |
| L6 | EINVAL for characters outside the collating sequence (may fail) | n/a | picolibc never fails |
| L7 | LC_GLOBAL_LOCALE or an invalid locale object is undefined | NOT TESTABLE | undefined behaviour |

### `<strings.h>` functions

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | ffs, ffsl, ffsll give the index of the lowest set bit, from 1; 0 for 0 | TESTED | ffsll.c:75-111 (new: every bit, with every pattern above it, and the sign bit); test-string/test-ffs.c (existing) |
| K1 | In the POSIX locale, strcasecmp and strncasecmp compare as if both strings were lowercased, then compared bytewise | TESTED (new) | strcasecmp_l.c:93-115 and 117-165 (`_` and `[` sort below letters; high-bit bytes have no case and sort above ASCII) |
| K2 | strncasecmp compares at most n bytes of possibly unterminated arrays | TESTED (new) | strcasecmp_l.c:103-109, 169-176; strnlen-pages.c:282-283 |
| K3 | The `_l` forms use the locale argument | TESTED (new) | strcasecmp_l.c:178-202. As for L2, the part with another current locale ran only for glibc |
| K4 | errno is unchanged in the POSIX locale | TESTED (new) | strcasecmp_l.c:113-114 |
| K5 | Other locales give unspecified results | n/a | |

### Extension: strsep

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | strsep returns every field, including empty ones at the start, the middle and the end, then NULL with *stringp NULL (BSD behaviour) | TESTED (new) | test-string/test-strsep.c:52-69. **Deviation 8, fixed** |

### Counts

There are 84 requirement rows:
- 73 TESTED; 19 of these had some coverage before this unit (marked "existing"), though none at page boundaries or every alignment.
- 5 NOT TESTABLE.
- 6 n/a.
- 0 UNTESTED.

## Deviations found and fixed

1. **strerror gave "Unknown error" for 30 error numbers that `<errno.h>` defines**, most of which Linux returns: ECHRNG, EL2NSYNC, EL3HLT, EL3RST, ELNRNG, EUNATCH, ENOCSI, EL2HLT, EBADE, EBADR, EXFULL, ENOANO, EBADRQC, EBADSLT, EBFONT, EDOTDOT, ENOTUNIQ, EBADFD, EREMCHG, EPFNOSUPPORT, ETOOMANYREFS, EUSERS, ENOMEDIUM, EHWPOISON, EISNAM, EKEYEXPIRED, EKEYREJECTED, EKEYREVOKED, and the picolibc-only EFTYPE, ELBIN, EPROCLIM and EDEADLOCK. POSIX: "if the value of errnum is a valid error number, the message string shall indicate what error occurred".
   - Fix: glibc's messages for each.
   - Also by reading: EALREADY's message was "Socket already connected" (EISCONN's meaning) and EINPROGRESS's "Connection already in progress" (EALREADY's). They are now "Operation already in progress" and "Operation now in progress". No test can tell a wrong message from a right one, so this part is proved by reading only.
   - EDQUOT and ESTALE are logging-misc's 2a963bc541. It is cherry-picked unchanged as patch 0001 so that test-strerror_r passes here; skip it with `git am --skip` if logging-misc's series goes in first. Patch 0002 adds its lines at the top of the table, away from 0001's. I checked with `git apply --cached` that 0002-0017 apply cleanly on top of audit/logging-misc.
   - Failing-before: `strerror(EBADE) is "Unknown error"` (and 31 more lines).
2. **strsignal(SIGTTOU) returned NULL.** The table named `SIGTTOUT`, which doesn't exist. Every other number below the table's end without a name (0 here) also gave NULL, and psignal printed it with `%s`. POSIX: "Upon successful completion, strsignal() shall return a pointer to a string".
   - Fix: SIGTTOU named. A number without a name now gives "Unknown signal N". That message had no space and printed garbage for negative or three-digit numbers; it now formats any int ("Real-time signal N" likewise).
   - Failing-before: `strsignal(SIGTTOU) gave no message`, `psignal(SIGTTOU) wrote "prefix: (null)"`.
3. **psignal overwrote the string strsignal had returned.** POSIX: "The implementation shall behave as if no function defined in this standard calls strsignal()". psignal (and psiginfo, through it) called strsignal, which builds real-time and unknown messages in its per-thread buffer.
   - Fix: strsignal.c gains an internal `__strsignal_r(sig, buf)`. strsignal passes its buffer, and psignal passes one on its stack.
   - Failing-before: `psignal(62) changed strsignal(32)'s string from "Real-time signal 0" to "Real-time signal 30"`.
4. **`<string.h>` named the parameters of memchr, memmem, strchr, strpbrk, strrchr and strstr** (s, c, n, h, h_n, n_n, a), and of the GNU strerrorname_np and strerrordesc_np (errnum). An application macro with one of those names broke the header.
   - Fix: the names are dropped.
   - Failing-before: test-string_h doesn't compile (`string.h:87: expected ... before 'h_n'`).
5. **`<string.h>` declared free**, so that strdup's `__malloc_like` attribute could name it as the deallocator. free is reserved at file scope only when `<stdlib.h>` is included, so an application's own static free, or a macro named free, clashed with the header.
   - Fix: `__malloc_like` (sys/cdefs.h) names `__builtin_free`, as glibc does. That needs no declaration. gcc accepts it under -fno-builtin and in C++; clang uses only the plain `malloc` attribute.
   - Failing-before: test-string_h doesn't compile (`cdefs.h:292: expected ')' before numeric constant`, from `#define free )`).
6. **memccpy, ffsl and ffsll were visible without XSI.** All three are XSI in POSIX.1-2024. memccpy was declared for any `_POSIX_C_SOURCE`, and ffsl and ffsll for `_POSIX_C_SOURCE >= 202405`.
   - Fix: memccpy needs `__XSI_VISIBLE` (or BSD, or C23, where it is ISO C); ffsl and ffsll need `__XSI_VISIBLE >= 800` (or BSD). glibc also hides all three in strict mode.
   - Failing-before: `'memccpy' redeclared as different kind of symbol` (test-string_h), and the same for ffsl and ffsll (test-strings_h).
7. **The fortified string functions named their parameters** (`<ssp/string.h>`: dst, src, len in the inline `__*_ichk` definitions; `<ssp/ssp.h>`: handler and sig in set_fortify_handler). With `_FORTIFY_SOURCE` (which the picolibc test builds use), an application macro with one of those names broke `<string.h>`.
   - Fix: `__`-prefixed names, and none in the prototype.
   - Failing-before: test-string_h doesn't compile (`ssp/string.h:61: conflicting types for '__memcpy_ichk'`).
8. **strsep dropped an empty last field** (BSD extension). `strsep` on `"a:"` gave `"a"` and then NULL; BSD and glibc give `"a"`, `""` and then NULL, and `strsep("")` gives `""`. This was recorded in cross-cutting.md by the users unit.
   - Fix: `__strtok_r` in strsep's mode (no skipping of leading delimiters) returns the empty field at the end of the string. strtok and strtok_r are unchanged.
   - Failing-before: `strsep("a:") stopped before field 1 ""`.

**How the fixes were proved.** For each one, I reversed only its library change (`git show <fix> -- libc libos | git apply -R`; for 4, with `-C1`, because 6 changed a neighbouring line), rebuilt the test in builds/build-aarch64-rel, saw the failure quoted above, and reapplied the change.

## Deviations not fixed, and things noted

- **No assembly bug was found.** On real Graviton2 hardware, the aarch64 assembly (release build) and the armhf assembly (strcmp, strcpy, memchr and strlen in both builds; memcpy and memset in release) passed every alignment, length, page-boundary, high-bit and overlap case. That includes memchr, strnlen and strncmp with n = SIZE_MAX. glibc passed the same tests.
- **strerror_l, strcoll_l, strxfrm_l, strcasecmp_l and strncasecmp_l**: strerror_l, strcoll_l and strxfrm_l ignore their locale argument. That is correct for picolibc, which has one set of messages and collates by byte in every locale. strcasecmp_l uses tolower_l, so it follows its argument.
- **Linux error numbers with no picolibc name** (EUCLEAN, ENOTNAM, ENAVAIL, EREMOTEIO, EMEDIUMTYPE, ENOKEY, ERFKILL, and Linux's unused 41 and 58) are mapped to EINVAL by libos/linux's `__errno_map`. That isn't a strerror question, but strerror can't describe them. Recorded in cross-cutting.md.
- picolibc's EDEADLOCK (56) differs from EDEADLK (45), while on Linux they are equal. The Linux mapping never produces 56. It now has its own message.

## Left undone

- L2 and K3's "another current locale" checks need a picolibc build with `-Dmb-capable=true`, where C.UTF-8 exists. The x86_64 host could run test-strcoll_l and test-strcasecmp_l in such a configuration.
- The x86_64 host should run test-memchr-pages and test-strnlen-pages under qemu-user for aarch64 and armhf, and on bare-metal arm. Under qemu the guard pages work as on hardware (qemu-user honours PROT_NONE). On bare metal the tests fall back to unguarded buffers and check only the contents of each alignment and length.

## Test runs (verified by running)

- **aarch64-linux-gnu** (`-Dwerror=true`, minsize, full `meson test -t 4`): 1603 Ok, 0 Fail, 6 Expected Fail, 42 Skipped.
- **arm-linux-gnueabihf** (minsize): 1602 Ok, 0 Fail, 6 Expected Fail, 44 Skipped.
- **aarch64-linux-gnu, `--buildtype=release`** (the assembly string functions): 1603 Ok, 0 Fail, 6 Expected Fail, 42 Skipped.
- **arm-linux-gnueabihf, `--buildtype=release`**: 1602 Ok, 0 Fail, 6 Expected Fail, 44 Skipped.
- **clang native** (`CC=clang CXX=clang++ do-native-configure -Dwerror=true`, full suite): 1229 Ok, 0 Fail, 6 Expected Fail, 382 Skipped.
- **Native configuration** (`do-native-configure -Dwerror=true`, `ninja` only): builds cleanly.
- **CMake** (`do-cmake-thumbv7m-configure`, `ninja test`): 630 tests, 100% passed.
- After those runs I cut the work in the two page tests' alignment loops (positions near the start, middle and end only, instead of all of them; memmove at 8 source alignments, -40..40), because test-memchr-pages took 75 s under qemu-system in the CMake run. The page tests now take 11 s and 7 s there. I reran just those two tests (and their `-native` builds) on all five meson builds and in CMake: all pass. The native build still builds. Nothing else changed after the full runs.
- `clang-format --dry-run -Werror` is clean on every changed C file and header.
- The fix proofs are listed with each deviation above. Each was run in builds/build-aarch64-rel.
- The host security software killed nothing.

## Commits (audit/strings, on top of 6edd04ce22)

- 757ac2391e strerror: Give EDQUOT and ESTALE messages
- 585fdeaf0d strerror: Give a message for every error number <errno.h> defines
- 8bb4be6d5f strsignal: Name SIGTTOU, and never return a null pointer
- 94ba2ff441 strsep: Return the empty last field
- dec2e03046 string.h: Don't name the parameters of memchr, memmem, strchr and others
- 8526113ef8 string.h: Don't declare free
- 1773093478 string.h, strings.h: Hide memccpy, ffsl and ffsll without XSI
- f589c77366 ssp: Don't name parameters in the fortified string functions
- 8846d9a2f8 test-posix: Check the <string.h> and <strings.h> names in strict mode
- 5274b5eb4f test-posix: Check the string functions at guard pages and every alignment
- 08abe4cc8c test-posix: Check strerror, strerror_l and strerror_r for every error number
- 956dd4628d test-posix: Check strsignal's messages, and that psignal prints them
- 370ddaaabd test-posix: Check strtok_r, strndup and strcoll_l
- b899737863 test-posix: Check ffs, ffsl, ffsll and strcasecmp_l
- f3949f6e8a test-string: Check strsep's empty fields
- 8fe4ed9c69 psignal: Don't overwrite the string strsignal returned
- 4ad1dbe936 test-posix: Check that psignal leaves strsignal's string alone

The patches are in `/local/home/keithpac/arm64-testing/results/audit/strings/`, made with `git format-patch -o ... 6edd04ce22..audit/strings`. Nothing was committed to or pushed from the arm64-testing checkout.
