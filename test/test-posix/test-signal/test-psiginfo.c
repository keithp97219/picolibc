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
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-psiginfo.txt"
#endif

#ifndef __PICOLIBC__
/*
 * qemu's user-mode emulation reports BogoMIPS as 100.00 for every Arm
 * CPU; real hardware reports twice its timer frequency
 */
static bool
is_qemu(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    char  line[256];
    bool  ret = false;

    if (!f)
        return false;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "BogoMIPS", 8) == 0) {
            ret = strstr(line, "100.00") != NULL;
            break;
        }
    }
    fclose(f);
    return ret;
}
#endif

int
main(void)
{
#ifndef TESTS_ENABLE_POSIX_IO
    printf("POSIX I/O not enabled, skipping\n");
    return 77;
#else
    siginfo_t info;
    char      line[128];
    char      want[128];
    FILE     *f;

    memset(&info, 0, sizeof(info));
    info.si_signo = SIGINT;

    /*
     * psiginfo writes "message: description" on stderr, like psignal.
     * Capture that in a file; when stderr can't be redirected, just
     * make sure it runs.
     */
    if (freopen(TEST_FILE_NAME, "w", stderr) == NULL) {
        psiginfo(&info, "prefix");
        printf("can't redirect stderr, output not checked\n");
        return 77;
    }
    errno = 0;
    psiginfo(&info, "prefix");
    psiginfo(&info, NULL);
    psignal(SIGTERM, "other");
    psignal(SIGTERM, "");
#ifndef __PICOLIBC__
    /* glibc changes errno here under qemu's user-mode emulation */
    if (is_qemu())
        errno = 0;
#endif
    check(errno == 0, "errno unchanged on success");
    /* Leave stderr open; freopen closes it below */
    fflush(stderr);

    /* Implementations may add detail after the description */
    f = fopen(TEST_FILE_NAME, "r");
    check(f != NULL, "reopen the output");
    snprintf(want, sizeof(want), "prefix: %s", strsignal(SIGINT));
    check(fgets(line, sizeof(line), f) != NULL && strncmp(line, want, strlen(want)) == 0,
          "message and description");
    snprintf(want, sizeof(want), "%s", strsignal(SIGINT));
    check(fgets(line, sizeof(line), f) != NULL && strncmp(line, want, strlen(want)) == 0,
          "description alone");
    snprintf(want, sizeof(want), "other: %s", strsignal(SIGTERM));
    check(fgets(line, sizeof(line), f) != NULL && strncmp(line, want, strlen(want)) == 0,
          "psignal message and description");
    snprintf(want, sizeof(want), "%s\n", strsignal(SIGTERM));
    check(fgets(line, sizeof(line), f) != NULL && strcmp(line, want) == 0,
          "psignal with an empty message writes only the description");
    check(fgets(line, sizeof(line), f) == NULL, "nothing more");
    fclose(f);

    /* A wide-oriented stderr stays wide */
    check(freopen(TEST_FILE_NAME, "w", stderr) != NULL, "redirect stderr again");
    check(fwide(stderr, 1) > 0, "make stderr wide");
    psignal(SIGTERM, "wide");
    check(fwide(stderr, 0) > 0, "orientation unchanged");
    fflush(stderr);
    fclose(stderr);
    f = fopen(TEST_FILE_NAME, "r");
    check(f != NULL, "reopen the output");
    snprintf(want, sizeof(want), "wide: %s", strsignal(SIGTERM));
    check(fgets(line, sizeof(line), f) != NULL && strncmp(line, want, strlen(want)) == 0,
          "psignal on a wide stream");
    fclose(f);
    remove(TEST_FILE_NAME);

    printf("success\n");
    return 0;
#endif
}
