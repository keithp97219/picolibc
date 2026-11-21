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
#if defined(__PICOLIBC_POSIX_SIGNALS) && _POSIX_MAPPED_FILES > 0 && _POSIX_MEMORY_PROTECTION > 0 \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Faults deliver the signal-specific si_code values and the faulting address */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static sigjmp_buf            env;
static volatile sig_atomic_t caught;
static int                   code;
static void                 *addr;

static void
handler(int sig, siginfo_t *si, void *context)
{
    (void)context;
    caught = sig;
    code = si->si_code;
    addr = si->si_addr;
    siglongjmp(env, 1);
}

/* Reads *p, returning the signal it raised */
static int
fault(volatile char *p)
{
    caught = 0;
    code = 0;
    addr = NULL;
    if (sigsetjmp(env, 1) == 0)
        (void)*p;
    return caught;
}

int
main(void)
{
    struct sigaction act;
    long             page = sysconf(_SC_PAGESIZE);
    char            *p;
    char             name[] = "/tmp/test-si_code-XXXXXX";
    int              fd;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGSEGV, &act, NULL) == 0, "sigaction SIGSEGV");
    check(sigaction(SIGBUS, &act, NULL) == 0, "sigaction SIGBUS");

    /* SEGV_ACCERR: a mapped page without access */
    p = mmap(NULL, 2 * page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap");
    check(fault(p + 1) == SIGSEGV, "SIGSEGV for a PROT_NONE page");
    check(code == SEGV_ACCERR, "SEGV_ACCERR");
    check(addr == p + 1, "si_addr of the access fault");

    /* SEGV_MAPERR: an unmapped page */
    check(munmap(p + page, page) == 0, "munmap");
    check(fault(p + page + 2) == SIGSEGV, "SIGSEGV for an unmapped page");
    check(code == SEGV_MAPERR, "SEGV_MAPERR");
    check(addr == p + page + 2, "si_addr of the mapping fault");
    check(munmap(p, page) == 0, "munmap");

    /* BUS_ADRERR: a file mapping past the end of the file */
    fd = mkstemp(name);
    check(fd >= 0, "mkstemp");
    unlink(name);
    check(ftruncate(fd, page) == 0, "ftruncate");
    p = mmap(NULL, 2 * page, PROT_READ, MAP_SHARED, fd, 0);
    check(p != MAP_FAILED, "mmap file");
    check(fault(p) == 0, "page within the file");
    check(fault(p + page + 3) == SIGBUS, "SIGBUS past the end of the file");
    check(code == BUS_ADRERR, "BUS_ADRERR");
    check(addr == p + page + 3, "si_addr of the bus error");
    munmap(p, 2 * page);
    close(fd);

    /* A signal sent by kill has a non-signal-specific code */
    caught = 0;
    if (sigsetjmp(env, 1) == 0)
        kill(getpid(), SIGSEGV);
    check(caught == SIGSEGV && code == SI_USER, "SI_USER for SIGSEGV from kill");

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
