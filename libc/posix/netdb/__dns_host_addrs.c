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
#include <strings.h>
#ifdef __PICOLIBC_PTHREADS
#include <pthread.h>
#endif

/* Whether 'name' is 'domain' or within it, ignoring a final dot */
static bool
dns_in(const char *name, const char *domain)
{
    size_t len = strlen(name), dlen = strlen(domain);

    if (len && name[len - 1] == '.')
        len--;
    if (len < dlen || strncasecmp(name + len - dlen, domain, dlen) != 0)
        return false;
    return len == dlen || name[len - dlen - 1] == '.';
}

/* The A (AF_INET), AAAA (AF_INET6) or both (AF_UNSPEC) records of 'name' */
static int
dns_lookup(const struct __dns_conf *conf, const char *name, int family, struct __netdb_addr *addrs,
           int max, char *canon, size_t canonlen)
{
    int count = 0, err = 0, n;

    if (family != AF_INET6) {
        n = __dns_query(conf, name, __DNS_TYPE_A, addrs, max, canon, canonlen);
        /* A name that doesn't exist has no records of any type */
        if (n == EAI_NONAME)
            return EAI_NONAME;
        if (n < 0)
            err = n;
        else
            count = n;
    }
    if (family != AF_INET && count < max) {
        n = __dns_query(conf, name, __DNS_TYPE_AAAA, addrs + count, max - count,
                        count ? NULL : canon, canonlen);
        if (n > 0)
            count += n;
        else if (n < 0 && n != EAI_NONAME && !err)
            err = n;
    }
    if (count)
        return count;
    return err ? err : EAI_NONAME;
}

int
__dns_host_addrs(const char *name, int family, struct __netdb_addr *addrs, int max, char *canon,
                 size_t canonlen)
{
    struct __dns_conf conf;
    char              full[__DNS_NAME_MAX + 1];
    size_t            len = strlen(name);
    int               dots = 0, ret = EAI_NONAME, n, i;
    bool              absolute = len && name[len - 1] == '.';
#ifdef __PICOLIBC_PTHREADS
    int cancel;
#endif

    /* RFC 6761: .invalid names never resolve, and localhost is this machine */
    if (dns_in(name, "invalid"))
        return EAI_NONAME;
    if (dns_in(name, "localhost")) {
        n = 0;
        if (family != AF_INET6 && n < max) {
            addrs[n].family = AF_INET;
            (void)inet_pton(AF_INET, "127.0.0.1", addrs[n++].addr);
        }
        if (family != AF_INET && n < max) {
            addrs[n].family = AF_INET6;
            (void)inet_pton(AF_INET6, "::1", addrs[n++].addr);
        }
        if (canon && canonlen > strlen("localhost"))
            strcpy(canon, "localhost");
        return n;
    }

    for (i = 0; name[i]; i++)
        dots += name[i] == '.';

    /* A lookup isn't a cancellation point, and mustn't leak its socket */
#ifdef __PICOLIBC_PTHREADS
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);
#endif
    __dns_conf(&conf);

    /* A name with at least ndots dots is tried as given before the search domains, others after */
    if (!absolute && dots >= conf.ndots) {
        ret = dns_lookup(&conf, name, family, addrs, max, canon, canonlen);
        if (ret > 0)
            goto done;
    }
    for (i = 0; !absolute && i < conf.nsearch; i++) {
        if (len + 1 + strlen(conf.search[i]) > __DNS_NAME_MAX)
            continue;
        strcpy(full, name);
        strcat(full, ".");
        strcat(full, conf.search[i]);
        n = dns_lookup(&conf, full, family, addrs, max, canon, canonlen);
        if (n > 0 || ret == EAI_NONAME)
            ret = n;
        if (n > 0)
            goto done;
    }
    if (absolute || dots < conf.ndots) {
        n = dns_lookup(&conf, name, family, addrs, max, canon, canonlen);
        if (n > 0 || ret == EAI_NONAME)
            ret = n;
    }
done:
#ifdef __PICOLIBC_PTHREADS
    pthread_setcancelstate(cancel, NULL);
#endif
    return ret;
}
