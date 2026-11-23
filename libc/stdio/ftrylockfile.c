/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2024 Keith Packard
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

#ifndef __SINGLE_THREAD
/* Lock implementations need not provide this; without it, ftrylockfile blocks */
extern int __retarget_lock_try_acquire_recursive(_LOCK_T lock) __weak;

static int __no_thread_safety_analysis
try_lock(_LOCK_T lock)
{
    if (__retarget_lock_try_acquire_recursive)
        return __retarget_lock_try_acquire_recursive(lock);
    __lock_acquire_recursive(lock);
    return 0;
}
#endif

/* Returns zero when the lock was acquired, non-zero when another thread holds it */
int
ftrylockfile(FILE *f) __stdio_try_acquire_file(0, f) __no_thread_safety_analysis
{
#ifdef __SINGLE_THREAD
    (void)f;
    return 0;
#elif defined(__STDIO_LOCKING)
    if (!f->lock)
        __flockfile_init(f);
    if (f->lock == __LOCK_NONE)
        return 0;
    return try_lock(f->lock);
#else
    (void)f;
    return try_lock(&__lock___libc_recursive_mutex);
#endif
}
