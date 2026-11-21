# `<regex.h>` and `<wordexp.h>` audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-regex-wordexp`, branch `audit/regex-wordexp`,
based on 40921d6cce (`claude/posix-spec-audit`). The host is an AWS Graviton2,
aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2,
glibc 2.43, `/bin/sh` is dash 0.5.12) as an unprivileged user. Nothing pushed.

**Implementation:**
- `libc/posix/regex/`: Henry Spencer's BSD regex (regcomp.c, engine.c through
  regexec.c, regerror.c, regfree.c, regex2.h, cclass.h, cname.h).
- `libc/posix/wordexp/`: wordexp.c, wordfree.c.
- Headers: `libc/include/regex.h`, `libc/include/wordexp.h`.

**Tests:** `test/test-posix/test-regex/` (test-regcomp.c, test-regerror.c,
and the new test-regexec.c and test-regex_h.c), `test/test-regex.c`, and
`test/test-posix/test-wordexp/` (test-wordexp.c, and the new
test-wordexp-expand.c and test-wordexp_h.c). Line numbers are for the branch tip.
Abbreviations: `RC` test-regcomp.c, `RE` test-regexec.c, `RH` test-regex_h.c,
`RR` test-regerror.c, `W` test-wordexp.c, `WX` test-wordexp-expand.c,
`WH` test-wordexp_h.c.

**Spec source:** pubs.opengroup.org/onlinepubs/9799919799 (fetched 2026-11-21):
basedefs/regex.h, basedefs/wordexp.h, functions/regcomp (covers regexec,
regerror, regfree), functions/wordexp (covers wordfree), XBD 9 (Regular
Expressions), XSH 2.9.5.2 (cancellation points), and XCU 2.2, 2.5.3 and 2.6 for
what wordexp's expansions mean.

**Status key:** TESTED (file:line), UNTESTED, NOT TESTABLE (reason).
- Every TESTED row was run and passes for picolibc (`test-X`) on
  aarch64-linux-gnu and arm-linux-gnueabihf.
- It also passes against glibc (`test-X-native`) unless the row says
  "picolibc only" or names a glibc guard.
- regex tests run everywhere, including bare metal (no OS support needed);
  test-regexec sets `alarm(60)` where `__PICOLIBC_POSIX_SIGNALS` is defined,
  as a watchdog for the back-reference patterns.
- wordexp tests need `__PICOLIBC_POSIX_MULTI_PROCESS` and skip if `/bin/sh` is
  not executable.

### What wordexp leaves to the shell

picolibc's wordexp runs `/bin/sh -c` with a fixed script that does `eval
"set --; printf '%s\0' x <words>"` and reads back the null-separated fields.
So **every expansion (tilde, parameter, command substitution, arithmetic, field
splitting, pathname expansion, quote removal) is done by `/bin/sh`**, not only
command substitution. The library does:
- the WRDE_BADCHAR, WRDE_CMDSUB (WRDE_NOCMD) and quote/bracket-balance
  (WRDE_SYNTAX) checks before any shell runs (`__wordexp_check`);
- WRDE_UNDEF (`set -u`), WRDE_SHOWERR (stderr to /dev/null otherwise), IFS
  (passed to the script, see D9), WRDE_DOOFFS, WRDE_APPEND, WRDE_REUSE and the
  error bookkeeping.

What follows from that:
- Without `/bin/sh`, wordexp always fails (WRDE_SYNTAX), so the wordexp tests
  skip rather than splitting off the command-substitution parts.
- Shell-dependent behaviour (results checked here are dash's): `#` starting a
  word is taken as a comment; `$'...'` (new in XCU Issue 8) isn't supported by
  dash 0.5.12, and the checker treats `$'` as `$` then a single quote, which
  agrees with dash; which arithmetic operators work is up to the shell.
- Expansions never touch the network or the real home directory in the tests:
  tilde results are compared as strings with `$HOME`, a fake `HOME`, and
  `getpwuid(getuid())->pw_dir`; pathname expansion runs in a forked child
  chdir'ed into a fresh `mkdtemp()` directory.

## Requirement table

### `<regex.h>` (basedefs/regex.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | regex_t with `size_t re_nsub` | TESTED | RH:48-52, 120 |
| H2 | `<regex.h>` defines size_t (checked with regex.h as the first header) | TESTED | RH:44-45 |
| H3 | regoff_t is a signed integer type | TESTED | RH:98 |
| H4 | regoff_t holds any ptrdiff_t or ssize_t value | TESTED (glibc guard: its regoff_t is int) | RH:99-102 |
| H5 | regmatch_t with regoff_t rm_so, rm_eo | TESTED | RH:53-62, 120 |
| H6 | REG_EXTENDED, REG_ICASE, REG_NOSUB, REG_NEWLINE (non-zero, distinct bits) | TESTED | RH:81-86 |
| H7 | REG_MINIMAL (new in Issue 8) | UNTESTED: **missing** (D1, not fixed). RH:88-90, 138-140 check it when defined and print "no REG_MINIMAL" | |
| H8 | REG_NOTBOL, REG_NOTEOL (non-zero, distinct bits) | TESTED | RH:87 |
| H9 | The 13 error codes REG_NOMATCH ... REG_BADRPT, distinct and non-zero | TESTED | RH:105-136, RR:52-55 |
| H10 | Prototypes of regcomp, regerror, regexec, regfree | TESTED | RH:64-78 |
| H11 | Other names only with the REG_ prefix (or re_/rm_ members, _t types) | TESTED by reading `-E -dM` output: every extra macro is REG_ (REG_BASIC, REG_NOSPEC, REG_PEND, REG_STARTEND ...); the struct members are re_magic, re_endp, re_g. Only the known MALLOC_ALIGNMENT leak (cross-cutting) | |

### regcomp

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Returns 0 on success; compiles BRE by default, ERE with REG_EXTENDED | TESTED | RC:64-120, RE whole table |
| C2 | re_nsub counts `\(\)` / `()` subexpressions when REG_NOSUB isn't set | TESTED | RE:342-349, RC:192 |
| C3 | REG_ICASE: case-insensitive matching (XBD 4.1), for literals, brackets, classes and back-references | TESTED. **Back-references compared bytes exactly; fixed** (D4) | RE:85, 182-187, RC:107-108 |
| C4 | REG_NOSUB: regexec reports only success or failure, ignores pmatch | TESTED | RE:329-340, RC:200 |
| C5 | REG_NEWLINE: `.` and non-matching lists don't match newline; a matching list with a newline does | TESTED | RE:188-196, RC:111-114 |
| C6 | REG_NEWLINE: `^` matches after a newline, regardless of REG_NOTBOL | TESTED | RE:197, RC:118 |
| C7 | REG_NEWLINE: `$` matches before a newline, regardless of REG_NOTEOL | TESTED | RE:198 |
| C8 | Without REG_NEWLINE, newline is ordinary (matched by `.` and `[^x]`, not by `^`/`$`) | TESTED | RE:188-192 |
| C9 | REG_MINIMAL: shortest match by default for ERE duplication, `?` modifier inverts | UNTESTED: **not implemented** (D1) | |
| C10 | On failure returns an error code from `<regex.h>`; REG_BADPAT or a more precise one | TESTED for every code reachable from a pattern | RE:351-385, RC:122-134 |
| C11 | REG_BADBR for a bad `\{\}`: m > n, number too large (past RE_DUP_MAX) | TESTED (RE_DUP_MAX part picolibc only: glibc accepts counts past its 32767) | RE:360-377 |
| C12 | REG_EBRACE, REG_EBRACK, REG_EPAREN, REG_EESCAPE, REG_ERANGE, REG_ECTYPE, REG_ECOLLATE, REG_ESUBREG | TESTED | RE:351-385 |
| C13 | REG_ESPACE when out of memory | NOT TESTABLE: no portable way to make malloc fail at the right point | |
| C14 | XBD 9.2: an RE of 256 bytes is supported | TESTED | RE:387-396 |

### regexec

| # | Requirement | Status | Reference |
|---|---|---|---|
| E1 | Returns 0 on a match, REG_NOMATCH otherwise | TESTED | RE table, RC table |
| E2 | REG_NOTBOL: `^` doesn't match at the start of string | TESTED | RE:200-204, RC:117 |
| E3 | REG_NOTEOL: `$` doesn't match at the end | TESTED | RE:202, RC:119 |
| E4 | nmatch 0 ignores pmatch (may be NULL) | TESTED | RE:325-326, RC:210 |
| E5 | pmatch[0] is the whole match; pmatch[i] subexpression i, by its open parenthesis | TESTED | RE table |
| E6 | Unused elements up to nmatch-1 are -1 | TESTED | RE:319-324, and every RE row checks all 5 entries |
| E7 | More subexpressions than nmatch: still matches, records only nmatch; later entries untouched | TESTED | RE:305-317 |
| E8 | Rule 1: a repeated subexpression reports its last match | TESTED | RE:142-144, 110, 114 |
| E9 | Rule 2: a subexpression that didn't take part (after `*`, `?`, `{}`, or the other alternative) is -1 | TESTED | RE:136-141, 91, 119 |
| E10 | Rule 3: a contained subexpression reports within the containing one's reported match | TESTED (glibc guard: glibc reports a stale inner match) | RE:145-147, 215-225 |
| E11 | Rule 4: contained in a -1 subexpression means -1 | TESTED | RE:147 |
| E12 | Rule 5: a zero-length match reports the offset of the following character | TESTED | RE:148-151 |
| E13 | Locale changed between regcomp and regexec | NOT TESTABLE: undefined behaviour | |
| E14 | regexec doesn't change the compiled RE (repeated use) | TESTED | RE:404-409 |

### XBD 9 matching rules (through regexec)

| # | Requirement | Status | Reference |
|---|---|---|---|
| X1 | 9.1 leftmost, then longest; each subpattern longest from left to right | TESTED (the "each subpattern longest" case `(a\|ab)(c\|bcd)(d*)` is under a glibc guard) | RE:70-75, 215-225 |
| X2 | 9.1 "a null string is longer than no match": `\(a*\)*` on "bc" gives \1 = (0,0) | TESTED | RE:74 |
| X3 | 9.3.2/9.3.3 BRE: `\]` matches `]`; `*` is ordinary first, after `^`, and first in `\(` | TESTED | RE:94-99, RC:68 |
| X4 | 9.3.2: `\?`, `\+`, `\|` in a BRE: implementation-defined | Read: picolibc treats them as the literal characters (regcomp.c p_simp_re default case). Unescaped `+`, `?`, `\|`, `{` are literal in a BRE: RC:69-71 | |
| X5 | 9.3.4/9.4.4 `.` matches any character but NUL | TESTED | RC:110, test-regex.c |
| X6 | 9.3.5 `]` first is literal; `-` first or last is literal; `[^...]` | TESTED | RE:162-166, RC:85-87 |
| X7 | 9.3.5 ranges in the POSIX locale, including `[%--]` and `[--@]` | TESTED. **`[--@]` failed with REG_ERANGE; fixed** (D3) | RE:157-159 |
| X8 | 9.3.5 the special characters lose their meaning inside brackets (BRE and ERE lists) | TESTED | RE:167-169 |
| X9 | 9.3.5 collating symbols `[.].]`, `[.-.]`, `[.a.]` as a range end; an unknown one is invalid | TESTED | RE:154-156, 352-353 |
| X10 | 9.3.5 equivalence classes `[=a=]` (POSIX locale: the character itself) | TESTED | RE:153, RC:91 |
| X11 | 9.3.5 all 12 character classes; `[^[:class:]x]`; an unknown class is invalid | TESTED | RE:171-183, 354 |
| X12 | 9.3.5 multi-character collating elements | NOT TESTABLE: the POSIX locale has none (picolibc has no locale with them) | |
| X13 | 9.3.6 back-references: the standard's examples, including `\(a\)*\1` and `\(a\(b\)*\)*\2` not matching | TESTED. **The nested example matched; fixed** (D5) | RE:77-86, 215-225 |
| X14 | 9.3.6 back-reference to a subexpression that doesn't precede it is invalid | TESTED | RE:380-381 |
| X15 | 9.3.6/9.4.6 intervals `{m}`, `{m,}`, `{m,n}`, 0 <= m <= n <= RE_DUP_MAX, and the examples | TESTED | RE:87-93, 108-114, 360-377 |
| X16 | 9.3.8 BRE anchors: `^` first and `$` last are anchors, elsewhere literal; `^...$` whole string | TESTED | RE:100-107, RC:64-67 |
| X17 | 9.4.3 ERE: `)` is special only when matched with a preceding `(` | TESTED. **An unmatched `)` failed with REG_EPAREN; fixed** (D2) | RE:134 |
| X18 | 9.4.1/9.4.2 ERE: escaped special characters, `\]` and `\}` match themselves | TESTED | RE:131-133 |
| X19 | 9.4.6 ERE `+`, `*`, `?` and the standard's examples | TESTED | RE:108-116 |
| X20 | 9.4.6 the repetition modifier `?` (`*?`, `+?`, `??`, `{m,n}?`): shortest match (new in Issue 8) | UNTESTED: **not implemented** (D1); regcomp rejects `.*?` with REG_BADRPT | |
| X21 | 9.4.7/9.4.8 alternation and its precedence (`abba\|cde`) | TESTED | RE:117-122, RC:101-102 |
| X22 | 9.4.9 ERE anchors anywhere: `(^ab)`, `a^b` and `e$f` never match | TESTED | RE:123-130 |
| X23 | Undefined constructs (`a**`, `()`, `{` not starting an interval, leading `*` in an ERE) | NOT TESTABLE: undefined; picolibc returns REG_BADRPT/REG_EMPTY or takes them literally | |

### regerror, regfree

| # | Requirement | Status | Reference |
|---|---|---|---|
| R1 | errbuf_size 0: ignores errbuf, returns the size needed | TESTED | RR:68 |
| R2 | Returns the size including the null; fills the buffer | TESTED | RR:69-70 |
| R3 | Truncates and null-terminates when it doesn't fit; return value still the full size | TESTED | RR:81-84 |
| R4 | preg may be NULL | TESTED | RR:68 |
| R5 | preg that produced the error may be passed | TESTED | RR:87-89 |
| R6 | regfree frees the memory allocated by regcomp | NOT TESTABLE portably (no allocator introspection); read: regfree.c frees strip, sets, setbits, must, the jump tables and the guts | |
| R7 | regfree doesn't change errno | TESTED | RE:398-402 |

### `<wordexp.h>` (basedefs/wordexp.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| WH1 | wordexp_t with `size_t we_wordc`, `char **we_wordv`, `size_t we_offs` | TESTED | WH:48-62, 108-109 |
| WH2 | `<wordexp.h>` defines size_t | TESTED (wordexp.h is the first header) | WH:45 |
| WH3 | WRDE_APPEND, WRDE_DOOFFS, WRDE_NOCMD, WRDE_REUSE, WRDE_SHOWERR, WRDE_UNDEF (distinct bits) | TESTED | WH:72-81 |
| WH4 | WRDE_BADCHAR, WRDE_BADVAL, WRDE_CMDSUB, WRDE_NOSPACE, WRDE_SYNTAX (non-zero, distinct) | TESTED | WH:96-125 |
| WH5 | Prototypes of wordexp and wordfree | TESTED | WH:64-70 |
| WH6 | Namespace: nothing beyond size_t, we_, WRDE_ (and `_t`) | TESTED. **`<sys/types.h>` brought in all of `<stdint.h>`; fixed** (D6) | WH:83-92 |

### wordexp, wordfree

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | Expansions as for the arguments of a command line (XCU 2.6), one word per field / pathname, in order | TESTED | WX:137-399 |
| W2 | Tilde expansion: `~`, `~/x` from HOME; `~login`; quoted and non-leading `~` unchanged | TESTED | WX:143-165 |
| W3 | Parameter expansion, including `${x:-}`, `${x+}`, `${#x}`, `${x#}`, `${x%%}`, unset variables | TESTED | WX:176-184 |
| W4 | Command substitution, `$()` and backquotes, trailing newlines removed, result split | TESTED | WX:222-227, W:107 |
| W5 | Arithmetic expansion | TESTED (part under a glibc guard) | WX:210-217, W:105 |
| W6 | Field splitting with the default IFS, empty fields from quoting | TESTED (part under a glibc guard) | WX:242-251, W:89 |
| W7 | Field splitting with IFS from the application's environment | TESTED (picolibc requires it, other libraries may leave it to the shell). **dash ignores IFS from its environment; fixed** (D9) | WX:253-279 |
| W8 | Pathname expansion: sorted, `?`, `*`, brackets, a pattern without matches kept, quoted patterns not expanded, leading `.` | TESTED (expansion of a parameter's value under a glibc guard) | WX:339-399 |
| W9 | Quote removal (XCU 2.2) | TESTED | WX:285-296, W:91 |
| W10 | First pointer after the last word is NULL | TESTED (`words()` checks it everywhere) | WX:99 |
| W11 | Special parameters: unspecified | NOT TESTABLE (unspecified). Positional parameters are not special: **`$1` and `$2` expanded to the library's own script arguments; fixed** (D8) | WX:198-204 |
| W12 | wordfree frees the memory; doesn't change errno | TESTED for errno | WX:457-461 |
| W13 | WRDE_APPEND: earlier words kept in order, then the new ones; we_wordc is the total | TESTED | WX:413-416, 449-451 |
| W14 | WRDE_DOOFFS: we_offs null pointers first, kept across WRDE_APPEND | TESTED | WX:406-416 |
| W15 | Without WRDE_DOOFFS, no leading null pointers | TESTED | WX:443-448 |
| W16 | WRDE_NOCMD fails with WRDE_CMDSUB wherever the substitution is; quoted text and arithmetic are allowed | TESTED | WX:229-233, W:113-124 |
| W17 | WRDE_REUSE: as if wordfree then wordexp without it, also when the call fails | TESTED. **A failing call left the old words; fixed** (D7) | WX:438-441, 453-455 |
| W18 | WRDE_SHOWERR: stderr of command substitutions goes to /dev/null without it, and through with it | TESTED | WX:464-498 |
| W19 | WRDE_UNDEF: WRDE_BADVAL for an unset variable; defaults are still fine | TESTED (defaults under a glibc guard) | WX:186-193 |
| W20 | WRDE_SYNTAX for shell syntax errors (unbalanced quotes, substitutions) | TESTED. **Under WRDE_UNDEF every shell failure was WRDE_BADVAL; fixed** (D10) | WX:194-196, 317-324 |
| W21 | WRDE_BADCHAR for each of newline, `\|`, `&`, `;`, `<`, `>`, `(`, `)`, `{`, `}` unquoted; fine quoted or in a substitution | TESTED | WX:292-296, 305-315 |
| W22 | After WRDE_BADCHAR the number of expanded words is 0 | TESTED (glibc guard: glibc leaves we_wordc alone). **we_wordc was left unset; fixed** (D7) | WX:326-337 |
| W23 | Other errors with WRDE_APPEND leave we_wordc and we_wordv unchanged | TESTED for BADCHAR, SYNTAX (both kinds), BADVAL and CMDSUB | WX:418-432 |
| W24 | WRDE_NOSPACE: we_wordc/we_wordv reflect the words expanded so far | NOT TESTABLE: can't make malloc fail at a chosen point. Read: wordexp.c keeps the list consistent on each NOSPACE path | |
| W25 | Comment character at the start of a token: either literal or a comment | NOT TESTED (either is allowed); dash takes it as a comment | |
| W26 | XSH 2.9.5.2: wordexp may be a cancellation point; if it isn't, it mustn't become one by accident, and either way nothing may leak | TESTED (picolibc only, the native run accepts acting on it). **A pending request acted inside read/waitpid and left a zombie; fixed** (D11) | WX:503-542 |
| W27 | wordexp is not required to be thread-safe against concurrent setenv | NOT TESTABLE (no requirement) | |

### Counts

| Area | Requirements | Tested before | Tested after |
|---|---|---|---|
| `<regex.h>` | 11 | 3 | 9 (+1 by reading) |
| regcomp | 14 | 9 | 12 |
| regexec | 14 | 7 | 13 |
| XBD 9 rules | 23 | 11 | 19 (+1 by reading) |
| regerror, regfree | 7 | 5 | 6 |
| `<wordexp.h>` | 6 | 0 | 6 |
| wordexp, wordfree | 27 | 10 | 23 |
| **Total** | **102** | **45** | **88** |

"Tested before" is my count of the rows test-regcomp, test-regerror, test-regex
and test-wordexp already covered at the base (judged row by row, so
approximate). "Tested after" counts H11 (checked from `-E -dM` output) as
tested. The remaining 14 are NOT TESTABLE or not required (C13, E13, X12, X23,
R6, W11, W24, W25, W27), UNTESTED because unimplemented (H7, C9, X20), or read
only (X4).

## Deviations found and fixed

Each fix was proven by reverting only its library change (with `git apply -R`
of the commit, or, where later commits touch the same lines, by undoing the
change in a copy of the current file), rebuilding and running the named test on
aarch64-linux-gnu: every one failed as shown, and passes with the fix.

- **D2. An unmatched `)` in an ERE was an error.** XBD 9.4.3: "The
  <right-parenthesis> shall be special when matched with a preceding
  <left-parenthesis>", and 9.5.1 says the same for SPEC_CHAR. regcomp.c's
  p_ere_exp returned REG_EPAREN for `)` (Spencer's `#ifndef POSIX_MISTAKE`).
  Now it falls through to an ordinary character. Reverted: `"a)": regcomp failed
  with 8`. glibc agrees with the fix.
- **D3. `[--@]` was rejected.** XBD 9.3.5 gives `[--@]` as the range from `-` to
  `@`. p_bracket took the first `-` as a literal and p_b_term then failed on the
  second with REG_ERANGE. A leading `-` followed by `-` and an end point is now
  a range (`p_b_range`, split out of p_b_term). Reverted: `"[--@]*": regcomp
  failed with 11`. glibc agrees.
- **D4. REG_ICASE didn't apply to back-references.** regcomp: REG_ICASE
  "Perform matching in a case-insensitive manner". engine.c compared the
  back-reference with memcmp, so `\(.\)\1` failed on "aA". It now uses
  `backcmp`, which folds case under REG_ICASE. Reverted: `"\(.\)\1" on "xaAy":
  regexec returned 1, not 0`. glibc agrees.
- **D5. A back-reference used a stale inner match.** XBD 9.3.6: "If the
  containing subexpression does not match, or if there is no match for the
  contained subexpression within the string matched by the containing
  subexpression, then back-reference expressions corresponding to the contained
  subexpression shall not match", with `\(a\(b\)*\)*\2` failing to match
  "abab" as its example; regexec rule 3 says the same for pmatch. The
  back-reference matcher (engine.c backref) kept \2 from the first pass through
  \1. regcomp now records which subexpression contains each of 1 to 9
  (`findparents`, `re_guts.bparent`), and a back-reference or a reported match
  of an inner subexpression that started before its container's last match is
  treated as unmatched. Reverted: `"\(a\(b\)*\)*\2" on "abab": regexec returned
  0, not 1` and `"\(a\(b\)*\)*\1" on "abaa": match 2 is (1,2), not (-1,-1)`.
  glibc 2.43 has the same bug (its test rows are guarded). Only subexpressions 1
  to 9 are tracked, which covers every back-reference; for subexpressions past 9
  the non-back-reference matcher (dissect) was already right.
- **D6. `<wordexp.h>` leaked `<stdint.h>`.** It included `<sys/types.h>`, which
  includes all of `<stdint.h>` (known cross-cutting issue), so INT8_MAX,
  SIZE_MAX and the rest were defined in strict mode. It now takes size_t from
  `<stddef.h>` with `__need_size_t`, like `<regex.h>`. Reverted: test-wordexp_h
  fails to compile (`#error "<wordexp.h> defines INT8_MAX"`).
- **D7. Errors detected before running the shell left stale words.** wordexp:
  for WRDE_BADCHAR "the number of expanded words shall be 0", and WRDE_REUSE is
  "the same as if the application had called wordfree() and then called
  wordexp() without WRDE_REUSE". `__wordexp_check` ran before the reuse/reset
  code, so we_wordc kept whatever was there and WRDE_REUSE kept the old words.
  The check now runs after. With WRDE_APPEND nothing is touched, as the RETURN
  VALUE section requires. Reverted: `no words after WRDE_BADCHAR` and
  `WRDE_REUSE error words` fail.
- **D8. The words saw the library's positional parameters.** The script got the
  WRDE_UNDEF switch as `$1` and the words as `$2`, so `"$1 $2"` expanded to
  `[$1] [$2]`, and WRDE_UNDEF didn't report `$1` as unset. `eval` now starts
  with `set --`. (POSIX leaves only the *special* parameters unspecified.)
  Reverted: `"$1 $2": wrong words` and `"$1": wordexp returned 0, not 3`.
- **D9. IFS from the environment was ignored.** wordexp's fields are those of XCU
  2.6.5, and an application sets IFS in its environment to change them (glibc
  uses `getenv("IFS")`). XCU 2.5.3 lets a shell ignore an inherited IFS, and dash
  does, so picolibc always split with the default. The script now gets the
  caller's IFS as `$3` and sets it. Reverted: `IFS=:` gives `[a:b::c]` instead
  of four fields. The test requires this only of picolibc; other libraries may
  leave it to their shell.
- **D10. Under WRDE_UNDEF every shell failure was WRDE_BADVAL**, so `$((1+))`
  was reported as an undefined variable rather than WRDE_SYNTAX ("Shell syntax
  error"). When the words hold no command substitution (which can't be repeated
  safely), wordexp now runs them again without `set -u`: success means the
  failure was an unset variable (WRDE_BADVAL), failure means WRDE_SYNTAX. With a
  command substitution present it still answers WRDE_BADVAL. Reverted:
  `"$((1+))": wordexp returned 3, not 5`.
- **D11. Cancellation inside wordexp leaked.** wordexp is in the "may" list of
  XSH 2.9.5.2. A pending or arriving request acted in picolibc's read() or
  waitpid() wrappers, leaving the pipe, the output buffer and an unreaped child;
  the forked child inherited the pending request and could act on it before exec (inferred). wordexp now
  holds cancellation off (`pthread_setcancelstate`, under
  `__PICOLIBC_PTHREADS`), like the DNS lookups. Reverted: `wordexp with a
  cancellation request pending` and `no child left` fail.

## Deviations not fixed

- **D1. REG_MINIMAL and the `?` repetition modifier (Issue 8) are missing.**
  `<regex.h>` must define REG_MINIMAL, and XBD 9.4.6 adds `*?`, `+?`, `??` and
  `{m,n}?` as leftmost-shortest repetitions, with REG_MINIMAL making shortest
  the default. picolibc rejects `.*?` with REG_BADRPT (regcomp.c p_ere_exp,
  "multiple repetition"). Spencer's engine finds the overall extent with a
  leftmost-longest DFA and only then dissects it, so per-repetition shortest
  matching needs a different matcher (a backtracking or tagged-NFA engine), not
  a local change. Defining REG_MINIMAL without implementing it would be worse
  than leaving it out. glibc 2.43 lacks both too. Needs a decision on whether to
  replace or extend the engine.

## Other observations (not changed)

- glibc 2.43 deviations found by the native runs (tests guard them with
  `__GLIBC__`): regoff_t is `int` (too narrow for ptrdiff_t on 64-bit targets);
  it doesn't make each subpattern longest (`(a|ab)(c|bcd)(d*)` on "abcd" gives
  \1 = "a"); it reports a stale inner match and matches `\(a\(b\)*\)*\2` on
  "abab"; it accepts interval counts past RE_DUP_MAX. Its wordexp leaves
  we_wordc alone after WRDE_BADCHAR, reports WRDE_BADVAL for `${x:-y}` under
  WRDE_UNDEF, joins the first and last fields of a split value to the
  surrounding text (`x${S}y`), fails on an expansion with an empty unquoted
  field, doesn't apply pathname expansion to the result of a parameter
  expansion, and lacks several arithmetic operators. `a{` (undefined) is an
  error in glibc and literal in picolibc; the test doesn't check it.
- wordexp with SIGCHLD set to SIG_IGN: waitpid fails with ECHILD and wordexp
  returns WRDE_SYNTAX although the output arrived. Inferred by reading, not run.
  Not a POSIX requirement; a fallback that trusts the output (printf only runs
  once every expansion succeeded) would cover it.
- `__wordexp_shell` leaks its buffer if waitpid fails with anything but EINTR
  (read only; only reachable with the SIGCHLD case above).
- Nothing in wordexp's use of fork, pipe2, dup2, execv or waitpid looked wrong
  for the unistd-process unit; no process-layer bug was found.
- regcomp's Spencer extensions (REG_NOSPEC, REG_PEND, REG_STARTEND, `[[:<:]]`,
  `[[.space.]]`-style names) all use reserved names or are undefined syntax.

## Left undone

- REG_MINIMAL and the repetition modifier (D1).
- Multi-character collating elements and non-POSIX locales: picolibc has no
  locale that defines them, so `[[.ch.]]`-style behaviour can't be exercised.
- REG_ESPACE and WRDE_NOSPACE paths (no fault injection).

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`): full
  `meson test -t 4` (no `--no-rebuild`), 1550 tests: **1501 Ok, 0 Fail, 6
  Expected Fail, 43 Skipped.** None of this unit's 20 tests (including the
  `check-cdef-*`/`check-wundef-*` header checks for regex.h and wordexp.h)
  skip.
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`):
  full `meson test -t 4`, 1551 tests: **1499 Ok, 1 Fail, 6 Expected Fail, 45
  Skipped.** The one failure was test-regexec-native: 32-bit glibc returns
  REG_ESPACE for `a{0,32767}` (its RE_DUP_MAX). The test now accepts
  REG_ESPACE there; after that change the regex and wordexp suites were rerun
  on both targets (14 Ok, 0 Fail each), so the full-suite total with the final
  tree is 1500 Ok, 0 Fail on armhf. The full armhf suite was not rerun for that
  one-line test change.
- The final two test commits also reformatted the prototype checks in
  test-regex_h.c and test-wordexp_h.c after the full runs; both suites were
  rerun on both targets afterwards.
- Native configuration (`scripts/do-native-configure`, `builds/build-native`):
  `ninja` builds cleanly at the branch tip.
- CMake (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`, at the
  branch tip): **579 tests, 100% passed (257 ran, 322 skipped).**
  test-regcomp, test-regerror, test-regexec, test-regex_h and test-wordexp_h run
  there; test-wordexp and test-wordexp-expand skip (no multi-process support).
- Every fix was shown to be needed (see "Deviations found and fixed").
- `clang-format --dry-run --Werror` is clean on every regex and wordexp source,
  header and test.
- Not run here: bare-metal meson targets and qemu-user (the x86_64 host does
  those; nothing in these tests depends on kernel details qemu changes), and
  the SIGCHLD = SIG_IGN case (read only).

## Commits (audit/regex-wordexp, on top of 40921d6cce)

- `ab57902dce` regex: Treat an unmatched ')' in an ERE as an ordinary character
- `cae53dd147` regex: Accept a range starting at '-' first in a bracket expression
- `e7357cd6d0` regex: Ignore case in back-references under REG_ICASE
- `53f4a6ebaf` regex: Don't use a stale inner match in a back-reference
- `ad7c3151a5` wordexp: Don't include <sys/types.h> in <wordexp.h>
- `d1371328b1` wordexp: Clear the words before checking them
- `99aa935b8c` wordexp: Hide the script's positional parameters from the words
- `0d5e45b6c5` wordexp: Split fields with IFS from the environment
- `3478801e90` wordexp: Report a syntax error as WRDE_SYNTAX under WRDE_UNDEF
- `b54aca1035` wordexp: Don't act on cancellation requests
- `2b9ce78ff4` test-posix: Check matching, subexpressions, brackets, flags and <regex.h>
- `a06e3e4f0a` test-posix: Check the expansions, flags and errors of wordexp, and <wordexp.h>
