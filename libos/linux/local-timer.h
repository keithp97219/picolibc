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

/* Linux sigev_notify values */
#define LINUX_SIGEV_SIGNAL    0
#define LINUX_SIGEV_NONE      1
#define LINUX_SIGEV_THREAD_ID 4

/* The kernel's struct sigevent, which is always 64 bytes */
#define LINUX_SIGEV_SIZE 64

struct __kernel_sigevent {
    union __kernel_sigval sigev_value;
    __int32_t             sigev_signo;
    __int32_t             sigev_notify;
    union {
        __int32_t __pad[(LINUX_SIGEV_SIZE - sizeof(union __kernel_sigval) - 2 * sizeof(__int32_t))
                        / sizeof(__int32_t)];
        __int32_t sigev_tid;
    };
};

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
