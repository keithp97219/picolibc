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

#ifndef _FREEBSD_SOCKET_H_
#define _FREEBSD_SOCKET_H_

#define FREEBSD_AF_APPLETALK         16
#define FREEBSD_AF_ARP               35
#define FREEBSD_AF_ATM               30
#define FREEBSD_AF_BLUETOOTH         36
#define FREEBSD_AF_CCITT             10
#define FREEBSD_AF_CHAOS             5
#define FREEBSD_AF_CNT               21
#define FREEBSD_AF_COIP              20
#define FREEBSD_AF_DATAKIT           9
#define FREEBSD_AF_DIVERT            44
#define FREEBSD_AF_DLI               13
#define FREEBSD_AF_E164              26
#define FREEBSD_AF_ECMA              8
#define FREEBSD_AF_HYLINK            15
#define FREEBSD_AF_HYPERV            43
#define FREEBSD_AF_IEEE80211         37
#define FREEBSD_AF_IMPLINK           3
#define FREEBSD_AF_INET              2
#define FREEBSD_AF_INET6             28
#define FREEBSD_AF_INET6_SDP         42
#define FREEBSD_AF_INET_SDP          40
#define FREEBSD_AF_IPFWLOG           46
#define FREEBSD_AF_IPX               23
#define FREEBSD_AF_ISDN              26
#define FREEBSD_AF_ISO               7
#define FREEBSD_AF_LAT               14
#define FREEBSD_AF_LINK              18
#define FREEBSD_AF_LOCAL             1
#define FREEBSD_AF_MAX               46
#define FREEBSD_AF_NATM              29
#define FREEBSD_AF_NETBIOS           6
#define FREEBSD_AF_NETGRAPH          32
#define FREEBSD_AF_NETLINK           38
#define FREEBSD_AF_OSI               7
#define FREEBSD_AF_PUP               4
#define FREEBSD_AF_ROUTE             17
#define FREEBSD_AF_SCLUSTER          34
#define FREEBSD_AF_SIP               24
#define FREEBSD_AF_SLOW              33
#define FREEBSD_AF_SNA               11
#define FREEBSD_AF_UNIX              1
#define FREEBSD_AF_UNSPEC            0
#define FREEBSD_AF_VENDOR00          39
#define FREEBSD_AF_VENDOR01          41
#define FREEBSD_AF_VENDOR03          45
#define FREEBSD_AF_VENDOR04          47
#define FREEBSD_AF_VENDOR05          49
#define FREEBSD_AF_VENDOR06          51
#define FREEBSD_AF_VENDOR07          53
#define FREEBSD_AF_VENDOR08          55
#define FREEBSD_AF_VENDOR09          57
#define FREEBSD_AF_VENDOR10          59
#define FREEBSD_AF_VENDOR11          61
#define FREEBSD_AF_VENDOR12          63
#define FREEBSD_AF_VENDOR13          65
#define FREEBSD_AF_VENDOR14          67
#define FREEBSD_AF_VENDOR15          69
#define FREEBSD_AF_VENDOR16          71
#define FREEBSD_AF_VENDOR17          73
#define FREEBSD_AF_VENDOR18          75
#define FREEBSD_AF_VENDOR19          77
#define FREEBSD_AF_VENDOR20          79
#define FREEBSD_AF_VENDOR21          81
#define FREEBSD_AF_VENDOR22          83
#define FREEBSD_AF_VENDOR23          85
#define FREEBSD_AF_VENDOR24          87
#define FREEBSD_AF_VENDOR25          89
#define FREEBSD_AF_VENDOR26          91
#define FREEBSD_AF_VENDOR27          93
#define FREEBSD_AF_VENDOR28          95
#define FREEBSD_AF_VENDOR29          97
#define FREEBSD_AF_VENDOR30          99
#define FREEBSD_AF_VENDOR31          101
#define FREEBSD_AF_VENDOR32          103
#define FREEBSD_AF_VENDOR33          105
#define FREEBSD_AF_VENDOR34          107
#define FREEBSD_AF_VENDOR35          109
#define FREEBSD_AF_VENDOR36          111
#define FREEBSD_AF_VENDOR37          113
#define FREEBSD_AF_VENDOR38          115
#define FREEBSD_AF_VENDOR39          117
#define FREEBSD_AF_VENDOR40          119
#define FREEBSD_AF_VENDOR41          121
#define FREEBSD_AF_VENDOR42          123
#define FREEBSD_AF_VENDOR43          125
#define FREEBSD_AF_VENDOR44          127
#define FREEBSD_AF_VENDOR45          129
#define FREEBSD_AF_VENDOR46          131
#define FREEBSD_AF_VENDOR47          133
#define FREEBSD_PF_APPLETALK         16
#define FREEBSD_PF_ARP               35
#define FREEBSD_PF_ATM               30
#define FREEBSD_PF_BLUETOOTH         36
#define FREEBSD_PF_CCITT             10
#define FREEBSD_PF_CHAOS             5
#define FREEBSD_PF_CNT               21
#define FREEBSD_PF_COIP              20
#define FREEBSD_PF_DATAKIT           9
#define FREEBSD_PF_DIVERT            44
#define FREEBSD_PF_DLI               13
#define FREEBSD_PF_ECMA              8
#define FREEBSD_PF_HYLINK            15
#define FREEBSD_PF_IEEE80211         37
#define FREEBSD_PF_IMPLINK           3
#define FREEBSD_PF_INET              2
#define FREEBSD_PF_INET6             28
#define FREEBSD_PF_INET6_SDP         42
#define FREEBSD_PF_INET_SDP          40
#define FREEBSD_PF_IPFWLOG           46
#define FREEBSD_PF_IPX               23
#define FREEBSD_PF_ISDN              26
#define FREEBSD_PF_ISO               7
#define FREEBSD_PF_KEY               27
#define FREEBSD_PF_LAT               14
#define FREEBSD_PF_LINK              18
#define FREEBSD_PF_LOCAL             1
#define FREEBSD_PF_MAX               46
#define FREEBSD_PF_NATM              29
#define FREEBSD_PF_NETBIOS           6
#define FREEBSD_PF_NETGRAPH          32
#define FREEBSD_PF_NETLINK           38
#define FREEBSD_PF_OSI               7
#define FREEBSD_PF_PIP               25
#define FREEBSD_PF_PUP               4
#define FREEBSD_PF_ROUTE             17
#define FREEBSD_PF_RTIP              22
#define FREEBSD_PF_SCLUSTER          34
#define FREEBSD_PF_SIP               24
#define FREEBSD_PF_SLOW              33
#define FREEBSD_PF_SNA               11
#define FREEBSD_PF_UNIX              1
#define FREEBSD_PF_UNSPEC            0
#define FREEBSD_PF_XTP               19
#define FREEBSD_SCM_BINTIME          4
#define FREEBSD_SCM_CREDS            3
#define FREEBSD_SCM_MONOTONIC        6
#define FREEBSD_SCM_REALTIME         5
#define FREEBSD_SCM_RIGHTS           1
#define FREEBSD_SCM_TIMESTAMP        2
#define FREEBSD_SCM_TIME_INFO        7
#define FREEBSD_SHUT_RD              0
#define FREEBSD_SHUT_RDWR            2
#define FREEBSD_SHUT_WR              1
#define FREEBSD_SOCK_DGRAM           2
#define FREEBSD_SOCK_MAXADDRLEN      255
#define FREEBSD_SOCK_RAW             3
#define FREEBSD_SOCK_RDM             4
#define FREEBSD_SOCK_SEQPACKET       5
#define FREEBSD_SOCK_STREAM          1
#define FREEBSD_SOL_SOCKET           65535
#define FREEBSD_SOMAXCONN            128
#define FREEBSD_SO_ACCEPTCONN        2
#define FREEBSD_SO_ACCEPTFILTER      4096
#define FREEBSD_SO_BINTIME           8192
#define FREEBSD_SO_BROADCAST         32
#define FREEBSD_SO_DEBUG             1
#define FREEBSD_SO_DOMAIN            4121
#define FREEBSD_SO_DONTROUTE         16
#define FREEBSD_SO_ERROR             4103
#define FREEBSD_SO_FIB               4116
#define FREEBSD_SO_KEEPALIVE         8
#define FREEBSD_SO_LABEL             4105
#define FREEBSD_SO_LINGER            128
#define FREEBSD_SO_LISTENINCQLEN     4115
#define FREEBSD_SO_LISTENQLEN        4114
#define FREEBSD_SO_LISTENQLIMIT      4113
#define FREEBSD_SO_MAX_PACING_RATE   4120
#define FREEBSD_SO_NOSIGPIPE         2048
#define FREEBSD_SO_NO_DDP            32768
#define FREEBSD_SO_NO_OFFLOAD        16384
#define FREEBSD_SO_OOBINLINE         256
#define FREEBSD_SO_PEERLABEL         4112
#define FREEBSD_SO_PROTOCOL          4118
#define FREEBSD_SO_PROTOTYPE         4118
#define FREEBSD_SO_RCVBUF            4098
#define FREEBSD_SO_RCVLOWAT          4100
#define FREEBSD_SO_RCVTIMEO          4102
#define FREEBSD_SO_RERROR            131072
#define FREEBSD_SO_REUSEADDR         4
#define FREEBSD_SO_REUSEPORT         512
#define FREEBSD_SO_REUSEPORT_LB      65536
#define FREEBSD_SO_SETFIB            4116
#define FREEBSD_SO_SNDBUF            4097
#define FREEBSD_SO_SNDLOWAT          4099
#define FREEBSD_SO_SNDTIMEO          4101
#define FREEBSD_SO_SPLICE            4131
#define FREEBSD_SO_TIMESTAMP         1024
#define FREEBSD_SO_TS_BINTIME        1
#define FREEBSD_SO_TS_CLOCK          4119
#define FREEBSD_SO_TS_CLOCK_MAX      3
#define FREEBSD_SO_TS_DEFAULT        0
#define FREEBSD_SO_TS_MONOTONIC      3
#define FREEBSD_SO_TS_REALTIME       2
#define FREEBSD_SO_TS_REALTIME_MICRO 0
#define FREEBSD_SO_TYPE              4104
#define FREEBSD_SO_USELOOPBACK       64
#define FREEBSD_SO_USER_COOKIE       4117
#define FREEBSD_SO_VENDOR            2147483648L

#endif /* _FREEBSD_SOCKET_H_ */
