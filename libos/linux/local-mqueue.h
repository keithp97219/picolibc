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

#ifndef _LOCAL_MQUEUE_H_
#define _LOCAL_MQUEUE_H_

#include "local-linux.h"
#include "local-cancel.h"
#include "local-time.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>

/* The kernel's struct mq_attr */
struct __kernel_mq_attr {
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
    long __reserved[4];
};

/* The timed send and receive calls, 64-bit time where the kernel has them */
#ifdef LINUX_SYS_mq_timedsend_time64
#define LINUX_SYS_mq_timedsend_time_t    LINUX_SYS_mq_timedsend_time64
#define LINUX_SYS_mq_timedreceive_time_t LINUX_SYS_mq_timedreceive_time64
#else
#define LINUX_SYS_mq_timedsend_time_t    LINUX_SYS_mq_timedsend
#define LINUX_SYS_mq_timedreceive_time_t LINUX_SYS_mq_timedreceive
#endif

/* Linux names message queues without POSIX's leading '/' */
static inline const char *
_mq_name(const char *name)
{
    if (name[0] != '/' || name[1] == '\0') {
        errno = EINVAL;
        return NULL;
    }
    return name + 1;
}

static inline void
_mq_timeout(struct __kernel_timespec_time_t *kts, const struct timespec *ts)
{
    kts->tv_sec = ts->tv_sec;
    kts->tv_nsec = ts->tv_nsec;
}

static inline void
_mq_attr_from_linux(struct mq_attr *attr, const struct __kernel_mq_attr *kattr)
{
    attr->mq_flags = (kattr->mq_flags & LINUX_O_NONBLOCK) ? O_NONBLOCK : 0;
    attr->mq_maxmsg = kattr->mq_maxmsg;
    attr->mq_msgsize = kattr->mq_msgsize;
    attr->mq_curmsgs = kattr->mq_curmsgs;
}

int _mq_notify_thread(mqd_t mqdes, const struct sigevent *sev);

#endif /* _LOCAL_MQUEUE_H_ */
