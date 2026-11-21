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
#include "../posix-groups.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * strtok and strtok_r: tokens, separators that change from call to
 * call, the end of the string, state that needn't be initialized and
 * never points outside s, interleaved strings, and strtok_r in several
 * threads at once.
 */

static int ret;

#define fail(...)            \
    do {                     \
        printf(__VA_ARGS__); \
        printf("\n");        \
        ret = 1;             \
    } while (0)

#define MARK EDOM

/* Split s with sep, using strtok_r if r, and compare with the tokens in want */
static void
split(const char *input, const char *sep, const char * const *want, int r)
{
    char  buf[128];
    char *state, *tok;
    char *garbage = (char *)"not the string";
    int   i;

    snprintf(buf, sizeof(buf), "%s", input);
    /* The application needn't initialize the state */
    state = garbage;
    errno = MARK;
    for (i = 0;; i++) {
        tok = r ? strtok_r(i ? NULL : buf, sep, &state) : strtok(i ? NULL : buf, sep);
        if (errno != MARK)
            fail("%s changed errno", r ? "strtok_r" : "strtok");
        if (r && state != NULL && (state < buf || state > buf + strlen(input)))
            fail("strtok_r(\"%s\") left the state outside the string", input);
        if (tok == NULL || want[i] == NULL)
            break;
        if (strcmp(tok, want[i]) != 0)
            fail("%s(\"%s\", \"%s\") token %d is \"%s\", not \"%s\"", r ? "strtok_r" : "strtok",
                 input, sep, i, tok, want[i]);
        if (tok < buf || tok >= buf + sizeof(buf))
            fail("token %d isn't in the string", i);
    }
    if (tok != NULL || want[i] != NULL)
        fail("%s(\"%s\", \"%s\") gave %s token %d", r ? "strtok_r" : "strtok", input, sep,
             tok ? "an extra" : "no", i);
    /* Once there are no more tokens, later calls find none either */
    if (r ? strtok_r(NULL, sep, &state) != NULL : strtok(NULL, sep) != NULL)
        fail("%s(\"%s\") found another token after the end", r ? "strtok_r" : "strtok", input);
}

#define TOKENS(...) ((const char * const[]) { __VA_ARGS__, NULL })
#define NONE        ((const char * const[]) { NULL })

static void
both(const char *input, const char *sep, const char * const *want)
{
    split(input, sep, want, 0);
    split(input, sep, want, 1);
}

#if _POSIX_THREADS > 0
#include <pthread.h>

#define N_THREADS 4
#define ROUNDS    20000

static void *
tokenize(void *arg)
{
    long n = (long)arg;
    int  round;

    for (round = 0; round < ROUNDS; round++) {
        char  buf[96];
        char *state, *tok;
        int   count = 0;

        snprintf(buf, sizeof(buf), "%ld,%ld,,%ld;%ld", n, n + 1, n + 2, n + 3);
        for (tok = strtok_r(buf, ",;", &state); tok; tok = strtok_r(NULL, ",;", &state)) {
            if (atol(tok) != n + count)
                return (void *)1;
            count++;
        }
        if (count != 4)
            return (void *)1;
    }
    return NULL;
}
#endif

int
main(void)
{
    char  a[32], b[32];
    char *p, *state;

    /* The examples on strtok's page */
    both("LINE TO BE SEPARATED", " ", TOKENS("LINE", "TO", "BE", "SEPARATED"));
    both("key \t\n data\nrest", " \t\n", TOKENS("key", "data", "rest"));

    both("  a  b  ", " ", TOKENS("a", "b"));
    both("a", " ", TOKENS("a"));
    both("   ", " ", NONE);
    both("", " ", NONE);
    both("abc", "", TOKENS("abc"));
    both("a,b;;c", ",;", TOKENS("a", "b", "c"));
    /* Bytes with the high bit set */
    both("\x80x\x80\xff"
         "y\xff",
         "\x80\xff", TOKENS("x", "y"));
    both("\xe9t\xe9 \xe9", " ", TOKENS("\xe9t\xe9", "\xe9"));

    /* The separators may change from call to call */
    snprintf(a, sizeof(a), "a,b c,d");
    if ((p = strtok(a, ",")) == NULL || strcmp(p, "a") != 0)
        fail("strtok with \",\"");
    if ((p = strtok(NULL, " ")) == NULL || strcmp(p, "b") != 0)
        fail("strtok then with \" \"");
    if ((p = strtok(NULL, "")) == NULL || strcmp(p, "c,d") != 0)
        fail("strtok then with \"\"");
    snprintf(a, sizeof(a), "a,b c,d");
    if ((p = strtok_r(a, ",", &state)) == NULL || strcmp(p, "a") != 0)
        fail("strtok_r with \",\"");
    if ((p = strtok_r(NULL, " ", &state)) == NULL || strcmp(p, "b") != 0)
        fail("strtok_r then with \" \"");
    if ((p = strtok_r(NULL, "", &state)) == NULL || strcmp(p, "c,d") != 0)
        fail("strtok_r then with \"\"");

    /* The separator is overwritten with a null and the next search starts after it */
    snprintf(a, sizeof(a), "ab:cd");
    p = strtok(a, ":");
    if (p != a || a[2] != '\0' || strcmp(a + 3, "cd") != 0)
        fail("strtok didn't end the token in place");
    if ((p = strtok(NULL, ":")) != a + 3)
        fail("the next token isn't after the separator");

    /* strtok_r on two strings at once, and strtok's state is separate */
    snprintf(a, sizeof(a), "1 2 3");
    snprintf(b, sizeof(b), "x y z");
    {
        char *sa, *sb, *t;

        t = strtok(a, " ");
        if (t == NULL || strcmp(t, "1") != 0)
            fail("strtok 1");
        snprintf(b, sizeof(b), "x y z");
        if ((p = strtok_r(b, " ", &sb)) == NULL || strcmp(p, "x") != 0)
            fail("strtok_r x");
        if ((t = strtok(NULL, " ")) == NULL || strcmp(t, "2") != 0)
            fail("strtok_r disturbed strtok's state");
        if ((p = strtok_r(NULL, " ", &sb)) == NULL || strcmp(p, "y") != 0)
            fail("strtok disturbed strtok_r's state");
        snprintf(a, sizeof(a), "1 2 3");
        if ((p = strtok_r(a, " ", &sa)) == NULL || strcmp(p, "1") != 0)
            fail("strtok_r 1");
        if ((p = strtok_r(NULL, " ", &sb)) == NULL || strcmp(p, "z") != 0)
            fail("strtok_r z after another string");
        if ((p = strtok_r(NULL, " ", &sa)) == NULL || strcmp(p, "2") != 0)
            fail("strtok_r 2 after another string");
    }

#if _POSIX_THREADS > 0
    {
        pthread_t threads[N_THREADS];
        long      i;
        void     *result;

        for (i = 0; i < N_THREADS; i++)
            if (pthread_create(&threads[i], NULL, tokenize, (void *)(i * 10)) != 0)
                fail("pthread_create");
        for (i = 0; i < N_THREADS; i++) {
            if (pthread_join(threads[i], &result) != 0 || result != NULL)
                fail("strtok_r in thread %ld went wrong", i);
        }
    }
#endif

    if (ret == 0)
        printf("success\n");
    return ret;
}
