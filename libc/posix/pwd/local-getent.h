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

#ifndef _LOCAL_GETENT_H_
#define _LOCAL_GETENT_H_

#include <stdbool.h>
#include <string.h>

/*
 * Helpers for parsing the colon-separated lines of the user and group
 * databases
 */

/*
 * Split off the next ':'-separated field of *line, in place. At the
 * last field, *line becomes NULL; once it is NULL, there are no more
 * fields.
 */
static inline char *
__getent_field(char **line)
{
    char *field = *line;
    char *sep;

    if (!field)
        return NULL;
    sep = strchr(field, ':');
    if (sep) {
        *sep = '\0';
        *line = sep + 1;
    } else {
        *line = NULL;
    }
    return field;
}

/* Parse a user or group id: decimal digits only, no more than max */
static inline bool
__getent_id(const char *s, unsigned long max, unsigned long *value)
{
    unsigned long n = 0;
    unsigned      digit;

    if (*s == '\0')
        return false;
    do {
        if (*s < '0' || *s > '9')
            return false;
        digit = (unsigned)(*s - '0');
        if (n > (max - digit) / 10)
            return false;
        n = n * 10 + digit;
    } while (*++s);
    *value = n;
    return true;
}

#endif /* _LOCAL_GETENT_H_ */
