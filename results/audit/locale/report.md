# `<locale.h>`, `<langinfo.h>`, `<monetary.h>`, `<nl_types.h>`, `<libintl.h>`, `<iconv.h>` and the ctype/wctype `_l` functions: audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-locale`, branch `audit/locale`, based on 6edd04ce22 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot as an unprivileged user, with gcc 16, clang 21 and glibc 2.43.

**Interfaces (about 70).**
- `<locale.h>`: setlocale, localeconv, newlocale, duplocale, freelocale, uselocale, getlocalename_l (new in Issue 8), LC_*, LC_*_MASK, LC_GLOBAL_LOCALE, struct lconv, locale_t.
- `<langinfo.h>`: nl_langinfo, nl_langinfo_l and every item, including ALTMON_n and ABALTMON_n (new in Issue 8).
- `<monetary.h>`: strfmon, strfmon_l.
- `<nl_types.h>`: catopen, catgets, catclose.
- `<libintl.h>` (new in Issue 8): gettext, dgettext, dcgettext, ngettext, dngettext, dcngettext and their six `_l` forms, textdomain, bindtextdomain, bind_textdomain_codeset.
- `<iconv.h>`: iconv_open, iconv, iconv_close.
- `<ctype.h>`: isalnum_l ... isxdigit_l, tolower_l, toupper_l. `<wctype.h>`: iswalnum_l ... iswxdigit_l, iswctype_l, towlower_l, towupper_l, towctrans_l, wctype_l, wctrans_l. The functions without `_l` are covered only as "uses the thread's locale".
- The cross-cutting requirements: POSIX = C locale (XBD 7.2), LC_ALL/LC_*/LANG precedence (XBD 8.2), per-thread locales, which functions follow the thread's locale, and C.UTF-8.

**Out of scope.**
- Not in Issue 8: isascii, toascii, `_tolower`, `_toupper` (removed; their names are in `<ctype.h>`'s reserved is/to space, so they aren't leaks), YESSTR and NOSTR (checked only as a namespace item).
- strcoll_l, strerror_l, strcasecmp_l, strxfrm_l: the strings unit. strftime_l: the time unit. The wide-character conversions (mbrtowc, btowc, wcrtomb...) and the classification functions without `_l`: the wide unit.

**Configuration.** The default Linux configurations (`do-aarch64-linux-gnu-configure`, `do-arm-linux-gnueabihf-configure`) build without `mb-capable`, so picolibc has only the C locale: every locale_t is the C locale, and C.UTF-8, ISO-8859-1 and the iconv UTF-8 conversions don't exist. The requirements that need another locale (per-thread effects, newlocale's mask and base, the `_l` functions following their argument, iconv's EINVAL and indicator suffixes on characters outside ASCII) were run in an extra build, `builds/build-aarch64-mb` (`-Dmb-capable=true -Dmb-extended-charsets=true`). The tests find those locales at run time (`newlocale(..., "C.UTF-8", 0)`), under `#ifdef __PICOLIBC__` because C.UTF-8 is picolibc's own locale, and print what they skip.

**Implementation.**
- `libc/locale/`: setlocale.c, newlocale.c, duplocale.c, freelocale.c, uselocale.c, getlocalename_l.c, localeconv.c, nl_langinfo.c, locale_names.c, new locale_env.c, locale_private.h, catopen.c, catgets.c, catclose.c, gettext.c, textdomain.c.
- `libc/iconv/`, `libc/posix/monetary/`, `libc/ctype/`.
- Headers: `locale.h`, `sys/_locale.h`, `langinfo.h`, `monetary.h`, `nl_types.h`, `libintl.h`, `iconv.h`, `ctype.h`, `wctype.h`.
- The design: locale_t is an int naming the LC_CTYPE charset (`enum locale_id`); every other category has the POSIX locale's data in every locale. catopen never opens a catalog and gettext never finds a messages object (POSIX leaves both formats unspecified, and picolibc has no gencat or msgfmt).

**Tests.** New directories `test/test-posix/test-locale/`, `test-langinfo/`, `test-iconv/`, `test-ctype/`, `test-wctype/`, and new files in `test-monetary/`, `test-nl_types/` and `test-libintl/`. File names below drop the `test-` prefix; line numbers are for the branch tip.
- Environment changes happen in forked children, which first unset every LC_* variable and LANG (the chroot sets `LANG=en_US.UTF-8`).
- No test reads host locale or catalog files: "unknown" locales use the name `xx_XX.NO-SUCH-CODESET`, which no C library has, and the catalog and messages-object files are written in a fresh `mkdtemp()` directory.
- The `-native` builds run every check except those marked glibc below.

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799: basedefs/locale.h, langinfo.h, monetary.h, nl_types.h, libintl.h, iconv.h, ctype.h, wctype.h; functions/ for each interface; XBD 6.2 and chapters 7 (Locale) and 8 (Environment Variables); utilities/msgfmt; XSH 2.2.2 for the namespace.

**Status key.** TESTED (file:line), UNTESTED, NOT TESTABLE (reason). "(mb)" means the row ran only in the mb-capable build; in the default build the check is skipped at run time. "(new)" marks checks added here.

## Summary

- 144 requirements: 120 tested by running (about 25 before, in the existing test-gettext, test-textdomain, test-bindtextdomain, test-bind_textdomain_codeset, test-catopen, test-strfmon, test-strfmon_l and test-iconv), 4 checked by reading, 5 untested, 15 not testable.
- 12 deviations fixed, each with a test that fails without the fix; 4 left (locale objects carry only LC_CTYPE, nl_item's type on long-int32 targets, no catalog or messages-object format, the 256-character POSIX locale for the wide unit).

## Requirement table

### Locale model (XBD 6.2, 7.1, 7.2, 8.2)

| # | Requirement | Status | Reference |
|---|---|---|---|
| M1 | The POSIX locale is the default at entry to main(), for every category | TESTED (new) | locale/setlocale.c:276-278 |
| M2 | "C" and "POSIX" name the same locale, in setlocale and newlocale | TESTED (new) | setlocale.c:281-285, newlocale.c:185-201 |
| M3 | The POSIX locale's LC_CTYPE classes are those of 7.3.1.1; outside the portable characters only cntrl and punct may be added | TESTED (new) | ctype/isalpha_l.c:169-218 (every byte and EOF), wctype/iswalpha_l.c:157-191 |
| M4 | The POSIX locale's case mappings change only the portable letters | TESTED (new) | isalpha_l.c:200-214, iswalpha_l.c:167-190 |
| M5 | The POSIX locale's LC_NUMERIC and LC_MONETARY values (7.3.3.1, 7.3.4.1) | TESTED (new) | locale/localeconv.c:56-89 |
| M6 | The POSIX locale's LC_TIME and LC_MESSAGES values (7.3.5.3, 7.3.6.1) through nl_langinfo | TESTED (new) | langinfo/nl_langinfo.c:165-197 |
| M7 | The POSIX locale has 256 single-byte characters (XBD 6.2) | UNTESTED | Belongs to the wide unit (mbrtowc, btowc). picolibc's C locale treats bytes 0x80-0xff as invalid (cross-cutting.md) |
| M8 | Functions without a locale argument use the thread's locale if one is set, otherwise the global locale | TESTED (new; mb) | locale/uselocale.c:110-137 (MB_CUR_MAX, mbrtowc, iswalpha, towupper, nl_langinfo), ctype/isalpha_l.c:257-262 (isalpha, toupper with ISO-8859-1), wctype/iswalpha_l.c:248-255, strfmon-posix.c:173-176, nl_langinfo.c:210-212, gettext_l.c:159-162 |
| M9 | The functions with `_l` use their argument, not the current locale | TESTED (new; mb) | isalpha_l.c:247-260, iswalpha_l.c:239-253, nl_langinfo.c:227-233 |
| M10 | LC_ALL, then LC_<category>, then LANG, then the default; unset and empty values are skipped | TESTED (new) | setlocale.c:186-218, newlocale.c:160-172. **Deviation 1, fixed** |
| M11 | A locale picolibc doesn't support, named by a variable, makes setlocale(LC_ALL, "") fail without changing anything | TESTED (new) | setlocale.c:150-166 (each failing case checks the locale is unchanged), 195-214 |
| M12 | With C.UTF-8, the variable chosen sets LC_CTYPE | TESTED (new; mb) | setlocale.c:224-264, newlocale.c:284-289 |
| M13 | The C.UTF-8 locale (picolibc's claim with mb-capable): multibyte, its own LC_CTYPE classes and case, codeset "UTF-8", the other categories POSIX's | TESTED (new; mb) | setlocale.c:310-325, uselocale.c:110-137, iswalpha_l.c:233-258, nl_langinfo.c:224-235 |
| M14 | A new thread starts with the global locale (the thread-local locale isn't inherited, XSH pthread_create) | TESTED (new) | locale/uselocale-thread.c:92 |
| M15 | uselocale in one thread doesn't change another's | TESTED (new) | uselocale-thread.c:94-103, 128-135 (values everywhere, MB_CUR_MAX with mb) |
| M16 | setlocale doesn't affect a thread with its own locale | TESTED (new; mb) | uselocale.c:127-130 |
| M17 | No library function calls setlocale | NOT TESTABLE | A negative property; inferred by reading (no caller in libc) |
| M18 | Incompatible codesets between categories give undefined results | NOT TESTABLE | No requirement on the implementation |

### `<locale.h>` (basedefs/locale.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | NULL defined | TESTED (new) | locale/locale_h.c:52-54 |
| L2 | LC_ALL, LC_COLLATE, LC_CTYPE, LC_MESSAGES, LC_MONETARY, LC_NUMERIC, LC_TIME are macros, integer constant expressions with distinct values | TESTED (new) | locale_h.c:57-60, 75, 116-118 |
| L3 | LC_*_MASK and LC_ALL_MASK macros; each mask a bit, LC_ALL_MASK all of them | TESTED (new) | locale_h.c:62-66, 76, 120-126 |
| L4 | LC_GLOBAL_LOCALE defined, of type locale_t | TESTED (new) | locale_h.c:128-129 |
| L5 | locale_t defined | TESTED (new) | locale_h.c:102-108 |
| L6 | struct lconv has the 24 members with their types | TESTED (new) | locale_h.c:131-154 (_Generic) |
| L7 | Prototypes for the seven functions | TESTED (new) | locale_h.c:100-114 (getlocalename_l: not glibc, which lacks it) |
| L8 | The prototypes don't use the application's names | TESTED (new) | locale_h.c:37-45 (macros `category`, `locale`, `base`, `mask` defined before the header) |
| L9 | Namespace: nothing outside LC_[A-Z], the reserved names and the listed names in strict mode | TESTED (inferred) | Macro dump of each header in strict mode, by hand: only MALLOC_ALIGNMENT (known) |

### setlocale

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | A query (NULL) returns the current setting and changes nothing | TESTED (new) | setlocale.c:288-292 |
| S2 | The returned string restores the locale, for LC_ALL and each category | TESTED (new) | setlocale.c:293-298, 317-323 (mb: restores LC_CTYPE C.UTF-8) |
| S3 | An unknown locale returns NULL and changes nothing | TESTED (new) | setlocale.c:301-306, 312-315 (mb) |
| S4 | "" uses the environment (see M10-M12) | TESTED (new) | **Deviation 1, fixed** |
| S5 | LC_ALL "" checks every category before changing anything | TESTED (new) | setlocale.c:209, 214 |
| S6 | Each category's name, in the query and the return value, is that category's | UNTESTED | **Deviation left (mb only)**: picolibc reports the LC_CTYPE name for every category |
| S7 | The LC_ALL query encodes every category | UNTESTED | Same: with only LC_CTYPE varying, the one name restores everything |
| S8 | Avoids data races with functions that don't use the global locale | NOT TESTABLE | Inferred: it writes one int |

### localeconv

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | Members describe the current locale (POSIX values, see M5), also with a thread locale | TESTED (new) | localeconv.c:98-107 |
| C2 | char members are non-negative or CHAR_MAX; strings may be "" except decimal_point | TESTED (new) | localeconv.c:64-89 |
| C3 | No library function calls localeconv; no data races | NOT TESTABLE | Inferred: it returns a constant structure |

### newlocale, duplocale, freelocale, getlocalename_l

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | Creates a locale object for "C" and "POSIX", with all categories or one | TESTED (new) | newlocale.c:185-209 |
| N2 | EINVAL for a mask bit that isn't a category | TESTED (new) | newlocale.c:212-216 |
| N3 | ENOENT when a category's locale data isn't available | TESTED (new) | newlocale.c:219-224. **Deviation 2, fixed** (was EINVAL) |
| N4 | EINVAL may be given for a null locale name | TESTED (inferred) | newlocale.c's library code returns EINVAL; not called (a null string is outside the "valid string pointer" a test can pass portably) |
| N5 | The categories in the mask come from the named locale | TESTED (new; mb) | newlocale.c:255. **Deviation 2, fixed** (LC_CTYPE was tested against LC_CTYPE, not LC_CTYPE_MASK) |
| N6 | Categories not in the mask come from base, or from the POSIX locale for a new object | TESTED (new; mb) | newlocale.c:258-267. **Deviation 2, fixed** (base was ignored) |
| N7 | A failure leaves base valid and unchanged | TESTED (new) | newlocale.c:227-230 |
| N8 | "" uses the environment, ENOENT for an unknown value | TESTED (new) | newlocale.c:160-172. **Deviation 1, fixed** |
| N9 | ENOMEM | NOT TESTABLE | picolibc's locale objects allocate nothing |
| N10 | duplocale copies a locale object | TESTED (new) | newlocale.c:72-80 |
| N11 | duplocale(LC_GLOBAL_LOCALE) copies the global locale, and the copy doesn't follow later setlocale calls | TESTED (new; second part mb) | newlocale.c:240-248, 269-275 |
| N12 | freelocale doesn't modify errno | TESTED (new) | newlocale.c:196-198 |
| N13 | getlocalename_l gives a category's name, of an object or of LC_GLOBAL_LOCALE | TESTED (new; not glibc) | newlocale.c:188, 235, 244-246, 255-274 |
| N14 | getlocalename_l(LC_ALL) gives a string setlocale(LC_ALL) accepts and that restores the locale | TESTED (new; not glibc) | newlocale.c:189, 247 |
| N15 | getlocalename_l fails for an invalid category | TESTED (new; not glibc) | newlocale.c:190-194 |

### uselocale

| # | Requirement | Status | Reference |
|---|---|---|---|
| U1 | (locale_t)0 returns the current locale without changing it | TESTED (new) | locale/uselocale.c:76-77, 85-86. **Deviation 3, fixed** (it uninstalled the thread locale) |
| U2 | Installs a locale object and returns the previous one, or LC_GLOBAL_LOCALE | TESTED (new) | uselocale.c:84-88, 99 |
| U3 | LC_GLOBAL_LOCALE uninstalls the thread locale | TESTED (new) | uselocale.c:91-93, 100 |
| U4 | EINVAL may be given for an invalid object | TESTED (inferred) | Added to the library; not called (no portable invalid handle) |
| U5 | Effects on other threads and on functions | TESTED (new) | see M8, M14-M16 |

### `<langinfo.h>` and nl_langinfo

| # | Requirement | Status | Reference |
|---|---|---|---|
| I1 | nl_item and locale_t defined | TESTED (new) | langinfo/langinfo_h.c:72-150 |
| I2 | Every item constant defined, with distinct values (including ALTMON_1-12 and ABALTMON_1-12) | TESTED (new) | langinfo_h.c:72-150, 154-160. **Deviation 4, fixed** (ALTMON and ABALTMON were missing) |
| I3 | The item constants have type nl_item | UNTESTED | **Deviation left**: they are int enumerators, and nl_item is `__int32_t`, which is long on some targets (not on Linux) |
| I4 | Prototypes for nl_langinfo and nl_langinfo_l | TESTED (new) | langinfo_h.c:150-153 |
| I5 | YESSTR and NOSTR (removed) not defined in strict mode | TESTED (new) | langinfo/langinfo_h-names.c:41-42. **Deviation 5, fixed** |
| I6 | Every item gives the POSIX locale's value (DAY, ABDAY, MON, ABMON, D_T_FMT, D_FMT, T_FMT, T_FMT_AMPM, AM_STR, PM_STR, RADIXCHAR, YESEXPR, NOEXPR) | TESTED (new) | nl_langinfo.c:71-138, 165-178 |
| I7 | Items the POSIX locale leaves undefined (CODESET, THOUSEP, ERA*, ALT_DIGITS, CRNCYSTR, ALTMON, ABALTMON) still give a string | TESTED (new) | nl_langinfo.c:140-152, 179-185 |
| I8 | An invalid item gives an empty string | TESTED (new) | nl_langinfo.c:155-157, 186-192. **Deviation 6, fixed** (null pointer) |
| I9 | nl_langinfo_l uses its locale; nl_langinfo the thread's | TESTED (new) | nl_langinfo.c:204-212, 224-233 (mb) |
| I10 | CODESET names the codeset | TESTED (new; picolibc) | nl_langinfo.c:216-231 (accepted by iconv_open). **Deviation 7, fixed** (it was the locale name) |
| I11 | Pointer lifetime rules | NOT TESTABLE | Only permissions for the implementation |

### `<monetary.h>` and strfmon

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | locale_t, size_t, ssize_t defined; prototypes with restrict | TESTED (new) | monetary/strfmon-posix.c:115-124 |
| F2 | Plain characters copied; %% | TESTED (new) | strfmon-posix.c:52-54 |
| F3 | %n and %i; with neither '+' nor '(' the POSIX locale uses '-' before the value | TESTED (new) | strfmon-posix.c:55-58 |
| F4 | '+' fails with EINVAL when positive_sign and negative_sign are both empty | TESTED (new; not glibc) | strfmon-posix.c:151-158. **Deviation 8, fixed** |
| F5 | '(' encloses negative values | TESTED (new) | strfmon-posix.c:60-61 |
| F6 | '!' and '^' | TESTED (new) | strfmon-posix.c:63-64 |
| F7 | Field width, right- and left-justified with '-' | TESTED (new) | strfmon-posix.c:66-68 |
| F8 | Left precision, fill with =f, the fill ignored without left precision | TESTED (new) | strfmon-posix.c:70-75 |
| F9 | Left precision pads positive and negative formats to the same length, also with '(' | TESTED (new; not glibc) | strfmon-posix.c:70-73, 80-82. **Deviation 9, fixed** |
| F10 | Right precision rounds; 0 drops the radix character | TESTED (new) | strfmon-posix.c:77-79 |
| F11 | Default right precision from the locale | TESTED | test-monetary/strfmon.c (picolibc trims the default 6 digits; frac_digits is CHAR_MAX, so the value is implementation-defined) |
| F12 | Several conversions; extra arguments ignored | TESTED (new) | strfmon-posix.c:142-149 |
| F13 | E2BIG when the result and its null byte don't fit, at the exact size | TESTED (new) | strfmon-posix.c:160-170 |
| F14 | strfmon_l uses its locale; strfmon the thread's | TESTED (new) | strfmon-posix.c:137-141, 173-176; posix/monetary/strfmon_l.c |
| F15 | Grouping, currency symbols, sign positions | NOT TESTABLE | The POSIX locale has none, and picolibc has no other monetary data |
| F16 | Inf and NaN, maxsize > SSIZE_MAX | NOT TESTABLE | Unspecified / implementation-defined |

### `<nl_types.h>`, catopen, catgets, catclose

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | nl_catd, nl_item, NL_SETD, NL_CAT_LOCALE; prototypes | TESTED (new) | nl_types/catopen-files.c:65-68, 92-97 |
| T2 | catopen returns (nl_catd)-1 and sets errno on failure | TESTED (new) | catopen-files.c:75-87 and every call |
| T3 | ENOENT: missing catalog by pathname, with oflag 0 or NL_CAT_LOCALE | TESTED (new) | catopen-files.c:101-103 |
| T4 | ENOENT: empty name | TESTED (new) | catopen-files.c:104 |
| T5 | ENOENT: files exist but are all of an invalid format, by pathname and through NLSPATH | TESTED (new; not glibc) | catopen-files.c:120-126, 130-153 |
| T6 | catopen opens a catalog; NLSPATH, LANG and LC_MESSAGES select it; FD_CLOEXEC | NOT TESTABLE | The catalog format is implementation-defined and picolibc has none (no gencat), so no file is a valid catalog |
| T7 | catgets returns s when it can't retrieve the message | TESTED | nl_types/catopen.c:66 (on (nl_catd)-1, whose use POSIX leaves undefined) |
| T8 | catgets ENOMSG, EINTR; catclose of an open catalog | NOT TESTABLE | No catalog can be opened |
| T9 | EACCES, EMFILE, ENFILE, ENAMETOOLONG, ENOTDIR, ENOMEM (may fail) | NOT TESTABLE | catopen opens no file |

### `<libintl.h>` and the gettext family

| # | Requirement | Status | Reference |
|---|---|---|---|
| G1 | locale_t defined; prototypes for the 15 functions | TESTED (new) | libintl/gettext_l.c:125-141 (the `_l` ones: not glibc, which lacks them) |
| G2 | In the POSIX locale every gettext function returns msgid | TESTED | libintl/gettext.c:52-56; gettext_l.c:79-84, 100-103 (new) |
| G3 | ngettext and friends return msgid if n is 1, msgid_plural otherwise | TESTED | gettext.c:58-63; gettext_l.c:85-92, 104-109 (new, including n = ULONG_MAX) |
| G4 | A null domainname uses the current text domain | TESTED (new) | gettext_l.c:82, 90 |
| G5 | The `_l` forms use their locale | TESTED (new; not glibc) | gettext_l.c:98-113 |
| G6 | errno is never modified | TESTED | gettext.c:52-64; gettext_l.c:78, 115 (new) |
| G7 | In the POSIX locale LANGUAGE is ignored, and NLSPATH and bindings don't produce translations | TESTED (new) | gettext_l.c:168-214 (child with LANGUAGE, NLSPATH and a binding to a mkdtemp directory holding a file) |
| G8 | Lookup of messages objects, LANGUAGE search, codeset conversion | NOT TESTABLE | The messages-object format is unspecified (utilities/msgfmt) and picolibc reads none, so it always takes the "no suitable messages object" path |
| G9 | Returned strings aren't invalidated by later gettext calls or uselocale | TESTED (inferred) | They are msgid or msgid_plural themselves |
| G10 | textdomain sets, queries, and resets with "" to "messages" | TESTED | libintl/textdomain.c:51-55 |
| G11 | The text domain survives setlocale and uselocale | TESTED (new) | gettext_l.c:154-160 |
| G12 | bindtextdomain binds, rebinds and queries; default directory for an unbound domain | TESTED | libintl/bindtextdomain.c:52-59; gettext_l.c:165-166 |
| G13 | bindtextdomain with a null or empty domain returns NULL without changing errno | TESTED | bindtextdomain.c:62-65 |
| G14 | bindtextdomain returns a copy of dirname, valid until the next binding of that domain | TESTED (new) | gettext_l.c:146-151 |
| G15 | Bindings survive setlocale and uselocale | TESTED (new) | gettext_l.c:161 |
| G16 | bind_textdomain_codeset binds, queries, NULL when none is bound, NULL domain without errno change | TESTED | libintl/bind_textdomain_codeset.c:52-63 |
| G17 | ENOMEM (may fail); on failure the old binding stays | UNTESTED | Needs malloc to fail inside the call; inferred by reading textdomain.c (it allocates before changing anything) |

### `<iconv.h>`, iconv_open, iconv, iconv_close

| # | Requirement | Status | Reference |
|---|---|---|---|
| V1 | iconv_t and size_t defined; prototypes with restrict | TESTED (new) | iconv/iconv-posix.c:215-220 |
| V2 | iconv_open returns a descriptor, or (iconv_t)-1 with EINVAL for an unsupported conversion | TESTED | iconv-posix.c:222-226 (new); test-iconv/iconv.c (every charset pair through UTF-8) |
| V3 | EINVAL may be given for an unknown indicator suffix | TESTED (new; picolibc) | iconv-posix.c:228-231 |
| V4 | A full conversion updates all four pointers and counts | TESTED (new) | iconv-posix.c:98-101 |
| V5 | A null byte is a character like any other | TESTED (new) | iconv-posix.c:104-106. **Deviation 10, fixed** (it ended the input) |
| V6 | E2BIG stops before the first character that doesn't fit, with inbytesleft non-zero | TESTED (new) | iconv-posix.c:109-112, 201-202 (mb: multibyte). **Deviation 10, fixed** (non-mb: no E2BIG at all; mb: the character was consumed and buffered) |
| V7 | EILSEQ stops before an invalid input byte | TESTED (new) | iconv-posix.c:115-117, 205-206. **Deviation 10, fixed** (non-mb passed bytes 0x80-0xff through) |
| V8 | EINVAL for an incomplete character at the end, left unconverted | TESTED (new; mb) | iconv-posix.c:197-198. **Deviation 10, fixed** (it was consumed into the state and iconv succeeded) |
| V9 | A null inbuf or *inbuf resets the shift state and writes the reset sequence | TESTED (new) | iconv-posix.c:120-126 (stateless: nothing written). **Deviation 10, fixed** (no reset, and a crash without mb support). The JIS reset sequence is exercised by test-iconv/iconv.c's resets |
| V10 | //IGNORE discards invalid input and continues, without an error | TESTED (new; not glibc) | iconv-posix.c:129-136 |
| V11 | //IGNORE and //NON_IDENTICAL_DISCARD discard characters missing from the output codeset and count them | TESTED (new; mb, not glibc) | iconv-posix.c:174-191. **Deviation 10, fixed** (the output pointer moved back a byte) |
| V12 | //TRANSLIT converts and counts them | TESTED (new; mb) | iconv-posix.c:168-172 |
| V13 | Without a suffix, an implementation-defined conversion, counted | TESTED (new; mb, not glibc) | iconv-posix.c:155-165. **Deviation 10, fixed** (EILSEQ) |
| V14 | EBADF may be given for an invalid descriptor | TESTED (new; picolibc) | iconv-posix.c:233-238. **Deviation 11, fixed** (EINVAL) |
| V15 | iconv_close returns 0 and frees the descriptor | TESTED (new) | iconv-posix.c (every close) |
| V16 | Shift states for state-dependent encodings across calls | TESTED | test-iconv/iconv.c (JIS round trips in pieces; mb) |
| V17 | EMFILE, ENFILE, ENOMEM (may fail); FD_CLOEXEC | NOT TESTABLE | No descriptors; ENOMEM needs calloc to fail |

### `<ctype.h>` `_l` functions

| # | Requirement | Status | Reference |
|---|---|---|---|
| K1 | locale_t defined; prototypes for the 14 `_l` functions | TESTED (new) | ctype/isalpha_l.c:229-231 and every call |
| K2 | The prototypes and inline definitions don't use the application's names | TESTED (new) | isalpha_l.c:37-42 (macros `c` and `l`). **Deviation 12, fixed** |
| K3 | isalnum_l ... isxdigit_l give the locale's classes for EOF and every byte, as macros and as functions | TESTED (new) | isalpha_l.c:169-198, 235-237 |
| K4 | tolower_l and toupper_l convert only letters with a counterpart; everything else unchanged | TESTED (new) | isalpha_l.c:200-214 |
| K5 | They use their locale argument (ISO-8859-1 object while the global locale is POSIX) | TESTED (new; mb) | isalpha_l.c:247-254, 260 |
| K6 | LC_GLOBAL_LOCALE or an invalid object | NOT TESTABLE | Undefined behaviour |

### `<wctype.h>` `_l` functions

| # | Requirement | Status | Reference |
|---|---|---|---|
| W1 | wint_t, wctype_t, wctrans_t, locale_t, WEOF; prototypes | TESTED (new) | wctype/iswalpha_l.c:215-220 and every call |
| W2 | wctype_l and wctype return non-zero for the 12 standard classes, 0 for an unknown one | TESTED (new) | iswalpha_l.c:140-147 |
| W3 | iswctype_l and the isw*_l functions give the locale's classes | TESTED (new) | iswalpha_l.c:156-165, 177-191 (POSIX locale: ASCII exact, others only cntrl/punct) |
| W4 | iswctype_l(wc, 0, l) is 0 | TESTED (new) | iswalpha_l.c:166 |
| W5 | WEOF is in no class; towlower_l and towupper_l return it unchanged | TESTED (new) | iswalpha_l.c:194-196 |
| W6 | wctrans_l gives non-zero for tolower and toupper, 0 for an unknown name (EINVAL may be set) | TESTED (new) | iswalpha_l.c:149-153 |
| W7 | towctrans_l maps by the descriptor; returns wc for (wctrans_t)0 | TESTED (new) | iswalpha_l.c:170-174, 244 |
| W8 | towlower_l and towupper_l | TESTED (new) | iswalpha_l.c:167-169, 190, 242 |
| W9 | They use their locale argument (C.UTF-8 object while the global locale is POSIX) | TESTED (new; mb) | iswalpha_l.c:239-253 |
| W10 | LC_GLOBAL_LOCALE or an invalid object; a wctype_t from another locale | NOT TESTABLE | Undefined or unspecified |

## Deviations found and fixed

Each fix was proved by reversing only its library change (`git show <fix> -- libc libos | git apply -R`), rebuilding and running the named test; the failing check is quoted. "(mb)" means the proof ran in the mb-capable build.

1. **setlocale and newlocale ignored the environment for ""** (libc/locale/setlocale.c, newlocale.c; new locale_env.c). "" always meant the C locale. Now each category takes LC_ALL, then its LC_* variable, then LANG, skipping empty values; `setlocale(LC_ALL, "")` checks every category first and fails, changing nothing, if any names a locale picolibc lacks; newlocale fails with ENOENT. doc/build.md says so.
   - Failing checks: `setlocale.c: LANG unknown: setlocale(0, "") gave "C"`; `newlocale.c: LANG unknown: newlocale didn't fail with ENOENT`.
   - Behaviour change: without mb support, an environment naming a UTF-8 locale (the chroot has `LANG=en_US.UTF-8`) now makes `setlocale(LC_ALL, "")` return NULL. Programs that ignore the result keep the C locale, as before.
2. **newlocale's error, mask test and base** (libc/locale/newlocale.c).
   - An unknown locale gave EINVAL; POSIX says ENOENT. Failing check (with fix 1 also reversed, since it rewrote the same lines): `unknown locale is ENOENT`.
   - The mask was tested against LC_CTYPE (2, which is LC_COLLATE_MASK) instead of LC_CTYPE_MASK, so LC_CTYPE_MASK alone gave the C locale and LC_COLLATE_MASK changed the charset. Failing check (mb, that line reverted alone): `LC_CTYPE_MASK sets LC_CTYPE`.
   - Without LC_CTYPE_MASK, base's LC_CTYPE was dropped. Failing check (mb, that line reverted alone): `base keeps LC_CTYPE`.
3. **uselocale((locale_t)0) uninstalled the thread's locale** (libc/locale/uselocale.c) instead of only reporting it. It also accepted any value; it now gives EINVAL for one that isn't a locale. Failing checks: `uselocale.c: querying leaves it installed`, `uselocale-thread.c: main uninstalls its locale`.
4. **ALTMON_1..12 and ABALTMON_1..12 were missing** (libc/include/langinfo.h, nl_langinfo.c). Added after CRNCYSTR, so existing values don't change; the POSIX locale has none, so they return the usual month names. Failing at compile time: `'ALTMON_1' undeclared`.
5. **YESSTR and NOSTR were defined in strict mode** (langinfo.h). They were removed from POSIX and `<langinfo.h>` reserves no prefix for them. They are now `_NL_YESSTR`/`_NL_NOSTR` with the old values, and YESSTR/NOSTR are macros only for `_DEFAULT_SOURCE`. Failing at compile time: test-langinfo_h-names doesn't build (its `static int YESSTR;` collides with the macro).
6. **nl_langinfo returned a null pointer for an invalid item** (nl_langinfo.c), including YESSTR and NOSTR. POSIX requires an empty string. Failing check: `invalid item -1: got (null)`.
7. **CODESET gave the locale name** ("C", "C.UTF-8"), which isn't a codeset and which iconv_open rejects. It now gives "ASCII" for the C locale and the codeset part ("UTF-8") otherwise. Failing checks: `nl_langinfo.c: iconv_open of CODESET`; (mb) `uselocale.c: nl_langinfo follows it`.
8. **strfmon accepted '+'** (libc/posix/monetary/vstrfmon.c). With positive_sign and negative_sign both empty, as in the POSIX locale (picolibc's only monetary data), POSIX says it shall fail with EINVAL. test/test-monetary/test-strfmon.c expected glibc's output and now keeps that for glibc only. Failing check: `'+' is EINVAL`.
9. **strfmon didn't pad positive values to the width of parenthesised negatives** (vstrfmon.c). With a left precision and '(', a negative value has both parentheses but a positive one had only the leading space. test-strfmon.c's expectations (shared with Zephyr's test) changed the same way, keeping glibc's for glibc. Failing check: `strfmon "%(#3.2n" 12.5: got 7 "  12.50", expected "  12.50 "`.
10. **iconv** (libc/iconv/iconv.c, iconv_open.c, iconv_private.h), rewritten to convert one character at a time and commit its input, output and states only when its output fits:
    - Without mb support it copied what fit and returned 0: no E2BIG, bytes 0x80-0xff passed through as "ASCII", and a null `*inbuf` crashed (SIGSEGV). The ASCII conversion now runs there too, with suffix parsing.
    - A character whose output didn't fit was consumed and kept in the descriptor, so E2BIG came back with inbytesleft 0.
    - An incomplete character at the end was consumed into the state and iconv succeeded, instead of EINVAL with the bytes left.
    - A null byte ended the conversion and the rest of the input was dropped.
    - //IGNORE and //NON_IDENTICAL_DISCARD moved the output pointer back one byte per discarded character.
    - Without a suffix, a character missing from the output codeset failed with EILSEQ; POSIX.1-2024 asks for an implementation-defined conversion, counted. picolibc writes '?', as //TRANSLIT does.
    - A null inbuf didn't reset the shift states or write the reset sequence.
    - Failing checks with the old code (the test was temporarily made to report every failure): default build, `E2BIG`, `no room at all`, `EILSEQ`, `EILSEQ pointers`, `unknown suffix`, then SIGSEGV at `*inbuf null`; mb build, `null byte`, `E2BIG` (both), `E2BIG pointers`, `EINVAL`, `EINVAL pointers`, `EILSEQ pointers`, `*inbuf null`, `//IGNORE drops invalid bytes`, `non-identical conversion counted`, `//IGNORE discards and counts`, `//NON_IDENTICAL_DISCARD discards and counts`.
11. **iconv and iconv_close gave EINVAL for (iconv_t)-1.** POSIX lists EBADF; iconv's EINVAL means an incomplete character. Failing check: `iconv EBADF`.
12. **`<ctype.h>` named its parameters `c` and `l`**, in the prototypes and the inline functions, so an application macro with either name broke the header. Failing at compile time: `ctype.h:65: expected ';', ',' or ')' before '+' token`.

## Deviations left

- **Locale objects carry only LC_CTYPE** (mb-capable builds only). locale_t is an int naming the LC_CTYPE charset, so `setlocale(LC_NUMERIC, "C.UTF-8")` succeeds but reports, and `setlocale(category, NULL)` and `getlocalename_l(category, l)` later report, the LC_CTYPE name for every category, and `setlocale(LC_ALL, NULL)` never gives a composite string. The data of the other categories is the POSIX locale's in every locale, so the strings still restore the locale. Fixing the names needs a different locale_t (an ABI decision). Without mb support there is only "C", so nothing differs.
- **The `<langinfo.h>` constants are int enumerators, while nl_item is `__int32_t`** (sys/_types.h), which is long on targets such as 32-bit ARM EABI. POSIX gives the constants type nl_item. Making nl_item int would change its size on 16-bit-int targets (AVR, MSP430), so this is left for a decision; on Linux both are int.
- **catopen never opens a catalog and gettext never finds a messages object.** POSIX leaves both formats to the implementation (gencat, msgfmt), and picolibc has neither tool. catopen fails with ENOENT ("all files are of an invalid format"), and in the C locale gettext must return msgid anyway. Recorded as design, as unistd-config did for the NL_* limits.
- **picolibc's C locale isn't the 256-character single-byte locale of XBD 6.2**: mbtowc gives EILSEQ for bytes 0x80-0xff, so iconv's "ASCII" and CODESET follow suit. For the wide unit (cross-cutting.md).

## glibc 2.43 differences (the tests skip these checks for glibc with `__GLIBC__`)

- No getlocalename_l, no ABALTMON_ items, ALTMON_ only with `_GNU_SOURCE`, and none of the gettext `_l` functions.
- strfmon accepts '+' when both sign strings are empty, and doesn't pad positive values to match '('.
- catopen of a file that isn't a catalog fails without setting errno.
- iconv: //IGNORE ends with EILSEQ after dropping input; a character missing from the output codeset is EILSEQ without a suffix; //NON_IDENTICAL_DISCARD is accepted but doesn't discard.

## Verified by running versus inferred

Every TESTED row was run on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and (where not marked) the `-native` glibc build; rows marked (mb) ran in `build-aarch64-mb` (aarch64, `-Dmb-capable=true -Dmb-extended-charsets=true`, full suite). Rows marked "TESTED (inferred)" were read in the code, not run: L9 (namespace, from a macro dump of each header), N4, U4 and G9.

## Commits

Library fixes:
```
42e31b292d locale: Make uselocale((locale_t)0) leave the thread's locale alone
d7cace544b locale: Fix newlocale's error, its LC_CTYPE test and its use of base
dea1d11e2a locale: Take setlocale and newlocale's "" locale from the environment
b8f7396745 langinfo.h: Add ALTMON_1..ALTMON_12 and ABALTMON_1..ABALTMON_12
e92678387e locale: Return an empty string from nl_langinfo for an invalid item
3fb3ad3b53 locale: Report the codeset name, not the locale name, for CODESET
10afdc9440 langinfo.h: Define YESSTR and NOSTR only for _DEFAULT_SOURCE
38805fd80f strfmon: Fail with EINVAL for the '+' flag
f82975b2a3 strfmon: Pad positive values to the width of parenthesised negatives
6bdd39a195 iconv: Follow POSIX for full buffers, bad input and unconvertible characters
8deadd09d8 iconv: Report an invalid descriptor with EBADF
3dee71aceb ctype.h: Use reserved names for parameters
```

Tests (one commit per header area):
```
07930ea021 test-posix: Check <locale.h>, setlocale, newlocale, uselocale and localeconv
1ff73d9b2d test-posix: Check <langinfo.h> and nl_langinfo
d2fc3fda8c test-posix: Check strfmon's flags, precisions and errors in the POSIX locale
cf199e1eb0 test-posix: Check <nl_types.h> and catopen's failures for files
f93de59d7d test-posix: Check the gettext _l functions and the POSIX locale's messages
47a0af166b test-posix: Check iconv's pointers and errors, and the indicator suffixes
5052a3ad2d test-posix: Check the <ctype.h> and <wctype.h> _l functions
```

- New files: locale/locale_h, localeconv, newlocale, setlocale, uselocale, uselocale-thread; langinfo/langinfo_h, langinfo_h-names, nl_langinfo; monetary/strfmon-posix; nl_types/catopen-files; libintl/gettext_l; iconv/iconv-posix; ctype/isalpha_l; wctype/iswalpha_l.
- Changed: test/test-monetary/test-strfmon.c (expectations for fixes 8 and 9, glibc's kept under `__GLIBC__`).

## Verification

| Build | Ok | Fail | Expected Fail | Skipped |
|---|---|---|---|---|
| aarch64-linux-gnu, full suite | 1609 | 0 | 6 | 42 |
| arm-linux-gnueabihf, full suite | 1608 | 0 | 6 | 44 |
| aarch64-linux-gnu with `-Dmb-capable=true -Dmb-extended-charsets=true`, full suite | 1611 | 0 | 6 | 41 |
| clang native, full `meson test` | 1234 | 0 | 6 | 383 |

- The base had Ok 1579 and 42 Skipped on aarch64; the 30 new tests (15 files, each with a `-native` build) all pass.
- gcc native configuration: builds (ninja only).
- CMake thumbv7m `ninja test`: 100% of 633 tests passed (282 passed, 351 skipped); the new tests pass there except test-uselocale-thread, which skips (no threads).
- A last test-only change (clang's -Wunneeded-internal-declaration and an unused variable in the glibc build) came after the full aarch64 and arm runs; the test-locale suite was rerun on aarch64, arm and mb afterwards (12/12 each) and the gcc native build rebuilt.
- No incidents with the host security software.
