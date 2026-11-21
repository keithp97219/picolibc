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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* What <errno.h> defines, and errno itself */

#if defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer) || __has_feature(address_sanitizer)
#define SANITIZER_RUNTIME 1
#endif
#endif
#ifndef SANITIZER_RUNTIME
#define SANITIZER_RUNTIME 0
#endif

/* errno is a modifiable lvalue of type int */
#define is_int(e) _Generic((e), int: 1, default: 0)

/* Every name POSIX lists, usable in #if */
#if !(E2BIG > 0 && EACCES > 0 && EADDRINUSE > 0 && EADDRNOTAVAIL > 0 && EAFNOSUPPORT > 0           \
      && EAGAIN > 0 && EALREADY > 0 && EBADF > 0 && EBADMSG > 0 && EBUSY > 0 && ECANCELED > 0      \
      && ECHILD > 0 && ECONNABORTED > 0 && ECONNREFUSED > 0 && ECONNRESET > 0 && EDEADLK > 0       \
      && EDESTADDRREQ > 0 && EDOM > 0 && EDQUOT > 0 && EEXIST > 0 && EFAULT > 0 && EFBIG > 0       \
      && EHOSTUNREACH > 0 && EIDRM > 0 && EILSEQ > 0 && EINPROGRESS > 0 && EINTR > 0 && EINVAL > 0 \
      && EIO > 0 && EISCONN > 0 && EISDIR > 0 && ELOOP > 0 && EMFILE > 0 && EMLINK > 0             \
      && EMSGSIZE > 0 && EMULTIHOP > 0 && ENAMETOOLONG > 0 && ENETDOWN > 0 && ENETRESET > 0        \
      && ENETUNREACH > 0 && ENFILE > 0 && ENOBUFS > 0 && ENODEV > 0 && ENOENT > 0 && ENOEXEC > 0   \
      && ENOLCK > 0 && ENOLINK > 0 && ENOMEM > 0 && ENOMSG > 0 && ENOPROTOOPT > 0 && ENOSPC > 0    \
      && ENOSYS > 0 && ENOTCONN > 0 && ENOTDIR > 0 && ENOTEMPTY > 0 && ENOTRECOVERABLE > 0         \
      && ENOTSOCK > 0 && ENOTSUP > 0 && ENOTTY > 0 && ENXIO > 0 && EOPNOTSUPP > 0 && EOVERFLOW > 0 \
      && EOWNERDEAD > 0 && EPERM > 0 && EPIPE > 0 && EPROTO > 0 && EPROTONOSUPPORT > 0             \
      && EPROTOTYPE > 0 && ERANGE > 0 && EROFS > 0 && ESOCKTNOSUPPORT > 0 && ESPIPE > 0            \
      && ESRCH > 0 && ESTALE > 0 && ETIMEDOUT > 0 && ETXTBSY > 0 && EWOULDBLOCK > 0 && EXDEV > 0)
#error "an error number is missing or not positive"
#endif

#define E(name) { name, #name, is_int(name) }

static const struct {
    int         value;
    const char *name;
    int         is_int;
} errors[] = {
    E(E2BIG),           E(EACCES),          E(EADDRINUSE),   E(EADDRNOTAVAIL), E(EAFNOSUPPORT),
    E(EAGAIN),          E(EALREADY),        E(EBADF),        E(EBADMSG),       E(EBUSY),
    E(ECANCELED),       E(ECHILD),          E(ECONNABORTED), E(ECONNREFUSED),  E(ECONNRESET),
    E(EDEADLK),         E(EDESTADDRREQ),    E(EDOM),         E(EDQUOT),        E(EEXIST),
    E(EFAULT),          E(EFBIG),           E(EHOSTUNREACH), E(EIDRM),         E(EILSEQ),
    E(EINPROGRESS),     E(EINTR),           E(EINVAL),       E(EIO),           E(EISCONN),
    E(EISDIR),          E(ELOOP),           E(EMFILE),       E(EMLINK),        E(EMSGSIZE),
    E(EMULTIHOP),       E(ENAMETOOLONG),    E(ENETDOWN),     E(ENETRESET),     E(ENETUNREACH),
    E(ENFILE),          E(ENOBUFS),         E(ENODEV),       E(ENOENT),        E(ENOEXEC),
    E(ENOLCK),          E(ENOLINK),         E(ENOMEM),       E(ENOMSG),        E(ENOPROTOOPT),
    E(ENOSPC),          E(ENOSYS),          E(ENOTCONN),     E(ENOTDIR),       E(ENOTEMPTY),
    E(ENOTRECOVERABLE), E(ENOTSOCK),        E(ENOTSUP),      E(ENOTTY),        E(ENXIO),
    E(EOPNOTSUPP),      E(EOVERFLOW),       E(EOWNERDEAD),   E(EPERM),         E(EPIPE),
    E(EPROTO),          E(EPROTONOSUPPORT), E(EPROTOTYPE),   E(ERANGE),        E(EROFS),
    E(ESOCKTNOSUPPORT), E(ESPIPE),          E(ESRCH),        E(ESTALE),        E(ETIMEDOUT),
    E(ETXTBSY),         E(EWOULDBLOCK),     E(EXDEV),
};

#define N_ERRORS (sizeof(errors) / sizeof(errors[0]))

/* The pairs POSIX lets share a value */
static int
may_share(int a, int b)
{
    return (a == EAGAIN && b == EWOULDBLOCK) || (a == EWOULDBLOCK && b == EAGAIN)
        || (a == ENOTSUP && b == EOPNOTSUPP) || (a == EOPNOTSUPP && b == ENOTSUP);
}

static char messages[N_ERRORS][256];

int
main(void)
{
    int      ret = 0, *p;
    unsigned i, j;
    int      initial = errno;

    /*
     * Zero at program startup in the initial thread. A sanitizer
     * runtime (clang's -fsanitize=bounds in the -native build) runs
     * before main and leaves its own errno behind, so skip it there
     */
    if (initial != 0 && !SANITIZER_RUNTIME) {
        printf("errno is %d at startup\n", initial);
        ret = 1;
    }

    _Static_assert(is_int(errno), "errno is an int");
    p = &errno;
    *p = ERANGE;
    if (errno != ERANGE) {
        printf("errno isn't the object &errno points to\n");
        ret = 1;
    }
    errno = EDOM;
    errno += 1;
    if (*p != EDOM + 1) {
        printf("errno isn't modifiable\n");
        ret = 1;
    }

    for (i = 0; i < N_ERRORS; i++) {
        if (!errors[i].is_int) {
            printf("%s isn't an int\n", errors[i].name);
            ret = 1;
        }
        /* strerror's buffer may be reused, so copy each message */
        strncpy(messages[i], strerror(errors[i].value), sizeof(messages[i]) - 1);
    }

    /* Distinct values, apart from the pairs that may be equal */
    for (i = 0; i < N_ERRORS; i++)
        for (j = i + 1; j < N_ERRORS; j++) {
            if (errors[i].value == errors[j].value
                && !may_share(errors[i].value, errors[j].value)) {
                printf("%s and %s are both %d\n", errors[i].name, errors[j].name, errors[i].value);
                ret = 1;
            }
            /* Each distinct value has its own message */
            if (errors[i].value != errors[j].value && strcmp(messages[i], messages[j]) == 0) {
                printf("%s and %s share the message \"%s\"\n", errors[i].name, errors[j].name,
                       messages[i]);
                ret = 1;
            }
        }

    /* Successful calls don't set errno to zero */
    errno = EINVAL;
    (void)strtol("12", NULL, 10);
    (void)strerror(EIO);
    free(malloc(16));
    (void)getenv("PATH");
    (void)snprintf(messages[0], sizeof(messages[0]), "%d", 1);
    if (errno == 0) {
        printf("a successful call set errno to 0\n");
        ret = 1;
    }

    if (ret == 0)
        printf("success\n");
    return ret;
}
