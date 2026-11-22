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
/* posix_openpt, grantpt, unlockpt and ptsname are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_DEVICE_SPECIFIC

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/*
 * The speed functions store exactly what they are given, and tcsetattr
 * passes the speeds to the terminal, which a pseudo-terminal keeps
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Opens a pseudo-terminal pair, or skips the test when there are none */
static int
open_pty(int *subsidiary)
{
    char *name;
    int   manager = posix_openpt(O_RDWR | O_NOCTTY);

    if (manager < 0) {
        printf("no pseudo-terminals, skipping\n");
        exit(77);
    }
    check(grantpt(manager) == 0, "grantpt");
    check(unlockpt(manager) == 0, "unlockpt");
    name = ptsname(manager);
    check(name != NULL, "ptsname");
    *subsidiary = open(name, O_RDWR | O_NOCTTY);
    check(*subsidiary >= 0, "open subsidiary");
    return manager;
}

static const speed_t speeds[] = { B0,   B50,   B75,   B110,  B134,  B150,  B200,   B300,
                                  B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400 };

#define NSPEEDS (sizeof(speeds) / sizeof(speeds[0]))

int
main(void)
{
    int            subsidiary, manager = open_pty(&subsidiary);
    struct termios t, copy, got;
    size_t         i;

    /* Each function sets and gets exactly its own speed */
    check(tcgetattr(subsidiary, &t) == 0, "tcgetattr");
    for (i = 0; i < NSPEEDS; i++) {
        speed_t other = speeds[NSPEEDS - 1 - i];

        check(cfsetospeed(&t, other) == 0, "cfsetospeed");
        check(cfsetispeed(&t, speeds[i]) == 0, "cfsetispeed");
        check(cfgetispeed(&t) == speeds[i], "cfgetispeed returns what cfsetispeed set");
        check(cfgetospeed(&t) == other, "cfsetispeed leaves the output speed");
        check(cfsetospeed(&t, speeds[i]) == 0, "cfsetospeed");
        check(cfgetospeed(&t) == speeds[i], "cfgetospeed returns what cfsetospeed set");
        check(cfgetispeed(&t) == speeds[i], "cfsetospeed leaves the input speed");
        check(cfsetispeed(&t, other) == 0 && cfgetospeed(&t) == speeds[i],
              "cfsetispeed leaves the output speed");
    }

    /* Nothing changes on the terminal until tcsetattr */
    check(tcgetattr(subsidiary, &t) == 0, "tcgetattr");
    check(cfsetospeed(&t, cfgetospeed(&t) == B9600 ? B4800 : B9600) == 0, "cfsetospeed");
    check(tcgetattr(subsidiary, &got) == 0, "tcgetattr again");
    check(cfgetospeed(&got) != cfgetospeed(&t), "cfsetospeed alone doesn't set the terminal");

    /* tcsetattr sets the speeds, and leaves its argument alone */
    check(cfsetospeed(&t, B9600) == 0 && cfsetispeed(&t, B9600) == 0, "set speeds");
    copy = t;
    check(tcsetattr(subsidiary, TCSANOW, &t) == 0, "tcsetattr");
    check(memcmp(&copy, &t, sizeof(t)) == 0, "tcsetattr doesn't change its argument");
    memset(&got, 0, sizeof(got));
    check(tcgetattr(subsidiary, &got) == 0, "tcgetattr after tcsetattr");
    check(cfgetospeed(&got) == B9600, "output speed set");
    check(cfgetispeed(&got) == B9600, "input speed set");

    /* An input speed of zero means the output speed */
    check(cfsetospeed(&t, B2400) == 0 && cfsetispeed(&t, B0) == 0, "B0 input");
    check(tcsetattr(subsidiary, TCSANOW, &t) == 0, "tcsetattr B0 input");
    check(tcgetattr(subsidiary, &got) == 0, "tcgetattr B0 input");
    check(cfgetospeed(&got) == B2400, "output speed with B0 input");
    check(cfgetispeed(&got) == B2400, "B0 input speed is the output speed");

    /* Split speeds are either both set or neither */
    check(cfsetospeed(&t, B19200) == 0 && cfsetispeed(&t, B1200) == 0, "split speeds");
    check(tcsetattr(subsidiary, TCSADRAIN, &t) == 0, "tcsetattr split speeds");
    check(tcgetattr(subsidiary, &got) == 0, "tcgetattr split speeds");
    check((cfgetospeed(&got) == B19200 && cfgetispeed(&got) == B1200)
              || (cfgetospeed(&got) == B2400 && cfgetispeed(&got) == B2400),
          "split speeds both set or neither");

    check(close(subsidiary) == 0 && close(manager) == 0, "close");
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
