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

#include "local-freebsd.h"
#include <mqueue.h>
#include <freebsd/freebsd-mq_attr-struct.h>
#include <freebsd/freebsd-timespec-struct.h>

/*
 * The message queue system calls come from the mqueuefs module; without
 * it they raise SIGSYS. Returns whether it is loaded.
 */
int _mq_supported(void);

/* FreeBSD's names, like POSIX's portable ones, start with '/' */
static inline int
_mq_name(const char *name)
{
    if (name[0] != '/' || name[1] == '\0') {
        errno = EINVAL;
        return -1;
    }
    if (!_mq_supported()) {
        errno = ENOSYS;
        return -1;
    }
    return 0;
}

/* No descriptor is a message queue without the module */
static inline int
_mq_check(void)
{
    if (!_mq_supported()) {
        errno = EBADF;
        return -1;
    }
    return 0;
}

static inline void
_mq_attr_from_freebsd(struct mq_attr *attr, const struct __freebsd_mq_attr *kattr)
{
    attr->mq_flags = (kattr->mq_flags & FREEBSD_O_NONBLOCK) ? O_NONBLOCK : 0;
    attr->mq_maxmsg = kattr->mq_maxmsg;
    attr->mq_msgsize = kattr->mq_msgsize;
    attr->mq_curmsgs = kattr->mq_curmsgs;
}

static inline struct __freebsd_timespec *
_mq_timeout(struct __freebsd_timespec *kts, const struct timespec *ts)
{
    if (!ts)
        return NULL;
    kts->tv_sec = ts->tv_sec;
    kts->tv_nsec = ts->tv_nsec;
    return kts;
}

/*
 * mq_notify's SIGEV_THREAD, which needs threads; this libos's version
 * fails with EINVAL until the threads layer replaces it
 */
int _mq_notify_thread(mqd_t mqdes, const struct sigevent *sev);

#endif /* _LOCAL_MQUEUE_H_ */
