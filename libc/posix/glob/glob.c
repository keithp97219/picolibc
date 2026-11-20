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
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <glob.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * Expand the pattern one component at a time. Components without
 * pattern characters are appended as they are; the others are matched
 * against each entry of the directory built so far. Whatever names
 * remain at the end of the pattern and exist are the matches.
 */

struct glob_state {
    char   path[PATH_MAX];
    char **list;
    size_t count;
    size_t size;
    int    flags;
    int    (*errfunc)(const char *, int);
};

/* Does this component contain an unescaped *, ? or [ */
static int
glob_magic(const char *pat, size_t len, int noescape)
{
    size_t i;

    for (i = 0; i < len; i++) {
        switch (pat[i]) {
        case '\\':
            if (!noescape && i + 1 < len)
                i++;
            break;
        case '*':
        case '?':
        case '[':
            return 1;
        }
    }
    return 0;
}

static int
glob_add(struct glob_state *st, const char *name, int mark)
{
    size_t len = strlen(name);
    char  *copy;

    if (st->count == st->size) {
        size_t new_size = st->size ? st->size * 2 : 16;
        char **new_list = reallocarray(st->list, new_size, sizeof(*new_list));

        if (new_list == NULL)
            return GLOB_NOSPACE;
        st->list = new_list;
        st->size = new_size;
    }
    copy = malloc(len + 2);
    if (copy == NULL)
        return GLOB_NOSPACE;
    memcpy(copy, name, len + 1);
    if (mark) {
        copy[len] = '/';
        copy[len + 1] = '\0';
    }
    st->list[st->count++] = copy;
    return 0;
}

/* A directory couldn't be read, or a path was too long: give up if asked to */
static int
glob_error(struct glob_state *st, const char *dir, int err)
{
    if (err == ENOENT || err == ENOTDIR)
        return 0;
    if ((st->errfunc && st->errfunc(dir, err)) || (st->flags & GLOB_ERR))
        return GLOB_ABORTED;
    return 0;
}

static int
glob_expand(struct glob_state *st, size_t plen, const char *pat)
{
    int         noescape = (st->flags & GLOB_NOESCAPE) != 0;
    const char *end;
    size_t      len;
    struct stat sb;

    /* Copy separators */
    while (*pat == '/') {
        if (plen + 1 >= sizeof(st->path)) {
            st->path[plen] = '\0';
            return glob_error(st, st->path, ENAMETOOLONG);
        }
        st->path[plen++] = *pat++;
    }
    st->path[plen] = '\0';

    /* The end of the pattern: the path is a match if it exists */
    if (*pat == '\0') {
        int mark = 0;

        if (plen == 0)
            return 0;
        if (st->path[plen - 1] == '/') {
            /* A trailing slash only matches directories */
            if (stat(st->path, &sb) != 0 || !S_ISDIR(sb.st_mode))
                return 0;
        } else {
            if (lstat(st->path, &sb) != 0)
                return 0;
            if ((st->flags & GLOB_MARK)
                && (S_ISDIR(sb.st_mode)
                    || (S_ISLNK(sb.st_mode) && stat(st->path, &sb) == 0 && S_ISDIR(sb.st_mode))))
                mark = 1;
        }
        return glob_add(st, st->path, mark);
    }

    end = strchr(pat, '/');
    if (end == NULL)
        end = pat + strlen(pat);
    len = (size_t)(end - pat);

    if (!glob_magic(pat, len, noescape)) {
        /* Append the literal component, without its escapes */
        size_t i;

        for (i = 0; i < len; i++) {
            if (pat[i] == '\\' && !noescape && i + 1 < len)
                i++;
            if (plen + 1 >= sizeof(st->path)) {
                st->path[plen] = '\0';
                return glob_error(st, st->path, ENAMETOOLONG);
            }
            st->path[plen++] = pat[i];
        }
        return glob_expand(st, plen, end);
    } else {
        /* Match the component against each directory entry */
        const char    *dir = plen ? st->path : ".";
        char          *comp;
        DIR           *dirp;
        struct dirent *ent;
        int            ret = 0;

        comp = malloc(len + 1);
        if (comp == NULL)
            return GLOB_NOSPACE;
        memcpy(comp, pat, len);
        comp[len] = '\0';

        dirp = opendir(dir);
        if (dirp == NULL) {
            ret = glob_error(st, dir, errno);
            free(comp);
            return ret;
        }
        while ((ent = readdir(dirp)) != NULL) {
            size_t nlen;

            if (fnmatch(comp, ent->d_name, FNM_PERIOD | (noescape ? FNM_NOESCAPE : 0)) != 0)
                continue;
            nlen = strlen(ent->d_name);
            if (plen + nlen >= sizeof(st->path)) {
                ret = glob_error(st, dir, ENAMETOOLONG);
                if (ret != 0)
                    break;
                continue;
            }
            memcpy(st->path + plen, ent->d_name, nlen);
            ret = glob_expand(st, plen + nlen, end);
            /* The recursion overwrote the path; restore the directory */
            st->path[plen] = '\0';
            if (ret != 0)
                break;
        }
        closedir(dirp);
        free(comp);
        return ret;
    }
}

static int
glob_compare(const void *a, const void *b)
{
    return strcoll(*(char * const *)a, *(char * const *)b);
}

int
glob(const char * __restrict pattern, int flags, int (*errfunc)(const char *, int),
     glob_t * __restrict pglob)
{
    struct glob_state *st;
    size_t             offs, old, i;
    char             **pathv;
    int                ret;

    if (!(flags & GLOB_APPEND)) {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
        if (!(flags & GLOB_DOOFFS))
            pglob->gl_offs = 0;
    }
    offs = (flags & GLOB_DOOFFS) ? pglob->gl_offs : 0;

    st = calloc(1, sizeof(*st));
    if (st == NULL)
        return GLOB_NOSPACE;
    st->flags = flags;
    st->errfunc = errfunc;

    ret = glob_expand(st, 0, pattern);
    if (ret == 0 && st->count == 0) {
        if (flags & GLOB_NOCHECK)
            ret = glob_add(st, pattern, 0);
        else
            ret = GLOB_NOMATCH;
    }

    /* Add what was found, even after an error */
    if (st->count) {
        if (!(flags & GLOB_NOSORT))
            qsort(st->list, st->count, sizeof(*st->list), glob_compare);
        old = pglob->gl_pathv ? pglob->gl_pathc : 0;
        pathv = reallocarray(pglob->gl_pathv, offs + old + st->count + 1, sizeof(*pathv));
        if (pathv == NULL) {
            for (i = 0; i < st->count; i++)
                free(st->list[i]);
            ret = GLOB_NOSPACE;
        } else {
            if (pglob->gl_pathv == NULL)
                for (i = 0; i < offs; i++)
                    pathv[i] = NULL;
            memcpy(pathv + offs + old, st->list, st->count * sizeof(*pathv));
            pathv[offs + old + st->count] = NULL;
            pglob->gl_pathv = pathv;
            pglob->gl_pathc = old + st->count;
        }
    }
    free(st->list);
    free(st);
    return ret;
}

void
globfree(glob_t *pglob)
{
    size_t i;

    if (pglob->gl_pathv) {
        for (i = 0; i < pglob->gl_pathc; i++)
            free(pglob->gl_pathv[pglob->gl_offs + i]);
        free(pglob->gl_pathv);
    }
    pglob->gl_pathv = NULL;
    pglob->gl_pathc = 0;
}
