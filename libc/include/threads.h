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

#ifndef _THREADS_H_
#define _THREADS_H_

#include <sys/cdefs.h>
/* The OS supplies the thread types, initializers and limits */
#if defined(__has_include)
#if !__has_include(<sys/_pthread.h>)
#error "<threads.h> needs threads, which this system doesn't provide"
#endif
#endif
#include <sys/_pthread.h>
#include <time.h>

_BEGIN_STD_C

#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
#define thread_local _Thread_local
#endif

#define TSS_DTOR_ITERATIONS __PTHREAD_DESTRUCTOR_ITERATIONS

typedef pthread_t       thrd_t;
typedef int             (*thrd_start_t)(void *);
typedef pthread_mutex_t mtx_t;
typedef pthread_cond_t  cnd_t;
typedef pthread_key_t   tss_t;
typedef void            (*tss_dtor_t)(void *);

#ifndef __once_flag_defined
#define __once_flag_defined
typedef int once_flag;
#define ONCE_FLAG_INIT 0
#endif

enum {
    mtx_plain = 0,
    mtx_recursive = 1,
    mtx_timed = 2,
};

enum {
    thrd_success = 0,
    thrd_busy = 1,
    thrd_error = 2,
    thrd_nomem = 3,
    thrd_timedout = 4,
};

void call_once(once_flag *, void (*)(void)) __picolibc_export;

int  cnd_broadcast(cnd_t *) __picolibc_export;
void cnd_destroy(cnd_t *) __picolibc_export;
int  cnd_init(cnd_t *) __picolibc_export;
int  cnd_signal(cnd_t *) __picolibc_export;
int  cnd_timedwait(cnd_t  *__restrict, mtx_t  *__restrict, const struct timespec  *__restrict)
    __picolibc_export;
int    cnd_wait(cnd_t *, mtx_t *) __picolibc_export;

void   mtx_destroy(mtx_t *) __picolibc_export;
int    mtx_init(mtx_t *, int) __picolibc_export;
int    mtx_lock(mtx_t *) __picolibc_export;
int    mtx_timedlock(mtx_t    *__restrict, const struct timespec    *__restrict) __picolibc_export;
int    mtx_trylock(mtx_t *) __picolibc_export;
int    mtx_unlock(mtx_t *) __picolibc_export;

int    thrd_create(thrd_t *, thrd_start_t, void *) __picolibc_export;
thrd_t thrd_current(void) __picolibc_export;
int    thrd_detach(thrd_t) __picolibc_export;
int    thrd_equal(thrd_t, thrd_t) __picolibc_export;
_Noreturn void thrd_exit(int) __picolibc_export;
int            thrd_join(thrd_t, int *) __picolibc_export;
int            thrd_sleep(const struct timespec *, struct timespec *) __picolibc_export;
void           thrd_yield(void) __picolibc_export;

int            tss_create(tss_t *, tss_dtor_t) __picolibc_export;
void           tss_delete(tss_t) __picolibc_export;
void          *tss_get(tss_t) __picolibc_export;
int            tss_set(tss_t, void *) __picolibc_export;

_END_STD_C

#endif /* _THREADS_H_ */
