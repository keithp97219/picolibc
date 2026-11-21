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

#include "local-pthread.h"
#include <freebsd/freebsd-errno.h>
#include <freebsd/freebsd-thr_param-struct.h>

/* First code in the new thread, called by the kernel with the thread pointer set */
static void
pthread_start(void *arg)
{
    struct __pthread *t = arg;

    /* thr_new stored the id for this thread before it ran */
    __atomic_store_n(&t->tid, (int)t->os.ktid, __ATOMIC_RELAXED);
    _pthread_start(t);
}

int
_pthread_os_create(struct __pthread *t)
{
    struct __freebsd_thr_param param;
    char                       tls_arg_space[__PTHREAD_TLS_ARG_SIZE] __aligned(sizeof(void *));
    long                       ret;

    memset(&param, 0, sizeof(param));
    param.start_func = (void *)pthread_start;
    param.arg = t;
    /* The kernel starts the thread with its stack at stack_base + stack_size */
    param.stack_base = t->stack_top;
    param.stack_size = 0;
    param.tls_base = _pthread_tls_setup(t->tls, tls_arg_space);
    param.child_tid = &t->os.ktid;
    param.parent_tid = &t->os.ktid;
    param.flags = FREEBSD_THR_SYSTEM_SCOPE;

    ret = _syscall_raw(FREEBSD_SYS_thr_new, &param, sizeof(param));
    if (ret < 0)
        return ret == _FREEBSD_ERR(FREEBSD_ENOMEM) ? ENOMEM : EAGAIN;
    __atomic_store_n(&t->tid, (int)t->os.ktid, __ATOMIC_RELAXED);
    return 0;
}
