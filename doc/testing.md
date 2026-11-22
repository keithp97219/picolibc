## Building for the local processor

If you want to compile the library for your local processor to test
changes in the library, the meson configuration is happy to do that
for you. You won't need a meson cross compilation configuration file,
so all you need is the right compile options. They're mostly the same
as the embedded version, but you don't want the multi-architecture
stuff and I prefer plain debug to an -Os, as that makes debugging the
library easier.

The do-native-configure script has an example:

```sh
#!/bin/sh
DIR=`dirname $0`
meson $DIR \
    -Dmultilib=false \
    -Dnewlib-wide-orient=false\
    -Dincludedir=lib/picolibc/include \
    -Dlibdir=lib/picolibc/lib \
    -Dtests=true \
    --buildtype debug
```

Again, create a directory and build there:

    $ mkdir build-native
    $ cd build-native
    $ ../scripts/do-native-configure
    $ ninja

This will also build a test case for printf and scanf in the
'test' directory, which I used to fix up the floating point input and
output code.

## Claim-all mode for the POSIX tests

The tests in test/test-posix decide what to run from the `TEST_`
symbols in test/test-posix/test-flags.h: each POSIX option group
(`TEST_POSIX_FILE_SYSTEM`, `TEST_XSI_IPC`, ...) and option
(`TEST_POSIX_THREADS`, `TEST_XOPEN_CRYPT`, ...) is the edition of POSIX
the library meets for it (`202405L`, `200809L`) or 0. For picolibc they
copy what `<unistd.h>` claims, so a normal run skips the tests of what
the operating system layer doesn't provide. Parts that POSIX.1-2024
added to a group are tested only when the group is claimed at
`202405L` (`#if TEST_POSIX_FD_MGMT >= 202405L`).

Claim-all mode claims everything at POSIX.1-2024 instead, which tests
the claims both ways: what picolibc claims must still pass, and what it
doesn't claim must fail. Turn it on with a build option:

    $ ../scripts/do-x86_64-linux-gnu-configure -Dtests-claim-all=true
    $ ninja && meson test

or, with CMake, `-DTESTS_CLAIM_ALL=ON`. The POSIX tests are then built
with `TEST_CLAIM_ALL` defined (the `-native` variants are not), and
test/test-posix/claim-all.py does two things:

 * At configure time, it preprocesses each test with and without
   `TEST_CLAIM_ALL`. A test whose preprocessed text differs runs code
   the real claims leave out, so it is marked as expected to fail
   (meson's `should_fail`, CTest's `WILL_FAIL`). The table goes to
   test/test-posix/claim-all-expect.txt in the build directory, each
   test with the `TEST_` symbols it uses that are claimed below
   POSIX.1-2024, followed by the real value of every symbol.

 * At build time, it builds each test before the build does (with
   CMake, as the compile and link launcher). A test that doesn't
   compile or link, because a function, type or macro is missing, is
   replaced by a stand-in that prints the errors and fails, so it
   counts as a failed test instead of breaking the build.

A run should then be all OK or EXPECTEDFAIL (CTest shows a test
expected to fail as passed when it does fail). Anything else is worth
a look:

 * UNEXPECTEDPASS: a test of something picolibc doesn't claim passed.
   Perhaps the claim should be made; often the test only reached the
   parts that need no operating system support (a header check, or
   functions that only work on memory), and the group's other members
   are still missing.

 * FAIL: a test of something picolibc claims failed only because
   everything else was claimed too, which is a bug in the code or in
   the test's gates.

 * SKIP: a test that still skips at run time hides whether what it
   tests is there; tests that find stand-in functions fail instead when
   the group is claimed.
