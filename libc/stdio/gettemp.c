/*
 * Copyright (c) 1987 Regents of the University of California.
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
/* __gettemp, split out of MKTEMP.C */

#include "local-stdio.h"

/* Our names are six characters from [0-9a-z] */
/*
 * Names come from a generator of their own, so that mkstemp and the
 * others don't change the sequences of rand() or random(). It is
 * seeded from the stack address, which differs between processes
 * where the stack is randomized, and steps under the libc lock so
 * that threads don't race on it.
 */
static uint64_t __gettemp_state;

static uint32_t
__gettemp_random(void)
{
    uint64_t z;
    char     local;

    __LIBC_LOCK();
    if (__gettemp_state == 0)
        __gettemp_state = ((uint64_t)(uintptr_t)&local << 16) ^ 0x2545f4914f6cdd1dULL;
    z = __splitmix64(&__gettemp_state);
    __LIBC_UNLOCK();

    return (uint32_t)z;
}

#define NUM_ATTEMPT (36UL * 36UL * 36UL)

int
__gettemp(char *path, int suffixlen, int (*create)(const char *path, int flags), int flags)
{
    char    *start;
    char    *end;
    size_t   len;
    uint32_t attempt;

    len = strlen(path);
    if (suffixlen < 0 || (size_t)suffixlen > len) {
        errno = EINVAL;
        return -1;
    }
    end = path + len - suffixlen;
    start = end;

    /* Find the Xs, leaving the template alone unless there are six */
    while (path < start && start[-1] == 'X')
        start--;

    if (end - start < 6) {
        errno = EINVAL;
        return -1;
    }

    for (attempt = 0; attempt < NUM_ATTEMPT; attempt++) {
        char    *trv;
        uint32_t bits = 0;
        int      left = 0;

        /* Replace every X, six characters from each random value */
        for (trv = start; trv < end; trv++) {
            char c;

            if (left == 0) {
                bits = __gettemp_random();
                left = 6;
            }
            c = (char)(bits % 36);
            bits /= 36;
            left--;

            if (c < 10)
                c += '0';
            else
                c += 'a' - 10;
            *trv = c;
        }

        /*
         * Use open to check if the file exists to avoid depending on
         * stat or access. Don't rely on O_EXCL working (semihosting
         * ignores it), although if it doesn't, this introduces a race
         * condition. Don't wait for a FIFO or take a terminal.
         */
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_NOCTTY);
        if (fd >= 0) {
            close(fd);
            continue;
        }
        if (create) {
            /* Whatever stopped open, let the creation decide */
            int ret = create(path, flags);
            if (ret >= 0)
                return ret;
            if (errno != EEXIST)
                return -1;
        } else if (errno != EACCES) {
            if (errno != ENOENT)
                return -1;
            return 0;
        }
    }
    errno = EEXIST;
    return -1;
}
