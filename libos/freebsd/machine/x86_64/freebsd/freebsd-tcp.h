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

#ifndef _FREEBSD_TCP_H_
#define _FREEBSD_TCP_H_

#define FREEBSD_TCP_BBR_ALGORITHM           1083
#define FREEBSD_TCP_BBR_DRAIN_PG            1070
#define FREEBSD_TCP_BBR_EXTRA_STATE         1107
#define FREEBSD_TCP_BBR_FLOOR_MIN_TSO       1108
#define FREEBSD_TCP_BBR_HDWR_PACE           1105
#define FREEBSD_TCP_BBR_IWINTSO             1067
#define FREEBSD_TCP_BBR_MAX_RTO             1081
#define FREEBSD_TCP_BBR_MIN_RTO             1080
#define FREEBSD_TCP_BBR_MIN_TOPACEOUT       1109
#define FREEBSD_TCP_BBR_PACE_CROSS          1090
#define FREEBSD_TCP_BBR_PACE_DEL_TAR        1087
#define FREEBSD_TCP_BBR_PACE_OH             1077
#define FREEBSD_TCP_BBR_PACE_PER_SEC        1086
#define FREEBSD_TCP_BBR_PACE_SEG_MAX        1088
#define FREEBSD_TCP_BBR_PACE_SEG_MIN        1089
#define FREEBSD_TCP_BBR_POLICER_DETECT      1111
#define FREEBSD_TCP_BBR_PROBE_RTT_GAIN      1101
#define FREEBSD_TCP_BBR_PROBE_RTT_INT       1072
#define FREEBSD_TCP_BBR_PROBE_RTT_LEN       1102
#define FREEBSD_TCP_BBR_RACK_INIT_RATE      1112
#define FREEBSD_TCP_BBR_RACK_RTT_USE        1098
#define FREEBSD_TCP_BBR_RETRAN_WTSO         1099
#define FREEBSD_TCP_BBR_SEND_IWND_IN_TSO    1103
#define FREEBSD_TCP_BBR_STARTUP_LOSS_EXIT   1074
#define FREEBSD_TCP_BBR_STARTUP_PG          1069
#define FREEBSD_TCP_BBR_TMR_PACE_OH         1096
#define FREEBSD_TCP_BBR_TSLIMITS            1076
#define FREEBSD_TCP_BBR_TSTMP_RAISES        1110
#define FREEBSD_TCP_BBR_USEDEL_RATE         1079
#define FREEBSD_TCP_BBR_USE_RACK_CHEAT      1104
#define FREEBSD_TCP_BBR_USE_RACK_RR         1104
#define FREEBSD_TCP_BBR_UTTER_MAX_TSO       1106
#define FREEBSD_TCP_CA_NAME_MAX             16
#define FREEBSD_TCP_CCALGOOPT               65
#define FREEBSD_TCP_CONGESTION              64
#define FREEBSD_TCP_DATA_AFTER_CLOSE        1100
#define FREEBSD_TCP_DEFER_OPTIONS           1136
#define FREEBSD_TCP_DELACK                  72
#define FREEBSD_TCP_DGP_UPPER_BOUNDS        1153
#define FREEBSD_TCP_FASTOPEN                1025
#define FREEBSD_TCP_FASTOPEN_MAX_COOKIE_LEN 16
#define FREEBSD_TCP_FASTOPEN_MIN_COOKIE_LEN 4
#define FREEBSD_TCP_FASTOPEN_PSK_LEN        16
#define FREEBSD_TCP_FILLCW_RATE_CAP         1158
#define FREEBSD_TCP_FIN_IS_RST              73
#define FREEBSD_TCP_FUNCTION_ALIAS          8193
#define FREEBSD_TCP_FUNCTION_BLK            8192
#define FREEBSD_TCP_FUNCTION_NAME_LEN_MAX   32
#define FREEBSD_TCP_GP_USE_LTBW             1162
#define FREEBSD_TCP_HAS_PLAYOUT_MS          64
#define FREEBSD_TCP_HDWR_RATE_CAP           1130
#define FREEBSD_TCP_HDWR_UP_ONLY            1132
#define FREEBSD_TCP_HONOR_HPTS_MIN          1155
#define FREEBSD_TCP_HYBRID_PACING           1150
#define FREEBSD_TCP_HYBRID_PACING_CSPR      4
#define FREEBSD_TCP_HYBRID_PACING_CU        1
#define FREEBSD_TCP_HYBRID_PACING_DTL       2
#define FREEBSD_TCP_HYBRID_PACING_ENABLE    16
#define FREEBSD_TCP_HYBRID_PACING_H_MS      8
#define FREEBSD_TCP_HYBRID_PACING_SENDTIME  16384
#define FREEBSD_TCP_HYBRID_PACING_SETMSS    4096
#define FREEBSD_TCP_HYBRID_PACING_S_MSS     32
#define FREEBSD_TCP_HYBRID_PACING_USER_MASK 4095
#define FREEBSD_TCP_HYBRID_PACING_WASSET    8192
#define FREEBSD_TCP_IDLE_REDUCE             70
#define FREEBSD_TCP_INFO                    32
#define FREEBSD_TCP_IWND_NB                 43
#define FREEBSD_TCP_IWND_NSEG               44
#define FREEBSD_TCP_KEEPCNT                 1024
#define FREEBSD_TCP_KEEPIDLE                256
#define FREEBSD_TCP_KEEPINIT                128
#define FREEBSD_TCP_KEEPINTVL               512
#define FREEBSD_TCP_LOG                     34
#define FREEBSD_TCP_LOGBUF                  35
#define FREEBSD_TCP_LOGDUMP                 37
#define FREEBSD_TCP_LOGDUMPID               38
#define FREEBSD_TCP_LOGID                   36
#define FREEBSD_TCP_LOGID_CNT               46
#define FREEBSD_TCP_LOG_HTTPD_RANGE_END     2
#define FREEBSD_TCP_LOG_HTTPD_RANGE_START   1
#define FREEBSD_TCP_LOG_HTTPD_TS            1
#define FREEBSD_TCP_LOG_HTTPD_TS_REQ        2
#define FREEBSD_TCP_LOG_ID_LEN              64
#define FREEBSD_TCP_LOG_LIMIT               74
#define FREEBSD_TCP_LOG_TAG                 47
#define FREEBSD_TCP_LOG_USER_HTTPD          1
#define FREEBSD_TCP_MAXHLEN                 60
#define FREEBSD_TCP_MAXOLEN                 40
#define FREEBSD_TCP_MAXSEG                  2
#define FREEBSD_TCP_MAXUNACKTIME            68
#define FREEBSD_TCP_MAXWIN                  65535
#define FREEBSD_TCP_MAX_SACK                4
#define FREEBSD_TCP_MAX_WINSHIFT            14
#define FREEBSD_TCP_MINMSS                  216
#define FREEBSD_TCP_MSS                     536
#define FREEBSD_TCP_NODELAY                 1
#define FREEBSD_TCP_NOOPT                   8
#define FREEBSD_TCP_NOPUSH                  4
#define FREEBSD_TCP_NO_PRR                  1122
#define FREEBSD_TCP_NO_TIMELY               1154
#define FREEBSD_TCP_NUM_CNT_COUNTERS        13
#define FREEBSD_TCP_NUM_PROC_COUNTERS       11
#define FREEBSD_TCP_PACING_DND              1151
#define FREEBSD_TCP_PACING_RATE_CAP         1131
#define FREEBSD_TCP_PERF_INFO               78
#define FREEBSD_TCP_PROC_ACCOUNTING         76
#define FREEBSD_TCP_RACK_ABC_VAL            1133
#define FREEBSD_TCP_RACK_DGP_IN_REC         1148
#define FREEBSD_TCP_RACK_DO_DETECTION       1097
#define FREEBSD_TCP_RACK_DSACK_OPT          1141
#define FREEBSD_TCP_RACK_EARLY_SEG          1060
#define FREEBSD_TCP_RACK_ENABLE_HYSTART     1142
#define FREEBSD_TCP_RACK_FORCE_MSEG         1117
#define FREEBSD_TCP_RACK_GP_INCREASE_CA     1114
#define FREEBSD_TCP_RACK_GP_INCREASE_REC    1116
#define FREEBSD_TCP_RACK_GP_INCREASE_SS     1115
#define FREEBSD_TCP_RACK_HI_BETA            1144
#define FREEBSD_TCP_RACK_MBUF_QUEUE         1050
#define FREEBSD_TCP_RACK_MEASURE_CNT        1135
#define FREEBSD_TCP_RACK_MIN_TO             1058
#define FREEBSD_TCP_RACK_NONRXT_CFG_RATE    1123
#define FREEBSD_TCP_RACK_NO_PUSH_AT_MAX     1126
#define FREEBSD_TCP_RACK_PACE_ALWAYS        1055
#define FREEBSD_TCP_RACK_PACE_MAX_SEG       1054
#define FREEBSD_TCP_RACK_PACE_MIN_SEG       1147
#define FREEBSD_TCP_RACK_PACE_RATE_CA       1118
#define FREEBSD_TCP_RACK_PACE_RATE_REC      1120
#define FREEBSD_TCP_RACK_PACE_RATE_SS       1119
#define FREEBSD_TCP_RACK_PACE_TO_FILL       1127
#define FREEBSD_TCP_RACK_PACING_BETA        1138
#define FREEBSD_TCP_RACK_PACING_BETA_ECN    1139
#define FREEBSD_TCP_RACK_PACING_DIVISOR     1146
#define FREEBSD_TCP_RACK_PKT_DELAY          1064
#define FREEBSD_TCP_RACK_PROFILE            1129
#define FREEBSD_TCP_RACK_PRR_SENDALOT       1057
#define FREEBSD_TCP_RACK_REORD_FADE         1062
#define FREEBSD_TCP_RACK_REORD_THRESH       1061
#define FREEBSD_TCP_RACK_RR_CONF            1113
#define FREEBSD_TCP_RACK_SET_RXT_OPTIONS    1143
#define FREEBSD_TCP_RACK_SPLIT_LIMIT        1145
#define FREEBSD_TCP_RACK_TIMER_SLOP         1140
#define FREEBSD_TCP_RACK_TLP_REDUCE         1052
#define FREEBSD_TCP_RACK_TLP_THRESH         1063
#define FREEBSD_TCP_RACK_TLP_USE            1095
#define FREEBSD_TCP_REC_ABC_VAL             1134
#define FREEBSD_TCP_REC_IS_DYN              1156
#define FREEBSD_TCP_REMOTE_UDP_ENCAPS_PORT  71
#define FREEBSD_TCP_REUSPORT_LB_NUMA        1026
#define FREEBSD_TCP_REUSPORT_LB_NUMA_CURDOM -1
#define FREEBSD_TCP_REUSPORT_LB_NUMA_NODOM  -2
#define FREEBSD_TCP_RXTLS_ENABLE            41
#define FREEBSD_TCP_RXTLS_MODE              42
#define FREEBSD_TCP_SHARED_CWND_ALLOWED     75
#define FREEBSD_TCP_SHARED_CWND_ENABLE      1124
#define FREEBSD_TCP_SHARED_CWND_TIME_LIMIT  1128
#define FREEBSD_TCP_SIDECHAN_DIS            1157
#define FREEBSD_TCP_SS_EEXIT                1152
#define FREEBSD_TCP_STACK_SPEC_INFO         1160
#define FREEBSD_TCP_STATS                   33
#define FREEBSD_TCP_TIMELY_DYN_ADJ          1125
#define FREEBSD_TCP_TLS_MODE_IFNET          2
#define FREEBSD_TCP_TLS_MODE_NONE           0
#define FREEBSD_TCP_TLS_MODE_SW             1
#define FREEBSD_TCP_TLS_MODE_TOE            3
#define FREEBSD_TCP_TXTLS_ENABLE            39
#define FREEBSD_TCP_TXTLS_MODE              40
#define FREEBSD_TCP_USER_LOG                48
#define FREEBSD_TCP_USE_CMP_ACKS            77
#define FREEBSD_TCP_VENDOR                  2147483648L

#endif /* _FREEBSD_TCP_H_ */
