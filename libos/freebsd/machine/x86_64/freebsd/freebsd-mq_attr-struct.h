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

#ifndef _FREEBSD_MQ_ATTR_STRUCT_H_
#define _FREEBSD_MQ_ATTR_STRUCT_H_

struct __freebsd_mq_attr {
    long      mq_flags;
    long      mq_maxmsg;
    long      mq_msgsize;
    long      mq_curmsgs;
    __uint8_t __adjust_32[32];
};

_Static_assert(sizeof(struct __freebsd_mq_attr) == 64, "struct mq_attr size");
_Static_assert(__builtin_offsetof(struct __freebsd_mq_attr, mq_flags) == 0,
               "struct mq_attr.mq_flags");
_Static_assert(__builtin_offsetof(struct __freebsd_mq_attr, mq_maxmsg) == 8,
               "struct mq_attr.mq_maxmsg");
_Static_assert(__builtin_offsetof(struct __freebsd_mq_attr, mq_msgsize) == 16,
               "struct mq_attr.mq_msgsize");
_Static_assert(__builtin_offsetof(struct __freebsd_mq_attr, mq_curmsgs) == 24,
               "struct mq_attr.mq_curmsgs");

#endif /* _FREEBSD_MQ_ATTR_STRUCT_H_ */
