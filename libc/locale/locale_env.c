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

#define _DEFAULT_SOURCE
#include "locale_private.h"

static const char * const __category_names[_LC_LAST] = {
    [LC_COLLATE] = "LC_COLLATE", [LC_CTYPE] = "LC_CTYPE", [LC_MONETARY] = "LC_MONETARY",
    [LC_NUMERIC] = "LC_NUMERIC", [LC_TIME] = "LC_TIME",   [_LC_MESSAGES] = "LC_MESSAGES",
};

/* An environment variable that is unset or empty doesn't count */
static const char *
__locale_env(const char *name)
{
    const char *value = getenv(name);

    if (value && *value)
        return value;
    return NULL;
}

/*
 * The locale the environment names for category (not LC_ALL), in
 * the order XBD 8.2 gives: LC_ALL, the category's own variable, then
 * LANG, then the default locale. Returns locale_INVALID when the
 * chosen value names a locale picolibc doesn't have.
 */
enum locale_id
__find_locale_env(int category)
{
    const char *name = __locale_env("LC_ALL");

    if (!name)
        name = __locale_env(__category_names[category]);
    if (!name)
        name = __locale_env("LANG");
    if (!name)
        return _DEFAULT_LOCALE;
    return __find_locale(name);
}
