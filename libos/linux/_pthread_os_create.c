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
#include <linux/linux-errno.h>

#define LINUX_CLONE_PTHREAD                                                                      \
    (LINUX_CLONE_VM | LINUX_CLONE_FS | LINUX_CLONE_FILES | LINUX_CLONE_SIGHAND                   \
     | LINUX_CLONE_THREAD | LINUX_CLONE_SYSVSEM | LINUX_CLONE_SETTLS | LINUX_CLONE_PARENT_SETTID \
     | LINUX_CLONE_CHILD_CLEARTID)

/* Called by _linux_clone in the new thread */
static void *
pthread_start(void *arg)
{
    _pthread_start(arg);
}

int
_pthread_os_create(struct __pthread *t)
{
    char   tls_arg_space[__PTHREAD_TLS_ARG_SIZE] __aligned(sizeof(void *));
    void  *tls_arg = _pthread_tls_setup(t->tls, tls_arg_space);
    void **sp;
    long   ret;

    /* _linux_clone pops the start function and its argument from the new stack */
    sp = (void **)t->stack_top - 2;
    sp[0] = (void *)(uintptr_t)pthread_start;
    sp[1] = t;

    /*
     * The kernel stores the thread id in t->tid for both threads, and
     * clears and wakes it when the thread exits
     */
    ret = _linux_clone(LINUX_CLONE_PTHREAD, sp, &t->tid, &t->tid, tls_arg);
    if (ret < 0)
        return ret == -LINUX_ENOMEM ? ENOMEM : EAGAIN;
    return 0;
}
