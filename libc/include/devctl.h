/*
 * Copyright (c) 2016,2019 Joel Sherrill <joel@rtems.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _POSIX_DEVCTL_h_
#define _POSIX_DEVCTL_h_

#include <sys/cdefs.h>
#include <sys/_types.h>

/*
 * posix_devctl is part of POSIX.1-2024 (the DEVICE_CONTROL option),
 * derived from POSIX 1003.26-2003, whose applications define
 * _POSIX_26_C_SOURCE.
 */
#if __POSIX_VISIBLE >= 202405 || defined(_POSIX_26_C_SOURCE)

#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T)
typedef __picolibc_size_t size_t;
#endif

#ifndef _SIZE_T_DECLARED
#define _SIZE_T_DECLARED
#endif

#ifndef _SIZE_T
#define _SIZE_T
#endif

_BEGIN_STD_C

#if defined(__rtems__)
/*
 * The FACE Technical Standard, Edition 3.0 and later require the
 * definition of the subcommand SOCKCLOSE in <devctl.h>.
 *
 * Reference: https://www.opengroup.org/face
 *
 * Using 'D' should avoid the letters used by other users of <sys/ioccom.h>
 */
#include <sys/ioccom.h>

#define SOCKCLOSE _IO('D', 1) /* socket close */
#endif

/*
 * The posix_devctl() method is defined by POSIX 1003.26-2003. Aside
 * from the single method, it adds the following requirements:
 *
 *   + define _POSIX_26_VERSION to 200312L
 *   + add _SC_POSIX_26_VERSION in <unistd.h>. Return _POSIX_26_VERSION
 *   + application must define _POSIX_26_C_SOURCE to use posix_devctl().
 *   + posix_devctl() is prototyped in <devctl.h>
 */
int posix_devctl(int __fd, int __dcmd, void * __restrict __dev_data_ptr, size_t __nbyte,
                 int * __restrict __dev_info_ptr);

_END_STD_C

#endif /* __POSIX_VISIBLE >= 202405 || _POSIX_26_C_SOURCE */
#endif /*_POSIX_DEVCTL_h_ */
