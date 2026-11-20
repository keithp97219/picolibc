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
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* How many symbolic links to follow before reporting a loop */
#define REALPATH_SYMLOOP_MAX 40

/*
 * Resolve path one component at a time. 'out' holds the resolved
 * prefix, and 'rest' the components still to go; following a link
 * replaces 'rest' with the link's contents followed by what remained.
 */
char *
realpath(const char * __restrict path, char * __restrict resolved)
{
    char        out[PATH_MAX];
    char        rest[PATH_MAX];
    char        link[PATH_MAX];
    size_t      out_len;
    const char *next;
    int         links = 0;

    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (*path == '\0') {
        errno = ENOENT;
        return NULL;
    }
    if (strlen(path) >= sizeof(rest)) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    strcpy(rest, path);

    if (rest[0] == '/') {
        out[0] = '\0';
    } else {
        if (getcwd(out, sizeof(out)) == NULL)
            return NULL;
        if (strcmp(out, "/") == 0)
            out[0] = '\0';
    }
    out_len = strlen(out);

    next = rest;
    for (;;) {
        const char *name;
        size_t      name_len;
        struct stat st;
        ssize_t     link_len;

        /* Find the next component */
        while (*next == '/')
            next++;
        if (*next == '\0')
            break;
        name = next;
        while (*next != '\0' && *next != '/')
            next++;
        name_len = (size_t)(next - name);

        if (name_len == 1 && name[0] == '.')
            continue;
        if (name_len == 2 && name[0] == '.' && name[1] == '.') {
            while (out_len > 0 && out[out_len - 1] != '/')
                out_len--;
            if (out_len > 0)
                out_len--;
            out[out_len] = '\0';
            continue;
        }

        /* Append it */
        if (out_len + 1 + name_len >= sizeof(out)) {
            errno = ENAMETOOLONG;
            return NULL;
        }
        out[out_len] = '/';
        memcpy(out + out_len + 1, name, name_len);
        out[out_len + 1 + name_len] = '\0';

        if (lstat(out, &st) != 0)
            return NULL;

        if (!S_ISLNK(st.st_mode)) {
            out_len += 1 + name_len;
            /* Only directories may have more components after them */
            if (!S_ISDIR(st.st_mode)) {
                const char *more = next;

                while (*more == '/')
                    more++;
                if (*more != '\0' || *next == '/') {
                    errno = ENOTDIR;
                    return NULL;
                }
            }
            continue;
        }

        /* A link: resolve its contents in place of this component */
        if (++links > REALPATH_SYMLOOP_MAX) {
            errno = ELOOP;
            return NULL;
        }
        link_len = readlink(out, link, sizeof(link));
        if (link_len < 0)
            return NULL;
        if ((size_t)link_len >= sizeof(link) || (size_t)link_len + strlen(next) >= sizeof(rest)) {
            errno = ENAMETOOLONG;
            return NULL;
        }
        memmove(rest + link_len, next, strlen(next) + 1);
        memcpy(rest, link, (size_t)link_len);
        next = rest;

        if (link[0] == '/')
            out_len = 0;
        out[out_len] = '\0';
    }

    if (out_len == 0) {
        out[0] = '/';
        out[1] = '\0';
    }

    if (resolved == NULL)
        return strdup(out);
    return strcpy(resolved, out);
}
