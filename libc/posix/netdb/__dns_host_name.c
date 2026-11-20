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

#include "local-netdb.h"
#include <string.h>
#ifdef __PICOLIBC_PTHREADS
#include <pthread.h>
#endif

int
__dns_host_name(int family, const void *addr, char *buf, size_t len)
{
    static const char    hex[] = "0123456789abcdef";
    const unsigned char *a = addr;
    struct __dns_conf    conf;
    char                 name[sizeof("x.x.") * 32 + sizeof("ip6.arpa")];
    char                *p = name;
    int                  i, ret;
#ifdef __PICOLIBC_PTHREADS
    int cancel;
#endif

    /* The address, least significant part first, under in-addr.arpa or ip6.arpa */
    if (family == AF_INET) {
        for (i = 3; i >= 0; i--) {
            unsigned b = a[i];

            if (b >= 100)
                *p++ = (char)('0' + b / 100);
            if (b >= 10)
                *p++ = (char)('0' + b / 10 % 10);
            *p++ = (char)('0' + b % 10);
            *p++ = '.';
        }
        strcpy(p, "in-addr.arpa");
    } else {
        for (i = 15; i >= 0; i--) {
            *p++ = hex[a[i] & 0xf];
            *p++ = '.';
            *p++ = hex[a[i] >> 4];
            *p++ = '.';
        }
        strcpy(p, "ip6.arpa");
    }

    /* A lookup isn't a cancellation point, and mustn't leak its socket */
#ifdef __PICOLIBC_PTHREADS
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);
#endif
    __dns_conf(&conf);
    ret = __dns_query(&conf, name, __DNS_TYPE_PTR, NULL, 0, buf, len);
#ifdef __PICOLIBC_PTHREADS
    pthread_setcancelstate(cancel, NULL);
#endif
    if (ret > 0)
        return 0;
    return ret == 0 ? EAI_NONAME : ret;
}
