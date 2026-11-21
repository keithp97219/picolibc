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
#if defined(__PICOLIBC_POSIX_SINGLE_PROCESS)

#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

/*
 * confstr returns the length it needs and truncates to the buffer,
 * knows every name POSIX lists, and lists the width-restricted
 * programming environments
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* glibc has only the V7 names, and not the V7 threads ones */
#ifdef _CS_POSIX_V8_WIDTH_RESTRICTED_ENVS
#define ENV_PREFIX      "POSIX_V8_"
#define CS_WIDTH        _CS_POSIX_V8_WIDTH_RESTRICTED_ENVS
#define SC_ILP32_OFF32  _SC_V8_ILP32_OFF32
#define SC_ILP32_OFFBIG _SC_V8_ILP32_OFFBIG
#define SC_LP64_OFF64   _SC_V8_LP64_OFF64
#define SC_LPBIG_OFFBIG _SC_V8_LPBIG_OFFBIG
#else
#define ENV_PREFIX      "POSIX_V7_"
#define CS_WIDTH        _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS
#define SC_ILP32_OFF32  _SC_V7_ILP32_OFF32
#define SC_ILP32_OFFBIG _SC_V7_ILP32_OFFBIG
#define SC_LP64_OFF64   _SC_V7_LP64_OFF64
#define SC_LPBIG_OFFBIG _SC_V7_LPBIG_OFFBIG
#endif

static const struct {
    const char *name;
    int         sc;
} envs[] = {
    { "ILP32_OFF32",  SC_ILP32_OFF32  },
    { "ILP32_OFFBIG", SC_ILP32_OFFBIG },
    { "LP64_OFF64",   SC_LP64_OFF64   },
    { "LPBIG_OFFBIG", SC_LPBIG_OFFBIG },
};

#define NENVS (sizeof(envs) / sizeof(envs[0]))

/* The environment this test was compiled in, by its type sizes */
static const char *
this_env(void)
{
    if (sizeof(int) == 4 && sizeof(long) == 4 && sizeof(void *) == 4)
        return sizeof(off_t) == 4 ? "ILP32_OFF32" : "ILP32_OFFBIG";
    if (sizeof(int) == 4 && sizeof(long) == 8 && sizeof(void *) == 8 && sizeof(off_t) == 8)
        return "LP64_OFF64";
    return "LPBIG_OFFBIG";
}

#define FITS(t) (sizeof(t) <= sizeof(long))

int
main(void)
{
    char   buf[256];
    size_t len, i;

    len = confstr(_CS_PATH, NULL, 0);
    check(len > 1, "confstr length");
    check(confstr(_CS_PATH, buf, sizeof(buf)) == len && strlen(buf) == len - 1, "confstr");
    check(strstr(buf, "/bin") != NULL, "confstr _CS_PATH");
    check(confstr(_CS_PATH, buf, 2) == len && strlen(buf) == 1, "confstr truncates");
    errno = 0;
    check(confstr(-1, buf, sizeof(buf)) == 0 && errno == EINVAL, "confstr bad name");

    /* Every name POSIX lists is valid: a value, or 0 with errno unchanged */
    static const int names[] = {
        _CS_PATH,
#ifdef _CS_POSIX_V8_ILP32_OFF32_CFLAGS
        _CS_POSIX_V8_ILP32_OFF32_CFLAGS,
        _CS_POSIX_V8_ILP32_OFF32_LDFLAGS,
        _CS_POSIX_V8_ILP32_OFF32_LIBS,
        _CS_POSIX_V8_ILP32_OFFBIG_CFLAGS,
        _CS_POSIX_V8_ILP32_OFFBIG_LDFLAGS,
        _CS_POSIX_V8_ILP32_OFFBIG_LIBS,
        _CS_POSIX_V8_LP64_OFF64_CFLAGS,
        _CS_POSIX_V8_LP64_OFF64_LDFLAGS,
        _CS_POSIX_V8_LP64_OFF64_LIBS,
        _CS_POSIX_V8_LPBIG_OFFBIG_CFLAGS,
        _CS_POSIX_V8_LPBIG_OFFBIG_LDFLAGS,
        _CS_POSIX_V8_LPBIG_OFFBIG_LIBS,
        _CS_POSIX_V8_THREADS_CFLAGS,
        _CS_POSIX_V8_THREADS_LDFLAGS,
        _CS_POSIX_V8_WIDTH_RESTRICTED_ENVS,
        _CS_V8_ENV,
#elif !defined(__GLIBC__)
#error "_CS_POSIX_V8_* missing"
#endif
        _CS_POSIX_V7_ILP32_OFF32_CFLAGS,
        _CS_POSIX_V7_ILP32_OFF32_LDFLAGS,
        _CS_POSIX_V7_ILP32_OFF32_LIBS,
        _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,
        _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,
        _CS_POSIX_V7_ILP32_OFFBIG_LIBS,
        _CS_POSIX_V7_LP64_OFF64_CFLAGS,
        _CS_POSIX_V7_LP64_OFF64_LDFLAGS,
        _CS_POSIX_V7_LP64_OFF64_LIBS,
        _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,
        _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,
        _CS_POSIX_V7_LPBIG_OFFBIG_LIBS,
#ifdef _CS_POSIX_V7_THREADS_CFLAGS
        _CS_POSIX_V7_THREADS_CFLAGS,
        _CS_POSIX_V7_THREADS_LDFLAGS,
#elif !defined(__GLIBC__)
#error "_CS_POSIX_V7_THREADS_* missing"
#endif
        _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS,
        _CS_V7_ENV,
    };
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        errno = 0;
        len = confstr(names[i], NULL, 0);
        check(len > 0 || errno == 0, "a POSIX name is valid");
        if (len > 0 && len <= sizeof(buf)) {
            memset(buf, 'z', sizeof(buf));
            check(confstr(names[i], buf, sizeof(buf)) == len && strlen(buf) == len - 1,
                  "the value fits the size returned");
        }
    }

    /* Each width-restricted environment is one sysconf supports */
    len = confstr(CS_WIDTH, buf, sizeof(buf));
    check(len > 0 && len <= sizeof(buf), "width-restricted environments");
    {
        char  *line = buf, *end;
        int    found = 0;
        size_t j;

        while (*line) {
            end = strchr(line, '\n');
            if (end)
                *end = '\0';
            check(strncmp(line, ENV_PREFIX, strlen(ENV_PREFIX)) == 0, "environment name prefix");
            for (j = 0; j < NENVS; j++)
                if (strcmp(line + strlen(ENV_PREFIX), envs[j].name) == 0)
                    break;
            check(j < NENVS, "a known environment");
            check(sysconf(envs[j].sc) != -1, "a supported environment");
            if (strcmp(envs[j].name, this_env()) == 0)
                found = 1;
            if (!end)
                break;
            line = end + 1;
        }
        /* The environment this was compiled in qualifies when its types fit */
        for (j = 0; j < NENVS; j++)
            if (strcmp(envs[j].name, this_env()) == 0 && sysconf(envs[j].sc) > 0 && FITS(blksize_t)
                && FITS(cc_t) && FITS(mode_t) && FITS(nfds_t) && FITS(pid_t) && FITS(ptrdiff_t)
                && FITS(size_t) && FITS(speed_t) && FITS(ssize_t) && FITS(suseconds_t)
                && FITS(tcflag_t) && FITS(wchar_t) && FITS(wint_t))
                check(found, "this environment is width-restricted");
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
