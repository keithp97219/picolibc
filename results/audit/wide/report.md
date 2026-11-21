# `<wchar.h>`, `<uchar.h>`, `<wctype.h>`: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-wide`, branch `audit/wide`, based on 1bac8ef806 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot as an unprivileged user, with gcc 16, clang 21 and glibc 2.43.

**Interfaces (about 116).**
- `<wchar.h>`, POSIX additions: mbsnrtowcs, open_wmemstream, wcpcpy, wcpncpy, wcscasecmp, wcscasecmp_l, wcscoll_l, wcsdup, wcslcat and wcslcpy (new in Issue 8), wcsncasecmp, wcsncasecmp_l, wcsnlen, wcsnrtombs, wcsxfrm_l; XSI wcwidth and wcswidth.
- `<wchar.h>`, ISO C with CX text: btowc, wctob, mbrlen, mbrtowc, mbsinit, mbsrtowcs, wcrtomb, wcsrtombs; fgetwc, fgetws, fputwc, fputws, fwide, fwprintf, fwscanf, getwc, getwchar, putwc, putwchar, swprintf, swscanf, ungetwc, vfwprintf, vfwscanf, vswprintf, vswscanf, vwprintf, vwscanf, wprintf, wscanf; wcscat, wcschr, wcscmp, wcscoll, wcscpy, wcscspn, wcsftime, wcslen, wcsncat, wcsncmp, wcsncpy, wcspbrk, wcsrchr, wcsspn, wcsstr, wcstod, wcstof, wcstold, wcstok, wcstol, wcstoll, wcstoul, wcstoull, wcsxfrm, wmemchr, wmemcmp, wmemcpy, wmemmove, wmemset; the types, macros and namespace.
- `<uchar.h>` (new in Issue 8): c16rtomb, c32rtomb, mbrtoc16, mbrtoc32, char16_t, char32_t.
- `<wctype.h>`: iswalnum ... iswxdigit, iswctype, wctype, towlower, towupper, towctrans, wctrans, in the C locale and C.UTF-8.
- XSH 2.5 and 2.5.2 (stream orientation and encoding rules), and the backlog item from cross-cutting.md: the 256 single-byte characters of the POSIX locale (XBD 6.2).

**Out of scope.**
- mbrtoc8 and c8rtomb: C23 additions that POSIX.1-2024's `<uchar.h>` doesn't have. Checked only as a namespace item.
- The `_l` forms of `<wctype.h>`: the locale unit (round 8) did them. The wide-character functions removed from `<wchar.h>` by Issue 8 (wctype_t and the isw* functions there) are namespace-only; wctype_t ends in `_t`, so its presence is allowed.
- wcstoimax and wcstoumax are `<inttypes.h>` (stdlib unit).

**Implementation.**
- `libc/stdlib/` (mbrtowc, mbrlen, mbsinit, mbsrtowcs, mbsnrtowcs, wcrtomb, wcsrtombs, wcsnrtombs, btowc, wctob, mbtowc_r.c, wctomb_r.c), `libc/string/wcs*`, `libc/string/wmem*`, `libc/string/wcwidth.c`, `libc/uchar/`, `libc/ctype/isw*`, `tow*`, `wctype.c`, `wctrans.c`, and the wide stdio in `libc/stdio/` (fgetwc, fputwc, fgetws, fputws, ungetwc, fwide, vfwprintf and vfwscanf as `vfprintf.c` and `vfscanf.c` with WIDE_CHARS, swprintf and the wide string streams, open_wmemstream).
- Headers: `libc/include/wchar.h`, `uchar.h`, `wctype.h`, `stdio.h` (FILE's flags and push-back slot), `libc/stdio/local-stdio.h`.

**Configurations.** The default Linux configurations have no mb-capable support: only the C locale, whose characters are ASCII. As the locale unit did, I added `builds/build-aarch64-mb` (`-Dmb-capable=true -Dmb-extended-charsets=true`, aarch64) for UTF-8. The tests find C.UTF-8 at run time with `setlocale(LC_CTYPE, "C.UTF-8")` and print what they skip. The `-native` glibc builds have C.UTF-8 too, so they run the UTF-8 checks as well.

**Tests.** New directory `test/test-posix/test-uchar/` and new files in `test/test-posix/test-wchar/`. File names below drop the `test-` prefix; line numbers are for the branch tip.
- test-wchar: wchar_h.c, mbrtowc.c, mbsrtowcs.c, wcsnlen.c, wcwidth.c, fwide.c, fputwc.c, towctrans.c, wcstol.c (all new); open_wmemstream.c (existing).
- test-uchar: mbrtoc16.c (new).
- test-stdio/freopen.c (stdio unit's test): its orientation check now expects 0 after freopen.
- The `<wctype.h>` test (towctrans.c) is in test-wchar, not a new test-wctype directory, because the locale unit's patches create test-posix/test-wctype and aren't in this base; two units creating the same meson.build would not apply with `git am`. The coordinator may move it after the merge.
- Existing tests relied on: test/test-uchar.c (round trips through mbrtoc8/16/32 and back), test/test-wctomb.c, test/test-ctype/ (classification tables, test-wcase), test/test-stdio/test-wchar.c (fputws/fgetws round trip), test/test-wcsftime.c, test/libc-testsuite/wcstol.c, test/testsuite/newlib.wctype/twctrans.c, test/testsuite/newlib.stdio/swprintf.c, test/test-stdio/test-printf.c and test-printf-scanf.c (wide printf and scanf formats), test/test-posix/test-stdio/test-fgetc.c (EBADF for fgetwc and fputwc on the wrong direction).

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799: basedefs/wchar.h, uchar.h, wctype.h; functions/ for each interface above; XBD 6.2 (Character Encoding); XSH 2.5 and 2.5.2 (Standard I/O Streams, Stream Orientation and Encoding Rules).

**Status key.** TESTED (file:line), UNTESTED, NOT TESTABLE (reason). "(new)" means added by this unit; "(existing)" means an earlier test covers it. "(mb)" means the check runs only where C.UTF-8 exists (the mb build and the `-native` builds); elsewhere it is skipped at run time. Unless marked, TESTED rows ran on aarch64-linux-gnu and arm-linux-gnueabihf for picolibc and for the `-native` glibc build.

## Summary

- 123 requirements (plus one permission). 9 were covered, at least in part, by earlier tests (the round trips in test-uchar.c and test-wctomb.c, the ctype tables, the wide printf/scanf format tests, fputws/fgetws, wcstol, wcsftime, open_wmemstream, EBADF for the wrong direction). Now 107 are tested: 102 by running, 5 inferred from the code; 7 are untested and 9 not testable.
- 14 deviations fixed, each proved by a test that fails with only the library change reverted (one, mbrtoc32 with a null s, depends on an uninitialized value and couldn't be shown failing; its sibling checks were).
- Left: the POSIX locale's 256 single-byte characters (recommendation below), no per-stream mbstate_t or per-stream encoding rule, psignal/psiginfo orienting an unoriented stderr, and three smaller items.

## Requirement table

### `<wchar.h>` header

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | Types FILE, locale_t, mbstate_t, size_t, va_list, wchar_t, wint_t; struct tm declared | TESTED (new) | wchar_h.c:78-85 |
| H2 | wint_t holds every wchar_t and WEOF; no wider than long (in the default environment) | TESTED (new) | wchar_h.c:88-89, 95 |
| H3 | WCHAR_MAX, WCHAR_MIN, WEOF (a wint_t constant expression), NULL; usable in `#if` | TESTED (new) | wchar_h.c:91-101 |
| H4 | Every synopsis function declared with its type | TESTED (new) | wchar_h.c:106-186 (`_Generic` on the designator); wcwidth.c:60-61 (XSI) |
| H5 | Each is a function even where a macro exists (wcschr, wcspbrk, wcsrchr, wcsstr, wmemchr) | TESTED (new) | wchar_h.c:182-186, 212-221 |
| H6 | Strict POSIX doesn't declare the XSI wcwidth/wcswidth or GNU/BSD extensions, nor free | TESTED (new) | wchar_h.c:52, 195-201. **Deviation 6, fixed** (free) |
| H7 | Parameter names don't collide with application macros | TESTED (new) | wchar_h.c:46-62. **Deviation 6, fixed** |
| H8 | wcslcat and wcslcpy take restrict pointers | TESTED (inferred) | header read; parameter qualifiers don't affect type compatibility, so `_Generic` can't see it. Fixed with deviation 6 |
| H9 | "Arguments can point to arrays containing wchar_t values that do not correspond to members of the character set"; processed as specified | TESTED (new) | wcstol.c:153-167, wcsnlen.c (the string functions don't look at the values) |
| H10 | Inclusion may make `<ctype.h>`, `<string.h>`, ... symbols visible | n/a | a permission |

### Restartable conversions (mbrtowc, mbrlen, wcrtomb, mbsinit, btowc, wctob)

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | mbrtowc: returns 0 for the null character (stored, initial state), 1..n for a complete character, -2 for an incomplete one with all n bytes processed, -1 with EILSEQ for an encoding error | TESTED (new) | mbrtowc.c:84-92, 160-197 (mb) |
| R2 | mbrtowc with a null pwc converts without storing | TESTED (new) | mbrtowc.c:88 |
| R3 | mbrtowc with a null s is mbrtowc(NULL, "", 1, ps); pwc and n ignored | TESTED (new) | mbrtowc.c:94-97 |
| R4 | The mbstate_t carries a partial character between calls; the return counts the bytes that complete it | TESTED (new; mb) | mbrtowc.c:165-177 (two, three and four-byte characters split) |
| R5 | A null ps uses the function's own internal object, initially the initial state | TESTED (new) | mbrtowc.c:122-127; (mb) 220-229 |
| R6 | "as if no function defined in this volume calls mbrtowc()" (nor mbrlen, wcrtomb); each has its own internal state | TESTED (new; mb) | mbrtowc.c:220-229. **Deviation 2, fixed** (mbrlen used mbrtowc's state) |
| R7 | mbrtowc, mbrlen and wcrtomb don't change errno on success | TESTED (new) | mbrtowc.c:89, 103, 112, 178, 205 |
| R8 | mbrlen(s, n, ps) is mbrtowc(NULL, s, n, ps or its own internal state) | TESTED (new) | mbrtowc.c:100-102, 176-177, 197 |
| R9 | wcrtomb stores the bytes (at most MB_CUR_MAX); L'\0' stores a null byte and leaves the initial state | TESTED (new) | mbrtowc.c:106-109, 202-204 (mb) |
| R10 | wcrtomb with a null s is wcrtomb(buf, L'\0', ps) | TESTED (new) | mbrtowc.c:111 |
| R11 | wcrtomb of an invalid wide character: -1, EILSEQ | TESTED (new) | mbrtowc.c:116-119 (C locale, U+10000), 207-208 (mb, a surrogate) |
| R12 | mbsinit: non-zero for a null pointer or the initial state, zero mid-character | TESTED (new) | mbrtowc.c:78-79, 87, 166-168 |
| R13 | btowc(EOF) is WEOF; btowc gives the wide character of a single-byte character, WEOF for a byte that isn't one | TESTED (new) | mbrtowc.c:130-135, 212-213 (mb: 0xc3 is not a character) |
| R14 | wctob(WEOF) is EOF; wctob gives the byte of a single-byte character, EOF otherwise; errno unchanged | TESTED (new) | mbrtowc.c:131-139, 214-215 (mb) |
| R15 | The POSIX locale has 256 single-byte characters: btowc doesn't return WEOF for 0-255, and mbrtowc, mbrlen, mbsrtowcs and mbsnrtowcs never give EILSEQ there (XBD 6.2 and the CX text) | UNTESTED | **Deviation 15, not fixed**: picolibc (and glibc 2.43) treat 0x80-0xff as invalid in the C locale. See the recommendation below |
| R16 | EINVAL for an invalid conversion state ("may fail") | NOT TESTABLE | the contents of an invalid mbstate_t are undefined; picolibc doesn't detect it |
| R17 | Null-ps calls need not be thread-safe but avoid data races with other calls | TESTED (inferred) | each internal object is a separate static; read in the code |

### mbsrtowcs, mbsnrtowcs, wcsrtombs, wcsnrtombs

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | Convert up to and including the null, which is stored; return the count without it; *src becomes a null pointer and the state the initial state | TESTED (new) | mbsrtowcs.c:81-86, 107-110, 171-174 (mb) |
| S2 | Stop after len codes (len bytes for wcsrtombs); *src just past the last character converted | TESTED (new) | mbsrtowcs.c:88-92, 111-114 |
| S3 | A character that would exceed len bytes isn't stored (wcsrtombs) | TESTED (new; mb) | mbsrtowcs.c:191-193 |
| S4 | A null dst: nothing stored, len ignored, *src unchanged | TESTED (new) | mbsrtowcs.c:94-96, 104, 115-116, 175-176, 189-190 |
| S5 | mbsnrtowcs reads at most nmc bytes; *src is past the last byte processed; a character cut off by nmc is completed by the next call | TESTED (new) | mbsrtowcs.c:98-104; (mb) 178-184 |
| S6 | wcsnrtombs converts at most nwc wide characters | TESTED (new) | mbsrtowcs.c:118-122 |
| S7 | Encoding error: -1 and EILSEQ; *src past the last character converted | TESTED (new) | mbsrtowcs.c:136-152 (C locale), 196-213 (mb) |
| S8 | A null ps uses each function's own internal state | TESTED (new) | mbsrtowcs.c:125-134, 146-152. **Deviation 1, fixed** (crash without mb-capable) |
| S9 | Each function has its own internal state; "as if no function calls these functions" | TESTED (new; mb) | mbsrtowcs.c:215-226. **Deviation 2, fixed** (mbsrtowcs shared mbsnrtowcs's state, wcsrtombs wcsnrtombs's). strftime calling mbsnrtowcs with a null ps is left (below) |
| S10 | errno unchanged on success | TESTED (new) | mbsrtowcs.c:123, 134, 194 |
| S11 | EINVAL for an invalid state | NOT TESTABLE | as R16 |

### POSIX string additions

| # | Requirement | Status | Reference |
|---|---|---|---|
| P1 | wcsnlen: the smaller of the length and maxlen | TESTED (new) | wcsnlen.c:80-84 |
| P2 | wcsnlen "shall never examine more than the first maxlen characters" | TESTED (new) | wcsnlen.c:88-103 (guard page after the array). **Deviation 4, fixed** |
| P3 | wcpcpy copies with the null and returns the address of the null | TESTED (new) | wcsnlen.c:118-122 |
| P4 | wcpncpy copies at most n, pads with nulls to n, returns the first null written or &ws1[n] | TESTED (new) | wcsnlen.c:124-136 |
| P5 | wcsdup returns a malloc'd copy that free accepts | TESTED (new) | wcsnlen.c:155-160 |
| P6 | wcsdup: null pointer and ENOMEM when it can't allocate | UNTESTED | read: malloc's errno passes through. Exhausting memory reliably needs the volatile-malloc trick (strings unit); not done |
| P7 | wcslcpy copies at most dstsize-1, terminates unless dstsize is 0, returns wcslen(src) | TESTED (new) | wcsnlen.c:138-144 |
| P8 | wcslcat appends at most dstsize-wcslen(dst)-1, no null at or beyond dst+dstsize, returns the initial length of dst plus wcslen(src) | TESTED (new) | wcsnlen.c:145-152 |
| P9 | wcscasecmp, wcsncasecmp and the `_l` forms: in the POSIX locale, as if both were converted to lowercase; n limits | TESTED (new) | wcsnlen.c:172-185 |
| P10 | wcscoll_l, wcsxfrm_l use their locale (POSIX: by code); wcsxfrm with n 0 and a null ws1 returns the length; transformed strings compare as wcscoll | TESTED (new) | wcsnlen.c:187-198 |
| P11 | The copies, the comparisons and wcsnlen don't change errno | TESTED (new) | wcsnlen.c:153, 200 |
| P12 | wcscasecmp_l and friends with LC_GLOBAL_LOCALE or an invalid locale | NOT TESTABLE | undefined |
| P13 | wcscasecmp etc. outside the POSIX locale | NOT TESTABLE | unspecified |

### wcwidth and wcswidth (XSI)

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | wcwidth: 0 for the null character, the column count for a printable character, -1 for one that isn't printable | TESTED (new) | wcwidth.c:69-74, 91-101. **Deviation 5, fixed** (controls gave 0 without mb-capable; non-ASCII had widths in the C locale with it) |
| X2 | wcswidth: 0 for an empty string, the sum to the null or n, -1 if any of the first n isn't printable | TESTED (new) | wcwidth.c:76-81, 101 |
| X3 | Neither changes errno on valid input | TESTED (new) | wcwidth.c:82 |
| X4 | Declared only with `_XOPEN_SOURCE` | TESTED (new) | wcwidth.c:60-61; wchar_h.c:195-196 |

### Stream orientation and fwide (XSH 2.5.2)

| # | Requirement | Status | Reference |
|---|---|---|---|
| O1 | A new stream has no orientation; stdin, stdout and stderr are unoriented at start-up | TESTED (new) | fwide.c:74, 86-89. **Deviation 11, fixed** |
| O2 | The first byte I/O function makes a stream byte-oriented (fputc, fputs, fprintf, fwrite, fgetc, fgets, fread, ungetc) | TESTED (new) | fwide.c:106-139 (ungetc not checked for glibc) |
| O3 | The first wide-character I/O function makes it wide-oriented (fputwc, fputws, fwprintf, fgetwc, ungetwc) | TESTED (new) | fwide.c:141-159 |
| O4 | Other operations (fflush, fseek, ftell, clearerr, ferror, feof) don't orient | TESTED (new) | fwide.c:161-168 |
| O5 | fwide(f, 0) doesn't change it; fwide with a mode sets it only on an unoriented stream; returns >0, <0 or 0 | TESTED (new) | fwide.c:92-104. **Deviation 11, fixed** |
| O6 | fwide doesn't change errno on success | TESTED (new) | fwide.c:94-96 |
| O7 | freopen removes the orientation | TESTED (new) | fwide.c:170-186; stdio/freopen.c:110 |
| O8 | perror, psignal and psiginfo don't orient an unoriented stream | TESTED (new) for perror | fwide.c:188-194 (not glibc). psignal and psiginfo: **deviation 16, not fixed** |
| O9 | perror/psignal on a wide-oriented stream behave as wide output | TESTED (existing) for psignal | test-signal/psiginfo.c:115-130 (stays wide, text right). Bytes are written, which is the same for portable text (read) |
| O10 | fwide EBADF for an invalid stream ("may fail") | NOT TESTABLE | an invalid FILE pointer is undefined in practice |
| O11 | Each wide-oriented stream has its own mbstate_t; fgetpos/fsetpos save and restore it | UNTESTED | **Deviation 17, not fixed**: FILE has no mbstate_t. Not observable for stateless encodings (UTF-8, the single-byte charsets) |
| O12 | The encoding rule is fixed by LC_CTYPE when the stream becomes wide-oriented | UNTESTED | **Deviation 17, not fixed**: picolibc converts with the LC_CTYPE in effect at each call (mb builds) |

### Wide-character stream I/O

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | Wide output converts as if by wcrtomb: fputwc, fputws and fwprintf write each character's multibyte encoding | TESTED (new) | fputwc.c:146-153; (mb) 222-227. **Deviation 10, fixed** (raw wchar_t bytes were written) |
| W2 | Wide input converts as if by mbrtowc: fgetwc and fgetws read multibyte characters | TESTED (new) | fputwc.c:177-189; (mb) 233-238. **Deviation 10, fixed** |
| W3 | fputwc of a code that isn't a character: WEOF, EILSEQ, and (CX) the error indicator | TESTED (new) | fputwc.c:155-162 (not glibc); (mb) 229-232 (not glibc) |
| W4 | fputws: -1, errno and the error indicator on failure | TESTED (new) | fputwc.c:164-168 (not glibc) |
| W5 | fwprintf: negative and EILSEQ for an invalid wide character | TESTED (new) | fputwc.c:169-171 (not glibc) |
| W6 | fputwc write error: WEOF, errno, error indicator | TESTED (new) | fputwc.c:278-291 (/dev/full, unbuffered, ENOSPC). **Deviation 8, fixed** |
| W7 | fgetwc of bytes that aren't a character: WEOF, EILSEQ, (CX) the error indicator | TESTED (new; mb) | fputwc.c:244-249 |
| W8 | fgetwc at end-of-file in the middle of a character: an encoding error (ISO C "including too few bytes") | TESTED (new; mb) | fputwc.c:250-257 (errno not checked for glibc). Fixed with deviation 10 |
| W9 | fgetwc at end-of-file: WEOF and the end-of-file indicator; errno unchanged on success | TESTED (new) | fputwc.c:188, 192, 238 |
| W10 | fgetws: up to the newline or n-1, null-terminated; a partial last line at end-of-file is returned; NULL at end-of-file | TESTED (new) | fputwc.c:186-189; test-stdio/test-wchar.c (existing) |
| W11 | fgetws after an error: the error indicator and a null pointer | TESTED (new; mb) | fputwc.c:259-264. **Deviation 13, fixed** |
| W12 | ungetwc(WEOF) fails and leaves the stream alone | TESTED (new) | fputwc.c:180-182 |
| W13 | ungetwc pushes back one character, which the next read returns; clears end-of-file; the position afterwards is as before | TESTED (new) | fputwc.c:183-191 |
| W14 | At least one character of push-back for any wide character | TESTED (new; mb) | fputwc.c:239-241 (U+1F600). **Deviation 12, fixed** |
| W15 | EBADF for a stream not open for reading or writing | TESTED (existing) | test-stdio/test-fgetc.c:105-109 |
| W16 | swprintf: negative and (CX) EOVERFLOW when n or more characters are needed, including n 0 | TESTED (new) | fputwc.c:195-211 (errno not glibc). **Deviation 14, fixed** |
| W17 | The wide printf and scanf conversions | TESTED (existing) | test-stdio/test-printf.c, test-printf-scanf.c, newlib.stdio/swprintf.c |
| W18 | open_wmemstream: wide characters in the buffer, size in wide characters | TESTED (existing) | open_wmemstream.c |
| W19 | fwprintf EOVERFLOW past INT_MAX | UNTESTED | needs INT_MAX characters of output; the stdio unit covered the byte path (same code) |
| W20 | EAGAIN, EINTR, EIO, EFBIG, EPIPE, ENXIO, ENOMEM, EOVERFLOW for the wide functions | UNTESTED (inferred) | they share the byte functions' descriptor paths, which the stdio unit tested; only ENOSPC was run here |
| W21 | Timestamps marked for update | NOT TESTABLE | the stdio unit's reasoning: timestamp granularity |

### ISO C wide functions (CX requirements)

| # | Requirement | Status | Reference |
|---|---|---|---|
| I1 | wcstol, wcstoul, wcstoll, wcstoull: value and endptr; 0 and endptr = nptr when nothing converts | TESTED (new) | wcstol.c:68-77; libc-testsuite/wcstol.c (existing) |
| I2 | (CX) EINVAL for an unsupported base | TESTED (new) | wcstol.c:79-85 |
| I3 | ERANGE with LONG_MIN/LONG_MAX/LLONG_MIN/ULONG_MAX/ULLONG_MAX | TESTED (new) | wcstol.c:87-104 |
| I4 | wcstod, wcstof: value and endptr; overflow gives ±HUGE_VAL and ERANGE; (CX) underflow gives ERANGE | TESTED (new) | wcstol.c:108-124 |
| I5 | wcstold | TESTED (inferred) | shares wcstod's code (read); prototype in wchar_h.c:175 |
| I6 | wcstok keeps its place in the third argument; NULL when no token is left | TESTED (new) | wcstol.c:134-144 |
| I7 | wcsftime: the count without the null, 0 when it doesn't fit | TESTED (new) | wcstol.c:146-149; test-wcsftime.c (existing, formats) |
| I8 | wcscat, wcsncat, wcsncmp, wcsncpy, wcsspn, wcscspn, wcschr, wcsrchr, wcsstr, wmemcmp, wmemmove, wmemset, wmemchr; errno unchanged | TESTED (new) | wcstol.c:151-167, wchar_h.c:212-223 |
| I9 | wcscoll and wcsxfrm in the C locale | TESTED (new) | wcsnlen.c:187-198 |
| I10 | wcscoll, wcsxfrm EINVAL outside the collating sequence ("may fail") | NOT TESTABLE | picolibc collates by code; every value is in the domain |
| I11 | getwc, getwchar, putwc, putwchar, wprintf, wscanf, vwprintf, vwscanf, vfwscanf, vswscanf | TESTED (inferred) | aliases or thin wrappers of the tested functions (read); getwchar/putwchar/wprintf on the standard streams are not run (the test harness owns stdout) |

### `<wctype.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | wint_t, wctrans_t, wctype_t, locale_t, WEOF; prototypes | TESTED (new) | towctrans.c:60-84 |
| C2 | The isw* functions give the LC_CTYPE classes; in the POSIX locale the portable characters have the `<ctype.h>` classes | TESTED (new) | towctrans.c:125-137 |
| C3 | The POSIX locale classifies characters outside the portable set only as cntrl or punct, if at all | TESTED (new) | towctrans.c:160-167 |
| C4 | WEOF has no class; towlower/towupper keep WEOF | TESTED (new) | towctrans.c:129-130, 147 |
| C5 | wctype gives non-zero for the 12 standard names, 0 for an unknown one; iswctype(wc, wctype(name)) matches the function; iswctype(wc, 0) is 0 | TESTED (new) | towctrans.c:120-136 |
| C6 | wctrans("tolower"/"toupper") non-zero, unknown name 0; towctrans maps by the descriptor; towctrans(wc, 0) returns wc | TESTED (new) | towctrans.c:117-145 |
| C7 | towlower/towupper in the POSIX locale map only A-Z/a-z | TESTED (new) | towctrans.c:139-142, 168-169 |
| C8 | iswdigit and iswxdigit are the ASCII digits only | TESTED (new) | towctrans.c:150 |
| C9 | The functions follow LC_CTYPE (C.UTF-8: é is alpha and lower, case maps to É; U+3000 is space; ¿ is punct) | TESTED (new; mb) | towctrans.c:171-181; test-ctype (existing tables) |
| C10 | errno unchanged by the classification and mapping functions | TESTED (new) | towctrans.c:170, 180 |
| C11 | wctype/wctrans "may set errno" (EINVAL) for an unknown name | TESTED (inferred) | allowed either way; picolibc sets EINVAL (seen while writing the test) |
| C12 | Arguments that are neither WEOF nor a wchar_t value | NOT TESTABLE | undefined |

### `<uchar.h>` (new in Issue 8)

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | mbstate_t, size_t; char16_t is uint_least16_t and char32_t is uint_least32_t | TESTED (new) | mbrtoc16.c:81-87 |
| U2 | c16rtomb, c32rtomb, mbrtoc16, mbrtoc32 prototypes | TESTED (new) | mbrtoc16.c:89-93 |
| U3 | No plain parameter names | TESTED (new) | mbrtoc16.c:38-54. **Deviation 7, fixed** |
| U4 | mbrtoc8, c8rtomb (C23) not declared in strict C17 mode | TESTED (new) | mbrtoc16.c:95-99. **Deviation 7, fixed** |
| U5 | mbrtoc16/mbrtoc32 return values as for mbrtowc; null pc16/pc32 | TESTED (new) | mbrtoc16.c:119-123; (mb) 172-174 |
| U6 | mbrtoc16 gives a surrogate pair, the second as (size_t)-3 | TESTED (new; mb) | mbrtoc16.c:177-182; test-uchar.c (existing) |
| U7 | mbrtoc16/mbrtoc32 with a null s are calls with "" and 1, nothing stored | TESTED (new) | mbrtoc16.c:128-135. **Deviation 3, fixed** |
| U8 | c16rtomb/c32rtomb with a null s are calls with a null character | TESTED (new) | mbrtoc16.c:137-140. **Deviation 3, fixed** |
| U9 | c16rtomb takes surrogate pairs (0 stored for the high one); lone or unpaired surrogates are EILSEQ | TESTED (new; mb) | mbrtoc16.c:184-204 |
| U10 | c32rtomb EILSEQ for a surrogate or a value beyond U+10FFFF | TESTED (new) | mbrtoc16.c:149-156 |
| U11 | Each function has its own internal state for a null ps | TESTED (new) | mbrtoc16.c:142-147; (mb) 206-211 |
| U12 | EINVAL for an invalid state | NOT TESTABLE | as R16 |

### Backlog: the POSIX locale's 256 characters

| # | Requirement | Status | Reference |
|---|---|---|---|
| B1 | XBD 6.2: "The POSIX locale shall contain 256 single-byte characters" | UNTESTED | **Deviation 15, not fixed**; recommendation below |

## Deviations found and fixed

Each fix was proved by reversing only its library change (`git show <fix> -- libc libos | git apply -R -C1`; where later commits rewrote the same lines, I removed the change by hand), rebuilding the named test and seeing the failure quoted. "(mb)" means the proof ran in build-aarch64-mb.

1. **mbsnrtowcs and wcsnrtombs dereferenced a null ps without mb-capable** (libc/stdlib/mbsnrtowcs.c, wcsnrtombs.c). Their internal state existed only with `__MB_CAPABLE`, so in the default Linux builds `wcsrtombs(dst, &src, n, NULL)` always crashed, and `mbsrtowcs(..., NULL)` crashed on an invalid byte. The internal state is now there in every configuration. Failing: test-mbsrtowcs killed by SIGSEGV.
2. **mbrlen, mbsrtowcs and wcsrtombs shared another function's internal state** (mbrlen.c, mbsrtowcs.c, wcsrtombs.c). With a null ps, mbrlen used mbrtowc's object, mbsrtowcs mbsnrtowcs's, wcsrtombs wcsnrtombs's, so a partial UTF-8 character left by one was finished by the other. POSIX gives each "its own internal mbstate_t object" and says no function calls them. Failing (mb): `mbrlen's own state: mbrlen("\xa9", 1, NULL) == (size_t)-1`, `mbsrtowcs's own state: ...`.
3. **mbrtoc16, mbrtoc32, c16rtomb and c32rtomb with a null s** (libc/uchar/). mbrtoc16 and mbrtoc32 read the wide character mbrtowc never stored: mbrtoc16 wrote it to *pc16 (and could start a surrogate pair), mbrtoc32 could fail with EILSEQ. c16rtomb and c32rtomb used their character argument, so `c32rtomb(NULL, 0xd800, ps)` failed and `c16rtomb(NULL, 0xd800, ps)` left a pending surrogate. ISO C: a null s means `mbrtoc16(NULL, "", 1, ps)` and `c16rtomb(buf, 0, ps)`. Failing: `mbrtoc16 with a null s stores nothing: c16 == 0x1234`; with only c16rtomb reverted, `c16rtomb with a null s ignores c16`; with only c32rtomb, `c32rtomb with a null s ignores c32`. The mbrtoc32 half reads an uninitialized value and passed even reverted on this host; it is fixed by reading.
4. **wcsnlen read ws[maxlen]** (libc/string/wcsnlen.c) before checking the count. "The wcsnlen() function shall never examine more than the first maxlen characters." Failing: test-wcsnlen killed by SIGSEGV at the guard page.
5. **wcwidth gave widths to characters that aren't printable** (libc/string/wcwidth.c). Without mb-capable, control characters gave 0; with it, every character from U+00A0 up had a width in the C locale, where none is printable. POSIX: -1 "if wc does not correspond to a printable wide-character code"; wcswidth follows. Failing: `wcwidth of a control character` (default build); (mb) `wcwidth of U+00E9 in the C locale`.
6. **`<wchar.h>` declared free and named parameters** (ungetwc's wc; wcschr, wcspbrk, wcsrchr, wcsstr and wmemchr's s, c, a, h, n). free is reserved only with `<stdlib.h>`. wcsdup's attribute now names `__builtin_free` (as the strings unit did for `<string.h>` through `__malloc_like`), the names are gone, and wcslcat/wcslcpy have the synopsis's restrict. Failing: test-wchar_h doesn't compile (`wchar.h:163: expected ... before '*' token`, `cdefs.h:292: expected ')' before numeric constant`).
7. **`<uchar.h>` named every parameter, and declared mbrtoc8, c8rtomb and char8_t in strict C17 mode.** They are now unnamed, and the C23 names need C23 or `_DEFAULT_SOURCE`. Failing: test-mbrtoc16 doesn't compile (`uchar.h:65: expected ... before 'c8'`).
8. **fputwc didn't set the error indicator after a write error** (libc/stdio/fputwc.c). POSIX: "for errors other than [EILSEQ] the error indicator for the stream shall be set". Failing: `fputwc ENOSPC: fputwc(L'a', f) == WEOF && ferror(f) && errno == ENOSPC`.
9. **(Preparation, no behaviour change) FILE's flags became 16 bits** (stdio.h). All eight bits were used. The wider field takes padding that already follows it wherever pointers are at least 2-byte aligned; on AVR FILE grows by one byte.
10. **Wide stream I/O moved raw wchar_t bytes** (fgetwc.c, fputwc.c). `fputwc(L'A', f)` wrote four bytes to a file or terminal, and fgetwc read four. XSH 2.5: "Each conversion shall occur as if by a call to the mbrtowc() function" (wcrtomb for output). fputwc now writes the multibyte encoding and fgetwc reads bytes until mbrtowc completes a character. An invalid sequence, or end-of-file in the middle of a character, sets the error indicator and EILSEQ; a wide character the locale can't encode does the same on output. swprintf, swscanf and open_wmemstream keep storing wchar_t values; a new `__SWCHAR` flag marks those streams. fputws, fgetws, vfwprintf and vfwscanf go through these functions, so they convert too. Failing: `single bytes written: written(f, bytes, sizeof(bytes)) == 6 && memcmp(bytes, "Abc4de", 6) == 0` (default and mb).
11. **Streams had no unoriented state** (fwide.c and the byte and wide functions). A stream was byte-oriented from the start (fwide(f, 0) gave -1 on a new stream and on stdin), fwide(f, 1) made a stream wide after byte I/O, and fwide(f, -1) did the reverse. POSIX: streams start unoriented, the first byte or wide I/O function sets the orientation, fwide changes only an unoriented stream, and returns 0 for none. A new `__SBYTE` flag records byte orientation; getc, putc, fread, fwrite, fputs, puts, ungetc and the byte printf and scanf set it, the wide functions set `__SWIDE`, freopen clears both, and perror restores an unoriented stderr. This answers the stdio unit's note. Failing: `standard streams unoriented: in == 0 && out == 0 && err == 0`.
12. **ungetwc truncated characters above U+FFFE** (stdio.h). The push-back slot held 16 bits, so `ungetwc(0x1f600, f)` succeeded and fgetwc then returned 0xf600. With `__MB_CAPABLE` and a 4-byte wchar_t the slot is now 32 bits (as RISC-V, MicroBlaze and LoongArch already had); FILE grows by four bytes on 32-bit targets in that configuration only, keyed on picolibc.h. Failing (mb): `ungetwc of U+1F600`.
13. **fgetws returned a partial line after a read or encoding error** (fgetws.c). POSIX: "If an error occurs, the error indicator for the stream shall be set, and fgetws() shall return a null pointer". Failing (mb): `fgetws EILSEQ: fgetws(line, 16, f) == NULL && errno == EILSEQ ...`.
14. **swprintf didn't set EOVERFLOW** (filewstrput.c, swprintf.c, vswprintf.c). It returned -1 with errno unchanged when the output didn't fit, and EBADF for n 0. POSIX: "[EOVERFLOW] The number of wide characters requested to be written was n or more." Failing: `swprintf EOVERFLOW: errno == EOVERFLOW`.

## Deviations not fixed, and things noted

15. **The POSIX locale has 128 characters in picolibc, not 256** (XBD 6.2; btowc, mbrtowc, mbrlen, mbsrtowcs: "In the POSIX locale an [EILSEQ] error cannot occur since all byte values are valid characters"; btowc "shall not return WEOF if c has a value in the range 0 to 255"). `__ascii_mbtowc` and `__ascii_wctomb` (libc/stdlib/mbtowc_r.c, wctomb_r.c) reject 0x80-0xff, deliberately since f929c8fde6 ("In ASCII locale, mbtowc for c >= 0x80 should fail", January 2025), and test/test-ctype/test-ctype.c:193 checks that mbrtowc fails there. glibc 2.43 behaves the same (verified). **Recommendation:** map bytes 0x80-0xff to wide characters in the C locale, in both directions, with no classes (iswalpha etc. already return 0 there) and wcwidth -1. Two choices of wide value:
    - U+0080-U+00FF (what picolibc's non-mb mbstowcs did until the stdlib unit replaced it): the simplest code, but `fputwc` in a UTF-8 locale then writes those characters as Latin-1 letters.
    - musl's U+DF80-U+DFFF (lone surrogates that can't be anything else): the bytes round-trip through wide strings unchanged, and nothing mistakes them for real characters.
    Either one changes mbtowc, wctomb, mbrtowc, wcrtomb, btowc, wctob and the mbs/wcs conversions in the C locale, test-ctype's check, and iconv's "ASCII" codeset and nl_langinfo(CODESET) ("ASCII" would no longer describe the locale). It's a behaviour and documentation decision, not ABI: no type or layout changes. Left for the maintainer.
16. **psignal and psiginfo make an unoriented stderr byte-oriented.** XSH 2.5: they "shall not change the orientation of the stream". perror is fixed here; psignal is libc/signal/psignal.c, which the strings unit changed (not in this base), so I left it to avoid a conflict. The fix is perror's three lines (needs local-stdio.h's flags).
17. **No per-stream conversion state or encoding rule** (XSH 2.5.2). FILE has no mbstate_t, so each fgetwc/fputwc starts from the initial state, fgetpos/fsetpos don't save one, and the stream uses the LC_CTYPE of each call rather than the one in effect when it became wide-oriented. For UTF-8 and the single-byte charsets with a 4-byte wchar_t (every Linux target) nothing is lost between characters. It is wrong for the stateful JIS charset (mb-extended-charsets) and for UTF-16 surrogate pairs where wchar_t is 2 bytes. Fixing it means an mbstate_t (and a locale) in FILE: an ABI decision.
18. **strftime calls mbsnrtowcs with a null ps** (libc/time/strftime.c:509-551, era parsing), so it uses mbsnrtowcs's internal state, which "no function" may do. Only reachable with ERA data, which picolibc's locales don't have, so it can't be shown; a local mbstate_t would fix it. For the time unit's area.
19. **2-byte wchar_t: ungetwc(0xffff) is lost** (the slot stores c + 1). No picolibc Linux target has a 2-byte wchar_t; with io-wchar but without mb-capable, swscanf's push-back of a wide value above 0xfffe is also truncated (16-bit slot). Neither is reachable on this host.
20. Byte output functions on a wide-oriented stream write bytes (perror, psignal on a wide stderr). POSIX says they behave as wide output; for their ASCII text the bytes are the same.
21. **ABI notes for the merge:** FILE's flags are 16 bits (AVR: +1 byte); FILE's push-back slot is 32 bits with mb-capable and a 4-byte wchar_t (+4 bytes on 32-bit targets in that configuration); new FILE flags `__SBYTE` (0x0100) and `__SWCHAR` (0x0200). Code that builds `FDEV_SETUP_STREAM` FILEs by hand is unaffected; wide streams built by hand with `__SWIDE` meaning "raw wchar_t" (as swprintf did) need `__SWCHAR` too.
22. glibc 2.43 gaps met here (the tests guard them with `__GLIBC__`): ungetc and perror orient a stream the wrong way (ungetc doesn't orient; perror makes stderr byte-oriented); fmemopen streams start byte-oriented; fputwc writes something for characters the locale can't encode (U+10000 in the C locale, a surrogate in UTF-8) and reports success; fgetwc treats a character cut off by end-of-file as end-of-file without EILSEQ; encoding errors don't set the error indicator; swprintf doesn't set EOVERFLOW; wcslcpy and wcslcat need `_DEFAULT_SOURCE`.

## Left undone

- R15/B1 (the 256-character POSIX locale): recommendation above.
- P6 (wcsdup ENOMEM), W19, W20: inferred from shared code, not run.
- The x86_64 host should run test-fwide, test-fputwc and test-mbrtowc on bare metal: they use fmemopen there (no file system), and test-fwide's standard-stream check reads the semihost FILE shared by stdin, stdout and stderr.
- The UCS-2/UCS-4 mbtowc return of -1 for a truncated character (from the locale unit's cross-cutting note) is iconv-only (`__MB_EXTENDED_CHARSETS_UCS` isn't an LC_CTYPE charset), so I left it to whoever next touches iconv.

## Test runs (verified by running)

| Build | Ok | Fail | Expected Fail | Skipped |
|---|---|---|---|---|
| aarch64-linux-gnu (`-Dwerror=true`, minsize, full `meson test -t 4`) | 1656 | 0 | 6 | 41 |
| arm-linux-gnueabihf (full) | 1654 | 0 | 6 | 44 |
| aarch64-linux-gnu with `-Dmb-capable=true -Dmb-extended-charsets=true` (full) | 1658 | 0 | 6 | 40 |
| clang native (`CC=clang CXX=clang++ do-native-configure -Dwerror=true`, full) | 1266 | 0 | 6 | 397 |

- gcc native configuration (`do-native-configure -Dwerror=true`, `ninja` only): builds cleanly.
- CMake (`do-cmake-thumbv7m-configure`, `ninja test`): 657 tests, 100% passed (the ten new tests included).
- The base had Ok 1246 for clang native (coordinator's run); the 20 new tests (10 files, each with a `-native` build) account for the difference.
- `clang-format --dry-run -Werror` is clean on every changed C file and header.
- The fix proofs are listed with each deviation above (builds/build-aarch64-linux-gnu and build-aarch64-mb).
- **Incident:** in an earlier full aarch64 run (before the last test commit), test-accept4-native (glibc, not touched here) was "killed by signal 9 SIGKILL" with no output, the host security software's signature; it passed when rerun alone, and in the final full run.

## Commits (audit/wide, on top of 1bac8ef806)

- ae92eea011 stdlib: Don't dereference a null mbstate_t in mbsnrtowcs and wcsnrtombs
- adea731313 stdlib: Give mbrlen, mbsrtowcs and wcsrtombs their own internal state
- 5cbc7203e5 uchar: Treat a null s as ISO C says in mbrtoc16, mbrtoc32, c16rtomb and c32rtomb
- 8b37399151 wcsnlen: Don't read the character after the first maxlen
- 5271b8f4c7 wcwidth: Return -1 for characters that aren't printable in the locale
- 1ec7efd017 wchar.h: Don't declare free or name parameters
- 7daa5b9bf0 uchar.h: Don't name parameters, and hide the char8_t functions before C23
- fff68e605f stdio: Set the error indicator when fputwc can't write
- 7ad4eaf3c3 stdio.h: Make FILE's flags 16 bits
- df16871c90 stdio: Convert between wide and multibyte characters in wide stream I/O
- 59c2bd866a stdio: Give streams no orientation until a byte or wide function is used
- 3caf7f12df stdio.h: Keep ungetwc's character in 32 bits in mb-capable builds
- 17ff77f580 stdio: Return a null pointer from fgetws after a read or encoding error
- 2b2005393b stdio: Fail with EOVERFLOW when swprintf's output doesn't fit
- 167a9876cd test-posix: Check the <wchar.h> types, macros, prototypes and namespace
- 654a7427b9 test-posix: Check the restartable conversions and their states
- 3ac73b063a test-posix: Check the POSIX wide-string functions and wcwidth
- e2a2711ed3 test-posix: Check stream orientation and fwide
- b446568b73 test-posix: Check wide stream conversion, encoding errors and ungetwc
- 51f878671c test-posix: Check the <wctype.h> classes and mappings
- 708af2308b test-posix: Check <uchar.h> and the char16_t and char32_t conversions
- 8c8893c42d test-posix: Check the wcsto* errors, wcstok, wcsftime and the wcs basics

The patches are in `/local/home/keithpac/arm64-testing/results/audit/wide/`, made with `git format-patch -o ... 1bac8ef806..audit/wide`. Nothing was committed to or pushed from the arm64-testing checkout.

**Merge notes.** This unit adds `test-uchar` to test/test-posix/meson.build and CMakeLists.txt (the locale unit adds test-ctype, test-iconv, test-langinfo, test-locale and test-wctype in the same lists). It edits libc/stdio/perror.c, fputwc.c, fgetwc.c, fgetws.c, fwide.c, freopen.c, ungetc.c, ungetwc.c, fgetc.c, fputc.c, fputs.c, puts.c, fread.c, fwrite.c, vfprintf.c, vfscanf.c, swprintf.c, vswprintf.c, filewstrput.c, open_memstream.c, local-stdio.h and stdio.h, which no round-8 unit touched. `<wchar.h>`'s wcsdup uses `__malloc_like_with_free(__builtin_free, 1)` directly, so it doesn't depend on the strings unit's change to `__malloc_like`; once both are in, it could go back to `__malloc_like`. No file here overlaps the stdlib, strings or locale patches.
