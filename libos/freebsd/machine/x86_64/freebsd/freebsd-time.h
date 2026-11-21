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

#ifndef _FREEBSD_TIME_H_
#define _FREEBSD_TIME_H_

#define FREEBSD_CLOCK_BOOTTIME           4
#define FREEBSD_CLOCK_MONOTONIC          4
#define FREEBSD_CLOCK_MONOTONIC_COARSE   12
#define FREEBSD_CLOCK_MONOTONIC_FAST     12
#define FREEBSD_CLOCK_MONOTONIC_PRECISE  11
#define FREEBSD_CLOCK_PROCESS_CPUTIME_ID 15
#define FREEBSD_CLOCK_PROF               2
#define FREEBSD_CLOCK_REALTIME           0
#define FREEBSD_CLOCK_REALTIME_COARSE    10
#define FREEBSD_CLOCK_REALTIME_FAST      10
#define FREEBSD_CLOCK_REALTIME_PRECISE   9
#define FREEBSD_CLOCK_SECOND             13
#define FREEBSD_CLOCK_TAI                16
#define FREEBSD_CLOCK_THREAD_CPUTIME_ID  14
#define FREEBSD_CLOCK_UPTIME             5
#define FREEBSD_CLOCK_UPTIME_FAST        8
#define FREEBSD_CLOCK_UPTIME_PRECISE     7
#define FREEBSD_CLOCK_VIRTUAL            1
#define FREEBSD_ITIMER_PROF              2
#define FREEBSD_ITIMER_REAL              0
#define FREEBSD_ITIMER_VIRTUAL           1
#define FREEBSD_TIMER_ABSTIME            1

#endif /* _FREEBSD_TIME_H_ */
