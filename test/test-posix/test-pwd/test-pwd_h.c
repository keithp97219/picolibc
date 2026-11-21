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

#define _POSIX_C_SOURCE 202405L
/* The application's own macros, defined before the header */
#define uid     +
#define pwd     +
#define buffer  +
#define bufsize +
#define result  +
#define name    +
#define stream  +
#include <pwd.h>
#undef uid
#undef pwd
#undef buffer
#undef bufsize
#undef result
#undef name
#undef stream

#include <stdio.h>

/*
 * <pwd.h> defines struct passwd with the POSIX members, gid_t, uid_t
 * and size_t, and the prototypes POSIX gives. In strict POSIX mode,
 * without XSI, it declares neither the XSI functions nor extensions,
 * and its prototypes don't use the application's names
 */

/* The header alone defines these */
static uid_t         u __attribute__((unused));
static gid_t         g __attribute__((unused));
static size_t        s __attribute__((unused));

static struct passwd pw __attribute__((unused));

_Static_assert(_Generic(pw.pw_name, char *: 1, default: 0), "pw_name");
_Static_assert(_Generic(pw.pw_uid, uid_t: 1, default: 0), "pw_uid");
_Static_assert(_Generic(pw.pw_gid, gid_t: 1, default: 0), "pw_gid");
_Static_assert(_Generic(pw.pw_dir, char *: 1, default: 0), "pw_dir");
_Static_assert(_Generic(pw.pw_shell, char *: 1, default: 0), "pw_shell");

static struct passwd *(* const p_getpwnam)(const char *) = getpwnam;
static int (* const p_getpwnam_r)(const char *, struct passwd *, char *, size_t, struct passwd **)
    = getpwnam_r;
static struct passwd *(* const p_getpwuid)(uid_t) = getpwuid;
static int (* const p_getpwuid_r)(uid_t, struct passwd *, char *, size_t, struct passwd **)
    = getpwuid_r;

/* XSI interfaces */
static int getpwent;
static int setpwent;
static int endpwent;

/* Extensions */
static int fgetpwent;
static int fgetpwent_r;
static int getpwent_r;
static int putpwent;
static int getpw;

#ifdef _PATH_PASSWD
#error "_PATH_PASSWD defined in strict mode"
#endif

int
main(void)
{
    (void)p_getpwnam;
    (void)p_getpwnam_r;
    (void)p_getpwuid;
    (void)p_getpwuid_r;
    (void)getpwent;
    (void)setpwent;
    (void)endpwent;
    (void)fgetpwent;
    (void)fgetpwent_r;
    (void)getpwent_r;
    (void)putpwent;
    (void)getpw;
    printf("success\n");
    return 0;
}
