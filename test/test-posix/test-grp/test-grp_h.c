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
#define gid    +
#define grp    +
#define buffer +
#define size   +
#define result +
#define name   +
#define stream +
#include <grp.h>
#undef gid
#undef grp
#undef buffer
#undef size
#undef result
#undef name
#undef stream

#include <stdio.h>

/*
 * <grp.h> defines struct group with the POSIX members, gid_t and
 * size_t, and the prototypes POSIX gives. In strict POSIX mode,
 * without XSI, it declares neither the XSI functions nor extensions,
 * and its prototypes don't use the application's names
 */

/* The header alone defines these */
static gid_t        g __attribute__((unused));
static size_t       s __attribute__((unused));

static struct group gr __attribute__((unused));

_Static_assert(_Generic(gr.gr_name, char *: 1, default: 0), "gr_name");
_Static_assert(_Generic(gr.gr_gid, gid_t: 1, default: 0), "gr_gid");
_Static_assert(_Generic(gr.gr_mem, char **: 1, default: 0), "gr_mem");

static struct group *(* const p_getgrgid)(gid_t) = getgrgid;
static int           (          *const p_getgrgid_r)(gid_t, struct group *, char *, size_t, struct group **)
    = getgrgid_r;
static struct group *(* const p_getgrnam)(const char *) = getgrnam;
static int (* const p_getgrnam_r)(const char *, struct group *, char *, size_t, struct group **)
    = getgrnam_r;

/* XSI interfaces */
static int getgrent;
static int setgrent;
static int endgrent;

/* Extensions */
static int fgetgrent;
static int fgetgrent_r;
static int getgrent_r;
static int initgroups;
static int setgroups;
static int putgrent;

#ifdef _PATH_GROUP
#error "_PATH_GROUP defined in strict mode"
#endif

int
main(void)
{
    (void)p_getgrgid;
    (void)p_getgrgid_r;
    (void)p_getgrnam;
    (void)p_getgrnam_r;
    (void)getgrent;
    (void)setgrent;
    (void)endgrent;
    (void)fgetgrent;
    (void)fgetgrent_r;
    (void)getgrent_r;
    (void)initgroups;
    (void)setgroups;
    (void)putgrent;
    printf("success\n");
    return 0;
}
