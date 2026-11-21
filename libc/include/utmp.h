/*
Copyright (c) 1991, 1993
The Regents of the University of California.  All rights reserved.
All or some portions of this file are derived from material licensed
to the University of California by American Telephone and Telegraph
Co. or Unix System Laboratories, Inc. and are reproduced herein with
the permission of UNIX System Laboratories, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the University nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
 */

/*
 * The traditional user accounting interface, built on <utmpx.h>:
 * struct utmp is struct utmpx, and each function uses its utmpx
 * counterpart. picolibc keeps no database, so utmpname accepts any
 * name and updwtmp records nothing.
 */

#ifndef _UTMP_H_
#define _UTMP_H_

#include <utmpx.h>

_BEGIN_STD_C

#define UT_LINESIZE 32
#define UT_NAMESIZE 32
#define UT_HOSTSIZE 256

#define _PATH_UTMP  "/var/run/utmp"
#define _PATH_WTMP  "/var/log/wtmp"
#define UTMP_FILE   _PATH_UTMP
#define WTMP_FILE   _PATH_WTMP

#define utmp        utmpx
#define ut_name     ut_user
#define ut_time     ut_tv.tv_sec

void         endutent(void);
struct utmp *getutent(void);
struct utmp *getutid(const struct utmp *);
struct utmp *getutline(const struct utmp *);
struct utmp *pututline(const struct utmp *);
void         setutent(void);
void         updwtmp(const char *, const struct utmp *);
int          utmpname(const char *);

_END_STD_C

#endif /* _UTMP_H_ */
