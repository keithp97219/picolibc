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

#define _DEFAULT_SOURCE
#include "local-pthread.h"
#include <stdlib.h>

/* The change every thread makes, and how many have yet to make it */
static struct {
    long         nr, a, b, c;
    unsigned int remaining;
    int          failed;
} setxid;

/* Runs in each other thread, from the handler of _PTHREAD_SIGCANCEL */
void
_pthread_setxid_act(void)
{
    if (_syscall_raw(setxid.nr, setxid.a, setxid.b, setxid.c) < 0)
        __atomic_store_n(&setxid.failed, 1, __ATOMIC_RELAXED);
    if (__atomic_sub_fetch(&setxid.remaining, 1, __ATOMIC_ACQ_REL) == 0)
        _futex_wake(&setxid.remaining, 1, false);
}

long
_pthread_setxid(long nr, long a, long b, long c)
{
    struct __pthread *self, *t;
    unsigned int      left;
    long              ret;
    int               tid;

    /* With one thread, the kernel's per-thread change is the whole change */
    if (__atomic_load_n(&_pthread_count, __ATOMIC_ACQUIRE) <= 1)
        return _syscall(nr, a, b, c);

    self = _pthread_self();
    _pthread_sigcancel_init();
    pthread_mutex_lock(&_pthread_all_lock);
    ret = _syscall_raw(nr, a, b, c);
    if (ret == 0) {
        setxid.nr = nr;
        setxid.a = a;
        setxid.b = b;
        setxid.c = c;
        setxid.failed = 0;
        /* Count one extra, dropped below, so no wake comes before all are signalled */
        __atomic_store_n(&setxid.remaining, 1, __ATOMIC_RELEASE);
        for (t = _pthread_all; t; t = t->all_next) {
            tid = __atomic_load_n(&t->tid, __ATOMIC_ACQUIRE);
            if (t == self || !tid)
                continue;
            __atomic_add_fetch(&setxid.remaining, 1, __ATOMIC_ACQ_REL);
            __atomic_store_n(&t->setxid_pending, 1, __ATOMIC_RELEASE);
            /* A thread that has just exited takes no signal and needs no change */
            if (_syscall_raw(LINUX_SYS_tgkill, getpid(), tid, _PTHREAD_SIGCANCEL) < 0
                && __atomic_exchange_n(&t->setxid_pending, 0, __ATOMIC_ACQ_REL))
                __atomic_sub_fetch(&setxid.remaining, 1, __ATOMIC_ACQ_REL);
        }
        left = __atomic_sub_fetch(&setxid.remaining, 1, __ATOMIC_ACQ_REL);
        while (left) {
            (void)_futex_wait(&setxid.remaining, left, false, CLOCK_REALTIME, NULL);
            left = __atomic_load_n(&setxid.remaining, __ATOMIC_ACQUIRE);
        }
        /* Threads with different credentials can't be put right */
        if (setxid.failed)
            abort();
    }
    pthread_mutex_unlock(&_pthread_all_lock);
    if (ret < 0) {
        errno = _syscall_errno(ret);
        return -1;
    }
    return ret;
}
