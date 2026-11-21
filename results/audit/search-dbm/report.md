# `<search.h>` and `<ndbm.h>` audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-search-dbm`, branch `audit/search-dbm`, based on 1bac8ef806 (`claude/posix-spec-audit`). The host is an AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16, glibc 2.43) as an unprivileged user who has supplementary groups. Nothing was pushed.

**Implementation:**
- `libc/search/`:
  - hcreate.c and hcreate_r.c: hcreate, hdestroy and hsearch, built on the `_r` versions with chained buckets.
  - lsearch.c, lfind.c.
  - tsearch.c, tfind.c, tdelete.c, twalk.c: unbalanced BSD trees.
  - insque.c, remque.c.
  - ndbm.c, on the 4.4BSD hash code (hash.c, hash_page.c, hash_buf.c, hash_bigkey.c, hash_func.c, hash_log2.c).
- Headers: `libc/include/search.h`, `libc/include/ndbm.h`.
- A database is a single file, `<file>.db` (DBM_SUFFIX). POSIX.1-2024 allows this: "When one file is used, the name of the database file shall be formed by appending the suffix .db". There are no .dir/.pag files.

**Tests:**
- `test/test-posix/test-search/`: test-insque.c, test-remque.c, test-lfind.c and test-lsearch.c are extended; test-search_h.c, test-hsearch.c and test-tsearch.c are new.
- `test/test-posix/test-ndbm/` (new): test-ndbm_h.c, test-dbm_open.c, test-dbm_store.c.
- `test/testsuite/newlib.search/hsearchtest.c` (existing).

Line numbers are for the branch tip. Abbreviations:

| Abbreviation | File |
|---|---|
| `SH` | test-search_h.c |
| `HS` | test-hsearch.c |
| `TS` | test-tsearch.c |
| `IQ` | test-insque.c |
| `RQ` | test-remque.c |
| `LF` | test-lfind.c |
| `LS` | test-lsearch.c |
| `NH` | test-ndbm_h.c |
| `DO` | test-dbm_open.c |
| `DS` | test-dbm_store.c |
| `HT` | hsearchtest.c |

**Spec source:** pubs.opengroup.org/onlinepubs/9799919799, fetched 2026-11-21:
- basedefs/search.h and basedefs/ndbm.h;
- functions/hcreate, functions/lsearch, functions/tsearch and functions/insque;
- functions/dbm_clearerr, which covers all the dbm_ functions;
- XSH 2.2.2 for reserved names: `<ndbm.h>` reserves `dbm_` and `DBM_`, and `<search.h>` reserves nothing.

**Status key:** TESTED (file:line), UNTESTED, NOT TESTABLE (reason).
- **Where TESTED rows ran:** every TESTED row was run and passes for picolibc (`test-X`) on aarch64-linux-gnu and arm-linux-gnueabihf.
- **glibc runs of the search tests:** they also pass against glibc (`test-X-native`) unless the row says otherwise. glibc 2.43 has no posix_tnode, so those checks are guarded with `__GLIBC__`.
- **glibc runs of the ndbm tests:** glibc has no `<ndbm.h>` here (gdbm's compatibility headers aren't installed). A meson `cc.has_function('dbm_open')` check skips the ndbm tests' -native builds. So every ndbm row has in practice been run only against picolibc, although the tests are written portably.
- **Search tests:** they need no OS support and run everywhere, including bare metal. Their out-of-memory checks need `__PICOLIBC_POSIX_MULTI_PROCESS` (they fork and set RLIMIT_DATA in the child). Those checks are skipped under qemu-user, which doesn't apply RLIMIT_DATA.
- **ndbm tests:** they need `__PICOLIBC_POSIX_FILE_SYSTEM`; test-dbm_open also needs FILE_ATTRIBUTES and FD_MGMT. They work in a fresh `mkdtemp()` directory. The largest database is 20000 records of about 110 bytes (about 2.5MB).

## Requirement table

### `<search.h>` (basedefs/search.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| S1 | ENTRY, for struct entry, with `char *key` and `void *data` | TESTED | SH:49-59, 138; HT |
| S2 | ACTION, an enumeration with FIND and ENTER | TESTED | SH:62, 64; HT |
| S3 | VISIT, an enumeration with preorder, postorder, endorder, leaf | TESTED | SH:63, 65-67 |
| S4 | `<search.h>` defines size_t (checked with search.h as the first header) | TESTED | SH:47 |
| S5 | posix_tnode, a typedef for void (new in Issue 8) | TESTED (glibc 2.43 lacks it; guarded). **Missing; fixed** (D1) | SH:69-76 |
| S6 | The prototypes use posix_tnode for nodes: `posix_tnode **restrict` in tdelete, `posix_tnode *const *` in tfind, `const posix_tnode *` in twalk | TESTED. **tfind took `void **`, so a pointer to a const root needed a cast; fixed** (D1) | SH:78-104; TS:253-261 (tfind through `tnode *const *`) |
| S7 | Namespace: nothing beyond the names above (the header reserves no prefix) | TESTED. **hcreate_r, hdestroy_r, hsearch_r, tdestroy, struct hsearch_data and all of `<stdint.h>` were visible in strict mode; fixed** (D2) | SH:106-128 |

### hcreate, hdestroy, hsearch

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | hcreate allocates the table and returns non-zero | TESTED | HS:227; HT |
| H2 | hcreate returns 0 when it can't allocate the table; ENOMEM (may fail) | TESTED (in a child with RLIMIT_DATA; not under qemu) | HS:180-192 |
| H3 | nel is an estimate and may be adjusted upward; entries past nel either go in or report a full table | TESTED (picolibc's chained buckets never fill; glibc's table is full after 5) | HS:271-291 |
| H4 | hdestroy disposes of the table | TESTED (2000 tables of 1000 entries under a 16MiB data limit; not under qemu). **The entries were leaked; fixed** (D3) | HS:116-143 |
| H5 | hcreate may follow hdestroy, and the old data is no longer accessible | TESTED | HS:265-269 |
| H6 | hsearch returns a pointer into the table where the entry is (the same pointer each time) | TESTED | HS:241-244 |
| H7 | item.key is the comparison key; item.data is the associated data | TESTED | HS:229-239 |
| H8 | The comparison is strcmp: a copy of the key finds the entry, and a prefix, an extension, a change of case or "" is another key | TESTED | HS:76-87, 235-254; HT |
| H9 | ENTER inserts an item that isn't there | TESTED | HS:229-233, 260-263; HT |
| H10 | ENTER of a key that's there returns the existing entry, with its data unchanged | TESTED | HS:246-248 |
| H11 | FIND makes no entry | TESTED | HS:256-257 |
| H12 | Null pointer when FIND doesn't find the item | TESTED | HS:250-257; HT |
| H13 | Null pointer when ENTER finds the table full | TESTED in the portable form of H3 (picolibc's table never fills) | HS:271-291 |
| H14 | hsearch ENOMEM (may fail) | TESTED (child: ENTER with no room for data until it returns NULL, then FIND still works; not under qemu) | HS:145-178 |
| H15 | hsearch before hcreate | NOT TESTABLE: the application "shall ensure" hcreate comes first (picolibc dereferences a null table) | |
| H16 | Need not be thread-safe | NOT TESTABLE (no requirement) | |

### lsearch, lfind

| # | Requirement | Status | Reference |
|---|---|---|---|
| L1 | lsearch returns a pointer to the matching entry | TESTED | LS:93, 97, 111 |
| L2 | lsearch adds a missing entry at the end and returns a pointer to it | TESTED | LS:95, 105 |
| L3 | `*nelp` is incremented when an entry is added, and not otherwise | TESTED | LS:93-97, 111 |
| L4 | The whole element (width bytes; 7 here) is copied, and other elements are untouched | TESTED | LS:100-108 |
| L5 | compar is called with pointers to the elements, and only zero versus non-zero matters | TESTED (a comparison returning only 0 or 1; one argument is always the key) | LF:59-63, 86; LS:61-75, 112 |
| L6 | lfind returns a null pointer when the entry isn't found, and adds nothing | TESTED | LF:76-77 |
| L7 | Empty table: lfind finds nothing, and lsearch adds the first element | TESTED | LF:80-81; LS:114-118 |
| L8 | lfind searches a table of a const type | TESTED | LF:65, 89-90 |
| L9 | "Both functions shall return a null pointer in case of error" | NOT TESTABLE: no errors are defined | |

### tsearch, tfind, tdelete, twalk

| # | Requirement | Status | Reference |
|---|---|---|---|
| T1 | Only the sign of compar's result is used (it returns ±1000 here) | TESTED | TS:99-114 |
| T2 | compar need not compare every byte: an equal element with other data finds the first one's node | TESTED | TS:387-395 |
| T3 | Every comparison involves the key and a stored element (one argument is always the key) | TESTED | TS:99-114, 385, 400, 427 |
| T4 | tsearch returns the node already there for an equal element | TESTED | TS:393-394 |
| T5 | tsearch inserts a missing element and returns its node | TESTED | TS:380-384 |
| T6 | Only the pointer is stored: `*(elem **)node` is the key pointer itself | TESTED | TS:367, 383 |
| T7 | Empty tree (*rootp NULL): *rootp is set to the new root | TESTED | TS:364-367 |
| T8 | tfind returns the node, or a null pointer when it's missing | TESTED | TS:392, 397-399 |
| T9 | tdelete removes the node; everything else stays findable and in order | TESTED | TS:411-431 |
| T10 | tdelete sets *rootp to the new root when the root changes | TESTED | TS:434-442 |
| T11 | Deleting a root with no children sets *rootp to NULL | TESTED | TS:375-378 |
| T12 | tdelete returns the parent of the deleted node | TESTED (parents taken from a walk before each deletion, for every other node of 500, and for a leaf of the seven-node tree) | TS:411-425, 475-477 |
| T13 | tdelete returns an unspecified non-null pointer when the root is deleted | TESTED | TS:377, 424, 438 |
| T14 | tdelete returns a null pointer when the node isn't found | TESTED | TS:362, 406-409 |
| T15 | tsearch, tfind and tdelete return a null pointer for a null rootp | TESTED | TS:354-358 |
| T16 | tsearch returns a null pointer when there is no space for a node | TESTED (child with RLIMIT_DATA; the tree is usable afterwards; not under qemu) | TS:302-335, 492-510 |
| T17 | twalk walks depth-first, left to right; each node is visited as preorder, postorder and endorder, or once as a leaf | TESTED (the exact walk of the seven-node tree, and a check that rebuilds the tree from a walk of 500 nodes, inserted scattered and then sorted) | TS:156-221, 444-461, 403, 487 |
| T18 | twalk's level argument: the root is at level 0, and children are one level deeper | TESTED | TS:156-221, 444-461 |
| T19 | Any node can be the root of a walk below it | TESTED | TS:462-473 |
| T20 | Node pointers, both passed and returned, can be cast to pointer-to-pointer-to-element | TESTED | TS:114, 128-137, 367 |
| T21 | Changing the tree inside compar or action, or using it concurrently after a change: undefined | NOT TESTABLE (undefined) | |
| T22 | Thread-safe as long as threads use different trees | UNTESTED. By reading: the functions keep no state outside the tree | |
| T23 | Balanced trees are encouraged (RATIONALE) | Not a requirement. picolibc's trees are unbalanced, so sorted input gives a linked list (the test inserts 500 sorted elements without trouble) | |

### insque, remque

| # | Requirement | Status | Reference |
|---|---|---|---|
| Q1 | insque inserts after pred in a linear list, including at the end | TESTED | IQ:81-89 |
| Q2 | insque(&e, NULL) sets both of e's pointers to null | TESTED | IQ:77-80 |
| Q3 | Circular lists, started by pointing the first element at itself | TESTED | IQ:91-99 |
| Q4 | remque in a linear list: from the middle, the head and the tail, and the last element | TESTED | RQ:63-76 |
| Q5 | remque in a circular list, down to one element pointing at itself | TESTED | RQ:78-88 |
| Q6 | Any structure and member names work, and further members are untouched | TESTED | IQ:64-69, 101-109 |
| Q7 | Element values are untouched | TESTED | IQ:89 |

### `<ndbm.h>` (basedefs/ndbm.h)

| # | Requirement | Status | Reference |
|---|---|---|---|
| N1 | datum with `void *dptr` and `size_t dsize` | TESTED. **dsize was int; fixed** (D4) | NH:50-60, 111 |
| N2 | `<ndbm.h>` defines size_t (checked as the first header) | TESTED | NH:47 |
| N3 | `<ndbm.h>` defines mode_t | TESTED | NH:48 |
| N4 | The DBM type | TESTED | NH:62-63 |
| N5 | DBM_INSERT and DBM_REPLACE, which differ | TESTED | NH:65-66 |
| N6 | The nine prototypes | TESTED | NH:68-87 |
| N7 | Namespace: only dbm_ and DBM_ names | TESTED. **The type DB and all of `<stdint.h>` were defined; fixed** (D5) | NH:89-101 |

### dbm_open, dbm_close, dbm_fetch, dbm_store, dbm_delete, dbm_firstkey, dbm_nextkey, dbm_error, dbm_clearerr

| # | Requirement | Status | Reference |
|---|---|---|---|
| B1 | One file (file.db) or two (file.dir and file.pag), named from the file argument (a path into a subdirectory here), and nothing else | TESTED (picolibc makes one .db file) | DO:90-129, 242-254, 388, 401 |
| B2 | O_RDONLY: the database can be read, and dbm_store and dbm_delete fail with a negative value | TESTED | DO:265-277; DS:280 |
| B3 | O_RDWR | TESTED | throughout DO and DS |
| B4 | O_WRONLY: a write-only database whose files are opened for reading and writing | TESTED (F_GETFL of the new descriptors). **Rejected with EINVAL; fixed** (D6) | DO:328-352 |
| B5 | O_CLOEXEC sets FD_CLOEXEC on the database's descriptors | TESTED | DO:354-368 |
| B6 | O_CREAT on an existing database has no effect (data and mode are kept) | TESTED | DO:305-310 |
| B7 | O_CREAT on a new database creates it, owned by the effective user ID | TESTED | DO:242-262 |
| B8 | Group: the parent directory's or the effective group ID, with some way to get the directory's | TESTED (both; a set-group-ID directory owned by a second group passes that group on) | DO:256-262, 404-437 |
| B9 | O_DSYNC, O_RSYNC, O_SYNC | UNTESTED. By reading: the flags are passed to open() unchanged | |
| B10 | O_CREAT\|O_EXCL fails for an existing database (leaving it intact) and succeeds for a new one | TESTED | DO:286-303 |
| B11 | O_EXCL with two files: remove the first if the second exists ("should") | NOT TESTABLE: picolibc uses one file | |
| B12 | O_TRUNC empties an existing database and keeps its mode and owner | TESTED for the mode; an unprivileged user can't change the owner anyway | DO:312-326 |
| B13 | file_mode means the same as open's third argument (the umask applies) | TESTED (umask 027 with mode 0666 gives 0640) | DO:236-262, 303 |
| B14 | Pathnames up to {PATH_MAX}-4 bytes, with a last component up to {NAME_MAX}-4 bytes, are accepted | TESTED for a {NAME_MAX}-4 component. By reading: dbm_open refuses with ENAMETOOLONG from 1021 bytes on (MAXPATHLEN - 3), which is past {PATH_MAX}-4 | DO:374-389 |
| B15 | dbm_close closes the database (its descriptors go away) | TESTED | DO:370-372 |
| B16 | Key/content pairs of at least 1023 bytes | TESTED (23+1000 and 1000+23) | DS:200-210 |
| B17 | dbm_fetch returns the record's content | TESTED | DS:177-196, 266-277 |
| B18 | DBM_REPLACE replaces an existing record | TESTED | DS:186-188 |
| B19 | DBM_INSERT leaves an existing record alone and returns 1 | TESTED | DS:181-184 |
| B20 | Either mode inserts a new record | TESTED | DS:177-190 |
| B21 | dbm_store returns an error when a block fills with inseparable data | NOT TESTABLE: the BSD hash code moves big pairs to overflow pages, so a block never fills | |
| B22 | dbm_delete deletes a record and its key | TESTED | DS:212-216, 286-306 |
| B23 | dbm_store and dbm_delete return 0 on success and a negative value on failure (missing key, read-only database) | TESTED | DS:175, 215, 280; DO:270-272 |
| B24 | dbm_firstkey and dbm_nextkey return every key once (5 keys; 20000; 10000 after deletes) | TESTED. **Every record was lost past 81 records; fixed** (D7) | DS:121-147, 218-232, 277, 296 |
| B25 | At the end, dptr is a null pointer; an empty database has no first key | TESTED. **dbm_nextkey after the end started again from the first key; fixed** (D8) | DS:173, 233-234 |
| B26 | firstkey/nextkey errors give a null dptr and set the error condition | UNTESTED: nothing here can make the hash code's reads fail. By reading: hash_seq's error returns don't set `hashp->error` (D9, not fixed) | |
| B27 | dbm_fetch gives a null dptr when no record matches (or after an error) | TESTED for no match | DS:174, 197-198, 214 |
| B28 | dbm_error returns 0 when no error is set | TESTED | DS:172, 235, 262 |
| B29 | dbm_clearerr clears the error condition | TESTED (after a failed store on a read-only database) | DO:273-274; DS:279-282 |
| B30 | dbm_open returns (DBM *)0 on error (a missing database without O_CREAT, O_EXCL on an existing one, an unwritable directory) | TESTED | DO:279-290, 391-399 |
| B31 | Keys and contents can be arbitrary binary data (null bytes; the length is dsize) | TESTED | DS:192-198 |
| B32 | The database persists across dbm_close (reopened read-only, then read-write) | TESTED. Part of D7 | DS:249-306; DO:265-277 |
| B33 | Two databases open at once are separate | TESTED | DS:237-245 |
| B34 | dptr may point into static storage that later calls change | NOT TESTABLE (a permission, not a requirement) | |
| B35 | Need not be thread-safe | NOT TESTABLE (no requirement) | |

### Counts

| Area | Requirements | Tested before | Tested after |
|---|---|---|---|
| `<search.h>` | 7 | 2 | 7 |
| hcreate, hdestroy, hsearch | 16 | 4 | 14 |
| lsearch, lfind | 9 | 5 | 8 |
| tsearch, tfind, tdelete, twalk | 23 | 0 | 20 |
| insque, remque | 7 | 3 | 7 |
| `<ndbm.h>` | 7 | 0 | 7 |
| dbm_* | 35 | 0 | 29 |
| **Total** | **104** | **14** | **92** |

"Tested before" is my own row-by-row count of what hsearchtest, test-insque, test-remque, test-lfind and test-lsearch already covered at the base, so it is approximate. Nothing tested tsearch or ndbm before.

The other 12 rows are:
- NOT TESTABLE or not requirements: H15, H16, L9, T21, T23, B11, B21, B34, B35.
- UNTESTED: T22, B9, B26.

## Deviations found and fixed

Each fix was proven the same way:
1. Revert only its library change, with `git show <fix> -- libc | git apply -R`. Where a later commit touches the same lines, undo the change by hand in the current file.
2. Rebuild and run the named test on aarch64-linux-gnu.

Each test failed as shown below, and passes with the fix.

- **D1. posix_tnode was missing, and tfind took `void **`** (libc/include/search.h, libc/search/tfind.c).
  - Issue 8 adds `typedef void posix_tnode` and declares `tfind(const void *, posix_tnode *const *, ...)`.
  - The header now defines posix_tnode and uses it in the four tree prototypes. tfind's definition takes `void *const *`.
  - No ABI change (posix_tnode is void).
  - Reverted: test-search_h fails to compile (`unknown type name 'posix_tnode'`).
  - With posix_tnode present but the old tfind:
    - test-search_h fails with `initialization ... from incompatible pointer type`;
    - test-tsearch fails with `passing argument 2 of 'tfind' discards 'const' qualifier`.
- **D2. `<search.h>` declared extensions and leaked `<stdint.h>`.**
  - XSH 2.2.2 reserves nothing for `<search.h>`, yet in strict mode it declared hcreate_r, hdestroy_r, hsearch_r, tdestroy and struct hsearch_data.
  - `<sys/types.h>` and `<machine/ansi.h>` brought in INT8_MAX, SIZE_MAX, FD_SETSIZE and the rest.
  - The header now takes size_t from `<stddef.h>` and declares the extensions only under `__BSD_VISIBLE`. That macro is set by default and by `_GNU_SOURCE`, so programs without feature-test macros still see them.
  - hcreate.c and tdestroy.c now define `_DEFAULT_SOURCE`. hsearchtest.c now includes `<stdint.h>` for uintptr_t.
  - Reverted: test-search_h fails to compile (`'hcreate_r' redeclared as different kind of symbol`, plus the `#error`s).
- **D3. hdestroy leaked every entry** (libc/search/hcreate_r.c).
  - hdestroy "shall dispose of the search table", but the loop in hdestroy_r that freed the entries was under `#if 0`. FreeBSD's version of that loop also frees the keys, which here belong to the application.
  - hdestroy now frees the entries, but not the keys or data.
  - Reverted: `hdestroy frees the entries: child failed (status 0x6400)`, because ENTER returned NULL under the 16MiB data limit.
- **D4. datum's dsize was an int** (libc/include/ndbm.h).
  - basedefs/ndbm.h requires `size_t dsize`.
  - Reverted: test-ndbm_h fails to compile (`returning 'int *' from a function with incompatible return type 'size_t *'`).
  - This is an ABI change on 64-bit targets, although the structure stays 16 bytes (see cross-cutting.md).
- **D5. `<ndbm.h>` defined DB and leaked `<stdint.h>`.**
  - The header reserves only dbm_ and DBM_, but it typedef'd `DB` (the db(3) handle) and included `<sys/types.h>`.
  - It now typedefs DBM directly from `struct __db`, defines mode_t itself, and takes size_t from `<stddef.h>`.
  - Reverted: test-ndbm_h fails to compile (`'DB' redeclared as different kind of symbol`, `#error "<ndbm.h> defines INT8_MAX"`).
- **D6. dbm_open rejected O_WRONLY** (libc/search/ndbm.c).
  - POSIX says O_WRONLY means "Open the database for writing only. The underlying file(s) used to store the database shall be opened for reading and writing."
  - `__hash_open` returned EINVAL for O_WRONLY. dbm_open now turns O_WRONLY into O_RDWR before opening, as FreeBSD does.
  - Reverted: `dbm_open O_WRONLY: db != NULL`.
- **D7. Every ndbm database lost all its records once it went past 81** (libc/search/hash.c, alloc_segs).
  - Cause: d1dc8cdf20 ("libc/search: Handle alloc failure in __expand_table", 2023) moved the store of the first segment out of alloc_segs' loop, but left `hashp->nsegs` one short.
  - Effect: the table first expands at 82 records (dbm_open uses a fill factor of 40 and two initial buckets). There, `__expand_table` took segment 0 to be missing and replaced it with an empty one. Every bucket's cached page, and so every record, was lost.
  - By reading: hdestroy then freed that segment twice.
  - alloc_segs now counts the first segment.
  - test-dbm_store's 20000-record database found it.
  - Reverted: `fetch many`, `walk many`, `delete half`, `walk half` and `fetch half` all fail.
- **D8. dbm_nextkey went back to the start after the end** (libc/search/hash.c, hash_seq).
  - POSIX: "When the end of the database is reached, the dptr member of the key is a null pointer", and dbm_nextkey returns "the next key until all of the keys in the database have been returned".
  - At the end, hash_seq reset its cursor to -1. The next R_NEXT took -1 to mean "start from the first key", so a loop that called dbm_nextkey once more got the keys again.
  - The cursor now stays at the end until R_FIRST.
  - Reverted: `past the end: dbm_nextkey(db).dptr == NULL`.

## Deviations and problems not fixed

- **D9. Errors in firstkey/nextkey don't set the error condition** (B26).
  - When `__get_buf` fails inside hash_seq, dbm_firstkey and dbm_nextkey return a null dptr but leave `hashp->error` alone, so dbm_error() still returns 0.
  - RETURN VALUE requires that "the error condition of the database shall be set".
  - hash_access's read failures under dbm_fetch and dbm_store behave the same way.
  - The fix would set `hashp->error` on each ERROR return. No test here can make the page reads fail, so it is left as found (by reading only).
- **hcreate's rounding shifts an int** (libc/search/hcreate_r.c:110, `nel = 1 << p2` with `unsigned int p2`).
  - On 64-bit targets, nel above 2^30 overflows int, which is undefined behaviour.
  - On aarch64, 2^30+1 gives a table size of 0xffffffff80000000, so hcreate fails with ENOMEM; 2^31+1 gives a table of one bucket.
  - `(size_t)1 << p2` fixes it, but a test would have to allocate 16GiB or more, so it isn't changed (by reading only).
- **The hash directory's size is stored in bytes** (libc/search/hash.c, `__expand_table`: `hashp->DSIZE = dirsize << 1`, where dirsize is `DSIZE * sizeof(SEGMENT *)`).
  - DSIZE then claims eight times as many entries as were allocated.
  - Past 512 segments (about 131072 buckets, or about 5 million records with dbm_open's parameters), a new segment pointer is written past the end of the directory.
  - FreeBSD has the same line. It should be `hashp->DSIZE <<= 1`.
  - Not changed: a test would need a database far bigger than the few megabytes allowed here (by reading only).

## Other observations (not changed)

- **FD_CLOEXEC is always set:** hash.c sets it with F_SETFD whenever `__HAVE_FCNTL`, with or without O_CLOEXEC. POSIX only says what O_CLOEXEC does, and FreeBSD behaves the same.
- **tdelete's return for the root:** its "unspecified non-null pointer" is the freed root node itself. The RATIONALE says it mustn't be dereferenced, so this is allowed.
- **Undefined hcreate/hsearch cases:** hcreate while a table exists fails with EINVAL. hsearch before hcreate, or after hdestroy, dereferences a null table. Neither case is defined.
- **The table never fills:** picolibc's ENTER never reports a full table, because the buckets are chained lists. glibc's open-addressed table fills; test-hsearch-native prints "table full after 5 entries".
- **Path length:** dbm_open's file argument is limited to 1020 bytes (MAXPATHLEN 1024 less ".db"). Linux allows 4096 (the known PATH_MAX cross-cutting issue).
- **Link-time namespace:** libc/search/hcreate.c calls hcreate_r and its siblings, which POSIX doesn't reserve. A static link can pick up an application's own `hsearch_r` (see cross-cutting.md).
- **No `_POSIX_*` dependence:** neither header, nor libc/search, depends on the `_POSIX_*` option macros, `__PICOLIBC_POSIX_*` or the profile macros.
- **Round 8:** none of the stdlib, strings or locale patches touch libc/search, `search.h` or `ndbm.h`, so there is no merge-order dependence.

## Left undone

- D9, and the two problems above that were found only by reading (none can be provoked within this host's limits).
- O_SYNC, O_DSYNC and O_RSYNC (B9), which dbm_open only passes through to open.

## Test runs (verified by running)

**aarch64-linux-gnu** (`builds/build-aarch64-linux-gnu`, `-Dwerror=true`):
- Full `meson test -t 4` at the branch tip (no `--no-rebuild`): **1645 Ok, 0 Fail, 6 Expected Fail, 41 Skipped.**
- Every new test runs, and so do the `check-cdef`/`check-wundef` header checks for search.h and ndbm.h.
- The -native ndbm tests aren't built (glibc has no `<ndbm.h>`).

**arm-linux-gnueabihf** (`builds/build-arm-linux-gnueabihf`, `-Dwerror=true`):
- Full `meson test -t 4` at the branch tip: **1643 Ok, 0 Fail, 6 Expected Fail, 44 Skipped.**

**clang native** (`builds/build-native-clang`, `CC=clang CXX=clang++ do-native-configure -Dwerror=true`):
- At the branch tip: **1253 Ok, 0 Fail, 6 Expected Fail, 399 Skipped.**
- test-dbm_open and test-dbm_store skip there (no file-system group in that configuration). The search tests and test-ndbm_h pass.
- The first clang run failed to compile test-tsearch.c (`-Wsometimes-uninitialized` on `status`). The fix went into the test commit, and everything above was rerun after it.

**Native configuration** (`scripts/do-native-configure`, `builds/build-native`): `ninja` builds cleanly at the branch tip.

**CMake** (`scripts/do-cmake-thumbv7m-configure`, then `ninja test`, at the branch tip):
- **653 tests, 100% passed (287 ran, 366 skipped).**
- test-ndbm_h, test-hsearch, test-search_h, test-tsearch and the other search tests run there. test-dbm_open and test-dbm_store skip.

**Fix proofs:** every fix was shown to be needed (see "Deviations found and fixed").

**Formatting:** `clang-format --dry-run --Werror` is clean on every source, header and test touched.

**Incidents:** host security software never interfered.

**Not run here:**
- bare-metal meson targets and qemu-user (the x86_64 host runs those). Under qemu, the RLIMIT_DATA checks in test-hsearch and test-tsearch print a message and are skipped.
- The out-of-range hcreate and directory-size problems (by reading only).

## Commits (audit/search-dbm, on top of 1bac8ef806)

- `eeedc6e0ad` search.h: Add posix_tnode and use it in the tree function prototypes
- `e50d82826f` search.h: Hide the extensions and don't include <sys/types.h>
- `2f1dae14cf` search: Free the entries in hdestroy
- `8f67e4270f` ndbm.h: Make datum's dsize a size_t
- `9581897bd2` ndbm.h: Don't define DB or include <sys/types.h>
- `0fcfa95c51` ndbm: Accept O_WRONLY in dbm_open
- `1c2c87c800` search: Count the first segment in alloc_segs
- `46c0d799d7` search: Keep dbm_nextkey at the end of the database
- `0d343e37b0` test-posix: Check <search.h>, hsearch, the tree functions and the queues
- `9d2f5d2701` test-posix: Check <ndbm.h>, dbm_open's flags and files, and the records
