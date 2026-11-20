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

#ifndef _LIBINTL_H_
#define _LIBINTL_H_

#include <sys/cdefs.h>
#include <sys/_locale.h>

_BEGIN_STD_C

char *bindtextdomain(const char *, const char *) __picolibc_export;
char *bind_textdomain_codeset(const char *, const char *) __picolibc_export;
char *dcgettext(const char *, const char *, int) __picolibc_export;
char *dcgettext_l(const char *, const char *, int, locale_t) __picolibc_export;
char *dcngettext(const char *, const char *, const char *, unsigned long int, int)
    __picolibc_export;
char *dcngettext_l(const char *, const char *, const char *, unsigned long int, int, locale_t)
    __picolibc_export;
char *dgettext(const char *, const char *) __picolibc_export;
char *dgettext_l(const char *, const char *, locale_t) __picolibc_export;
char *dngettext(const char *, const char *, const char *, unsigned long int) __picolibc_export;
char *dngettext_l(const char *, const char *, const char *, unsigned long int, locale_t)
    __picolibc_export;
char *gettext(const char *) __picolibc_export;
char *gettext_l(const char *, locale_t) __picolibc_export;
char *ngettext(const char *, const char *, unsigned long int) __picolibc_export;
char *ngettext_l(const char *, const char *, unsigned long int, locale_t) __picolibc_export;
char *textdomain(const char *) __picolibc_export;

_END_STD_C

#endif /* _LIBINTL_H_ */
