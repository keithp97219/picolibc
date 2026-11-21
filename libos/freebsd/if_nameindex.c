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

#include "local-socket.h"
#include <stdlib.h>
#include <string.h>

/*
 * SIOCGIFCONF fills a buffer with an entry per address, each the
 * interface name followed by the address, at least a struct sockaddr
 * long; every interface has at least its link-level address
 */
static char *
_if_conf(long fd, int *len)
{
    struct __freebsd_ifconf ifc;
    size_t                  size = 4096;
    char                   *buf = NULL;

    for (;;) {
        char *nbuf = realloc(buf, size);

        if (!nbuf)
            break;
        buf = nbuf;
        ifc.ifc_len = (int)size;
        ifc.ifc_buf = buf;
        if (_syscall(FREEBSD_SYS_ioctl, fd, FREEBSD_SIOCGIFCONF, &ifc) < 0)
            break;
        /* A full buffer may have been cut short */
        if ((size_t)ifc.ifc_len + 2 * sizeof(struct __freebsd_ifreq) + 256 < size) {
            *len = ifc.ifc_len;
            return buf;
        }
        if (size > 16 * 1024 * 1024)
            break;
        size *= 2;
    }
    free(buf);
    return NULL;
}

/* The names are stored after the array, so if_freenameindex frees a single block */
struct if_nameindex *
if_nameindex(void)
{
    struct if_nameindex *list = NULL;
    long                 fd;
    char                *conf, *names;
    int                  len = 0, off;
    size_t               count = 0, i;

    fd = _syscall(FREEBSD_SYS_socket, AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return NULL;
    conf = _if_conf(fd, &len);
    if (!conf)
        goto fail;

    /* Each name appears once per address; an array that large is enough */
    list = malloc((size_t)len / IF_NAMESIZE * (sizeof(*list) + IF_NAMESIZE) + sizeof(*list));
    if (!list)
        goto fail;
    names = (char *)list + (size_t)len / IF_NAMESIZE * sizeof(*list) + sizeof(*list);

    for (off = 0; off + (int)sizeof(struct __freebsd_ifreq) <= len;) {
        struct __freebsd_ifreq    ifr;
        struct __freebsd_sockaddr sa;
        size_t                    salen;

        memcpy(&ifr, conf + off, sizeof(ifr));
        memcpy(&sa, conf + off + IF_NAMESIZE, sizeof(sa));
        salen = sa.sa_len > sizeof(sa) ? sa.sa_len : sizeof(sa);
        off += IF_NAMESIZE + (int)salen;

        ifr.ifr_name[IF_NAMESIZE - 1] = '\0';
        for (i = 0; i < count; i++)
            if (!strcmp(list[i].if_name, ifr.ifr_name))
                break;
        if (i < count)
            continue;
        if (_syscall(FREEBSD_SYS_ioctl, fd, FREEBSD_SIOCGIFINDEX, &ifr) < 0)
            continue;
        list[count].if_index = (unsigned short)ifr.ifr_index;
        list[count].if_name = names + count * IF_NAMESIZE;
        strcpy(list[count].if_name, ifr.ifr_name);
        count++;
    }
    list[count].if_index = 0;
    list[count].if_name = NULL;
    free(conf);
    (void)_syscall_raw(FREEBSD_SYS_close, fd);
    return list;

fail:
    free(conf);
    (void)_syscall_raw(FREEBSD_SYS_close, fd);
    errno = ENOBUFS;
    return NULL;
}
