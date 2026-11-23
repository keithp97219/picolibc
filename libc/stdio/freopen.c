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
freopen(const char *pathname, const char *mode, FILE *stream)
{
    FILE                *ret = NULL;
    struct __file_bufio *pf = (struct __file_bufio *)stream;
    int                  fd;
    int                  stdio_flags;
    int                  open_flags;
    int                  buf_size;

    __flockfile(stream);
    /* Can't reopen FILEs which aren't buffered */
    if (!(stream->flags & __SBUF))
        goto exit;

    stdio_flags = __stdio_flags(mode, &open_flags);
    if (stdio_flags == 0)
        goto exit;

    /* Flush first, ignoring any failure */
    (void)__fflush_locked(stream);

    __bufio_lock(stream);

    if (pathname != NULL) {
        /*
         * Close the old file before opening the new one, ignoring any
         * failure, so the new descriptor can take the old number
         */
        (void)bufio_close(pf);
        fd = open(pathname, open_flags, 0666);
    } else {
        fd = _FDEV_BUFIO_FD(pf);
#ifdef __HAVE_FCNTL
        /* Fail with EBADF when the descriptor isn't valid */
        if (fcntl(fd, F_GETFD) < 0)
            fd = -1;
#endif
    }

    /* Errors and end-of-file are cleared, as is the orientation */
    stream->flags &= ~(__SRD | __SWR | __SERR | __SEOF | __SWIDE);
    __atomic_store_ungetc(&stream->unget, 0);
    pf->ptr = (void *)(intptr_t)(fd);
    pf->dir = 0;
    pf->len = 0;
    pf->off = 0;

    /* Switch to POSIX backend */
    pf->bflags &= ~(__BFPTR | __BSYNC);
    pf->read_int = read;
    pf->write_int = write;
    pf->lseek_int = lseek;
    pf->close_int = close;

    if (fd < 0) {
        /* The old file is closed even though the open failed */
        __bufio_unlock(stream);
        goto exit;
    }

    stream->flags |= stdio_flags;
    if (pathname != NULL)
        pf->pos = 0;
    else {
        pf->bflags |= __BSYNC;
        bufio_sync_pos(pf);
    }

    /* Reset buffer mode and size */
    int (*setvbuf)(struct __file *, char *buf, int mode, size_t size) = 0;

    buf_size = bufio_get_buf_size(fd);
    if (buf_size != pf->size || (pf->bflags & __BLBF)) {
        setvbuf = pf->xfile.setvbuf;
        if (!setvbuf)
            setvbuf = __bufio_setvbuf;
    }

    ret = stream;
    __bufio_unlock(stream);

    if (setvbuf)
        (void)(*setvbuf)(stream, NULL, _IOFBF, buf_size);

exit:
    __funlock_return(stream, ret);
}
