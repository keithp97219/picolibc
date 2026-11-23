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

#ifdef __STDIO_EXIT_FLUSH

/*
 * The streams opened at run time, which fflush(NULL) and exit flush.
 * A stream is added when it is opened and stays on the list until
 * fclose removes it; flushing never takes it off.
 *
 * Lock order: the list lock, then a stream's locks (the stdio lock,
 * then the bufio lock), then __LIBC_LOCK. Walking the list holds the
 * list lock while flushing each stream, so fclose can't free a stream
 * out from under the walk. Nothing takes the list lock while holding a
 * stream lock: fclose removes the stream before taking its locks and
 * the open functions add it after setting it up.
 *
 * As in other C libraries, a thread holding flockfile on one stream
 * while it opens or closes another, or calls fflush(NULL), can deadlock
 * with a thread flushing every stream.
 */

FILE *__stdio_file_list;

#ifndef __SINGLE_THREAD
static _LOCK_RECURSIVE_T __stdio_list_lock;
#endif

static void __no_thread_safety_analysis
stdio_list_lock(void)
{
#ifndef __SINGLE_THREAD
    __LIBC_LOCK();
    if (!__stdio_list_lock)
        __lock_init_recursive(__stdio_list_lock);
    __LIBC_UNLOCK();
    __lock_acquire_recursive(__stdio_list_lock);
#endif
}

static void __no_thread_safety_analysis
stdio_list_unlock(void)
{
#ifndef __SINGLE_THREAD
    __lock_release_recursive(__stdio_list_lock);
#endif
}

void
__bufio_add_file(FILE *f)
{
    struct __file_bufio *bf = (struct __file_bufio *)f;

    stdio_list_lock();
    bf->next = __stdio_file_list;
    __stdio_file_list = f;
    stdio_list_unlock();
}

void
__bufio_remove_file(FILE *f)
{
    FILE               **prev;
    struct __file_bufio *bf;

    stdio_list_lock();
    for (prev = &__stdio_file_list; (bf = (struct __file_bufio *)*prev); prev = &bf->next) {
        if (*prev == f) {
            *prev = bf->next;
            break;
        }
    }
    stdio_list_unlock();
}

/* Flush every stream on the list, leaving them all there */
int
__bufio_flush_all(void)
{
    FILE *f;
    int   ret = 0;

    stdio_list_lock();
    for (f = __stdio_file_list; f; f = ((struct __file_bufio *)f)->next)
        if (fflush(f) != 0)
            ret = EOF;
    stdio_list_unlock();
    return ret;
}

static void
_bufio_exit_flush(void)
{
    (void)__bufio_flush_all();
}

__attribute__((constructor)) static void
_bufio_exit_register(void)
{
    atexit(_bufio_exit_flush);
}

#endif
