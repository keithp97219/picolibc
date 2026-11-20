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
#include <errno.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static const char msgid[] = "hello";
static const char plural[] = "hellos";

int
main(void)
{
    /* With no translation, the message ID comes back, and errno is untouched */
    errno = 1234;
    check(gettext(msgid) == msgid, "gettext");
    check(dgettext("test", msgid) == msgid, "dgettext");
    check(dcgettext("test", msgid, LC_MESSAGES) == msgid, "dcgettext");
    check(errno == 1234, "errno unchanged");

    /* The plural form is used unless n is 1 */
    check(ngettext(msgid, plural, 1) == msgid, "ngettext 1");
    check(ngettext(msgid, plural, 0) == plural, "ngettext 0");
    check(ngettext(msgid, plural, 2) == plural, "ngettext 2");
    check(dngettext("test", msgid, plural, 1) == msgid, "dngettext 1");
    check(dngettext("test", msgid, plural, 3) == plural, "dngettext 3");
    check(dcngettext("test", msgid, plural, 3, LC_MESSAGES) == plural, "dcngettext 3");
    check(errno == 1234, "errno still unchanged");

    printf("success\n");
    return 0;
}
