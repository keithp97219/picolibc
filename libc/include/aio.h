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

#ifndef _AIO_H_
#define _AIO_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

_BEGIN_STD_C

struct aiocb {
    int             aio_fildes;     /* File descriptor */
    off_t           aio_offset;     /* File offset */
    volatile void  *aio_buf;        /* Location of buffer */
    size_t          aio_nbytes;     /* Length of transfer */
    int             aio_reqprio;    /* Request priority offset */
    struct sigevent aio_sigevent;   /* Signal number and value */
    int             aio_lio_opcode; /* Operation to be performed */

    /* Private to the implementation */
    int             __aio_error;
    ssize_t         __aio_return;
};

/* aio_cancel results */
#define AIO_ALLDONE     0
#define AIO_CANCELED    1
#define AIO_NOTCANCELED 2

/* lio_listio modes */
#define LIO_NOWAIT 0
#define LIO_WAIT   1

/* aio_lio_opcode values */
#define LIO_NOP   0
#define LIO_READ  1
#define LIO_WRITE 2

int     aio_cancel(int, struct aiocb *) __picolibc_export;
int     aio_error(const struct aiocb *) __picolibc_export;
int     aio_fsync(int, struct aiocb *) __picolibc_export;
int     aio_read(struct aiocb *) __picolibc_export;
ssize_t aio_return(struct aiocb *) __picolibc_export;
int     aio_suspend(const struct aiocb * const[], int, const struct timespec *) __picolibc_export;
int     aio_write(struct aiocb *) __picolibc_export;
int     lio_listio(int, struct aiocb * __restrict const[__restrict], int,
                   struct sigevent * __restrict) __picolibc_export;

_END_STD_C

#endif /* _AIO_H_ */
