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

/* What <mqueue.h> alone provides */

#define _POSIX_C_SOURCE 202405L
#include <mqueue.h>

#if !defined(O_RDONLY) || !defined(O_WRONLY) || !defined(O_RDWR) || !defined(O_CREAT) \
    || !defined(O_EXCL) || !defined(O_NONBLOCK)
#error <mqueue.h> must define the <fcntl.h> open flags
#endif

/* size_t, ssize_t and struct timespec */
_Static_assert(sizeof(size_t) > 0 && sizeof(ssize_t) > 0 && sizeof(struct timespec) > 0,
               "size_t, ssize_t and struct timespec");

/* struct sigevent may be incomplete */
typedef const struct sigevent *sigevent_ptr;

/* mqd_t isn't an array, as functions can't return arrays */
typedef mqd_t                  mqd_function(void);

/* struct mq_attr has long members */
#define is_long(member) _Generic(((struct mq_attr *)0)->member, long: 1, default: 0)
_Static_assert(is_long(mq_flags) && is_long(mq_maxmsg) && is_long(mq_msgsize)
                   && is_long(mq_curmsgs),
               "struct mq_attr members");

#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || (defined(_POSIX_MESSAGE_PASSING) && _POSIX_MESSAGE_PASSING > 0)

/* Each function has the specified prototype */
static int     (    *const p_mq_close)(mqd_t) = mq_close;
static int     (    *const p_mq_getattr)(mqd_t, struct mq_attr *) = mq_getattr;
static int     (    *const p_mq_notify)(mqd_t, const struct sigevent *) = mq_notify;
static mqd_t   (  *const p_mq_open)(const char *, int, ...) = mq_open;
static ssize_t (* const p_mq_receive)(mqd_t, char *, size_t, unsigned *) = mq_receive;
static int     (    *const p_mq_send)(mqd_t, const char *, size_t, unsigned) = mq_send;
static int (* const p_mq_setattr)(mqd_t, const struct mq_attr * restrict, struct mq_attr * restrict)
    = mq_setattr;
static ssize_t (* const p_mq_timedreceive)(mqd_t, char * restrict, size_t, unsigned * restrict,
                                           const struct timespec * restrict)
    = mq_timedreceive;
static int (* const p_mq_timedsend)(mqd_t, const char *, size_t, unsigned, const struct timespec *)
    = mq_timedsend;
static int (* const p_mq_unlink)(const char *) = mq_unlink;

#include <stdio.h>

int
main(void)
{
    (void)p_mq_close;
    (void)p_mq_getattr;
    (void)p_mq_notify;
    (void)p_mq_open;
    (void)p_mq_receive;
    (void)p_mq_send;
    (void)p_mq_setattr;
    (void)p_mq_timedreceive;
    (void)p_mq_timedsend;
    (void)p_mq_unlink;
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
