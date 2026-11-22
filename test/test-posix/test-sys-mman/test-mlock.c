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
#if TEST_POSIX_MEMLOCK && TEST_POSIX_MEMLOCK_RANGE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "../is-qemu.h"

/* mlock and mlockall lock memory, or report why they can't */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Locking needs privilege or room under RLIMIT_MEMLOCK; either answer is allowed */
static int
locked_or_refused(int ret)
{
    return ret == 0 || (ret == -1 && (errno == EPERM || errno == ENOMEM || errno == EAGAIN));
}

int
main(void)
{
    long  page = sysconf(_SC_PAGESIZE);
    char *mem;

    check(page > 0, "_SC_PAGESIZE");
    mem = mmap(NULL, 2 * page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(mem != MAP_FAILED, "mmap");

    check(locked_or_refused(mlock(mem, page)), "mlock");
    check(munlock(mem, page) == 0, "munlock");
    /* munlock of memory that isn't locked is fine */
    check(munlock(mem, 2 * page) == 0, "munlock unlocked");

    check(locked_or_refused(mlockall(MCL_CURRENT)), "MCL_CURRENT");
    check(locked_or_refused(mlockall(MCL_CURRENT | MCL_FUTURE)), "MCL_FUTURE");
    check(munlockall() == 0, "munlockall");

    errno = 0;
    check(mlockall(0) == -1 && errno == EINVAL, "no flags");
    /* No system has a flag this high, and an unimplemented flag must fail */
    /* QEMU's user-mode emulation drops flags it doesn't know */
    if (!is_qemu()) {
        errno = 0;
        check(mlockall(MCL_CURRENT | 0x40000000) == -1 && errno == EINVAL, "unimplemented flag");
        errno = 0;
        check(mlockall(MCL_FUTURE | 0x100) == -1 && errno == EINVAL, "unimplemented flag 0x100");
    }
    errno = 0;
    check(mlock(mem, (size_t)-page) == -1 && (errno == ENOMEM || errno == EINVAL), "huge range");

    /* A range that isn't all mapped */
    check(munmap(mem + page, page) == 0, "munmap second page");
    errno = 0;
    /* QEMU's user-mode emulation doesn't check the range is mapped */
    check((munlock(mem, 2 * page) == -1 && errno == ENOMEM) || is_qemu(), "munlock of a hole");
    if (mlock(mem, page) == 0) {
        errno = 0;
        check(mlock(mem, 2 * page) == -1 && errno == ENOMEM, "mlock of a hole");
        check(munlock(mem, page) == 0, "munlock");
    } else {
        printf("can't lock memory (errno %d), skipping mlock of a hole\n", errno);
    }
    check(munmap(mem, page) == 0, "munmap");
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
