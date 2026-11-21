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

#ifndef _FREEBSD_THR_H_
#define _FREEBSD_THR_H_

#define FREEBSD_THR_C_RUNTIME             4
#define FREEBSD_THR_SUSPENDED             1
#define FREEBSD_THR_SYSTEM_SCOPE          2
#define FREEBSD_UMTX_ABSTIME              1
#define FREEBSD_UMTX_CHECK_UNPARKING      1
#define FREEBSD_UMTX_CONTESTED            (-0x7fffffffffffffffL - 1)
#define FREEBSD_UMTX_OP_CV_BROADCAST      10
#define FREEBSD_UMTX_OP_CV_SIGNAL         9
#define FREEBSD_UMTX_OP_CV_WAIT           8
#define FREEBSD_UMTX_OP_GET_MIN_TIMEOUT   27
#define FREEBSD_UMTX_OP_LOCK              0
#define FREEBSD_UMTX_OP_MUTEX_LOCK        5
#define FREEBSD_UMTX_OP_MUTEX_TRYLOCK     4
#define FREEBSD_UMTX_OP_MUTEX_UNLOCK      6
#define FREEBSD_UMTX_OP_MUTEX_WAIT        17
#define FREEBSD_UMTX_OP_MUTEX_WAKE        18
#define FREEBSD_UMTX_OP_MUTEX_WAKE2       22
#define FREEBSD_UMTX_OP_NWAKE_PRIVATE     21
#define FREEBSD_UMTX_OP_ROBUST_LISTS      26
#define FREEBSD_UMTX_OP_RW_RDLOCK         12
#define FREEBSD_UMTX_OP_RW_UNLOCK         14
#define FREEBSD_UMTX_OP_RW_WRLOCK         13
#define FREEBSD_UMTX_OP_SEM2_WAIT         23
#define FREEBSD_UMTX_OP_SEM2_WAKE         24
#define FREEBSD_UMTX_OP_SEM_WAIT          19
#define FREEBSD_UMTX_OP_SEM_WAKE          20
#define FREEBSD_UMTX_OP_SET_CEILING       7
#define FREEBSD_UMTX_OP_SET_MIN_TIMEOUT   28
#define FREEBSD_UMTX_OP_SHM               25
#define FREEBSD_UMTX_OP_UNLOCK            1
#define FREEBSD_UMTX_OP_WAIT              2
#define FREEBSD_UMTX_OP_WAIT_UINT         11
#define FREEBSD_UMTX_OP_WAIT_UINT_PRIVATE 15
#define FREEBSD_UMTX_OP_WAKE              3
#define FREEBSD_UMTX_OP_WAKE_PRIVATE      16
#define FREEBSD_UMTX_OP__32BIT            2147483648L
#define FREEBSD_UMTX_OP__I386             1073741824
#define FREEBSD_UMTX_SHM_ALIVE            8
#define FREEBSD_UMTX_SHM_CREAT            1
#define FREEBSD_UMTX_SHM_DESTROY          4
#define FREEBSD_UMTX_SHM_LOOKUP           2
#define FREEBSD_UMTX_UNOWNED              0

#endif /* _FREEBSD_THR_H_ */
