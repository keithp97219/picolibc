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

#ifndef _LOCAL_SHM_H_
#define _LOCAL_SHM_H_

#include "local-freebsd.h"
#include <limits.h>
#include <string.h>

#define _SHM_PATH_MAX (NAME_MAX + 2)

/*
 * Build the kernel's name for a shared memory object or message
 * queue: one '/' and the name. Portable names start with '/' and
 * contain no other; as libos/linux does, the leading '/' is optional.
 * Returns 0 or an errno value.
 */
static inline int
_shm_path(char *path, const char *name)
{
    size_t name_len;

    while (name[0] == '/')
        name++;
    name_len = strlen(name);
    if (name_len == 0 || strchr(name, '/') != NULL)
        return EINVAL;
    /* As on Linux, where they would name directories */
    if (name[0] == '.' && (name_len == 1 || (name_len == 2 && name[1] == '.')))
        return EINVAL;
    if (name_len > NAME_MAX)
        return ENAMETOOLONG;
    path[0] = '/';
    memcpy(path + 1, name, name_len + 1);
    return 0;
}

#endif /* _LOCAL_SHM_H_ */
