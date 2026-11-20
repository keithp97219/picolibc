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

#include "local-aio.h"
#include <stdlib.h>

struct __aio_thread {
    void         (*function)(union sigval);
    union sigval value;
};

static void *
__aio_thread_run(void *arg)
{
    struct __aio_thread t = *(struct __aio_thread *)arg;

    free(arg);
    t.function(t.value);
    return NULL;
}

/* Delivers a completion notification */
void
__aio_notify(const struct sigevent *sev)
{
    switch (sev->sigev_notify) {
    case SIGEV_SIGNAL:
        __aio_signal(sev->sigev_signo, sev->sigev_value);
        break;
    case SIGEV_THREAD: {
        struct __aio_thread *t = malloc(sizeof(*t));
        pthread_attr_t       attr;
        pthread_t            thread;

        if (!t)
            break;
        t->function = sev->sigev_notify_function;
        t->value = sev->sigev_value;
        /* Leave the application's attributes as they are */
        if (sev->sigev_notify_attributes)
            attr = *(pthread_attr_t *)sev->sigev_notify_attributes;
        else
            pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&thread, &attr, __aio_thread_run, t) != 0)
            free(t);
        break;
    }
    default:
        break;
    }
}
