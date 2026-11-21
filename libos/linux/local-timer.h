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

#include "local-sigaction.h"
#include "local-time.h"
#include <picolibc.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/*
 * The timer_settime and timer_gettime system calls, paired with an
 * itimerspec that matches them: the 64-bit time ones on 32-bit
 * architectures that have them, like the clock_* calls in local-time.h
 */
#ifdef LINUX_SYS_timer_settime64
#define LINUX_SYS_timer_settime_time_t LINUX_SYS_timer_settime64
#define LINUX_SYS_timer_gettime_time_t LINUX_SYS_timer_gettime64

#else
#define LINUX_SYS_timer_settime_time_t LINUX_SYS_timer_settime
#define LINUX_SYS_timer_gettime_time_t LINUX_SYS_timer_gettime
#endif

#include <linux/linux-itimerspec64-struct.h>
#include <linux/linux-sigevent-struct.h>

#define __kernel_itimerspec_time_t __kernel_itimerspec64

/* Linux sigev_notify values */
#define LINUX_SIGEV_SIGNAL    0
#define LINUX_SIGEV_NONE      1
#define LINUX_SIGEV_THREAD_ID 4

/* picolibc's timer_t holds the kernel's timer id */
static inline int
_timer_id(timer_t timerid)
{
    return (int)timerid;
}

/*
 * SIGEV_THREAD timers have a helper thread, which the kernel signals
 * directly with a Linux real-time signal picolibc's signal numbers
 * never reach. Returns 0 or an errno value.
 */
#define _TIMER_SIGNAL 33

int  _timer_create_thread(int kclock, const struct sigevent *evp, timer_t *timerid);
bool _timer_delete_thread(timer_t timerid);

#endif /* _LOCAL_TIMER_H_ */
