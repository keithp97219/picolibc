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

#include "local-stdio.h"
#include <stdio-bufio.h>
#include <stdio_ext.h>

/*
 * Solaris/glibc/musl stream introspection. Only bufio streams
 * have a buffer; other streams report an empty one.
 */

static struct __file_bufio *
__fbufio(FILE *stream)
{
    if (stream->flags & __SBUF)
        return (struct __file_bufio *)stream;
    return NULL;
}

size_t
__fbufsize(FILE *stream)
{
    struct __file_bufio *bf = __fbufio(stream);

    return bf ? (size_t)bf->size : 0;
}

size_t
__fpending(FILE *stream)
{
    struct __file_bufio *bf = __fbufio(stream);
    size_t               ret = 0;

    if (bf) {
        __bufio_lock(stream);
        if (bf->dir == __SWR)
            ret = (size_t)bf->len;
        __bufio_unlock(stream);
    }
    return ret;
}

int
__flbf(FILE *stream)
{
    struct __file_bufio *bf = __fbufio(stream);

    return bf ? (bf->bflags & __BLBF) != 0 : 0;
}

int
__freadable(FILE *stream)
{
    return (stream->flags & __SRD) != 0;
}

int
__fwritable(FILE *stream)
{
    return (stream->flags & __SWR) != 0;
}

/* Read-only, or the last operation was a read */
int
__freading(FILE *stream)
{
    struct __file_bufio *bf;

    if ((stream->flags & (__SRD | __SWR)) == __SRD)
        return 1;
    if (__atomic_load_ungetc(&stream->unget) != 0)
        return 1;
    bf = __fbufio(stream);
    return bf ? bf->dir == __SRD : 0;
}

/* Write-only, or the last operation was a write */
int
__fwriting(FILE *stream)
{
    struct __file_bufio *bf;

    if ((stream->flags & (__SRD | __SWR)) == __SWR)
        return 1;
    bf = __fbufio(stream);
    return bf ? bf->dir == __SWR : 0;
}

int
__fsetlocking(FILE *stream, int type)
{
    (void)stream;
    (void)type;
    return FSETLOCKING_INTERNAL;
}

/* Discard buffered input and output, and any pushed-back character */
void
__fpurge(FILE *stream)
{
    struct __file_bufio *bf = __fbufio(stream);

    if (bf)
        __bufio_lock(stream);
    __atomic_store_ungetc(&stream->unget, 0);
    if (bf) {
        bf->len = 0;
        bf->off = 0;
        __bufio_unlock(stream);
    }
}

/* Bytes that can be read without touching the device */
size_t
__freadahead(FILE *stream)
{
    struct __file_bufio *bf = __fbufio(stream);
    size_t               ret = __atomic_load_ungetc(&stream->unget) != 0;

    if (bf) {
        __bufio_lock(stream);
        if (bf->dir == __SRD)
            ret += (size_t)(bf->len - bf->off);
        __bufio_unlock(stream);
    }
    return ret;
}

/*
 * The buffered input, when it can be read directly. A pushed-back
 * character isn't in the buffer, so there's no pointer then.
 */
const char *
__freadptr(FILE *stream, size_t *sizep)
{
    struct __file_bufio *bf = __fbufio(stream);
    const char          *ret = NULL;

    if (bf && __atomic_load_ungetc(&stream->unget) == 0) {
        __bufio_lock(stream);
        if (bf->dir == __SRD && bf->off < bf->len) {
            *sizep = (size_t)(bf->len - bf->off);
            ret = bf->buf + bf->off;
        }
        __bufio_unlock(stream);
    }
    return ret;
}

void
__freadptrinc(FILE *stream, size_t inc)
{
    struct __file_bufio *bf = __fbufio(stream);

    if (bf) {
        __bufio_lock(stream);
        bf->off += (int)inc;
        __bufio_unlock(stream);
    }
}
