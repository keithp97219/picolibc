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

#ifndef _MQUEUE_H_
#define _MQUEUE_H_

#include <sys/cdefs.h>
#include <sys/_types.h>
#define __need_size_t
#include <stddef.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

_BEGIN_STD_C

#ifndef _SSIZE_T_DECLARED
typedef __ssize_t ssize_t;
#define _SSIZE_T_DECLARED
#endif

/* Part of picolibc's OS-independent ABI, implemented by the OS layer */
typedef int mqd_t;

struct mq_attr {
    long mq_flags;   /* message queue flags (O_NONBLOCK) */
    long mq_maxmsg;  /* maximum number of messages */
    long mq_msgsize; /* maximum message size */
    long mq_curmsgs; /* number of messages currently queued */
};

int     mq_close(mqd_t) __picolibc_export;
int     mq_getattr(mqd_t, struct mq_attr *) __picolibc_export;
int     mq_notify(mqd_t, const struct sigevent *) __picolibc_export;
mqd_t   mq_open(const char *, int, ...) __picolibc_export;
ssize_t mq_receive(mqd_t, char *, size_t, unsigned *) __picolibc_export;
int     mq_send(mqd_t, const char *, size_t, unsigned) __picolibc_export;
int     mq_setattr(mqd_t, const struct mq_attr     *__restrict, struct mq_attr     *__restrict)
    __picolibc_export;
ssize_t mq_timedreceive(mqd_t, char * __restrict, size_t, unsigned * __restrict,
                        const struct timespec * __restrict) __picolibc_export;
int mq_timedsend(mqd_t, const char *, size_t, unsigned, const struct timespec *) __picolibc_export;
int mq_unlink(const char *) __picolibc_export;

_END_STD_C

#endif /* _MQUEUE_H_ */
