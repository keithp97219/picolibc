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
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DNS_HEADER     12
#define DNS_QUERY_MAX  (DNS_HEADER + __DNS_NAME_MAX + 2 + 4)
#define DNS_REPLY_MAX  1500
#define DNS_CLASS_IN   1
#define DNS_FLAG_QR    0x8000
#define DNS_FLAG_TC    0x0200
#define DNS_FLAG_RD    0x0100
#define DNS_RCODE_MASK 0x000f
#define DNS_JUMPS_MAX  64 /* compression pointers followed in one name */
#define DNS_CNAMES_MAX 8  /* links followed in one answer */

static unsigned
get16(const unsigned char *p)
{
    return ((unsigned)p[0] << 8) | p[1];
}

static void
put16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)(v >> 8);
    p[1] = (unsigned char)v;
}

/* Builds a query; returns its length, or 0 if the name can't be encoded */
static size_t
dns_build(unsigned char *q, unsigned id, const char *name, int type)
{
    unsigned char *p = q + DNS_HEADER;
    size_t         len = strlen(name);

    if (len && name[len - 1] == '.')
        len--;
    if (len == 0 || len > __DNS_NAME_MAX - 1)
        return 0;
    memset(q, 0, DNS_HEADER);
    put16(q, id);
    put16(q + 2, DNS_FLAG_RD);
    put16(q + 4, 1);
    while (len) {
        const char *dot = memchr(name, '.', len);
        size_t      label = dot ? (size_t)(dot - name) : len;

        if (label == 0 || label > 63)
            return 0;
        *p++ = (unsigned char)label;
        memcpy(p, name, label);
        p += label;
        name += label;
        len -= label;
        if (dot) {
            name++;
            len--;
        }
    }
    *p++ = 0;
    put16(p, (unsigned)type);
    put16(p + 2, DNS_CLASS_IN);
    return (size_t)(p + 4 - q);
}

/*
 * Reads the (possibly compressed) name at *pos into 'out' in dotted
 * form, moving *pos past it; returns false if it is malformed.
 */
static bool
dns_name(const unsigned char *msg, size_t len, size_t *pos, char *out, size_t outlen)
{
    size_t p = *pos, o = 0;
    int    jumps = 0;
    bool   jumped = false;

    for (;;) {
        unsigned l;

        if (p >= len)
            return false;
        l = msg[p];
        if ((l & 0xc0) == 0xc0) {
            if (p + 1 >= len || ++jumps > DNS_JUMPS_MAX)
                return false;
            if (!jumped)
                *pos = p + 2;
            jumped = true;
            p = ((l & 0x3f) << 8) | msg[p + 1];
            continue;
        }
        if (l & 0xc0)
            return false;
        p++;
        if (l == 0)
            break;
        if (p + l > len || o + l + 1 >= outlen)
            return false;
        if (o)
            out[o++] = '.';
        memcpy(out + o, msg + p, l);
        o += l;
        p += l;
    }
    out[o] = '\0';
    if (!jumped)
        *pos = p;
    return true;
}

static int
dns_now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* Whether the reply answers the query: the same id and question, from a server */
static bool
dns_matches(const unsigned char *q, size_t qlen, const unsigned char *r, size_t rlen)
{
    size_t i;

    if (rlen < qlen || get16(r) != get16(q) || !(get16(r + 2) & DNS_FLAG_QR) || get16(r + 4) != 1)
        return false;
    for (i = DNS_HEADER; i < qlen; i++)
        if (tolower(r[i]) != tolower(q[i]))
            return false;
    return true;
}

/*
 * Sends the query to one server and waits for its answer, ignoring
 * anything that doesn't match; returns the reply length, or 0 on
 * timeout or error.
 */
static size_t
dns_exchange(const struct __dns_server *srv, int timeout, const unsigned char *q, size_t qlen,
             unsigned char *r)
{
    struct sockaddr_in6 sa6;
    struct sockaddr_in  sa4;
    struct sockaddr    *sa;
    socklen_t           salen;
    size_t              ret = 0;
    int                 fd, deadline;

    if (srv->family == AF_INET) {
        memset(&sa4, 0, sizeof(sa4));
        sa4.sin_family = AF_INET;
        sa4.sin_port = htons(srv->port);
        memcpy(&sa4.sin_addr, srv->addr, 4);
        sa = (struct sockaddr *)&sa4;
        salen = sizeof(sa4);
    } else {
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = htons(srv->port);
        memcpy(&sa6.sin6_addr, srv->addr, 16);
        sa = (struct sockaddr *)&sa6;
        salen = sizeof(sa6);
    }

    /* Connecting means only the server's own replies arrive */
    fd = socket(srv->family, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return 0;
    if (connect(fd, sa, salen) < 0 || send(fd, q, qlen, 0) != (ssize_t)qlen)
        goto done;

    deadline = dns_now_ms() + timeout * 1000;
    for (;;) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN };
        int           left = deadline - dns_now_ms();
        ssize_t       n;

        if (left <= 0)
            break;
        n = poll(&pfd, 1, left);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        n = recv(fd, r, DNS_REPLY_MAX, 0);
        if (n < 0) {
            /* Nothing listens there */
            if (errno == ECONNREFUSED)
                break;
            continue;
        }
        if (dns_matches(q, qlen, r, (size_t)n)) {
            ret = (size_t)n;
            break;
        }
    }
done:
    close(fd);
    return ret;
}

/* Collects the answers to the question, following CNAME records */
static int
dns_answers(const unsigned char *r, size_t rlen, size_t qlen, const char *name, int type,
            struct __netdb_addr *addrs, int max, char *out, size_t outlen)
{
    char   target[__DNS_NAME_MAX + 1], owner[__DNS_NAME_MAX + 1], data[__DNS_NAME_MAX + 1];
    int    count = 0, links = 0;
    size_t len = strlen(name);
    bool   changed = true;

    if (len > __DNS_NAME_MAX)
        return EAI_FAIL;
    strcpy(target, name);
    if (len && target[len - 1] == '.')
        target[len - 1] = '\0';

    /* Records may come in any order, so look again after each CNAME */
    while (changed && count == 0) {
        unsigned n = get16(r + 6);
        size_t   pos = qlen;

        changed = false;
        while (n--) {
            unsigned rtype, rdlen;
            size_t   rdata;

            if (!dns_name(r, rlen, &pos, owner, sizeof(owner)) || pos + 10 > rlen)
                return EAI_FAIL;
            rtype = get16(r + pos);
            rdlen = get16(r + pos + 8);
            rdata = pos + 10;
            pos = rdata + rdlen;
            if (pos > rlen)
                return EAI_FAIL;
            if (get16(r + rdata - 8) != DNS_CLASS_IN || strcasecmp(owner, target) != 0)
                continue;
            if (rtype == __DNS_TYPE_CNAME && type != __DNS_TYPE_CNAME) {
                size_t p = rdata;

                if (++links > DNS_CNAMES_MAX || !dns_name(r, rlen, &p, data, sizeof(data)))
                    return EAI_FAIL;
                strcpy(target, data);
                changed = true;
            } else if (rtype == (unsigned)type && type == __DNS_TYPE_PTR) {
                size_t p = rdata;

                if (!dns_name(r, rlen, &p, data, sizeof(data)))
                    return EAI_FAIL;
                if (strlen(data) >= outlen)
                    return EAI_OVERFLOW;
                strcpy(out, data);
                return 1;
            } else if (rtype == (unsigned)type && count < max) {
                int    family = type == __DNS_TYPE_A ? AF_INET : AF_INET6;
                size_t alen = family == AF_INET ? 4 : 16;

                if (rdlen != alen)
                    return EAI_FAIL;
                addrs[count].family = family;
                memcpy(addrs[count].addr, r + rdata, alen);
                count++;
            }
        }
    }
    if (count && out) {
        if (strlen(target) >= outlen)
            return EAI_OVERFLOW;
        strcpy(out, target);
    }
    return count;
}

int
__dns_query(const struct __dns_conf *conf, const char *name, int type, struct __netdb_addr *addrs,
            int max, char *out, size_t outlen)
{
    unsigned char  q[DNS_QUERY_MAX];
    unsigned char *r;
    size_t         qlen;
    int            attempt, s, ret = EAI_AGAIN;

    qlen = dns_build(q, arc4random() & 0xffff, name, type);
    if (!qlen)
        return EAI_NONAME;
    r = malloc(DNS_REPLY_MAX);
    if (!r)
        return EAI_MEMORY;

    for (attempt = 0; attempt < conf->attempts; attempt++) {
        for (s = 0; s < conf->nservers; s++) {
            size_t rlen = dns_exchange(&conf->servers[s], conf->timeout, q, qlen, r);

            if (!rlen)
                continue;
            switch (get16(r + 2) & DNS_RCODE_MASK) {
            case 0:
                /* A truncated reply's answers are still usable */
                ret = dns_answers(r, rlen, qlen, name, type, addrs, max, out, outlen);
                goto done;
            case 3: /* the name doesn't exist */
                ret = EAI_NONAME;
                goto done;
            case 2: /* the server failed; another may not */
                ret = EAI_AGAIN;
                break;
            default: /* refused or not understood */
                ret = EAI_FAIL;
                break;
            }
        }
    }
done:
    free(r);
    return ret;
}
