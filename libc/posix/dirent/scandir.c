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
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct scandir_sort {
    int (*compar)(const struct dirent **, const struct dirent **);
};

static int
scandir_compar(const void *a, const void *b, void *thunk)
{
    const struct scandir_sort *sort = thunk;

    return sort->compar((const struct dirent **)a, (const struct dirent **)b);
}

int
scandir(const char *dir, struct dirent ***namelist, int (*sel)(const struct dirent *),
        int (*compar)(const struct dirent **, const struct dirent **))
{
    DIR            *dirp;
    struct dirent  *entry;
    struct dirent **list = NULL;
    size_t          count = 0, size = 0;
    int             save_errno;

    dirp = opendir(dir);
    if (!dirp)
        return -1;

    /* readdir returns NULL with errno unchanged at the end */
    errno = 0;
    while ((entry = readdir(dirp)) != NULL) {
        struct dirent *copy;

        if (sel && !sel(entry)) {
            errno = 0;
            continue;
        }
        if (count == INT_MAX) {
            errno = EOVERFLOW;
            break;
        }
        if (count == size) {
            size_t          new_size = size ? size * 2 : 16;
            struct dirent **new_list = reallocarray(list, new_size, sizeof(*list));

            if (!new_list)
                break;
            list = new_list;
            size = new_size;
        }
        copy = malloc(sizeof(*copy));
        if (!copy)
            break;
        memcpy(copy, entry, sizeof(*copy));
        list[count++] = copy;
        errno = 0;
    }

    save_errno = errno;
    closedir(dirp);
    if (save_errno) {
        while (count)
            free(list[--count]);
        free(list);
        errno = save_errno;
        return -1;
    }
    if (compar && count > 1) {
        struct scandir_sort sort = { compar };

        qsort_r(list, count, sizeof(*list), scandir_compar, &sort);
    }
    *namelist = list;
    return (int)count;
}
