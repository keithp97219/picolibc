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

#define _POSIX_C_SOURCE 202405L
#include <sched.h>

/*
 * Names POSIX doesn't reserve for <sched.h>, which it must leave alone
 * in strict mode: GNU extensions, and the <stdint.h> and <sys/types.h>
 * names a careless include would bring in. Checked before any other
 * header comes in.
 */
#if defined(SCHED_BATCH) || defined(SCHED_IDLE) || defined(SCHED_RESET_ON_FORK)
#error "<sched.h> defines GNU policies"
#endif
#if defined(INT8_MAX) || defined(SIZE_MAX) || defined(INTPTR_MAX) || defined(FD_SETSIZE)
#error "<sched.h> defines <stdint.h> or <sys/select.h> names"
#endif
static int sched_getcpu = 1;
static int CPU_SETSIZE = 2;

/* The <sched.h> header: types, structure, constants and prototypes */

#define same_type(a, T) _Generic((a), T: 1, default: 0)

static struct sched_param param;
static struct timespec    ts;
static pid_t              pid;
static time_t             t;

_Static_assert(same_type(param.sched_priority, int), "sched_priority");
_Static_assert(same_type(ts.tv_sec, time_t) && same_type(ts.tv_nsec, long), "timespec");
_Static_assert((pid_t)-1 < 0, "pid_t is signed");
_Static_assert(SCHED_FIFO != SCHED_RR && SCHED_FIFO != SCHED_OTHER && SCHED_RR != SCHED_OTHER,
               "policies are distinct");
#ifdef SCHED_SPORADIC
_Static_assert(SCHED_SPORADIC != SCHED_FIFO && SCHED_SPORADIC != SCHED_RR
                   && SCHED_SPORADIC != SCHED_OTHER,
               "SCHED_SPORADIC is distinct");
#endif

#include <stdio.h>
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_PRIORITY_SCHEDULING

static int (* const p_sched_get_priority_max)(int) = sched_get_priority_max;
static int (* const p_sched_get_priority_min)(int) = sched_get_priority_min;
static int (* const p_sched_getparam)(pid_t, struct sched_param *) = sched_getparam;
static int (* const p_sched_getscheduler)(pid_t) = sched_getscheduler;
static int (* const p_sched_rr_get_interval)(pid_t, struct timespec *) = sched_rr_get_interval;
static int (* const p_sched_setparam)(pid_t, const struct sched_param *) = sched_setparam;
static int (* const p_sched_setscheduler)(pid_t, int, const struct sched_param *)
    = sched_setscheduler;
static int (* const p_sched_yield)(void) = sched_yield;

int
main(void)
{
    if (!p_sched_get_priority_max || !p_sched_get_priority_min || !p_sched_getparam
        || !p_sched_getscheduler || !p_sched_rr_get_interval || !p_sched_setparam
        || !p_sched_setscheduler || !p_sched_yield) {
        printf("functions\n");
        return 1;
    }
    (void)param;
    (void)ts;
    (void)pid;
    (void)t;
    (void)sched_getcpu;
    (void)CPU_SETSIZE;
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    (void)param;
    (void)ts;
    (void)pid;
    (void)t;
    (void)sched_getcpu;
    (void)CPU_SETSIZE;
    printf("no system support, skipping\n");
    return 77;
}

#endif
