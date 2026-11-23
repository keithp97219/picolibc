/*
 * SPDX-FileCopyrightText: 2024-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "local-stdio.h"

#define GET_BUF(mf)  ((mf)->unit == 1 ? *(mf)->pbuf.c : (char *)*(mf)->pbuf.w)
#define GET_SIZE(mf) (*(mf->psize))

/* Elements are bytes for open_memstream, wide characters for open_wmemstream */
struct __file_open_mem {
    struct __file_ext xfile;
    union {
        char    **c;
        wchar_t **w;
    } pbuf;
    size_t *psize;
    size_t  unit;  /* element size */
    size_t  bsize; /* allocated buffer size */
    size_t  fsize; /* file content size */
    size_t  pos;   /* current position in the buffer */
};

static int
__open_mem_grow(struct __file_open_mem *mf, size_t additional_size)
{
    size_t bsize = mf->bsize;
    char  *tmp;

    if (additional_size > SIZE_MAX - mf->pos) {
        errno = ENOMEM;
        return _FDEV_ERR;
    }

    size_t required_size = mf->pos + additional_size;
    if (required_size < bsize)
        return 0;

    /* Avoid geometric growth, increase the buffer size by 1.5x */
    bsize += bsize / 2;

    /* seek can move position beyond calculated size */
    if (bsize < required_size)
        bsize = required_size;

    /* room for the null terminator */
    bsize += mf->unit;

    /* check for overflow */
    if (bsize < mf->bsize) {
        errno = ENOMEM;
        return _FDEV_ERR;
    }

    tmp = realloc(GET_BUF(mf), bsize);
    if (!tmp)
        return _FDEV_ERR;

    /* fill the new buffer with zeros */
    memset(tmp + mf->bsize, 0, bsize - mf->bsize);

    /* update structure */
    if (mf->unit == 1)
        *mf->pbuf.c = tmp;
    else
        *mf->pbuf.w = (wchar_t *)tmp;
    mf->bsize = bsize;

    return 0;
}

static int
__open_mem_put(char c, FILE *f)
{
    struct __file_open_mem *mf = (struct __file_open_mem *)f;

    /* Room for the new byte and the null terminator
     *
     * Allocation will zero fill the extended part of the buffer.
     * No need to set null terminator character explicitly.
     *
     * The first byte of a wide character makes room for all of it,
     * so that running out of memory never leaves part of one behind
     * to misalign those written after.
     */
    int ret = __open_mem_grow(mf, mf->unit - (mf->pos & (mf->unit - 1)) + mf->unit);
    if (ret != 0)
        return ret;

    GET_BUF(mf)[mf->pos++] = c;

    mf->fsize = mf->pos;

    return (unsigned char)c;
}

static int
__open_mem_flush(FILE *f)
{
    struct __file_open_mem *mf = (struct __file_open_mem *)f;

    GET_SIZE(mf) = mf->fsize / mf->unit;

    return 0;
}

/* Positions count elements, while pos and fsize count bytes */
static off_t
__open_mem_seek(FILE *f, off_t pos, int whence)
{
    struct __file_open_mem *mf = (struct __file_open_mem *)f;

    switch (whence) {
    case SEEK_SET:
        break;
    case SEEK_CUR:
        pos += mf->pos / mf->unit;
        break;
    case SEEK_END:
        pos += mf->fsize / mf->unit;
        break;
    default:
        errno = EINVAL;
        return (off_t)-1;
    }

    if (pos < 0) {
        errno = EINVAL;
        return (off_t)-1;
    }

    if ((uintmax_t)pos > SIZE_MAX / mf->unit) {
        errno = ENOMEM;
        return (off_t)-1;
    }
    size_t offset = (size_t)pos * mf->unit;
    if (mf->bsize < offset) {
        if (__open_mem_grow(mf, offset - mf->pos) != 0)
            return (off_t)-1;
    }
    mf->pos = offset;
    mf->fsize = offset;
    return pos;
}

static int
__open_mem_close(FILE *f)
{
    __open_mem_flush(f);
    free(f);
    return 0;
}

/* Exactly one of cbuf and wbuf is set, choosing bytes or wide characters */
FILE *
__open_memstream(char **cbuf, wchar_t **wbuf, size_t *size)
{
    struct __file_open_mem *mf;
    char                   *buf;
    size_t                  unit = wbuf ? sizeof(wchar_t) : 1;

    if ((!cbuf && !wbuf) || !size) {
        errno = EINVAL;
        return NULL;
    }

    mf = calloc(1, sizeof(struct __file_open_mem));
    if (mf == NULL) {
        return NULL;
    }

    buf = calloc(1, BUFSIZ);
    if (buf == NULL) {
        free(mf);
        return NULL;
    }

    *mf = (struct __file_open_mem) {
        .xfile = FDEV_SETUP_EXT(__open_mem_put, NULL, __open_mem_flush, __open_mem_close,
                                __open_mem_seek, NULL, wbuf ? (__SWR | __SWIDE | __SWCHAR) : __SWR),
        .psize = size,
        .unit = unit,
        .bsize = BUFSIZ,
        .fsize = 0,
        .pos = 0,
    };
    if (wbuf) {
        mf->pbuf.w = wbuf;
        *wbuf = (wchar_t *)buf;
    } else {
        mf->pbuf.c = cbuf;
        *cbuf = buf;
    }
    *size = 0;

    return (FILE *)mf;
}

FILE *
open_memstream(char **buf, size_t *size)
{
    return __open_memstream(buf, NULL, size);
}
