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

#ifndef _LOCAL_TIMER_H_
#define _LOCAL_TIMER_H_

#include <picolibc.h>
#include "local-signal.h"
#include "local-time.h"
#include <stdbool.h>
#include <freebsd/freebsd-timespec-struct.h>
#include <freebsd/freebsd-itimerspec-struct.h>
#include <freebsd/freebsd-sigevent.h>
#include <freebsd/freebsd-sigevent-struct.h>

/* picolibc's timer_t holds the kernel's timer id */
static inline int
_timer_id(timer_t timerid)
{
    return (int)timerid;
}

/*
 * SIGEV_THREAD timers need a helper thread, which the kernel signals
 * directly (SIGEV_THREAD_ID, with the thread's id in
 * sigev_notify_thread_id) with a signal picolibc's numbers never
 * reach. The threads code provides these two: _timer_create_thread
 * returns 0 or an errno value, and _timer_delete_thread returns
 * whether timerid was one of its timers, which it then deletes.
 */
#ifdef __PICOLIBC_PTHREADS
int  _timer_create_thread(long kclock, const struct sigevent *evp, timer_t *timerid);
bool _timer_delete_thread(timer_t timerid);
#endif

#endif /* _LOCAL_TIMER_H_ */
