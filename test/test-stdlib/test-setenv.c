/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 Keith Packard
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * setenv, unsetenv, putenv and getenv: putenv's string is the
 * environment's entry, setenv doesn't write into strings it didn't
 * allocate, and an environ the application sets (or nulls) is used
 * and never freed. Run with lock-valid.c, which rejects setenv calling
 * free with the libc lock held.
 */

extern char **environ;

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Whether environ holds the entry "name=value" */
static int
in_environ(const char *entry)
{
    char **e;

    if (!environ)
        return 0;
    for (e = environ; *e; e++)
        if (strcmp(*e, entry) == 0)
            return 1;
    return 0;
}

static void
test_setenv(void)
{
    char  name[] = "TEST_SETENV_A";
    char  value[] = "one";
    char *v;

    check(unsetenv("TEST_SETENV_A") == 0, "unsetenv of a missing name");
    check(getenv("TEST_SETENV_A") == NULL, "missing name");

    check(setenv(name, value, 0) == 0, "setenv adds");
    v = getenv("TEST_SETENV_A");
    check(v != NULL && strcmp(v, "one") == 0, "getenv finds it");
    check(in_environ("TEST_SETENV_A=one"), "environ has it");

    /* The strings are copied */
    name[0] = 'X';
    strcpy(value, "two");
    v = getenv("TEST_SETENV_A");
    check(v != NULL && strcmp(v, "one") == 0, "setenv copied the strings");

    /* overwrite 0 leaves the value; non-zero replaces it */
    check(setenv("TEST_SETENV_A", "two", 0) == 0, "setenv without overwrite");
    check(strcmp(getenv("TEST_SETENV_A"), "one") == 0, "not overwritten");
    check(setenv("TEST_SETENV_A", "a much longer value than before", 1) == 0, "overwrite");
    check(strcmp(getenv("TEST_SETENV_A"), "a much longer value than before") == 0, "overwritten");
    check(in_environ("TEST_SETENV_A=a much longer value than before"), "environ updated");
    check(setenv("TEST_SETENV_A", "", 1) == 0 && strcmp(getenv("TEST_SETENV_A"), "") == 0,
          "an empty value");

    /* The returned pointer points into environ's entry */
    {
        char **e;
        int    found = 0;

        v = getenv("TEST_SETENV_A");
        for (e = environ; *e; e++)
            if (*e + strlen("TEST_SETENV_A=") == v)
                found = 1;
        check(found, "getenv points into the environment");
    }

    /* EINVAL for empty names and names with '=', and nothing changes */
    errno = 0;
    check(setenv("", "x", 1) == -1 && errno == EINVAL, "setenv of an empty name");
    errno = 0;
    check(setenv("TEST=SETENV", "x", 1) == -1 && errno == EINVAL, "setenv of a name with '='");
    check(getenv("TEST") == NULL, "nothing set");
    errno = 0;
    check(unsetenv("") == -1 && errno == EINVAL, "unsetenv of an empty name");
    errno = 0;
    check(unsetenv("TEST_SETENV_A=") == -1 && errno == EINVAL, "unsetenv of a name with '='");
    check(getenv("TEST_SETENV_A") != NULL, "unchanged after the failed unsetenv");

    check(unsetenv("TEST_SETENV_A") == 0, "unsetenv");
    check(getenv("TEST_SETENV_A") == NULL && !in_environ("TEST_SETENV_A="), "removed");

    /* Many new names grow the environment array again and again */
    {
        int  i;
        char n[32];

        for (i = 0; i < 100; i++) {
            snprintf(n, sizeof(n), "TEST_SETENV_N%d", i);
            check(setenv(n, n, 1) == 0, "setenv many");
        }
        for (i = 0; i < 100; i++) {
            snprintf(n, sizeof(n), "TEST_SETENV_N%d", i);
            v = getenv(n);
            check(v != NULL && strcmp(v, n) == 0, "getenv many");
        }
        for (i = 0; i < 100; i += 2) {
            snprintf(n, sizeof(n), "TEST_SETENV_N%d", i);
            check(unsetenv(n) == 0, "unsetenv many");
        }
        for (i = 0; i < 100; i++) {
            snprintf(n, sizeof(n), "TEST_SETENV_N%d", i);
            check((getenv(n) == NULL) == (i % 2 == 0), "the right ones removed");
        }
    }
}

static void
test_putenv(void)
{
    static char entry[] = "TEST_PUTENV_A=first";
    static char other[] = "TEST_PUTENV_A=second";
    static char ro_name[] = "TEST_PUTENV_B=abcdef";
    static char noeq[] = "TEST_PUTENV_A";
    char       *v;

    check(putenv(entry) == 0, "putenv");
    v = getenv("TEST_PUTENV_A");
    check(v == entry + strlen("TEST_PUTENV_A="), "the string itself is in the environment");

    /* Changing the string changes the environment */
    strcpy(entry + strlen("TEST_PUTENV_A="), "FIRST");
    check(strcmp(getenv("TEST_PUTENV_A"), "FIRST") == 0, "altering the string");

    /* A second putenv replaces the definition */
    check(putenv(other) == 0, "putenv replaces");
    check(strcmp(getenv("TEST_PUTENV_A"), "second") == 0, "replaced");
    check(strcmp(entry, "TEST_PUTENV_A=FIRST") == 0, "the old string is left alone");

    /* setenv doesn't write into the application's string */
    check(putenv(ro_name) == 0, "putenv B");
    check(setenv("TEST_PUTENV_B", "xyz", 1) == 0, "setenv over putenv");
    check(strcmp(getenv("TEST_PUTENV_B"), "xyz") == 0, "setenv's value");
    check(strcmp(ro_name, "TEST_PUTENV_B=abcdef") == 0, "putenv's string unchanged");

    /* putenv after setenv takes the string again */
    check(putenv(ro_name) == 0 && getenv("TEST_PUTENV_B") == ro_name + 14, "putenv over setenv");

    /* A name alone either removes the variable or fails with EINVAL */
    errno = 0;
    if (putenv(noeq) == 0)
        check(getenv("TEST_PUTENV_A") == NULL, "putenv of a name removes it");
    else
        check(errno == EINVAL && getenv("TEST_PUTENV_A") != NULL, "putenv of a name: EINVAL");

    /* unsetenv removes putenv's definition */
    check(unsetenv("TEST_PUTENV_B") == 0 && getenv("TEST_PUTENV_B") == NULL, "unsetenv");
    check(strcmp(ro_name, "TEST_PUTENV_B=abcdef") == 0, "putenv's string after unsetenv");
}

/* The application may replace environ; setenv then works on the new array */
static void
test_environ(void)
{
    char **mine;
    char  *v;

    /* Let setenv allocate an array of its own first */
    check(setenv("TEST_ENVIRON_A", "1", 1) == 0, "setenv");

    mine = malloc(3 * sizeof(char *));
    check(mine != NULL, "malloc");
    mine[0] = "TEST_ENVIRON_B=2";
    mine[1] = "TEST_ENVIRON_C=3";
    mine[2] = NULL;
    environ = mine;

    check(getenv("TEST_ENVIRON_A") == NULL, "the old environment is gone");
    v = getenv("TEST_ENVIRON_C");
    check(v != NULL && strcmp(v, "3") == 0, "getenv searches the new array");
    check(setenv("TEST_ENVIRON_D", "4", 1) == 0, "setenv adds to the new array");
    check(strcmp(getenv("TEST_ENVIRON_D"), "4") == 0 && strcmp(getenv("TEST_ENVIRON_B"), "2") == 0,
          "both");
    /* The application's array isn't setenv's to free */
    check(mine[0] != NULL && strcmp(mine[0], "TEST_ENVIRON_B=2") == 0 && mine[2] == NULL,
          "the application's array is intact");
    check(setenv("TEST_ENVIRON_E", "5", 1) == 0, "setenv again");
    check(unsetenv("TEST_ENVIRON_B") == 0 && getenv("TEST_ENVIRON_B") == NULL, "unsetenv");
    check(strcmp(mine[1], "TEST_ENVIRON_C=3") == 0, "the application's strings are intact");
    free(mine);

    /* And with no environment at all */
    environ = NULL;
    check(getenv("TEST_ENVIRON_C") == NULL, "getenv with a null environ");
    check(unsetenv("TEST_ENVIRON_C") == 0, "unsetenv with a null environ");
    check(setenv("TEST_ENVIRON_F", "6", 1) == 0, "setenv with a null environ");
    check(strcmp(getenv("TEST_ENVIRON_F"), "6") == 0 && environ != NULL && environ[1] == NULL,
          "a new environment");
}

int
main(void)
{
    test_setenv();
    test_putenv();
    test_environ();
    printf("success\n");
    return 0;
}
