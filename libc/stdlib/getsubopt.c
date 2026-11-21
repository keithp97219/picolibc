/*-
 * Copyright (c) 1990, 1993
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
 */

#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * The SVID interface to getsubopt provides no way of figuring out which
 * part of the suboptions list wasn't matched.  This makes error messages
 * tricky...  The extern variable suboptarg is a pointer to the token
 * which didn't match.
 */
char *suboptarg;

/*
 * POSIX: suboptions are separated by commas alone, and the first '='
 * in one separates its token from its value; blanks are part of the
 * token or value.
 */
int
getsubopt(char ** __restrict optionp, char * const * __restrict tokens, char ** __restrict valuep)
{
    int   cnt;
    char *p;

    suboptarg = *valuep = NULL;

    if (!optionp || !*optionp)
        return (-1);

    p = *optionp;
    if (!*p)
        return (-1);

    /* save the start of the token, and find the end of the suboption. */
    suboptarg = p;
    while (*p && *p != ',' && *p != '=')
        p++;

    /* If there's an equals sign, terminate the token there and find the value */
    if (*p == '=') {
        *p++ = '\0';
        *valuep = p;
        while (*p && *p != ',')
            p++;
    }

    /* Terminate the suboption and point past the comma, if any */
    if (*p == ',')
        *p++ = '\0';

    /* set optionp for next round. */
    *optionp = p;

    for (cnt = 0; *tokens; ++tokens, ++cnt)
        if (!strcmp(suboptarg, *tokens))
            return (cnt);
    return (-1);
}
