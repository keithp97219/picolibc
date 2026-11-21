/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2019 Keith Packard
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

#include "local-stdio.h"

FILE *
fdopen(int fd, const char *mode)
{
    int                  stdio_flags;
    int                  open_flags;
    struct __file_bufio *bf;
    char                *buf;
    size_t               buf_size;

    stdio_flags = __stdio_flags(mode, &open_flags);
    if (stdio_flags == 0)
        return NULL;

#ifdef __PICOLIBC_POSIX_FD_MGMT
    /* Fail with EBADF for an invalid descriptor */
    int fd_flags = fcntl(fd, F_GETFD);
    if (fd_flags < 0)
        return NULL;

    /* 'e' sets FD_CLOEXEC; without it, the flag is left alone */
    if ((open_flags & O_CLOEXEC) && !(fd_flags & FD_CLOEXEC))
        if (fcntl(fd, F_SETFD, fd_flags | FD_CLOEXEC) < 0)
            return NULL;
#endif

    buf_size = bufio_get_buf_size(fd);

    /* Allocate file structure and necessary buffers */
    bf = calloc(1, sizeof(struct __file_bufio) + buf_size);

    if (bf == NULL)
        return NULL;

    buf = (char *)(bf + 1);

    /* The stream starts at the descriptor's offset */
    *bf = (struct __file_bufio)FDEV_SETUP_POSIX(fd, buf, buf_size, stdio_flags, __BFALL | __BSYNC);
    bufio_sync_pos(bf);

    bufio_add_file(&(bf->xfile.cfile.file));

    return &(bf->xfile.cfile.file);
}
