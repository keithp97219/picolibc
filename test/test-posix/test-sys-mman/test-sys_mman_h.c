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
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_MAPPED_FILES && TEST_POSIX_MEMORY_PROTECTION

#include <sys/mman.h>

/* The <sys/mman.h> header: constants, types and prototypes */

#define same_type(a, T) _Generic((a), T: 1, default: 0)

_Static_assert(PROT_NONE == 0 || (PROT_NONE & (PROT_READ | PROT_WRITE | PROT_EXEC)) == 0,
               "PROT_NONE");
_Static_assert(PROT_READ != PROT_WRITE && PROT_READ != PROT_EXEC && PROT_WRITE != PROT_EXEC,
               "PROT_ values");
_Static_assert((PROT_READ & PROT_WRITE) == 0 && (PROT_READ & PROT_EXEC) == 0
                   && (PROT_WRITE & PROT_EXEC) == 0,
               "PROT_ bits");
_Static_assert((MAP_SHARED & MAP_PRIVATE) == 0 && (MAP_SHARED & MAP_FIXED) == 0
                   && (MAP_PRIVATE & MAP_FIXED) == 0 && (MAP_ANONYMOUS & MAP_FIXED) == 0
                   && (MAP_ANONYMOUS & (MAP_SHARED | MAP_PRIVATE)) == 0,
               "MAP_ bits");
/* MAP_ANON is new in POSIX.1-2024, with the same value as MAP_ANONYMOUS */
_Static_assert(MAP_ANON == MAP_ANONYMOUS, "MAP_ANON");
_Static_assert((MS_ASYNC & MS_SYNC) == 0 && (MS_ASYNC & MS_INVALIDATE) == 0
                   && (MS_SYNC & MS_INVALIDATE) == 0 && MS_ASYNC != 0 && MS_SYNC != 0
                   && MS_INVALIDATE != 0,
               "MS_ bits");
_Static_assert((MCL_CURRENT & MCL_FUTURE) == 0 && MCL_CURRENT != 0 && MCL_FUTURE != 0, "MCL_ bits");
_Static_assert(POSIX_MADV_NORMAL != POSIX_MADV_RANDOM && POSIX_MADV_NORMAL != POSIX_MADV_SEQUENTIAL
                   && POSIX_MADV_NORMAL != POSIX_MADV_WILLNEED
                   && POSIX_MADV_NORMAL != POSIX_MADV_DONTNEED
                   && POSIX_MADV_RANDOM != POSIX_MADV_SEQUENTIAL
                   && POSIX_MADV_RANDOM != POSIX_MADV_WILLNEED
                   && POSIX_MADV_RANDOM != POSIX_MADV_DONTNEED
                   && POSIX_MADV_SEQUENTIAL != POSIX_MADV_WILLNEED
                   && POSIX_MADV_SEQUENTIAL != POSIX_MADV_DONTNEED
                   && POSIX_MADV_WILLNEED != POSIX_MADV_DONTNEED,
               "POSIX_MADV_ values");
_Static_assert(same_type(MAP_FAILED, void *), "MAP_FAILED is a void *");

/* The flags shm_open takes; glibc 2.43 leaves them to <fcntl.h> */
#ifndef __GLIBC__
_Static_assert(O_RDONLY != O_RDWR && O_CREAT != 0 && O_EXCL != 0 && O_TRUNC != 0
                   && ((O_CREAT | O_EXCL | O_TRUNC) & (O_RDONLY | O_RDWR)) == 0,
               "O_ flags");
#endif

static mode_t m;
static off_t  o;
static size_t s;
_Static_assert((off_t)-1 < 0, "off_t is signed");
_Static_assert((size_t)-1 > 0, "size_t is unsigned");

static int   (  *const p_mlock)(const void *, size_t) = mlock;
static int   (  *const p_mlockall)(int) = mlockall;
static void *(* const p_mmap)(void *, size_t, int, int, int, off_t) = mmap;
static int   (  *const p_mprotect)(void *, size_t, int) = mprotect;
static int   (  *const p_msync)(void *, size_t, int) = msync;
static int   (  *const p_munlock)(const void *, size_t) = munlock;
static int   (  *const p_munlockall)(void) = munlockall;
static int   (  *const p_munmap)(void *, size_t) = munmap;
static int   (  *const p_posix_madvise)(void *, size_t, int) = posix_madvise;
static int   (  *const p_shm_open)(const char *, int, mode_t) = shm_open;
static int   (  *const p_shm_unlink)(const char *) = shm_unlink;

/*
 * Names POSIX doesn't reserve for <sys/mman.h>, which it must leave
 * alone in strict mode. madvise and mlock2 are Linux extensions;
 * glibc 2.43 defines MCL_ONFAULT here regardless.
 */
static int   madvise = 1;
static int   mlock2 = 2;
#ifndef __GLIBC__
static int MCL_ONFAULT = 3;
#endif
#define MADV_NORMAL   4
#define MADV_DONTNEED 5
#define MLOCK_ONFAULT 6

#include <stdio.h>

int
main(void)
{
    if (!p_mlock || !p_mlockall || !p_mmap || !p_mprotect || !p_msync || !p_munlock || !p_munlockall
        || !p_munmap || !p_posix_madvise || !p_shm_open || !p_shm_unlink) {
        printf("functions\n");
        return 1;
    }
    (void)m;
    (void)o;
    (void)s;
#ifndef __GLIBC__
    (void)MCL_ONFAULT;
#endif
    if (madvise + mlock2 + MADV_NORMAL + MADV_DONTNEED + MLOCK_ONFAULT != 18) {
        printf("names\n");
        return 1;
    }
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
