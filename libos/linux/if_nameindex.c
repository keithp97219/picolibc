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

#include "local-socket.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

/*
 * Lists the interfaces in /sys/class/net. The names are stored after
 * the array, so if_freenameindex frees a single block.
 */
struct if_nameindex *
if_nameindex(void)
{
    struct if_nameindex *list = NULL;
    struct dirent       *ent;
    DIR                 *dir;
    size_t               count = 0, size = 0;
    char                *names;
    size_t               i;
    char                 (*found)[IF_NAMESIZE] = NULL;
    unsigned int        *index = NULL;

    dir = opendir("/sys/class/net");
    if (!dir)
        return NULL;
    while ((ent = readdir(dir)) != NULL) {
        unsigned int n;

        if (ent->d_name[0] == '.' || strlen(ent->d_name) >= IF_NAMESIZE)
            continue;
        n = if_nametoindex(ent->d_name);
        if (!n)
            continue;
        if (count == size) {
            size_t        nsize = size ? size * 2 : 8;
            void         *nfound = reallocarray(found, nsize, sizeof(*found));
            unsigned int *nindex;

            if (!nfound)
                goto fail;
            found = nfound;
            nindex = reallocarray(index, nsize, sizeof(*index));
            if (!nindex)
                goto fail;
            index = nindex;
            size = nsize;
        }
        strcpy(found[count], ent->d_name);
        index[count] = n;
        count++;
    }
    closedir(dir);
    dir = NULL;

    list = malloc((count + 1) * sizeof(*list) + count * IF_NAMESIZE);
    if (!list)
        goto fail;
    names = (char *)(list + count + 1);
    for (i = 0; i < count; i++) {
        list[i].if_index = index[i];
        list[i].if_name = names + i * IF_NAMESIZE;
        strcpy(list[i].if_name, found[i]);
    }
    list[count].if_index = 0;
    list[count].if_name = NULL;
fail:
    if (dir)
        closedir(dir);
    free(found);
    free(index);
    if (!list)
        errno = ENOBUFS;
    return list;
}
