/*-
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution.
 * Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "local.h"

/*
 * POSIX: the string itself becomes part of the environment, so
 * changing it later changes the environment. A string without '='
 * (or with an empty name) is rejected with EINVAL.
 */
int
putenv(char *str)
{
    char *equal;
    bool  used;

    if (str == NULL || (equal = strchr(str, '=')) == NULL || equal == str) {
        errno = EINVAL;
        return -1;
    }

    return __env_put(str, (size_t)(equal - str), str, false, true, &used);
}
