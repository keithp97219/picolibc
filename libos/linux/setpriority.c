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

#include "local-linux.h"
#ifdef __PICOLIBC_PTHREADS
#include "local-pthread.h"
#endif
#include <sys/resource.h>
#include <unistd.h>

/*
 * Linux keeps a nice value for each thread, but setpriority changes
 * the process: for this process, set the new value in every thread
 */
int
setpriority(int which, id_t who, int prio)
{
#ifdef __PICOLIBC_PTHREADS
    if (which == PRIO_PROCESS && (who == 0 || who == (id_t)getpid())
        && __atomic_load_n(&_pthread_count, __ATOMIC_ACQUIRE) > 1) {
        struct __pthread *self = _pthread_self(), *t;
        int               ret, tid;

        /* Threads are created and listed with this lock held */
        pthread_mutex_lock(&_pthread_all_lock);
        ret = (int)_syscall(LINUX_SYS_setpriority, PRIO_PROCESS, 0, prio);
        if (ret == 0) {
            for (t = _pthread_all; t; t = t->all_next) {
                tid = __atomic_load_n(&t->tid, __ATOMIC_ACQUIRE);
                if (t != self && tid)
                    (void)_syscall_raw(LINUX_SYS_setpriority, PRIO_PROCESS, tid, prio);
            }
        }
        pthread_mutex_unlock(&_pthread_all_lock);
        return ret;
    }
#endif
    return (int)_syscall(LINUX_SYS_setpriority, which, who, prio);
}
