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
 * The default environment, as programs that define no feature-test
 * macros (SQLite's shell) get it without -std=c..
 */
#define _DEFAULT_SOURCE
#include <unistd.h>
#include "../test-flags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>

/*
 * getrusage is XSI, and declared by default (as by glibc, musl and
 * the BSDs); struct rusage has the BSD fields beyond POSIX's ru_utime
 * and ru_stime (SQLite's test harness reads ru_minflt and ru_majflt).
 * Linux and FreeBSD count page faults.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Every field, so a missing one fails to compile */
static long
total(const struct rusage *ru)
{
    return ru->ru_utime.tv_sec + ru->ru_utime.tv_usec + ru->ru_stime.tv_sec + ru->ru_stime.tv_usec
        + ru->ru_maxrss + ru->ru_ixrss + ru->ru_idrss + ru->ru_isrss + ru->ru_minflt + ru->ru_majflt
        + ru->ru_nswap + ru->ru_inblock + ru->ru_oublock + ru->ru_msgsnd + ru->ru_msgrcv
        + ru->ru_nsignals + ru->ru_nvcsw + ru->ru_nivcsw;
}

#if (defined(__linux__) || defined(__FreeBSD__)) && TEST_POSIX_MULTI_PROCESS \
    && TEST_POSIX_MAPPED_FILES

#define TOUCH (4 * 1024 * 1024)

int
main(void)
{
    struct rusage before, after;
    char         *mem;

    memset(&before, 0xff, sizeof(before));
    check(getrusage(RUSAGE_SELF, &before) == 0, "getrusage");
    /* FreeBSD samples the resident set size at clock ticks: may be 0 */
    check(before.ru_maxrss >= 0, "resident set size");
    check(before.ru_minflt > 0, "page reclaims");
    check(before.ru_majflt >= 0 && before.ru_nvcsw >= 0 && before.ru_nivcsw >= 0, "other counts");

    /*
     * Touching fresh pages takes page faults (mmap's: picolibc's heap
     * on FreeBSD may be resident already)
     */
    mem = mmap(NULL, TOUCH, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(mem != MAP_FAILED, "mmap");
    memset(mem, 1, TOUCH);
    check(getrusage(RUSAGE_SELF, &after) == 0, "getrusage");
    check(after.ru_minflt > before.ru_minflt, "page reclaims grow");
    check(after.ru_maxrss >= before.ru_maxrss, "resident set size");
    printf("maxrss %ld minflt %ld majflt %ld nvcsw %ld nivcsw %ld (%ld)\n", after.ru_maxrss,
           after.ru_minflt, after.ru_majflt, after.ru_nvcsw, after.ru_nivcsw, total(&after));
    check(munmap(mem, TOUCH) == 0, "munmap");
    return 0;
}

#else

int
main(void)
{
    struct rusage ru;

    memset(&ru, 0, sizeof(ru));
    (void)total(&ru);
    (void)getrusage;
    printf("no OS rusage counts, skipping\n");
    return 77;
}

#endif
