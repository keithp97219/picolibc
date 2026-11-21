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

#ifndef _LOCAL_SYSLOG_H_
#define _LOCAL_SYSLOG_H_

#define _DEFAULT_SOURCE
#include <syslog.h>
#include <sys/lock.h>
#include <picolibc.h>
#ifdef __PICOLIBC_PTHREADS
#include <pthread.h>
#endif

/* The openlog settings, guarded by the C library lock */
extern const char *__syslog_ident;
extern int         __syslog_option;
extern int         __syslog_facility;
extern int         __syslog_mask;
extern int         __syslog_fd;

/* Connects to the system logger, returning a descriptor or -1 */
int                __syslog_connect(void);

/*
 * The logging functions call cancellation points with the lock held,
 * so they hold cancellation off; acting on a request there would leave
 * the lock taken and hang every other thread's libc use.
 */
static inline int
__syslog_cancel_disable(void)
{
    int cancel = 0;

#ifdef __PICOLIBC_PTHREADS
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);
#endif
    return cancel;
}

static inline void
__syslog_cancel_restore(int cancel)
{
#ifdef __PICOLIBC_PTHREADS
    pthread_setcancelstate(cancel, NULL);
#else
    (void)cancel;
#endif
}

#endif /* _LOCAL_SYSLOG_H_ */
