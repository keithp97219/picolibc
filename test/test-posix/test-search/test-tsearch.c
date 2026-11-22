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
/* tsearch, tfind, tdelete and twalk are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

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
 * The binary search tree functions: tsearch adds a node holding the
 * key pointer itself and returns it, or the node already there; tfind
 * only looks; tdelete returns the parent of the deleted node (non-null
 * for the root, null when missing); twalk visits each node in
 * depth-first, left-to-right order, as preorder, postorder and endorder,
 * or once as a leaf, with the root at level 0. The comparison sees the
 * key and the stored elements, and its result is only used for its
 * sign. tsearch returns a null pointer without memory (checked in a
 * child with no room left for data).
 */

/* glibc 2.43 lacks posix_tnode (new in Issue 8) */
#ifdef __GLIBC__
typedef void tnode;
#else
typedef posix_tnode tnode;
#endif

static int errors;

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            errors++;                                  \
        }                                              \
    } while (0)

/* Elements: the comparison looks only at value, not at tag */
struct elem {
    int value;
    int tag;
};

#define NELEM 500

static struct elem elems[NELEM];
static const void *cmp_key;
static int         cmp_bad;

/*
 * Returns a large magnitude rather than -1/1, so the functions have to
 * look at the sign only. One argument is always the key; the other must
 * be an element already in the tree
 */
static int
cmp(const void *a, const void *b)
{
    const struct elem *ea = a, *eb = b;

    if (cmp_key != NULL && a != cmp_key && b != cmp_key)
        cmp_bad++;
    if (ea->value < eb->value)
        return -1000;
    if (ea->value > eb->value)
        return 1000;
    return 0;
}

#define elem_of(node) (*(struct elem * const *)(node))

/* What twalk reported */
struct visit {
    const struct elem *e;
    VISIT              which;
    int                level;
};

#define MAX_VISITS (3 * NELEM)

static struct visit visits[MAX_VISITS];
static int          nvisits;

static void
record(const tnode *node, VISIT which, int level)
{
    if (nvisits < MAX_VISITS) {
        visits[nvisits].e = elem_of(node);
        visits[nvisits].which = which;
        visits[nvisits].level = level;
    }
    nvisits++;
}

static void
walk(const tnode *root)
{
    nvisits = 0;
    twalk(root, record);
}

/*
 * Rebuild the tree from the walk and check that it is a valid
 * depth-first, left-to-right walk of a binary search tree: each node
 * is visited once as a leaf, or as preorder, postorder and endorder
 * with its left subtree after preorder and its right subtree after
 * postorder, children are one level deeper, and the postorder and leaf
 * visits come in ascending order. Returns the number of nodes, or -1.
 */
static int pos;

static int
walk_subtree(int level, int *count, const struct elem **last)
{
    const struct visit *v;
    const struct elem  *e;
    int                 children = 0;

    if (pos >= nvisits || pos >= MAX_VISITS)
        return -1;
    v = &visits[pos++];
    if (v->level != level)
        return -1;
    e = v->e;
    if (v->which == leaf) {
        if (*last && (*last)->value >= e->value)
            return -1;
        *last = e;
        (*count)++;
        return 0;
    }
    if (v->which != preorder)
        return -1;
    /* Left subtree, if any */
    if (pos < nvisits && visits[pos].level == level + 1) {
        if (walk_subtree(level + 1, count, last) < 0)
            return -1;
        children++;
    }
    if (pos >= nvisits || visits[pos].e != e || visits[pos].which != postorder
        || visits[pos].level != level)
        return -1;
    pos++;
    if (*last && (*last)->value >= e->value)
        return -1;
    *last = e;
    (*count)++;
    /* Right subtree, if any */
    if (pos < nvisits && visits[pos].level == level + 1) {
        if (walk_subtree(level + 1, count, last) < 0)
            return -1;
        children++;
    }
    if (pos >= nvisits || visits[pos].e != e || visits[pos].which != endorder
        || visits[pos].level != level)
        return -1;
    pos++;
    /* A node with no children is a leaf */
    if (children == 0)
        return -1;
    return 0;
}

static int
walk_check(const tnode *root)
{
    int                count = 0;
    const struct elem *last = NULL;

    walk(root);
    if (nvisits == 0)
        return 0;
    pos = 0;
    if (walk_subtree(0, &count, &last) < 0 || pos != nvisits)
        return -1;
    return count;
}

/* Finds the parent of each node from a walk; NULL for the root */
static const struct elem *
parent_of(const struct elem *e)
{
    const struct elem *stack[NELEM];
    int                i;

    for (i = 0; i < nvisits && i < MAX_VISITS; i++) {
        if (visits[i].level >= NELEM)
            return NULL;
        if (visits[i].which == preorder || visits[i].which == leaf) {
            stack[visits[i].level] = visits[i].e;
            if (visits[i].e == e)
                return visits[i].level ? stack[visits[i].level - 1] : NULL;
        }
    }
    return NULL;
}

static tnode *
search(tnode **rootp, const struct elem *key)
{
    tnode *node;

    cmp_key = key;
    node = tsearch(key, rootp, cmp);
    cmp_key = NULL;
    return node;
}

static tnode *
find(tnode * const *rootp, const struct elem *key)
{
    tnode *node;

    cmp_key = key;
    node = tfind(key, rootp, cmp);
    cmp_key = NULL;
    return node;
}

static void *
delete(tnode **rootp, const struct elem *key)
{
    void *ret;

    cmp_key = key;
    ret = tdelete(key, rootp, cmp);
    cmp_key = NULL;
    return ret;
}

/* The expected walk of the tree holding 1 to 7, balanced or built from 4 2 6 1 3 5 7 */
static const struct {
    int   value;
    VISIT which;
    int   level;
} seven[] = {
    { 4, preorder,  0 },
    { 2, preorder,  1 },
    { 1, leaf,      2 },
    { 2, postorder, 1 },
    { 3, leaf,      2 },
    { 2, endorder,  1 },
    { 4, postorder, 0 },
    { 6, preorder,  1 },
    { 5, leaf,      2 },
    { 6, postorder, 1 },
    { 7, leaf,      2 },
    { 6, endorder,  1 },
    { 4, endorder,  0 },
};

#if TEST_POSIX_MULTI_PROCESS

#define CHILD_ELEM 100000

static struct elem child_elems[CHILD_ELEM];

/* With no room for data, tsearch returns a null pointer and the tree stays usable */
static void
nomem_child(void)
{
    tnode        *root = NULL;
    struct rlimit rl;
    int           i;
    tnode        *node = NULL;

    for (i = 0; i < CHILD_ELEM; i++)
        child_elems[i].value = i;
    if (tsearch(&child_elems[0], &root, cmp) == NULL)
        _exit(1);
    if (getrlimit(RLIMIT_DATA, &rl) != 0)
        _exit(2);
    rl.rlim_cur = 1;
    if (setrlimit(RLIMIT_DATA, &rl) != 0)
        _exit(3);
    for (i = 1; i < CHILD_ELEM; i++) {
        node = tsearch(&child_elems[i], &root, cmp);
        if (node == NULL)
            break;
    }
    if (node != NULL)
        _exit(4);
    if (tfind(&child_elems[0], &root, cmp) == NULL)
        _exit(5);
    if (tfind(&child_elems[i], &root, cmp) != NULL)
        _exit(6);
    if (tfind(&child_elems[i - 1], &root, cmp) == NULL)
        _exit(7);
    _exit(0);
}

#endif

int
main(void)
{
    tnode             *root = NULL, *node, *node2;
    void              *ret;
    struct elem        key, dup, seven_elems[8];
    const struct elem *parent;
    int                i, j, n;
    static const int   order[] = { 4, 2, 6, 1, 3, 5, 7 };

    for (i = 0; i < NELEM; i++) {
        /* A scattered insertion order */
        elems[i].value = (int)((i * 7919L) % NELEM);
        elems[i].tag = i;
    }

    /* A null rootp: tsearch, tfind and tdelete return a null pointer */
    key.value = 1;
    check(tsearch(&key, NULL, cmp) == NULL, "tsearch NULL rootp");
    check(tfind(&key, NULL, cmp) == NULL, "tfind NULL rootp");
    check(tdelete(&key, NULL, cmp) == NULL, "tdelete NULL rootp");

    /* An empty tree: tfind and tdelete find nothing and leave it empty */
    check(find(&root, &key) == NULL && root == NULL, "tfind empty");
    check(delete(&root, &key) == NULL && root == NULL, "tdelete empty");

    /* The first tsearch sets the root to the new node */
    node = search(&root, &elems[0]);
    check(node != NULL && root == node, "root set");
    check(elem_of(node) == &elems[0], "node holds the key pointer");

    /* One node is a leaf at level 0 */
    walk(root);
    check(nvisits == 1 && visits[0].e == &elems[0] && visits[0].which == leaf
              && visits[0].level == 0,
          "one node walk");

    /* Deleting the only node empties the tree and returns non-null */
    ret = delete(&root, &elems[0]);
    check(ret != NULL, "tdelete root returns non-null");
    check(root == NULL, "tdelete last node empties the tree");

    /* Build the tree */
    for (i = 0; i < NELEM; i++) {
        node = search(&root, &elems[i]);
        check(node != NULL && elem_of(node) == &elems[i], "tsearch adds");
    }
    check(cmp_bad == 0, "compar sees the key");

    /* Finding an equal element returns the node there, with the first element */
    for (i = 0; i < NELEM; i++) {
        dup.value = elems[i].value;
        dup.tag = -1;
        node = find(&root, &dup);
        check(node != NULL && elem_of(node) == &elems[i], "tfind");
        node2 = search(&root, &dup);
        check(node2 == node && elem_of(node2) == &elems[i], "tsearch finds");
    }
    dup.value = NELEM;
    check(find(&root, &dup) == NULL, "tfind missing");
    dup.value = -1;
    check(find(&root, &dup) == NULL, "tfind missing below");
    check(cmp_bad == 0, "compar sees the key");

    /* The walk is a depth-first, left-to-right walk of a search tree */
    n = walk_check(root);
    check(n == NELEM, "walk visits every node in order");

    /* Deleting a missing key returns a null pointer and changes nothing */
    dup.value = NELEM + 1;
    check(delete(&root, &dup) == NULL, "tdelete missing");
    check(walk_check(root) == NELEM, "tree after tdelete missing");

    /*
     * Delete every other element: a leaf's parent, from the walk,
     * is what tdelete returns; the rest of the tree stays valid
     */
    for (i = 0; i < NELEM; i += 2) {
        walk(root);
        parent = parent_of(&elems[i]);
        node = find(&root, &elems[i]);
        check(node != NULL, "present before tdelete");
        ret = delete(&root, &elems[i]);
        if (parent != NULL)
            check(ret != NULL && elem_of(ret) == parent, "tdelete returns the parent");
        else
            check(ret != NULL, "tdelete root returns non-null");
        check(find(&root, &elems[i]) == NULL, "gone after tdelete");
    }
    check(cmp_bad == 0, "compar sees the key");
    check(walk_check(root) == NELEM / 2, "walk after tdelete");
    for (i = 0; i < NELEM; i++) {
        node = find(&root, &elems[i]);
        check((node != NULL) == (i % 2 == 1), "membership after tdelete");
    }

    /* Delete the rest, always at the root */
    while (root != NULL) {
        const struct elem *e = elem_of(root);
        ret = delete(&root, e);
        check(ret != NULL, "tdelete root");
        check(root == NULL || elem_of(root) != e, "root replaced");
        if (ret == NULL)
            break;
    }

    /* The seven-node tree: its walk is fixed for any balanced or plain tree */
    for (i = 0; i < 7; i++) {
        seven_elems[order[i]].value = order[i];
        node = search(&root, &seven_elems[order[i]]);
        check(node != NULL, "seven");
    }
    walk(root);
    check(nvisits == (int)(sizeof(seven) / sizeof(seven[0])), "seven visits");
    for (i = 0; i < nvisits && i < (int)(sizeof(seven) / sizeof(seven[0])); i++) {
        if (visits[i].e->value != seven[i].value || visits[i].which != seven[i].which
            || visits[i].level != seven[i].level) {
            printf("visit %d: %d %d %d, expected %d %d %d\n", i, visits[i].e->value,
                   (int)visits[i].which, visits[i].level, seven[i].value, (int)seven[i].which,
                   seven[i].level);
            errors++;
        }
    }

    /* Any node can be the root of a walk below it, starting at level 0 */
    node = find(&root, &seven_elems[6]);
    walk(node);
    check(nvisits == 5, "subtree visits");
    if (nvisits == 5) {
        for (i = 0; i < 5; i++) {
            j = i + 7;
            check(visits[i].e->value == seven[j].value && visits[i].which == seven[j].which
                      && visits[i].level == seven[j].level - 1,
                  "subtree walk");
        }
    }

    /* Deleting a leaf returns its parent */
    ret = delete(&root, &seven_elems[5]);
    check(ret != NULL && elem_of(ret)->value == 6, "tdelete leaf parent");
    while (root != NULL)
        if (delete(&root, elem_of(root)) == NULL)
            break;

    /* Elements added in sorted order still work */
    for (i = 0; i < NELEM; i++) {
        elems[i].value = i;
        check(search(&root, &elems[i]) != NULL, "sorted tsearch");
    }
    check(walk_check(root) == NELEM, "sorted walk");
    for (i = NELEM - 1; i >= 0; i--)
        check(delete(&root, &elems[i]) != NULL, "sorted tdelete");
    check(root == NULL, "sorted empty");

#if TEST_POSIX_MULTI_PROCESS
    /* qemu-user doesn't apply RLIMIT_DATA */
    if (is_qemu()) {
        printf("qemu: skipping the RLIMIT_DATA check\n");
    } else {
        pid_t pid;
        int   status = 0;

        fflush(stdout);
        pid = fork();
        if (pid == 0)
            nomem_child();
        if (pid < 0 || waitpid(pid, &status, 0) != pid || !WIFEXITED(status)
            || WEXITSTATUS(status) != 0) {
            printf("tsearch without memory: child failed (status 0x%x)\n", (unsigned)status);
            errors++;
        }
    }
#endif

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}
