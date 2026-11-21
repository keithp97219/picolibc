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

#include "local-poll.h"
#include "local-cancel.h"
#include <stdlib.h>

/* Translate poll event bits between picolibc and FreeBSD */
static short
_poll_events_to_freebsd(short e)
{
    short r = 0;

    if (e & POLLIN)
        r |= FREEBSD_POLLIN;
    if (e & POLLPRI)
        r |= FREEBSD_POLLPRI;
    if (e & POLLOUT)
        r |= FREEBSD_POLLOUT;
    if (e & POLLERR)
        r |= FREEBSD_POLLERR;
    if (e & POLLHUP)
        r |= FREEBSD_POLLHUP;
    if (e & POLLNVAL)
        r |= FREEBSD_POLLNVAL;
    if (e & POLLRDNORM)
        r |= FREEBSD_POLLRDNORM;
    if (e & POLLRDBAND)
        r |= FREEBSD_POLLRDBAND;
    if (e & POLLWRNORM)
        r |= FREEBSD_POLLWRNORM;
    if (e & POLLWRBAND)
        r |= FREEBSD_POLLWRBAND;
    if (e & POLLRDHUP)
        r |= FREEBSD_POLLRDHUP;
    /* FreeBSD has no POLLMSG or POLLREMOVE */
    return r;
}

/*
 * FreeBSD's POLLWRNORM is POLLOUT, so report whichever of the two was
 * asked for
 */
static short
_poll_events_from_freebsd(short e, short events)
{
    short r = 0;

    if (e & FREEBSD_POLLIN)
        r |= POLLIN;
    if (e & FREEBSD_POLLPRI)
        r |= POLLPRI;
    if (e & FREEBSD_POLLOUT)
        r |= events & (POLLOUT | POLLWRNORM);
    if (e & FREEBSD_POLLERR)
        r |= POLLERR;
    if (e & FREEBSD_POLLHUP)
        r |= POLLHUP;
    if (e & FREEBSD_POLLNVAL)
        r |= POLLNVAL;
    if (e & FREEBSD_POLLRDNORM)
        r |= POLLRDNORM;
    if (e & FREEBSD_POLLRDBAND)
        r |= POLLRDBAND;
    if (e & FREEBSD_POLLWRBAND)
        r |= POLLWRBAND;
    if (e & FREEBSD_POLLRDHUP)
        r |= POLLRDHUP;
    return r;
}

/* Entries whose events are kept on the stack; more need an allocation */
#define POLL_STACK_EVENTS 64

int
_ppoll(struct pollfd *fds, nfds_t nfds, const struct __freebsd_timespec *kts,
       const struct __freebsd_sigset *kmask)
{
    short  stack_events[POLL_STACK_EVENTS];
    short *events = stack_events;
    void  *cancel;
    int    ret;

    /*
     * The events go to the kernel in its encoding, which can't be
     * turned back into picolibc's (POLLOUT and POLLWRNORM share a
     * bit), so the originals are kept aside
     */
    if (nfds > POLL_STACK_EVENTS) {
        events = malloc(nfds * sizeof(*events));
        if (!events) {
            errno = EAGAIN;
            return -1;
        }
    }
    for (nfds_t n = 0; n < nfds; n++) {
        events[n] = fds[n].events;
        fds[n].events = _poll_events_to_freebsd(events[n]);
    }
    cancel = _cancel_enter();
    ret = _syscall(FREEBSD_SYS_ppoll, fds, nfds, kts, kmask);
    _cancel_leave(cancel);
    for (nfds_t n = 0; n < nfds; n++) {
        fds[n].events = events[n];
        if (ret >= 0)
            fds[n].revents = _poll_events_from_freebsd(fds[n].revents, events[n]);
    }
    if (events != stack_events)
        free(events);
    return ret;
}
