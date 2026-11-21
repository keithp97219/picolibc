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

#ifndef _FREEBSD_IN_H_
#define _FREEBSD_IN_H_

#define FREEBSD_INADDR_ALLHOSTS_GROUP       3758096385L
#define FREEBSD_INADDR_ALLMDNS_GROUP        3758096635L
#define FREEBSD_INADDR_ALLRPTS_GROUP        3758096406L
#define FREEBSD_INADDR_ALLRTRS_GROUP        3758096386L
#define FREEBSD_INADDR_ANY                  0
#define FREEBSD_INADDR_BROADCAST            4294967295L
#define FREEBSD_INADDR_CARP_GROUP           3758096402L
#define FREEBSD_INADDR_LOOPBACK             2130706433
#define FREEBSD_INADDR_MAX_LOCAL_GROUP      3758096639L
#define FREEBSD_INADDR_NONE                 4294967295L
#define FREEBSD_INADDR_PFSYNC_GROUP         3758096624L
#define FREEBSD_INADDR_UNSPEC_GROUP         3758096384L
#define FREEBSD_INET6_ADDRSTRLEN            46
#define FREEBSD_INET_ADDRSTRLEN             16
#define FREEBSD_IPPROTO_3PC                 34
#define FREEBSD_IPPROTO_ADFS                68
#define FREEBSD_IPPROTO_AH                  51
#define FREEBSD_IPPROTO_AHIP                61
#define FREEBSD_IPPROTO_APES                99
#define FREEBSD_IPPROTO_ARGUS               13
#define FREEBSD_IPPROTO_AX25                93
#define FREEBSD_IPPROTO_BHA                 49
#define FREEBSD_IPPROTO_BLT                 30
#define FREEBSD_IPPROTO_BRSATMON            76
#define FREEBSD_IPPROTO_CARP                112
#define FREEBSD_IPPROTO_CFTP                62
#define FREEBSD_IPPROTO_CHAOS               16
#define FREEBSD_IPPROTO_CMTP                38
#define FREEBSD_IPPROTO_CPHB                73
#define FREEBSD_IPPROTO_CPNX                72
#define FREEBSD_IPPROTO_DCCP                33
#define FREEBSD_IPPROTO_DDP                 37
#define FREEBSD_IPPROTO_DGP                 86
#define FREEBSD_IPPROTO_DIVERT              258
#define FREEBSD_IPPROTO_DONE                257
#define FREEBSD_IPPROTO_DSTOPTS             60
#define FREEBSD_IPPROTO_EGP                 8
#define FREEBSD_IPPROTO_EMCON               14
#define FREEBSD_IPPROTO_ENCAP               98
#define FREEBSD_IPPROTO_EON                 80
#define FREEBSD_IPPROTO_ESP                 50
#define FREEBSD_IPPROTO_ETHERIP             97
#define FREEBSD_IPPROTO_FRAGMENT            44
#define FREEBSD_IPPROTO_GGP                 3
#define FREEBSD_IPPROTO_GMTP                100
#define FREEBSD_IPPROTO_GRE                 47
#define FREEBSD_IPPROTO_HELLO               63
#define FREEBSD_IPPROTO_HIP                 139
#define FREEBSD_IPPROTO_HMP                 20
#define FREEBSD_IPPROTO_HOPOPTS             0
#define FREEBSD_IPPROTO_ICMP                1
#define FREEBSD_IPPROTO_ICMPV6              58
#define FREEBSD_IPPROTO_IDP                 22
#define FREEBSD_IPPROTO_IDPR                35
#define FREEBSD_IPPROTO_IDRP                45
#define FREEBSD_IPPROTO_IGMP                2
#define FREEBSD_IPPROTO_IGP                 85
#define FREEBSD_IPPROTO_IGRP                88
#define FREEBSD_IPPROTO_IL                  40
#define FREEBSD_IPPROTO_INLSP               52
#define FREEBSD_IPPROTO_INP                 32
#define FREEBSD_IPPROTO_IP                  0
#define FREEBSD_IPPROTO_IPCOMP              108
#define FREEBSD_IPPROTO_IPCV                71
#define FREEBSD_IPPROTO_IPEIP               94
#define FREEBSD_IPPROTO_IPIP                4
#define FREEBSD_IPPROTO_IPPC                67
#define FREEBSD_IPPROTO_IPV4                4
#define FREEBSD_IPPROTO_IPV6                41
#define FREEBSD_IPPROTO_IRTP                28
#define FREEBSD_IPPROTO_KRYPTOLAN           65
#define FREEBSD_IPPROTO_LARP                91
#define FREEBSD_IPPROTO_LEAF1               25
#define FREEBSD_IPPROTO_LEAF2               26
#define FREEBSD_IPPROTO_MAX                 256
#define FREEBSD_IPPROTO_MEAS                19
#define FREEBSD_IPPROTO_MH                  135
#define FREEBSD_IPPROTO_MHRP                48
#define FREEBSD_IPPROTO_MICP                95
#define FREEBSD_IPPROTO_MOBILE              55
#define FREEBSD_IPPROTO_MPLS                137
#define FREEBSD_IPPROTO_MTP                 92
#define FREEBSD_IPPROTO_MUX                 18
#define FREEBSD_IPPROTO_ND                  77
#define FREEBSD_IPPROTO_NHRP                54
#define FREEBSD_IPPROTO_NONE                59
#define FREEBSD_IPPROTO_NSP                 31
#define FREEBSD_IPPROTO_NVPII               11
#define FREEBSD_IPPROTO_OSPFIGP             89
#define FREEBSD_IPPROTO_PFSYNC              240
#define FREEBSD_IPPROTO_PGM                 113
#define FREEBSD_IPPROTO_PIGP                9
#define FREEBSD_IPPROTO_PIM                 103
#define FREEBSD_IPPROTO_PRM                 21
#define FREEBSD_IPPROTO_PUP                 12
#define FREEBSD_IPPROTO_PVP                 75
#define FREEBSD_IPPROTO_RAW                 255
#define FREEBSD_IPPROTO_RCCMON              10
#define FREEBSD_IPPROTO_RDP                 27
#define FREEBSD_IPPROTO_ROUTING             43
#define FREEBSD_IPPROTO_RSVP                46
#define FREEBSD_IPPROTO_RVD                 66
#define FREEBSD_IPPROTO_SATEXPAK            64
#define FREEBSD_IPPROTO_SATMON              69
#define FREEBSD_IPPROTO_SCCSP               96
#define FREEBSD_IPPROTO_SCTP                132
#define FREEBSD_IPPROTO_SDRP                42
#define FREEBSD_IPPROTO_SEND                259
#define FREEBSD_IPPROTO_SHIM6               140
#define FREEBSD_IPPROTO_SKIP                57
#define FREEBSD_IPPROTO_SPACER              32767
#define FREEBSD_IPPROTO_SRPC                90
#define FREEBSD_IPPROTO_ST                  7
#define FREEBSD_IPPROTO_SVMTP               82
#define FREEBSD_IPPROTO_SWIPE               53
#define FREEBSD_IPPROTO_TCF                 87
#define FREEBSD_IPPROTO_TCP                 6
#define FREEBSD_IPPROTO_TLSP                56
#define FREEBSD_IPPROTO_TP                  29
#define FREEBSD_IPPROTO_TPXX                39
#define FREEBSD_IPPROTO_TRUNK1              23
#define FREEBSD_IPPROTO_TRUNK2              24
#define FREEBSD_IPPROTO_TTP                 84
#define FREEBSD_IPPROTO_UDP                 17
#define FREEBSD_IPPROTO_UDPLITE             136
#define FREEBSD_IPPROTO_VINES               83
#define FREEBSD_IPPROTO_VISA                70
#define FREEBSD_IPPROTO_VMTP                81
#define FREEBSD_IPPROTO_WBEXPAK             79
#define FREEBSD_IPPROTO_WBMON               78
#define FREEBSD_IPPROTO_WSN                 74
#define FREEBSD_IPPROTO_XNET                15
#define FREEBSD_IPPROTO_XTP                 36
#define FREEBSD_IPV6_AUTOFLOWLABEL          59
#define FREEBSD_IPV6_BINDANY                64
#define FREEBSD_IPV6_BINDV6ONLY             27
#define FREEBSD_IPV6_CHECKSUM               26
#define FREEBSD_IPV6_DEFAULT_MULTICAST_HOPS 1
#define FREEBSD_IPV6_DEFAULT_MULTICAST_LOOP 1
#define FREEBSD_IPV6_DONTFRAG               62
#define FREEBSD_IPV6_DSTOPTS                50
#define FREEBSD_IPV6_FLOWID                 67
#define FREEBSD_IPV6_FLOWTYPE               68
#define FREEBSD_IPV6_FW_ADD                 30
#define FREEBSD_IPV6_FW_DEL                 31
#define FREEBSD_IPV6_FW_FLUSH               32
#define FREEBSD_IPV6_FW_GET                 34
#define FREEBSD_IPV6_FW_ZERO                33
#define FREEBSD_IPV6_HOPLIMIT               47
#define FREEBSD_IPV6_HOPOPTS                49
#define FREEBSD_IPV6_IPSEC_POLICY           28
#define FREEBSD_IPV6_JOIN_GROUP             12
#define FREEBSD_IPV6_LEAVE_GROUP            13
#define FREEBSD_IPV6_MAX_GROUP_SRC_FILTER   512
#define FREEBSD_IPV6_MAX_MEMBERSHIPS        4095
#define FREEBSD_IPV6_MAX_SOCK_SRC_FILTER    128
#define FREEBSD_IPV6_MSFILTER               74
#define FREEBSD_IPV6_MULTICAST_HOPS         10
#define FREEBSD_IPV6_MULTICAST_IF           9
#define FREEBSD_IPV6_MULTICAST_LOOP         11
#define FREEBSD_IPV6_NEXTHOP                48
#define FREEBSD_IPV6_ORIGDSTADDR            72
#define FREEBSD_IPV6_PATHMTU                44
#define FREEBSD_IPV6_PKTINFO                46
#define FREEBSD_IPV6_PORTRANGE              14
#define FREEBSD_IPV6_PORTRANGE_DEFAULT      0
#define FREEBSD_IPV6_PORTRANGE_HIGH         1
#define FREEBSD_IPV6_PORTRANGE_LOW          2
#define FREEBSD_IPV6_PREFER_TEMPADDR        63
#define FREEBSD_IPV6_RECVDSTOPTS            40
#define FREEBSD_IPV6_RECVFLOWID             70
#define FREEBSD_IPV6_RECVHOPLIMIT           37
#define FREEBSD_IPV6_RECVHOPOPTS            39
#define FREEBSD_IPV6_RECVORIGDSTADDR        72
#define FREEBSD_IPV6_RECVPATHMTU            43
#define FREEBSD_IPV6_RECVPKTINFO            36
#define FREEBSD_IPV6_RECVRSSBUCKETID        71
#define FREEBSD_IPV6_RECVRTHDR              38
#define FREEBSD_IPV6_RECVTCLASS             57
#define FREEBSD_IPV6_RSSBUCKETID            69
#define FREEBSD_IPV6_RTHDR                  51
#define FREEBSD_IPV6_RTHDRDSTOPTS           35
#define FREEBSD_IPV6_RTHDR_LOOSE            0
#define FREEBSD_IPV6_RTHDR_STRICT           1
#define FREEBSD_IPV6_RTHDR_TYPE_0           0
#define FREEBSD_IPV6_SOCKOPT_RESERVED1      3
#define FREEBSD_IPV6_TCLASS                 61
#define FREEBSD_IPV6_UNICAST_HOPS           4
#define FREEBSD_IPV6_USE_MIN_MTU            42
#define FREEBSD_IPV6_V6ONLY                 27
#define FREEBSD_IPV6_VLAN_PCP               75
#define FREEBSD_IP_ADD_MEMBERSHIP           12
#define FREEBSD_IP_ADD_SOURCE_MEMBERSHIP    70
#define FREEBSD_IP_BINDANY                  24
#define FREEBSD_IP_BLOCK_SOURCE             72
#define FREEBSD_IP_DEFAULT_MULTICAST_LOOP   1
#define FREEBSD_IP_DEFAULT_MULTICAST_TTL    1
#define FREEBSD_IP_DONTFRAG                 67
#define FREEBSD_IP_DROP_MEMBERSHIP          13
#define FREEBSD_IP_DROP_SOURCE_MEMBERSHIP   71
#define FREEBSD_IP_DUMMYNET3                49
#define FREEBSD_IP_DUMMYNET_CONFIGURE       60
#define FREEBSD_IP_DUMMYNET_DEL             61
#define FREEBSD_IP_DUMMYNET_FLUSH           62
#define FREEBSD_IP_DUMMYNET_GET             64
#define FREEBSD_IP_FLOWID                   90
#define FREEBSD_IP_FLOWTYPE                 91
#define FREEBSD_IP_FW3                      48
#define FREEBSD_IP_FW_ADD                   50
#define FREEBSD_IP_FW_DEL                   51
#define FREEBSD_IP_FW_FLUSH                 52
#define FREEBSD_IP_FW_GET                   54
#define FREEBSD_IP_FW_NAT_CFG               56
#define FREEBSD_IP_FW_NAT_DEL               57
#define FREEBSD_IP_FW_NAT_GET_CONFIG        58
#define FREEBSD_IP_FW_NAT_GET_LOG           59
#define FREEBSD_IP_FW_RESETLOG              55
#define FREEBSD_IP_FW_TABLE_ADD             40
#define FREEBSD_IP_FW_TABLE_DEL             41
#define FREEBSD_IP_FW_TABLE_FLUSH           42
#define FREEBSD_IP_FW_TABLE_GETSIZE         43
#define FREEBSD_IP_FW_TABLE_LIST            44
#define FREEBSD_IP_FW_ZERO                  53
#define FREEBSD_IP_HDRINCL                  2
#define FREEBSD_IP_IPSEC_POLICY             21
#define FREEBSD_IP_MAX_GROUP_SRC_FILTER     512
#define FREEBSD_IP_MAX_MEMBERSHIPS          4095
#define FREEBSD_IP_MAX_SOCK_MUTE_FILTER     128
#define FREEBSD_IP_MAX_SOCK_SRC_FILTER      128
#define FREEBSD_IP_MINTTL                   66
#define FREEBSD_IP_MSFILTER                 74
#define FREEBSD_IP_MULTICAST_IF             9
#define FREEBSD_IP_MULTICAST_LOOP           11
#define FREEBSD_IP_MULTICAST_TTL            10
#define FREEBSD_IP_MULTICAST_VIF            14
#define FREEBSD_IP_ONESBCAST                23
#define FREEBSD_IP_OPTIONS                  1
#define FREEBSD_IP_ORIGDSTADDR              27
#define FREEBSD_IP_PORTRANGE                19
#define FREEBSD_IP_PORTRANGE_DEFAULT        0
#define FREEBSD_IP_PORTRANGE_HIGH           1
#define FREEBSD_IP_PORTRANGE_LOW            2
#define FREEBSD_IP_RECVDSTADDR              7
#define FREEBSD_IP_RECVFLOWID               93
#define FREEBSD_IP_RECVIF                   20
#define FREEBSD_IP_RECVOPTS                 5
#define FREEBSD_IP_RECVORIGDSTADDR          27
#define FREEBSD_IP_RECVRETOPTS              6
#define FREEBSD_IP_RECVRSSBUCKETID          94
#define FREEBSD_IP_RECVTOS                  68
#define FREEBSD_IP_RECVTTL                  65
#define FREEBSD_IP_RETOPTS                  8
#define FREEBSD_IP_RSSBUCKETID              92
#define FREEBSD_IP_RSVP_OFF                 16
#define FREEBSD_IP_RSVP_ON                  15
#define FREEBSD_IP_RSVP_VIF_OFF             18
#define FREEBSD_IP_RSVP_VIF_ON              17
#define FREEBSD_IP_SENDSRCADDR              7
#define FREEBSD_IP_TOS                      3
#define FREEBSD_IP_TTL                      4
#define FREEBSD_IP_UNBLOCK_SOURCE           73
#define FREEBSD_IP_VLAN_PCP                 75

#endif /* _FREEBSD_IN_H_ */
