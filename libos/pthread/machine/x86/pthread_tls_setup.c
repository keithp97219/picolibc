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

#define _DEFAULT_SOURCE
#include "../../local-pthread.h"
#include <machine/_tls.h>
#include <string.h>

extern char __x86_tls_tcb_offset[];

static inline __tcb_head_t *
current_tcb(void)
{
    __tcb_head_t *tcb;
#ifdef __x86_64
    __asm__("movq %%fs:0, %0" : "=r"(tcb));
#else
    __asm__("movl %%gs:0, %0" : "=r"(tcb));
#endif
    return tcb;
}

void *
_pthread_tls_setup(void *tls, void *arg_space)
{
    __tcb_head_t *tcb = (__tcb_head_t *)((char *)tls + (uintptr_t)__x86_tls_tcb_offset);

    /* Copy the creator's TCB to share the stack guard, then point it at itself */
    memcpy(tcb, current_tcb(), sizeof(*tcb));
    tcb->__tcb = tcb;
#ifdef __THREAD_LOCAL_STORAGE_STACK_GUARD
    tcb->__self = tcb;
#endif

#ifdef __x86_64
    /* x86_64 takes the %fs base address */
    (void)arg_space;
    return tcb;
#else
    /*
     * i386 takes a struct user_desc for set_thread_area. Reuse the
     * creator's GDT entry, found from %gs, so each thread gets the
     * same selector with its own base.
     */
    unsigned int  gs;
    unsigned int *desc = arg_space;

    __asm__("movl %%gs, %0" : "=r"(gs));
    desc[0] = gs >> 3;        /* entry_number */
    desc[1] = (uintptr_t)tcb; /* base_addr */
    desc[2] = 0xfffff;        /* limit */
    desc[3] = 0x51;           /* seg_32bit, limit_in_pages, useable */
    return desc;
#endif
}
