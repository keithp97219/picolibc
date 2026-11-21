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
#include <wordexp.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <picolibc.h>
#ifdef __PICOLIBC_PTHREADS
#include <pthread.h>
#endif

/*
 * Checks for characters the shell would treat as operators outside of
 * quotes, and command substitution when it isn't allowed.
 */
static int
__wordexp_check(const char *words, int flags, bool *cmdsub)
{
    /*
     * What each open bracket began: a for $((, c for $(, b for ${ and
     * p for a plain ( or { inside one of those
     */
    char nest[32];
    int  depth = 0;
    bool sq = false, dq = false, bq = false;

#define PUSH(k)                         \
    do {                                \
        if (depth == (int)sizeof(nest)) \
            return WRDE_SYNTAX;         \
        nest[depth++] = (k);            \
    } while (0)

    for (; *words; words++) {
        char c = *words;
        char top = depth ? nest[depth - 1] : 0;

        if (sq) {
            if (c == '\'')
                sq = false;
            continue;
        }
        switch (c) {
        case '\\':
            if (words[1])
                words++;
            continue;
        case '\'':
            if (!dq)
                sq = true;
            continue;
        case '"':
            dq = !dq;
            continue;
        case '`':
            if (flags & WRDE_NOCMD)
                return WRDE_CMDSUB;
            *cmdsub = true;
            bq = !bq;
            continue;
        case '$':
            if (words[1] == '(' && words[2] == '(') {
                PUSH('a');
                words += 2;
            } else if (words[1] == '(') {
                if (flags & WRDE_NOCMD)
                    return WRDE_CMDSUB;
                *cmdsub = true;
                PUSH('c');
                words++;
            } else if (words[1] == '{') {
                PUSH('b');
                words++;
            }
            continue;
        case '(':
        case '{':
            if (depth)
                PUSH('p');
            else if (!dq && !bq)
                return WRDE_BADCHAR;
            continue;
        case ')':
        case '}':
            if (!depth) {
                if (!dq && !bq)
                    return WRDE_BADCHAR;
                continue;
            }
            if (top == 'a') {
                /*
                 * An arithmetic expansion ends only at )); the shell
                 * may take anything else for a command substitution
                 */
                if (c != ')' || words[1] != ')')
                    return (flags & WRDE_NOCMD) ? WRDE_CMDSUB : WRDE_SYNTAX;
                words++;
            } else if (c == '}' ? top == 'c' : top == 'b') {
                return WRDE_SYNTAX;
            }
            depth--;
            continue;
        case '|':
        case '&':
        case ';':
        case '<':
        case '>':
        case '\n':
            /* Arithmetic has no use for these, but a command would */
            if (top == 'a' && (c == ';' || c == '\n')) {
                if (flags & WRDE_NOCMD)
                    return WRDE_CMDSUB;
                *cmdsub = true;
            }
            if (!depth && !dq && !bq)
                return WRDE_BADCHAR;
            continue;
        default:
            continue;
        }
    }
#undef PUSH
    if (sq || dq || bq || depth)
        return WRDE_SYNTAX;
    return 0;
}

/* Runs the shell, returning its output in *out and the wait status */
static int
__wordexp_shell(const char *words, int flags, char **out, size_t *len)
{
    /*
     * $1 turns on set -u, $2 holds the words and $3, when present, is
     * IFS from the environment, which a shell may ignore (XCU 2.5.3).
     * set -- clears the positional parameters before the words are
     * expanded, so $1 and the rest aren't the script's. printf ends
     * each field with a null byte, and the leading x keeps an empty
     * expansion from looking like one empty field.
     */
    static const char script[] = "[ -n \"$1\" ] && set -u; [ $# -gt 2 ] && IFS=$3; "
                                 "eval \"set --; printf '%s\\\\0' x $2\"";
    char             *argv[] = {
        "sh",          "-c", (char *)script, "sh", (flags & WRDE_UNDEF) ? "1" : "", (char *)words,
        getenv("IFS"), NULL
    };
    char  *buf = NULL;
    size_t size = 0, used = 0;
    int    fds[2], status;
    pid_t  pid;

    /* Other threads' children mustn't hold the pipe open */
    if (pipe2(fds, O_CLOEXEC) < 0)
        return -1;
    pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return -1;
    }
    if (pid == 0) {
        if (fds[1] == 1)
            fcntl(1, F_SETFD, 0);
        else
            dup2(fds[1], 1);
        if (!(flags & WRDE_SHOWERR)) {
            int null = open("/dev/null", O_WRONLY | O_CLOEXEC);

            if (null >= 0)
                dup2(null, 2);
        }
        execv("/bin/sh", argv);
        _exit(127);
    }
    close(fds[1]);
    for (;;) {
        ssize_t n;

        if (used == size) {
            char *nbuf = realloc(buf, size ? size * 2 : 256);

            if (!nbuf) {
                free(buf);
                buf = NULL;
                break;
            }
            buf = nbuf;
            size = size ? size * 2 : 256;
        }
        n = read(fds[0], buf + used, size - used);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        used += (size_t)n;
    }
    close(fds[0]);
    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            return -1;
    *out = buf;
    *len = used;
    if (!buf)
        return -2;
    return status;
}

/* Whether the shell's output shows that it expanded the words */
static bool
__wordexp_ok(int status, size_t len)
{
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 && len >= 2;
}

static int
__wordexp(const char * __restrict words, wordexp_t * __restrict we, int flags)
{
    char  *out, *p, **wordv;
    size_t len, count = 0, offs, i;
    int    status, ret;
    bool   cmdsub = false;

    /* REUSE is as if wordfree, then wordexp, so even an error frees the words */
    if (flags & WRDE_REUSE) {
        wordfree(we);
        flags &= ~WRDE_APPEND;
    }
    if (!(flags & WRDE_APPEND)) {
        we->we_wordc = 0;
        we->we_wordv = NULL;
        if (!(flags & WRDE_DOOFFS))
            we->we_offs = 0;
    }
    ret = __wordexp_check(words, flags, &cmdsub);
    if (ret)
        return ret;

    status = __wordexp_shell(words, flags, &out, &len);
    if (status == -2)
        return WRDE_NOSPACE;
    if (status < 0)
        return WRDE_SYNTAX;
    if (!__wordexp_ok(status, len)) {
        free(out);
        if (!(flags & WRDE_UNDEF))
            return WRDE_SYNTAX;
        /*
         * set -u failed the shell, or something else did. Without
         * commands to repeat, try again without it to tell which.
         */
        if (!cmdsub) {
            status = __wordexp_shell(words, flags & ~WRDE_UNDEF, &out, &len);
            if (status == -2)
                return WRDE_NOSPACE;
            if (status >= 0)
                free(out);
            if (status < 0 || !__wordexp_ok(status, len))
                return WRDE_SYNTAX;
        }
        return WRDE_BADVAL;
    }

    /* Count the fields after the leading x */
    for (i = 0; i < len; i++)
        if (out[i] == '\0')
            count++;
    count--;
    offs = (flags & WRDE_DOOFFS) ? we->we_offs : 0;
    wordv = realloc(we->we_wordv, (offs + we->we_wordc + count + 1) * sizeof(char *));
    if (!wordv) {
        free(out);
        return WRDE_NOSPACE;
    }
    if (!we->we_wordv)
        for (i = 0; i < offs; i++)
            wordv[i] = NULL;
    we->we_wordv = wordv;
    p = out + 2;
    for (i = 0; i < count; i++) {
        char *w = strdup(p);

        if (!w) {
            wordv[offs + we->we_wordc] = NULL;
            free(out);
            return WRDE_NOSPACE;
        }
        wordv[offs + we->we_wordc++] = w;
        p += strlen(p) + 1;
    }
    wordv[offs + we->we_wordc] = NULL;
    free(out);
    return 0;
}

int
wordexp(const char * __restrict words, wordexp_t * __restrict we, int flags)
{
    int ret;
#ifdef __PICOLIBC_PTHREADS
    int cancel;

    /*
     * wordexp may be a cancellation point, but acting on a request in
     * the middle would leak the pipe, the output and the child
     */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);
#endif
    ret = __wordexp(words, we, flags);
#ifdef __PICOLIBC_PTHREADS
    pthread_setcancelstate(cancel, NULL);
#endif
    return ret;
}
