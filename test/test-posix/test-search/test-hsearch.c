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

#define _POSIX_C_SOURCE 202405L
/* hcreate, hdestroy and hsearch are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

#include <errno.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TEST_POSIX_MULTI_PROCESS
#include <sys/resource.h>
#include <sys/wait.h>
#include "../is-qemu.h"
#endif

/*
 * hsearch manages one hash table: ENTER adds an item that isn't there
 * and returns the entry, keeping the first data for a key; FIND only
 * looks, with strcmp; a null pointer means not found, or full. hdestroy
 * disposes of the table (including its entries, checked with a data
 * limit), after which hcreate makes a new, empty one. ENOMEM is checked
 * in children with no room left for data.
 */

static int errors;

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            errors++;                                  \
        }                                              \
    } while (0)

#define NKEYS 100

static char keys[NKEYS][8];

static ENTRY *
lookup(const char *key, ACTION action, void *data)
{
    char  buf[sizeof(keys[0])];
    ENTRY item;

    /* A copy of the key: the comparison is strcmp, not the pointer */
    strcpy(buf, key);
    item.key = action == FIND ? buf : (char *)key;
    item.data = data;
    return hsearch(item, action);
}

#if TEST_POSIX_MULTI_PROCESS

/* Children have no room to print, so a failed check exits with its number */
static int step;

#define child_check(condition) \
    do {                       \
        step++;                \
        if (!(condition))      \
            _exit(step);       \
    } while (0)

static int
limit_data(rlim_t cur)
{
    struct rlimit rl;

    if (getrlimit(RLIMIT_DATA, &rl) != 0)
        return -1;
    rl.rlim_cur = cur;
    return setrlimit(RLIMIT_DATA, &rl);
}

#define CYCLES     2000
#define CYCLE_KEYS 1000

static char cycle_keys[CYCLE_KEYS][8];

/*
 * hdestroy disposes of the whole table: 2000 tables of 1000 entries
 * fit in 16MiB only if each is freed
 */
static void
destroy_child(void)
{
    int   c, i;
    ENTRY item;

    for (i = 0; i < CYCLE_KEYS; i++)
        snprintf(cycle_keys[i], sizeof(cycle_keys[i]), "c%d", i);
    child_check(limit_data((rlim_t)16 << 20) == 0);
    for (c = 0; c < CYCLES; c++) {
        child_check(hcreate(CYCLE_KEYS * 2) != 0);
        for (i = 0; i < CYCLE_KEYS; i++) {
            item.key = cycle_keys[i];
            item.data = NULL;
            if (hsearch(item, ENTER) == NULL)
                _exit(100);
        }
        hdestroy();
    }
    _exit(0);
}

#define MANY_KEYS 100000

static char many_keys[MANY_KEYS][8];

/* With no room for data, ENTER fails with ENOMEM and FIND still works */
static void
enter_child(void)
{
    int    i;
    ENTRY  item;
    ENTRY *e = NULL;

    for (i = 0; i < MANY_KEYS; i++)
        snprintf(many_keys[i], sizeof(many_keys[i]), "m%d", i);
    child_check(hcreate(16) != 0);
    item.key = many_keys[0];
    item.data = NULL;
    child_check(hsearch(item, ENTER) != NULL);
    child_check(limit_data(1) == 0);
    for (i = 1; i < MANY_KEYS; i++) {
        item.key = many_keys[i];
        errno = 0;
        e = hsearch(item, ENTER);
        if (e == NULL)
            break;
    }
    child_check(e == NULL);
    child_check(errno == ENOMEM);
    item.key = many_keys[0];
    child_check(hsearch(item, FIND) != NULL);
    item.key = many_keys[i - 1];
    child_check(hsearch(item, FIND) != NULL);
    item.key = many_keys[i];
    child_check(hsearch(item, FIND) == NULL);
    _exit(0);
}

/* Without room for the table, hcreate returns 0 and sets ENOMEM */
static void
create_child(void)
{
    int ret;

    child_check(limit_data(1) == 0);
    errno = 0;
    ret = hcreate(1000000);
    child_check(ret == 0);
    child_check(errno == ENOMEM);
    _exit(0);
}

static void
run_child(void (*f)(void), const char *name)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    if (pid < 0) {
        printf("fork failed for %s\n", name);
        errors++;
        return;
    }
    if (pid == 0)
        f();
    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("%s: child failed (status 0x%x)\n", name, (unsigned)status);
        errors++;
    }
}

#endif /* TEST_POSIX_MULTI_PROCESS */

int
main(void)
{
    int    i, total;
    ENTRY *e, *e2;
    char   other[] = "k1";

    for (i = 0; i < NKEYS; i++)
        snprintf(keys[i], sizeof(keys[i]), "k%d", i);

    check(hcreate(NKEYS * 2) != 0, "hcreate");

    /* ENTER returns the new entry, holding the item's key and data */
    for (i = 0; i < NKEYS; i++) {
        e = lookup(keys[i], ENTER, &keys[i]);
        check(e != NULL && strcmp(e->key, keys[i]) == 0 && e->data == &keys[i], "ENTER new");
    }

    /* FIND compares with strcmp, so a copy of the key finds the entry */
    for (i = 0; i < NKEYS; i++) {
        e = lookup(keys[i], FIND, NULL);
        check(e != NULL && strcmp(e->key, keys[i]) == 0 && e->data == &keys[i], "FIND");
    }

    /* The entry stays where it is: the same pointer each time */
    e = lookup("k7", FIND, NULL);
    e2 = lookup("k7", FIND, NULL);
    check(e != NULL && e == e2, "same entry");

    /* ENTER of a key that's there finds it and doesn't replace its data */
    e = lookup(other, ENTER, other);
    check(e != NULL && e->data == &keys[1] && e->key == keys[1], "ENTER existing");

    /* Keys that aren't there, including prefixes and extensions */
    check(lookup("k", FIND, NULL) == NULL, "missing prefix");
    check(lookup("k10x", FIND, NULL) == NULL, "missing extension");
    check(lookup("K1", FIND, NULL) == NULL, "case");
    check(lookup("", FIND, NULL) == NULL, "empty key");

    /* FIND doesn't add anything */
    check(lookup("zz", FIND, NULL) == NULL && lookup("zz", FIND, NULL) == NULL,
          "FIND adds nothing");

    /* The empty string is a key like any other */
    e = lookup("", ENTER, &total);
    check(e != NULL && e->data == &total, "ENTER empty key");
    check(lookup("", FIND, NULL) == e, "FIND empty key");

    /* hdestroy disposes of the table; hcreate makes a new, empty one */
    hdestroy();
    check(hcreate(4) != 0, "hcreate after hdestroy");
    for (i = 0; i < NKEYS; i++)
        check(lookup(keys[i], FIND, NULL) == NULL, "empty after hdestroy");

    /*
     * nel is only an estimate: entering more items either works or,
     * when the table is full, returns a null pointer (setting ENOMEM),
     * and every item that went in can be found
     */
    total = 0;
    for (i = 0; i < NKEYS; i++) {
        errno = 0;
        e = lookup(keys[i], ENTER, &keys[i]);
        if (e == NULL) {
            check(errno == ENOMEM, "full table");
            break;
        }
        total++;
    }
    for (i = 0; i < total; i++) {
        e = lookup(keys[i], FIND, NULL);
        check(e != NULL && e->data == &keys[i], "FIND past the estimate");
    }
    if (total < NKEYS)
        printf("table full after %d entries\n", total);
    hdestroy();

#if TEST_POSIX_MULTI_PROCESS
    /* qemu-user doesn't apply RLIMIT_DATA */
    if (is_qemu()) {
        printf("qemu: skipping the RLIMIT_DATA checks\n");
    } else {
        run_child(destroy_child, "hdestroy frees the entries");
        run_child(enter_child, "ENTER without memory");
        run_child(create_child, "hcreate without memory");
    }
#endif

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}
