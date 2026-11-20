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

/*
 * picolibc has no message catalogs, so every lookup finds no
 * translation and returns the message ID, picking the plural form
 * when n is not 1. These never modify errno.
 */

#define _POSIX_C_SOURCE 202405L
#include <libintl.h>

char *
gettext(const char *msgid)
{
    return (char *)msgid;
}

char *
gettext_l(const char *msgid, locale_t locale)
{
    (void)locale;
    return (char *)msgid;
}

char *
dgettext(const char *domainname, const char *msgid)
{
    (void)domainname;
    return (char *)msgid;
}

char *
dgettext_l(const char *domainname, const char *msgid, locale_t locale)
{
    (void)domainname;
    (void)locale;
    return (char *)msgid;
}

char *
dcgettext(const char *domainname, const char *msgid, int category)
{
    (void)domainname;
    (void)category;
    return (char *)msgid;
}

char *
dcgettext_l(const char *domainname, const char *msgid, int category, locale_t locale)
{
    (void)domainname;
    (void)category;
    (void)locale;
    return (char *)msgid;
}

char *
ngettext(const char *msgid, const char *msgid_plural, unsigned long int n)
{
    return (char *)(n == 1 ? msgid : msgid_plural);
}

char *
ngettext_l(const char *msgid, const char *msgid_plural, unsigned long int n, locale_t locale)
{
    (void)locale;
    return ngettext(msgid, msgid_plural, n);
}

char *
dngettext(const char *domainname, const char *msgid, const char *msgid_plural, unsigned long int n)
{
    (void)domainname;
    return ngettext(msgid, msgid_plural, n);
}

char *
dngettext_l(const char *domainname, const char *msgid, const char *msgid_plural,
            unsigned long int n, locale_t locale)
{
    (void)domainname;
    (void)locale;
    return ngettext(msgid, msgid_plural, n);
}

char *
dcngettext(const char *domainname, const char *msgid, const char *msgid_plural, unsigned long int n,
           int category)
{
    (void)domainname;
    (void)category;
    return ngettext(msgid, msgid_plural, n);
}

char *
dcngettext_l(const char *domainname, const char *msgid, const char *msgid_plural,
             unsigned long int n, int category, locale_t locale)
{
    (void)domainname;
    (void)category;
    (void)locale;
    return ngettext(msgid, msgid_plural, n);
}
