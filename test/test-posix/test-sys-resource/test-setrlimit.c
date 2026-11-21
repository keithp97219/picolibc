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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "../is-qemu.h"

/*
 * setrlimit changes soft limits up to the hard limit, lowers hard
 * limits irreversibly and rejects bad requests. Every change is made
 * in a child, so the test's own limits stay as they were. Resources
 * whose hard limit is below 2 units are skipped.
 *
 * A stack limit below what the stack already holds may take effect at
 * once (FreeBSD unmaps the stack beyond it), so the stack limits are
 * in units of STACK_UNIT bytes, which the child's stack stays within;
 * other limits are in units of 1.
 */

static const int resources[] = {
    RLIMIT_CORE, RLIMIT_CPU, RLIMIT_DATA, RLIMIT_FSIZE, RLIMIT_NOFILE, RLIMIT_STACK, RLIMIT_AS,
};

#define NUM_RESOURCE (sizeof(resources) / sizeof(resources[0]))

/* qemu-user doesn't apply these to itself, and reports success without checking */
#define IS_MEM(r)  ((r) == RLIMIT_DATA || (r) == RLIMIT_STACK || (r) == RLIMIT_AS)

#define SKIPPED    77

#define STACK_UNIT ((rlim_t)16 * 1024 * 1024)

/*
 * In the child, with limits that may stop printf from working, a
 * failed check exits with its number
 */
static int step;

#define check(condition)  \
    do {                  \
        step++;           \
        if (!(condition)) \
            _exit(step);  \
    } while (0)

static int
same(int resource, rlim_t cur, rlim_t max)
{
    struct rlimit rl;

    return getrlimit(resource, &rl) == 0 && rl.rlim_cur == cur && rl.rlim_max == max;
}

static void
child(int resource, int qemu)
{
    struct rlimit old, rl;
    int           relaxed = qemu && IS_MEM(resource);
    rlim_t        u = resource == RLIMIT_STACK ? STACK_UNIT : 1;
    int           ret;

    check(getrlimit(resource, &old) == 0);
    if (old.rlim_max != RLIM_INFINITY && old.rlim_max < 2 * u)
        _exit(SKIPPED);

    /* The soft limit can be anything up to the hard limit */
    rl.rlim_max = old.rlim_max;
    rl.rlim_cur = u;
    check(setrlimit(resource, &rl) == 0);
    check(relaxed || same(resource, u, old.rlim_max));
    rl.rlim_cur = old.rlim_max;
    check(setrlimit(resource, &rl) == 0);
    check(relaxed || same(resource, old.rlim_max, old.rlim_max));

    /* A soft limit above the hard limit is EINVAL, and changes nothing */
    rl.rlim_cur = 2 * u;
    rl.rlim_max = u;
    errno = 0;
    ret = setrlimit(resource, &rl);
    check(relaxed || (ret == -1 && errno == EINVAL));
    check(relaxed || same(resource, old.rlim_max, old.rlim_max));

    if (old.rlim_max == RLIM_INFINITY) {
        /* RLIM_INFINITY is larger than any other value, even where rlim_t has larger ones */
        rl.rlim_cur = RLIM_INFINITY;
        rl.rlim_max = 2 * u;
        errno = 0;
        ret = setrlimit(resource, &rl);
        check(relaxed || (ret == -1 && errno == EINVAL));
        if ((rlim_t)RLIM_INFINITY + 1 != 0) {
            rl.rlim_cur = (rlim_t)RLIM_INFINITY + 1;
            rl.rlim_max = RLIM_INFINITY;
            check(setrlimit(resource, &rl) == 0);
            rl.rlim_cur = RLIM_INFINITY;
            check(setrlimit(resource, &rl) == 0);
        }
    }

    /* Lowering the hard limit is allowed */
    rl.rlim_cur = u;
    rl.rlim_max = 2 * u;
    check(setrlimit(resource, &rl) == 0);
    check(relaxed || same(resource, u, 2 * u));

    /* Raising it again takes privilege */
    rl.rlim_max = 3 * u;
    errno = 0;
    ret = setrlimit(resource, &rl);
    if (geteuid() != 0) {
        check(relaxed || (ret == -1 && errno == EPERM));
        check(relaxed || same(resource, u, 2 * u));
    }

    /* An invalid resource is EINVAL */
    errno = 0;
    check(setrlimit(9999, &rl) == -1 && errno == EINVAL);
    _exit(0);
}

int
main(void)
{
    size_t r;
    int    qemu, status, skipped = 0;
    pid_t  pid;

    qemu = is_qemu();
    if (qemu)
        printf("Running under QEMU, tests relaxed\n");

    for (r = 0; r < NUM_RESOURCE; r++) {
        pid = fork();
        if (pid < 0) {
            printf("fork failed\n");
            return 1;
        }
        if (pid == 0)
            child(resources[r], qemu);
        if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status)) {
            printf("resource %d: child failed, status %#x\n", resources[r], status);
            return 1;
        }
        if (WEXITSTATUS(status) == SKIPPED) {
            printf("resource %d: hard limit below 2 units, skipped\n", resources[r]);
            skipped++;
        } else if (WEXITSTATUS(status) != 0) {
            printf("resource %d: check %d failed\n", resources[r], WEXITSTATUS(status));
            return 1;
        }
    }
    if (skipped == (int)NUM_RESOURCE)
        return 77;
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
