/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)syslimits.h	8.1 (Berkeley) 6/2/93
 * $FreeBSD: src/sys/sys/syslimits.h,v 1.10 2001/06/18 20:24:54 wollman Exp $
 */

#ifndef _SYS_SYSLIMITS_H_
#define _SYS_SYSLIMITS_H_

#include <sys/cdefs.h>

#if __POSIX_VISIBLE

/* Runtime invariant values */
#define ARG_MAX    65536 /* max bytes for an exec function */
#define ATEXIT_MAX 32    /* max atexit functions */
#define CHILD_MAX  40    /* max simultaneous processes */
#define IOV_MAX    1024  /* max elements in i/o vector */
#define OPEN_MAX   64    /* max open files per process */
#define TZNAME_MAX 10    /* max time zone name length */
#define PASS_MAX   32    /* max password length for getpass */

/* Pathname values */
#define LINK_MAX  32767      /* max file link count */
#define MAX_CANON 255        /* max bytes in term canon input line */
#define MAX_INPUT 255        /* max bytes in terminal input */
#define NAME_MAX  __NAME_MAX /* max bytes in a file name */
#define PATH_MAX  1024       /* max bytes in pathname */
#define PIPE_BUF  512        /* max bytes for atomic pipe writes */

/*
 * Runtime increasable values; sysconf reports the value a system
 * actually has, which may be larger
 */
#define BC_BASE_MAX        99   /* max obase in bc(1) */
#define BC_DIM_MAX         2048 /* max array elements in bc(1) */
#define BC_SCALE_MAX       99   /* max scale in bc(1) */
#define BC_STRING_MAX      1000 /* max string constant in bc(1) */
#define CHARCLASS_NAME_MAX 14   /* max bytes in a character class name */
#define COLL_WEIGHTS_MAX   2    /* max weights for order keyword */
#define EXPR_NEST_MAX      32   /* max expressions nested in expr(1) */
#define LINE_MAX           2048 /* max bytes in an input line */
#define NGROUPS_MAX        8    /* max supplementary group IDs */
#define RE_DUP_MAX         255  /* max RE's in interval notation */

/* Other invariant values; catopen never opens a catalog */
#define NL_MSGMAX  32767 /* max message number */
#define NL_SETMAX  255   /* max set number */
#define NL_TEXTMAX 2048  /* max bytes in a message */

/* Numerical limits POSIX adds */
#define LONG_BIT (__SIZEOF_LONG__ * 8) /* bits in a long */
#define WORD_BIT (__SIZEOF_INT__ * 8)  /* bits in an int */

/* The coarsest CLOCK_REALTIME and CLOCK_MONOTONIC resolution allowed */
#define _POSIX_CLOCKRES_MIN                 20000000

#define AIO_LISTIO_MAX                      64 /* max requests in one lio_listio */
#define AIO_PRIO_DELTA_MAX                  20 /* max aio_reqprio; accepted but unused */

#define _POSIX_AIO_LISTIO_MAX               2
#define _POSIX_AIO_MAX                      1
#define _POSIX_ARG_MAX                      4096
#define _POSIX_CHILD_MAX                    25
#define _POSIX_DELAYTIMER_MAX               32
#define _POSIX_HOST_NAME_MAX                255
#define _POSIX_LINK_MAX                     8
#define _POSIX_LOGIN_NAME_MAX               9
#define _POSIX_MAX_CANON                    255
#define _POSIX_MAX_INPUT                    255
#define _POSIX_MQ_OPEN_MAX                  8
#define _POSIX_MQ_PRIO_MAX                  32
#define _POSIX_NAME_MAX                     14
#define _POSIX_NGROUPS_MAX                  8
#define _POSIX_OPEN_MAX                     20
#define _POSIX_PATH_MAX                     256
#define _POSIX_PIPE_BUF                     512
#define _POSIX_RE_DUP_MAX                   255
#define _POSIX_RTSIG_MAX                    8
#define _POSIX_SEM_NSEMS_MAX                256
#define _POSIX_SEM_VALUE_MAX                32767
#define _POSIX_SIGQUEUE_MAX                 32
#define _POSIX_SSIZE_MAX                    32767
#define _POSIX_SS_REPL_MAX                  4
#define _POSIX_STREAM_MAX                   8
#define _POSIX_SYMLINK_MAX                  255
#define _POSIX_SYMLOOP_MAX                  8
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS 4
#define _POSIX_THREAD_KEYS_MAX              128
#define _POSIX_THREAD_THREADS_MAX           64
#define _POSIX_TIMER_MAX                    32
#define _POSIX_TRACE_EVENT_NAME_MAX         30
#define _POSIX_TRACE_NAME_MAX               8
#define _POSIX_TRACE_SYS_MAX                8
#define _POSIX_TRACE_USER_EVENT_MAX         32
#define _POSIX_TTY_NAME_MAX                 9
#define _POSIX_TZNAME_MAX                   6
#define _POSIX2_BC_BASE_MAX                 99
#define _POSIX2_BC_DIM_MAX                  2048
#define _POSIX2_BC_SCALE_MAX                99
#define _POSIX2_BC_STRING_MAX               1000
#define _POSIX2_CHARCLASS_NAME_MAX          14
#define _POSIX2_COLL_WEIGHTS_MAX            2
#define _POSIX2_EXPR_NEST_MAX               32
#define _POSIX2_LINE_MAX                    2048
#define _POSIX2_RE_DUP_MAX                  255

#endif /* __POSIX_VISIBLE */

#ifdef _XOPEN_SOURCE
#define _XOPEN_IOV_MAX  16
#define _XOPEN_NAME_MAX 255
#define _XOPEN_PATH_MAX 1024
#endif

#if __MISC_VISIBLE || __POSIX_VISIBLE >= 202405
#define NSIG_MAX       64  /* signal numbers are less than this; matches sigset_t */
#define GETENTROPY_MAX 256 /* max length for one getentropy call */
#endif

#if __XSI_VISIBLE
#define NZERO      20 /* default nice value; Linux nice values run from -20 to 19 */
#define NL_LANGMAX 14 /* max bytes in a LANG name */
#endif

#endif
