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

#ifndef _LOCAL_PTHREAD_OS_H_
#define _LOCAL_PTHREAD_OS_H_

/*
 * A stub OS for libos/pthread, standing in for an RTOS backend such
 * as Zephyr's: its thread creation places TLS itself, and the system
 * keeps running when the last thread exits. stub-os.c implements the
 * hooks, running each new thread to completion on the caller's stack
 * when the test asks.
 */

#define _PTHREAD_HAVE_OS_TLS    1
#define _PTHREAD_HAVE_LAST_EXIT 1

/* The stack bytes the stub OS says it takes for TLS, not a multiple of the alignment */
#define STUB_OS_TLS_SIZE 1000

struct __pthread_os {
    int started;
};

/* _pthread_os_exit clears the thread id once the thread is done */
#define _pthread_exit_word(t) ((__pthread_futex_t *)&(t)->tid)
#define _PTHREAD_EXIT_DONE    0U

#endif /* _LOCAL_PTHREAD_OS_H_ */
