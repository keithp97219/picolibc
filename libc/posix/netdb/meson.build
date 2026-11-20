#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2024 Keith Packard
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

# <netdb.h>, which needs the network stack

dir_srcs = []
if network != 'none'
  dir_srcs += [
    '__hosts_db.c',
    '__netdb_db.c',
    '__netdb_lookup.c',
    '__networks_db.c',
    '__protocols_db.c',
    '__services_db.c',
    'endhostent.c',
    'endnetent.c',
    'endprotoent.c',
    'endservent.c',
    'freeaddrinfo.c',
    'gai_strerror.c',
    'getaddrinfo.c',
    'gethostent.c',
    'getnameinfo.c',
    'getnetbyaddr.c',
    'getnetbyname.c',
    'getnetent.c',
    'getprotobyname.c',
    'getprotobynumber.c',
    'getprotoent.c',
    'getservbyname.c',
    'getservbyport.c',
    'getservent.c',
    'sethostent.c',
    'setnetent.c',
    'setprotoent.c',
    'setservent.c',
  ]
  if network_dns
    dir_srcs += [
      '__dns_conf.c',
      '__dns_host_addrs.c',
      '__dns_host_name.c',
      '__dns_query.c',
      '__dns_resolv_conf.c',
    ]
  endif
endif
