# POSIX.1-2024 audit: `<fnmatch.h>` / `<glob.h>`

Worktree `/agentic/picolibc-project/worktrees/audit-glob`, branch `audit/glob`,
based on `14ebe3b96b`. Nothing pushed.

Spec sources (fetched 2026-11-20 from pubs.opengroup.org/onlinepubs/9799919799):
XBD `<fnmatch.h>`, `<glob.h>`; XSH `fnmatch()`, `glob()`; XCU 2.14 (2.14.1,
2.14.2, 2.14.3); XBD 9.3.5; XBD 4.1 (case-insensitive comparisons, which
`FNM_CASEFOLD` refers to).

Legend: **T** = tested (file:line, line numbers at the branch tip),
**U** = untested, **NT** = not testable (reason). Test files:

- `F` = `test/test-posix/test-fnmatch/test-fnmatch.c`
- `FC` = `test/test-posix/test-fnmatch/test-fnmatch-casefold.c` (new; strict
  `_POSIX_C_SOURCE 202405L`; skipped on glibc, which has no `FNM_IGNORECASE`)
- `G` = `test/test-posix/test-glob/test-glob.c`
- `GE` = `test/test-posix/test-glob/test-glob-errfunc.c` (new)

"Verified" means I ran it: picolibc on x86_64-linux-gnu (and arm bare-metal
for fnmatch), plus the `-native` (glibc 2.43) builds. "Inferred" means from
reading the code only.

## Requirement table

### Headers

| # | Requirement | Spec | Status | Reference |
|---|---|---|---|---|
| H1 | `<fnmatch.h>` defines FNM_NOMATCH, FNM_PATHNAME, FNM_PERIOD, FNM_NOESCAPE | XBD fnmatch.h | T | F:73-78 (non-zero, separate bits) |
| H2 | `<fnmatch.h>` defines FNM_CASEFOLD and FNM_IGNORECASE (Issue 8) | XBD fnmatch.h | T (was a deviation, fixed) | FC:58-59; compile fails before fix |
| H3 | `int fnmatch(const char *, const char *, int)` prototype | XBD fnmatch.h | T | F:71,76 (function pointer of that type) |
| H4 | `glob_t` with `size_t gl_pathc`, `char **gl_pathv`, `size_t gl_offs`; `size_t` defined | XBD glob.h | T | G:161-170 (pointer-type checks) |
| H5 | GLOB_APPEND, GLOB_DOOFFS, GLOB_ERR, GLOB_MARK, GLOB_NOCHECK, GLOB_NOESCAPE, GLOB_NOSORT (OR-able bits) | XBD glob.h | T | G:173-180 |
| H6 | GLOB_ABORTED, GLOB_NOMATCH, GLOB_NOSPACE (non-zero, distinct) | XBD glob.h | T | G:181-184 |
| H7 | `glob()` / `globfree()` prototypes | XBD glob.h | T | G:166-167 |

### fnmatch() and pattern notation (XCU 2.14.1/2.14.2, XBD 9.3.5)

| # | Requirement | Spec | Status | Reference |
|---|---|---|---|---|
| P1 | Ordinary character matches itself (bit pattern) | XCU 2.14.1 | T | F:81-93, 203-207 |
| P2 | `?` matches any single character (not the empty string) | XCU 2.14.1 | T | F:203 |
| P3 | `*` matches any string including the null string; greedy-with-backtracking semantics | XCU 2.14.2 | T | F:205-207 |
| P4 | No limit on the number of `*` in a valid pattern | XCU 2.14.2 | T (was a deviation, fixed) | F:209-220 |
| P5 | Backslash escapes the next character, also inside brackets (FNM_NOESCAPE clear) | XCU 2.14.1, XSH fnmatch | T | F:87, 222-228 |
| P6 | Pattern ending in an unescaped backslash | XSH fnmatch | NT: unspecified | picolibc matches a literal `\` |
| P7 | FNM_NOESCAPE: backslash is ordinary | XSH fnmatch | T | F:89, 230-234 |
| P8 | `[` that does not introduce a valid bracket expression matches itself | XCU 2.14.1 | T | F:121, 153-154, 249-250 |
| P9 | Bracket matching list, ranges | XBD 9.3.5 | T | F:127-131 |
| P10 | Non-matching list with `!` | XCU 2.14.1 | T | F:132, 137, 142 |
| P11 | Bracket starting with `^` | XCU 2.14.1 | NT: unspecified | picolibc treats it like `!` |
| P12 | `]` first in the list (after `!`) is literal | XBD 9.3.5 | T | F:134-138 |
| P13 | `-` first/last is literal; `-` as a range end; `[%--]`, `[--@]`, `[][.-.]-0]` | XBD 9.3.5 | T | F:139-149 |
| P14 | `*`, `?`, `[` lose their meaning inside brackets | XBD 9.3.5 | T | F:150-151 |
| P15 | Collating symbols `[.c.]`, including `[.].]` | XBD 9.3.5 | T | F:156-158 |
| P16 | Range end points may be collating symbols | XBD 9.3.5 | T (was a deviation, fixed) | F:166-170 |
| P17 | Equivalence classes `[=c=]` (C locale: the character itself) | XBD 9.3.5 | T | F:159 |
| P18 | All 12 required character classes | XBD 9.3.5 | T | F:172-200 |
| P19 | Ranges in the POSIX locale follow the collation sequence (byte value) including bytes ≥ 0x80 | XBD 9.3.5, XCU 2.14.1 | T (was a deviation on signed-char targets, fixed) | F:161-165 |
| P20 | Multi-character collating symbol not in the locale is invalid | XBD 9.3.5 | NT: invalid-construct handling is unspecified for patterns | picolibc treats the whole `[` as literal |
| P21 | Unknown class name `[[:foo:]]` | XBD 9.3.5 | NT: undefined | picolibc: matches nothing |
| P22 | `?` matches one *character* in a multibyte locale | XCU 2.14.1 | U — known deviation | F:288-293 is `#ifndef __PICOLIBC__`; fnmatch is byte based |
| F1 | FNM_PATHNAME: `/` only matched by `/`, not by `*`, `?` or a bracket expression | XSH fnmatch | T | F:85, 236-245 |
| F2 | FNM_PATHNAME: a bracket expression containing `/` is still a bracket expression for other characters; unclosed `[` before `/` is literal | XSH fnmatch, XCU 2.14.1 | T (was a deviation, fixed) | F:246-251 |
| F3 | Without FNM_PATHNAME `/` is ordinary | XSH fnmatch | T | F:203, 207, 244 |
| F4 | FNM_PERIOD: leading `.` not matched by `*`, `?`, non-matching list, range, class | XSH fnmatch, XCU 2.14.3 rule 2 | T | F:91-93, 253-258 |
| F5 | FNM_PERIOD: `[.abc]` vs leading period | XCU 2.14.3 | NT: unspecified | — |
| F6 | FNM_PERIOD without FNM_PATHNAME: only the first character is leading | XSH fnmatch | T | F:260-262 |
| F7 | FNM_PERIOD with FNM_PATHNAME: a period after `/` is leading, including after an earlier `*` | XSH fnmatch | T (was a deviation, fixed) | F:263-280 |
| F8 | FNM_CASEFOLD / FNM_IGNORECASE: case-insensitive per XBD 4.1, including classes and ranges | XSH fnmatch, XBD 4.1 | T (class/range part was a deviation, fixed) | FC:58-86 |
| F9 | Return 0 on match, FNM_NOMATCH otherwise | XSH fnmatch RETURN VALUE | T | throughout (`y`/`n` compare against 0 and FNM_NOMATCH exactly) |
| F10 | Other non-zero on error; no errors defined | XSH fnmatch | NT: picolibc never reports an error now (it used to return -1 for > 16 `*`) | — |

### glob() / globfree()

| # | Requirement | Spec | Status | Reference |
|---|---|---|---|---|
| G1 | Pattern rules of XCU 2.14 (`*`, `?`, brackets) per component | XSH glob | T | G:212-216 |
| G2 | Leading period matched only explicitly | XCU 2.14.3 rule 2 | T | G:213-214 |
| G3 | `.`/`..` may be ignored | XCU 2.14.3 | NT: optional | — |
| G4 | `/` never matched by a wildcard; `/` inside `[...]` makes `[` ordinary (`a[b/c]d` matches only that literal path) | XCU 2.14.3 rule 1 | T | G:219, 264-265 |
| G5 | Components without special characters are literal and must exist | XCU 2.14.3 | T | G:223-226 |
| G6 | Backslash escapes; GLOB_NOESCAPE disables it | XSH glob | T | G:228-231 |
| G7 | Results sorted by LC_COLLATE (C locale: byte order) | XSH glob | T | G:212-216, 252-253 |
| G8 | `gl_pathv` null-terminated, `gl_pathc` = count | XSH glob | T | G:108 (`expands`), 270, 283, 303 |
| G9 | GLOB_APPEND: earlier names first in their order, then new ones; `gl_pathc` total | XSH glob | T | G:239-249, 298-307 |
| G10 | GLOB_DOOFFS: `gl_offs` leading NULLs | XSH glob | T | G:239-244, 289-296 |
| G11 | GLOB_MARK: slash appended to directories (after symlink resolution), not files; `/` unchanged | XSH glob | T (`/` check picolibc-only: glibc returns `//`) | G:220-221, 255-262 |
| G12 | GLOB_NOCHECK: pattern returned unchanged (backslashes kept) when nothing matches; normal results otherwise | XSH glob, XCU 2.14.3 | T | G:233-237, 280-287 |
| G13 | GLOB_NOSORT: same names, unspecified order | XSH glob | T | G:267-278 |
| G14 | GLOB_NOMATCH when nothing matches without GLOB_NOCHECK | XSH glob ERRORS | T | G:225-226, 231; GE:186 |
| G15 | errfunc called with path and errno when a directory can't be opened/read; returning 0 continues | XSH glob | T (EACCES skipped as root) | GE:153-163 |
| G16 | errfunc non-zero → GLOB_ABORTED, `gl_pathc`/`gl_pathv` reflect paths so far | XSH glob | T | GE:165-173 |
| G17 | GLOB_ERR → GLOB_ABORTED even if errfunc returns 0, and with NULL errfunc | XSH glob | T | GE:175-181 |
| G18 | No GLOB_ERR and NULL errfunc → error ignored | XSH glob | T | GE:146-151 |
| G19 | EACCES, ELOOP, ENAMETOOLONG are in the set of errors reported | XSH glob | T (PATH_MAX overflow part was a deviation, fixed) | GE:153-163, 183-216 |
| G20 | Read permission needed on directories with special components; search on the others | XSH glob, XCU 2.14.3 | partly T (unreadable dir: GE:153-159); search-only case U | see "left undone" |
| G21 | readdir() failure reported to errfunc | XSH glob | NT: no portable way to make readdir fail; inferred from reading that glob.c does not check readdir errors (deviation, not fixed) | glob.c:195 |
| G22 | GLOB_NOSPACE on allocation failure | XSH glob ERRORS | NT: needs malloc fault injection; by reading, every allocation failure returns GLOB_NOSPACE | glob.c:91-100, 183-185, 239-241, 258-262 |
| G23 | Doesn't fail because of large files | XSH glob | NT: needs > 2 GiB files; glob.c only uses lstat/stat on matched names (inferred) | — |
| G24 | globfree frees, doesn't change errno | XSH globfree | T | G:309-312 |
| G25 | GLOB_APPEND/GLOB_DOOFFS application restrictions | XSH glob | NT: application requirements | — |

Counts: 54 requirements — 41 tested (8 of them were deviations fixed here),
1 partly tested, 2 untested (P22 multibyte `?`, G20 search-only), 10 not
testable (unspecified, undefined, optional, or needing fault injection).

## Deviations found

All "before" failures verified by running the new tests against the reverted
source; all "after" passes verified by running.

1. **FNM_PATHNAME and `/` in brackets** — `fnmatch.c` rangematch returned
   NOMATCH as soon as it saw `/` in a bracket with FNM_PATHNAME, so
   `fnmatch("[/", "[/", FNM_PATHNAME)` failed (XCU 2.14.1: a `[` that does not
   introduce a valid bracket expression matches itself) and
   `fnmatch("a[/b]c", "abc", FNM_PATHNAME)` / `"[!/]"` vs `"a"` failed (XSH
   fnmatch only forbids the bracket matching a `/` in the *string*). Fixed by
   dropping the check (the string-side `/` check already exists). glibc and
   musl behave the same as the fix. Commit `9e0006eab9`, test `d975a7e339`.
2. **FNM_PERIOD lost after `*`** — the recursive call cleared FNM_PERIOD and
   reset the string start, so `fnmatch("*b/*", "ab/.x",
   FNM_PATHNAME|FNM_PERIOD)` matched (XSH fnmatch: a period after a slash is
   leading). Fixed by passing the original string start and flags. Commit
   `b1c41c76a2`, test `1d2c4c43b0`.
3. **Recursion limit** — fnmatch recursed once per `*` with a depth limit of
   16 and returned -1 beyond it, so valid patterns with ≥ 17 `*` failed
   (`*a*a…` with 20 stars vs 24 `a`s). Rewrote the matcher to backtrack only
   to the most recent `*` (no recursion, constant stack). A 3M-case
   differential fuzz of old vs new (patterns over `ab/.*?[]!-\`, all flag
   combinations except FNM_PERIOD where old was wrong) showed no other
   difference. Commit `39311b024a` (also updates test/libc-testsuite/fnmatch.c, whose `#ifdef __PICOLIBC__` case expected the old 64-star failure), test `8def479d46`.
4. **FNM_CASEFOLD/FNM_IGNORECASE hidden** — `fnmatch.h` defined them only
   under `__GNU_VISIBLE`; POSIX.1-2024 requires both. Now visible for
   `__POSIX_VISIBLE >= 202405` too. Commit `22c8032180`, test `e23cf28706`
   (fails to compile before).
5. **FNM_CASEFOLD with classes and ranges** — rangematch folded the string
   character to lower case, so `[[:upper:]]` did not match even `A`, and
   `[Z-a]` (which contains `Z`, `_`, `a`) matched nothing for `z`. XBD 4.1:
   anything matching case-sensitively must match with either case. Now a
   bracket element matches if the character or either of its cases would.
   glibc also fails the `[[:lower:]]`/`A`, `[[:upper:]]`/`a` and `[Z-a]`
   cases; the test file is picolibc-only anyway (glibc lacks FNM_IGNORECASE).
   Commit `491b578a08`, test `a61771bd8d` (plain FNM_CASEFOLD checks: `45d4bfcd0d`).
6. **Collating symbol as range end** — `[a-[.c.]]` treated `[` as the end
   point (XBD 9.3.5: either end may be a collating symbol). Factored element
   parsing into `bracketchar()` used for both ends. Commit `873dae21f1`,
   test `9d2838e45c`.
7. **Ranges compared as signed char** — on x86 `[\001-\377]` did not match
   `\200` (C/POSIX locale collation is byte order). Now compared as unsigned
   char (also for `__collate_range_cmp`). Not observable on arm, where char is
   unsigned. Commit `1773f0a984`, test `9873c71f48`.
8. **glob silently dropped over-long paths** — when the path exceeded
   picolibc's PATH_MAX (1024) glob just skipped it, so the match vanished with
   no errfunc call and GLOB_ERR ignored (XSH glob: ENAMETOOLONG must be in the
   reported set). Now reported through errfunc/GLOB_ERR as ENAMETOOLONG.
   Commit `31c19d9eb0`, test `f39791cb08`. Note picolibc's PATH_MAX (1024) is
   smaller than Linux's (4096), so patterns 1024–4095 bytes long that glibc
   expands now report ENAMETOOLONG in picolibc rather than silently failing.

Also changed incidentally (no separate test): ctype class functions are now
called with `unsigned char` values (was `char`, undefined for bytes ≥ 0x80
with signed char); `[\[:alpha:]]` — an escaped `[` — is now an ordinary `[`
rather than starting a class.

glibc divergences guarded with `#ifdef __PICOLIBC__` (POSIX followed):
`glob("/", GLOB_MARK)` returns `//` in glibc (G:257-262); glibc doesn't call
errfunc for a looping symlink reached through `*` (GE:160-163; the direct
`loop/x*` case is checked on both).

## Left undone

- **Multibyte characters (P22)**: fnmatch works on bytes, so in a UTF-8
  locale `?` matches one byte of a multibyte character. Fixing this means a
  wide-character matcher (mbrtowc per step); out of scope for the pilot.
- **readdir errors (G21)**: glob.c doesn't distinguish end-of-directory from
  a readdir error (would need `errno = 0` before readdir and a check after).
  Easy to add, but I found no portable way to make readdir fail in a test, so
  per the "fix needs a failing test" rule I left it.
- **Search-only permission (G20)**: a directory with read but no search
  permission in a non-final literal component. Behavior differs between
  implementations (glibc may not stat entries); not tested.
- **Invalid collating symbols/unknown classes (P20, P21)** are unspecified;
  picolibc's behavior is documented above, not tested.
- **CMake**: both new tests are registered in the CMakeLists.txt files, but I
  did not run a CMake build.

## Test runs (verified)

- x86_64-linux-gnu (`builds/build-x86_64-linux-gnu`, `-Dwerror=true`), final tip:
  - `meson test -t 4 --suite test-fnmatch --suite test-glob`: 11 OK, 0 fail,
    1 skipped (`test-fnmatch-casefold-native`: glibc lacks FNM_IGNORECASE).
    Covers test-fnmatch, test-fnmatch-casefold, test-glob, test-glob-errfunc,
    their `_32` variants, and `-native` variants.
  - Full `meson test -t 4`: 1652 OK, 9 expected fail, 0 fail, 47 skipped.
    (An earlier full run caught `libc-testsuite:fnmatch` expecting the old
    16-level recursion error; that test was updated in the rewrite commit.)
- arm bare-metal (`builds/build-arm`, `scripts/do-arm-configure`, qemu):
  `meson test -t 20 --suite test-fnmatch`: 156 OK, 0 fail (test-fnmatch and
  test-fnmatch-casefold across all multilibs); `libc-testsuite` fnmatch OK.
- Each fix was checked by reverting only the library file and rebuilding: the
  new test failed (or, for the header, failed to compile) and passed again
  with the fix restored.
- Extra, not committed: a 3M-case differential fuzz of picolibc fnmatch
  against glibc 2.43 over patterns from `ab/.*?[]!-\` showed no differences
  after the fixes, other than glibc treating an escaped `\/` specially and
  unterminated `[.` sequences (both unspecified/invalid territory); a second
  fuzz (alphabet with `[ . : =` and FNM_CASEFOLD) showed the rangematch
  refactor in `873dae21f1` changed nothing else.
- Not run: CMake build, other cross targets.

## Commits (oldest first, on audit/glob)

- `934356ab53` test-posix: Check pattern notation and flags in fnmatch
- `9e0006eab9` fnmatch: Let bracket expressions contain a slash with FNM_PATHNAME
- `d975a7e339` test-posix: Check slashes in bracket expressions in fnmatch
- `b1c41c76a2` fnmatch: Keep FNM_PERIOD for the pattern after '*'
- `1d2c4c43b0` test-posix: Check FNM_PERIOD after '*' in fnmatch
- `39311b024a` fnmatch: Match any number of '*' without recursion
- `8def479d46` test-posix: Check patterns with many '*' in fnmatch
- `22c8032180` fnmatch.h: Define FNM_CASEFOLD and FNM_IGNORECASE for POSIX.1-2024
- `e23cf28706` test-posix: Check FNM_CASEFOLD and FNM_IGNORECASE in strict POSIX mode
- `45d4bfcd0d` test-posix: Check FNM_CASEFOLD matching in fnmatch
- `491b578a08` fnmatch: Match either case against bracket expressions with FNM_CASEFOLD
- `a61771bd8d` test-posix: Check FNM_CASEFOLD with classes and ranges in fnmatch
- `873dae21f1` fnmatch: Accept a collating symbol as the end of a range
- `9d2838e45c` test-posix: Check collating symbols as range ends in fnmatch
- `1773f0a984` fnmatch: Compare range ends as unsigned characters
- `9873c71f48` test-posix: Check ranges of non-ASCII bytes in fnmatch
- `413ab76c9a` test-posix: Check GLOB_MARK, GLOB_NOSORT, GLOB_NOCHECK and GLOB_APPEND in glob
- `31c19d9eb0` glob: Report paths longer than PATH_MAX as ENAMETOOLONG
- `f39791cb08` test-posix: Check errfunc and GLOB_ERR in glob
- `32d51f4f1c` test-posix: Check the fnmatch.h flags and prototype
- `c810cd1144` test-posix: Check the glob.h types, flags and errors
