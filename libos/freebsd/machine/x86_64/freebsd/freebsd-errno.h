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

#ifndef _FREEBSD_ERRNO_H_
#define _FREEBSD_ERRNO_H_

#define FREEBSD_EPERM           1
#define FREEBSD_ENOENT          2
#define FREEBSD_ESRCH           3
#define FREEBSD_EINTR           4
#define FREEBSD_EIO             5
#define FREEBSD_ENXIO           6
#define FREEBSD_E2BIG           7
#define FREEBSD_ENOEXEC         8
#define FREEBSD_EBADF           9
#define FREEBSD_ECHILD          10
#define FREEBSD_EDEADLK         11
#define FREEBSD_ENOMEM          12
#define FREEBSD_EACCES          13
#define FREEBSD_EFAULT          14
#define FREEBSD_ENOTBLK         15
#define FREEBSD_EBUSY           16
#define FREEBSD_EEXIST          17
#define FREEBSD_EXDEV           18
#define FREEBSD_ENODEV          19
#define FREEBSD_ENOTDIR         20
#define FREEBSD_EISDIR          21
#define FREEBSD_EINVAL          22
#define FREEBSD_ENFILE          23
#define FREEBSD_EMFILE          24
#define FREEBSD_ENOTTY          25
#define FREEBSD_ETXTBSY         26
#define FREEBSD_EFBIG           27
#define FREEBSD_ENOSPC          28
#define FREEBSD_ESPIPE          29
#define FREEBSD_EROFS           30
#define FREEBSD_EMLINK          31
#define FREEBSD_EPIPE           32
#define FREEBSD_EDOM            33
#define FREEBSD_ERANGE          34
#define FREEBSD_EAGAIN          35
#define FREEBSD_EWOULDBLOCK     35
#define FREEBSD_EINPROGRESS     36
#define FREEBSD_EALREADY        37
#define FREEBSD_ENOTSOCK        38
#define FREEBSD_EDESTADDRREQ    39
#define FREEBSD_EMSGSIZE        40
#define FREEBSD_EPROTOTYPE      41
#define FREEBSD_ENOPROTOOPT     42
#define FREEBSD_EPROTONOSUPPORT 43
#define FREEBSD_ESOCKTNOSUPPORT 44
#define FREEBSD_ENOTSUP         45
#define FREEBSD_EOPNOTSUPP      45
#define FREEBSD_EPFNOSUPPORT    46
#define FREEBSD_EAFNOSUPPORT    47
#define FREEBSD_EADDRINUSE      48
#define FREEBSD_EADDRNOTAVAIL   49
#define FREEBSD_ENETDOWN        50
#define FREEBSD_ENETUNREACH     51
#define FREEBSD_ENETRESET       52
#define FREEBSD_ECONNABORTED    53
#define FREEBSD_ECONNRESET      54
#define FREEBSD_ENOBUFS         55
#define FREEBSD_EISCONN         56
#define FREEBSD_ENOTCONN        57
#define FREEBSD_ESHUTDOWN       58
#define FREEBSD_ETOOMANYREFS    59
#define FREEBSD_ETIMEDOUT       60
#define FREEBSD_ECONNREFUSED    61
#define FREEBSD_ELOOP           62
#define FREEBSD_ENAMETOOLONG    63
#define FREEBSD_EHOSTDOWN       64
#define FREEBSD_EHOSTUNREACH    65
#define FREEBSD_ENOTEMPTY       66
#define FREEBSD_EPROCLIM        67
#define FREEBSD_EUSERS          68
#define FREEBSD_EDQUOT          69
#define FREEBSD_ESTALE          70
#define FREEBSD_EREMOTE         71
#define FREEBSD_EBADRPC         72
#define FREEBSD_ERPCMISMATCH    73
#define FREEBSD_EPROGUNAVAIL    74
#define FREEBSD_EPROGMISMATCH   75
#define FREEBSD_EPROCUNAVAIL    76
#define FREEBSD_ENOLCK          77
#define FREEBSD_ENOSYS          78
#define FREEBSD_EFTYPE          79
#define FREEBSD_EAUTH           80
#define FREEBSD_ENEEDAUTH       81
#define FREEBSD_EIDRM           82
#define FREEBSD_ENOMSG          83
#define FREEBSD_EOVERFLOW       84
#define FREEBSD_ECANCELED       85
#define FREEBSD_EILSEQ          86
#define FREEBSD_ENOATTR         87
#define FREEBSD_EDOOFUS         88
#define FREEBSD_EBADMSG         89
#define FREEBSD_EMULTIHOP       90
#define FREEBSD_ENOLINK         91
#define FREEBSD_EPROTO          92
#define FREEBSD_ENOTCAPABLE     93
#define FREEBSD_ECAPMODE        94
#define FREEBSD_ENOTRECOVERABLE 95
#define FREEBSD_EOWNERDEAD      96
#define FREEBSD_EINTEGRITY      97

#define FREEBSD_ERRNO_MAX       97

/* Kernel errno to picolibc errno; 0 marks one picolibc lacks */
static const unsigned char __freebsd_errno_map[FREEBSD_ERRNO_MAX + 1] = {
#ifdef EPERM
    [FREEBSD_EPERM] = EPERM,
#endif
#ifdef ENOENT
    [FREEBSD_ENOENT] = ENOENT,
#endif
#ifdef ESRCH
    [FREEBSD_ESRCH] = ESRCH,
#endif
#ifdef EINTR
    [FREEBSD_EINTR] = EINTR,
#endif
#ifdef EIO
    [FREEBSD_EIO] = EIO,
#endif
#ifdef ENXIO
    [FREEBSD_ENXIO] = ENXIO,
#endif
#ifdef E2BIG
    [FREEBSD_E2BIG] = E2BIG,
#endif
#ifdef ENOEXEC
    [FREEBSD_ENOEXEC] = ENOEXEC,
#endif
#ifdef EBADF
    [FREEBSD_EBADF] = EBADF,
#endif
#ifdef ECHILD
    [FREEBSD_ECHILD] = ECHILD,
#endif
#ifdef EDEADLK
    [FREEBSD_EDEADLK] = EDEADLK,
#endif
#ifdef ENOMEM
    [FREEBSD_ENOMEM] = ENOMEM,
#endif
#ifdef EACCES
    [FREEBSD_EACCES] = EACCES,
#endif
#ifdef EFAULT
    [FREEBSD_EFAULT] = EFAULT,
#endif
#ifdef ENOTBLK
    [FREEBSD_ENOTBLK] = ENOTBLK,
#endif
#ifdef EBUSY
    [FREEBSD_EBUSY] = EBUSY,
#endif
#ifdef EEXIST
    [FREEBSD_EEXIST] = EEXIST,
#endif
#ifdef EXDEV
    [FREEBSD_EXDEV] = EXDEV,
#endif
#ifdef ENODEV
    [FREEBSD_ENODEV] = ENODEV,
#endif
#ifdef ENOTDIR
    [FREEBSD_ENOTDIR] = ENOTDIR,
#endif
#ifdef EISDIR
    [FREEBSD_EISDIR] = EISDIR,
#endif
#ifdef EINVAL
    [FREEBSD_EINVAL] = EINVAL,
#endif
#ifdef ENFILE
    [FREEBSD_ENFILE] = ENFILE,
#endif
#ifdef EMFILE
    [FREEBSD_EMFILE] = EMFILE,
#endif
#ifdef ENOTTY
    [FREEBSD_ENOTTY] = ENOTTY,
#endif
#ifdef ETXTBSY
    [FREEBSD_ETXTBSY] = ETXTBSY,
#endif
#ifdef EFBIG
    [FREEBSD_EFBIG] = EFBIG,
#endif
#ifdef ENOSPC
    [FREEBSD_ENOSPC] = ENOSPC,
#endif
#ifdef ESPIPE
    [FREEBSD_ESPIPE] = ESPIPE,
#endif
#ifdef EROFS
    [FREEBSD_EROFS] = EROFS,
#endif
#ifdef EMLINK
    [FREEBSD_EMLINK] = EMLINK,
#endif
#ifdef EPIPE
    [FREEBSD_EPIPE] = EPIPE,
#endif
#ifdef EDOM
    [FREEBSD_EDOM] = EDOM,
#endif
#ifdef ERANGE
    [FREEBSD_ERANGE] = ERANGE,
#endif
#ifdef EAGAIN
    [FREEBSD_EAGAIN] = EAGAIN,
#endif
#ifdef EINPROGRESS
    [FREEBSD_EINPROGRESS] = EINPROGRESS,
#endif
#ifdef EALREADY
    [FREEBSD_EALREADY] = EALREADY,
#endif
#ifdef ENOTSOCK
    [FREEBSD_ENOTSOCK] = ENOTSOCK,
#endif
#ifdef EDESTADDRREQ
    [FREEBSD_EDESTADDRREQ] = EDESTADDRREQ,
#endif
#ifdef EMSGSIZE
    [FREEBSD_EMSGSIZE] = EMSGSIZE,
#endif
#ifdef EPROTOTYPE
    [FREEBSD_EPROTOTYPE] = EPROTOTYPE,
#endif
#ifdef ENOPROTOOPT
    [FREEBSD_ENOPROTOOPT] = ENOPROTOOPT,
#endif
#ifdef EPROTONOSUPPORT
    [FREEBSD_EPROTONOSUPPORT] = EPROTONOSUPPORT,
#endif
#ifdef ESOCKTNOSUPPORT
    [FREEBSD_ESOCKTNOSUPPORT] = ESOCKTNOSUPPORT,
#endif
#ifdef ENOTSUP
    [FREEBSD_ENOTSUP] = ENOTSUP,
#endif
#ifdef EPFNOSUPPORT
    [FREEBSD_EPFNOSUPPORT] = EPFNOSUPPORT,
#endif
#ifdef EAFNOSUPPORT
    [FREEBSD_EAFNOSUPPORT] = EAFNOSUPPORT,
#endif
#ifdef EADDRINUSE
    [FREEBSD_EADDRINUSE] = EADDRINUSE,
#endif
#ifdef EADDRNOTAVAIL
    [FREEBSD_EADDRNOTAVAIL] = EADDRNOTAVAIL,
#endif
#ifdef ENETDOWN
    [FREEBSD_ENETDOWN] = ENETDOWN,
#endif
#ifdef ENETUNREACH
    [FREEBSD_ENETUNREACH] = ENETUNREACH,
#endif
#ifdef ENETRESET
    [FREEBSD_ENETRESET] = ENETRESET,
#endif
#ifdef ECONNABORTED
    [FREEBSD_ECONNABORTED] = ECONNABORTED,
#endif
#ifdef ECONNRESET
    [FREEBSD_ECONNRESET] = ECONNRESET,
#endif
#ifdef ENOBUFS
    [FREEBSD_ENOBUFS] = ENOBUFS,
#endif
#ifdef EISCONN
    [FREEBSD_EISCONN] = EISCONN,
#endif
#ifdef ENOTCONN
    [FREEBSD_ENOTCONN] = ENOTCONN,
#endif
#ifdef ESHUTDOWN
    [FREEBSD_ESHUTDOWN] = ESHUTDOWN,
#endif
#ifdef ETOOMANYREFS
    [FREEBSD_ETOOMANYREFS] = ETOOMANYREFS,
#endif
#ifdef ETIMEDOUT
    [FREEBSD_ETIMEDOUT] = ETIMEDOUT,
#endif
#ifdef ECONNREFUSED
    [FREEBSD_ECONNREFUSED] = ECONNREFUSED,
#endif
#ifdef ELOOP
    [FREEBSD_ELOOP] = ELOOP,
#endif
#ifdef ENAMETOOLONG
    [FREEBSD_ENAMETOOLONG] = ENAMETOOLONG,
#endif
#ifdef EHOSTDOWN
    [FREEBSD_EHOSTDOWN] = EHOSTDOWN,
#endif
#ifdef EHOSTUNREACH
    [FREEBSD_EHOSTUNREACH] = EHOSTUNREACH,
#endif
#ifdef ENOTEMPTY
    [FREEBSD_ENOTEMPTY] = ENOTEMPTY,
#endif
#ifdef EPROCLIM
    [FREEBSD_EPROCLIM] = EPROCLIM,
#endif
#ifdef EUSERS
    [FREEBSD_EUSERS] = EUSERS,
#endif
#ifdef EDQUOT
    [FREEBSD_EDQUOT] = EDQUOT,
#endif
#ifdef ESTALE
    [FREEBSD_ESTALE] = ESTALE,
#endif
#ifdef EREMOTE
    [FREEBSD_EREMOTE] = EREMOTE,
#endif
#ifdef EBADRPC
    [FREEBSD_EBADRPC] = EBADRPC,
#endif
#ifdef ERPCMISMATCH
    [FREEBSD_ERPCMISMATCH] = ERPCMISMATCH,
#endif
#ifdef EPROGUNAVAIL
    [FREEBSD_EPROGUNAVAIL] = EPROGUNAVAIL,
#endif
#ifdef EPROGMISMATCH
    [FREEBSD_EPROGMISMATCH] = EPROGMISMATCH,
#endif
#ifdef EPROCUNAVAIL
    [FREEBSD_EPROCUNAVAIL] = EPROCUNAVAIL,
#endif
#ifdef ENOLCK
    [FREEBSD_ENOLCK] = ENOLCK,
#endif
#ifdef ENOSYS
    [FREEBSD_ENOSYS] = ENOSYS,
#endif
#ifdef EFTYPE
    [FREEBSD_EFTYPE] = EFTYPE,
#endif
#ifdef EAUTH
    [FREEBSD_EAUTH] = EAUTH,
#endif
#ifdef ENEEDAUTH
    [FREEBSD_ENEEDAUTH] = ENEEDAUTH,
#endif
#ifdef EIDRM
    [FREEBSD_EIDRM] = EIDRM,
#endif
#ifdef ENOMSG
    [FREEBSD_ENOMSG] = ENOMSG,
#endif
#ifdef EOVERFLOW
    [FREEBSD_EOVERFLOW] = EOVERFLOW,
#endif
#ifdef ECANCELED
    [FREEBSD_ECANCELED] = ECANCELED,
#endif
#ifdef EILSEQ
    [FREEBSD_EILSEQ] = EILSEQ,
#endif
#ifdef ENOATTR
    [FREEBSD_ENOATTR] = ENOATTR,
#endif
#ifdef EDOOFUS
    [FREEBSD_EDOOFUS] = EDOOFUS,
#endif
#ifdef EBADMSG
    [FREEBSD_EBADMSG] = EBADMSG,
#endif
#ifdef EMULTIHOP
    [FREEBSD_EMULTIHOP] = EMULTIHOP,
#endif
#ifdef ENOLINK
    [FREEBSD_ENOLINK] = ENOLINK,
#endif
#ifdef EPROTO
    [FREEBSD_EPROTO] = EPROTO,
#endif
#ifdef ENOTCAPABLE
    [FREEBSD_ENOTCAPABLE] = ENOTCAPABLE,
#endif
#ifdef ECAPMODE
    [FREEBSD_ECAPMODE] = ECAPMODE,
#endif
#ifdef ENOTRECOVERABLE
    [FREEBSD_ENOTRECOVERABLE] = ENOTRECOVERABLE,
#endif
#ifdef EOWNERDEAD
    [FREEBSD_EOWNERDEAD] = EOWNERDEAD,
#endif
#ifdef EINTEGRITY
    [FREEBSD_EINTEGRITY] = EINTEGRITY,
#endif
};

#endif /* _FREEBSD_ERRNO_H_ */
