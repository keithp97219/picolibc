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
#include "../test-flags.h"
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * strerror, strerror_l and strerror_r: a message for every error
 * number, POSIX's and the others Linux returns; zero and unknown
 * numbers; ERANGE for a short buffer; errno left alone; and no other
 * function overwriting strerror's string.
 */

#define E(name) { name, #name }

static const struct {
    int         value;
    const char *name;
} errors[] = {
    /* POSIX.1-2024 */
    E(E2BIG),        E(EACCES),       E(EADDRINUSE),      E(EADDRNOTAVAIL),
    E(EAFNOSUPPORT), E(EAGAIN),       E(EALREADY),        E(EBADF),
    E(EBADMSG),      E(EBUSY),        E(ECANCELED),       E(ECHILD),
    E(ECONNABORTED), E(ECONNREFUSED), E(ECONNRESET),      E(EDEADLK),
    E(EDESTADDRREQ), E(EDOM),         E(EDQUOT),          E(EEXIST),
    E(EFAULT),       E(EFBIG),        E(EHOSTUNREACH),    E(EIDRM),
    E(EILSEQ),       E(EINPROGRESS),  E(EINTR),           E(EINVAL),
    E(EIO),          E(EISCONN),      E(EISDIR),          E(ELOOP),
    E(EMFILE),       E(EMLINK),       E(EMSGSIZE),        E(EMULTIHOP),
    E(ENAMETOOLONG), E(ENETDOWN),     E(ENETRESET),       E(ENETUNREACH),
    E(ENFILE),       E(ENOBUFS),      E(ENODEV),          E(ENOENT),
    E(ENOEXEC),      E(ENOLCK),       E(ENOLINK),         E(ENOMEM),
    E(ENOMSG),       E(ENOPROTOOPT),  E(ENOSPC),          E(ENOSYS),
    E(ENOTCONN),     E(ENOTDIR),      E(ENOTEMPTY),       E(ENOTRECOVERABLE),
    E(ENOTSOCK),     E(ENOTSUP),      E(ENOTTY),          E(ENXIO),
    E(EOPNOTSUPP),   E(EOVERFLOW),    E(EOWNERDEAD),      E(EPERM),
    E(EPIPE),        E(EPROTO),       E(EPROTONOSUPPORT), E(EPROTOTYPE),
    E(ERANGE),       E(EROFS),        E(ESOCKTNOSUPPORT), E(ESPIPE),
    E(ESRCH),        E(ESTALE),       E(ETIMEDOUT),       E(ETXTBSY),
    E(EWOULDBLOCK),  E(EXDEV),
/* The others Linux can return, where <errno.h> names them (E* is reserved) */
#ifdef EADV
    E(EADV),
#endif
#ifdef EBADE
    E(EBADE),
#endif
#ifdef EBADFD
    E(EBADFD),
#endif
#ifdef EBADR
    E(EBADR),
#endif
#ifdef EBADRQC
    E(EBADRQC),
#endif
#ifdef EBADSLT
    E(EBADSLT),
#endif
#ifdef EBFONT
    E(EBFONT),
#endif
#ifdef ECHRNG
    E(ECHRNG),
#endif
#ifdef ECOMM
    E(ECOMM),
#endif
#ifdef EDEADLOCK
    E(EDEADLOCK),
#endif
#ifdef EDOTDOT
    E(EDOTDOT),
#endif
#ifdef EHOSTDOWN
    E(EHOSTDOWN),
#endif
#ifdef EHWPOISON
    E(EHWPOISON),
#endif
#ifdef EISNAM
    E(EISNAM),
#endif
#ifdef EKEYEXPIRED
    E(EKEYEXPIRED),
#endif
#ifdef EKEYREJECTED
    E(EKEYREJECTED),
#endif
#ifdef EKEYREVOKED
    E(EKEYREVOKED),
#endif
#ifdef EL2HLT
    E(EL2HLT),
#endif
#ifdef EL2NSYNC
    E(EL2NSYNC),
#endif
#ifdef EL3HLT
    E(EL3HLT),
#endif
#ifdef EL3RST
    E(EL3RST),
#endif
#ifdef ELIBACC
    E(ELIBACC),
#endif
#ifdef ELIBBAD
    E(ELIBBAD),
#endif
#ifdef ELIBEXEC
    E(ELIBEXEC),
#endif
#ifdef ELIBMAX
    E(ELIBMAX),
#endif
#ifdef ELIBSCN
    E(ELIBSCN),
#endif
#ifdef ELNRNG
    E(ELNRNG),
#endif
#ifdef EMEDIUMTYPE
    E(EMEDIUMTYPE),
#endif
#ifdef ENOANO
    E(ENOANO),
#endif
#ifdef ENOCSI
    E(ENOCSI),
#endif
#ifdef ENODATA
    E(ENODATA),
#endif
#ifdef ENOKEY
    E(ENOKEY),
#endif
#ifdef ENOMEDIUM
    E(ENOMEDIUM),
#endif
#ifdef ENONET
    E(ENONET),
#endif
#ifdef ENOPKG
    E(ENOPKG),
#endif
#ifdef ENOSR
    E(ENOSR),
#endif
#ifdef ENOSTR
    E(ENOSTR),
#endif
#ifdef ENOTBLK
    E(ENOTBLK),
#endif
#ifdef ENOTNAM
    E(ENOTNAM),
#endif
#ifdef ENOTUNIQ
    E(ENOTUNIQ),
#endif
#ifdef EPFNOSUPPORT
    E(EPFNOSUPPORT),
#endif
#ifdef EREMCHG
    E(EREMCHG),
#endif
#ifdef EREMOTE
    E(EREMOTE),
#endif
#ifdef EREMOTEIO
    E(EREMOTEIO),
#endif
#ifdef ERFKILL
    E(ERFKILL),
#endif
#ifdef ESHUTDOWN
    E(ESHUTDOWN),
#endif
#ifdef ESRMNT
    E(ESRMNT),
#endif
#ifdef ESTRPIPE
    E(ESTRPIPE),
#endif
#ifdef ETIME
    E(ETIME),
#endif
#ifdef ETOOMANYREFS
    E(ETOOMANYREFS),
#endif
#ifdef EUCLEAN
    E(EUCLEAN),
#endif
#ifdef EUNATCH
    E(EUNATCH),
#endif
#ifdef EUSERS
    E(EUSERS),
#endif
#ifdef EXFULL
    E(EXFULL),
#endif
};

#define N_ERRORS (sizeof(errors) / sizeof(errors[0]))

/* Numbers that aren't error numbers */
static const int unknown[] = { -1, -2, INT_MIN, INT_MAX, 4096, 100000 };

#define N_UNKNOWN (sizeof(unknown) / sizeof(unknown[0]))

#define MSG_MAX   256

static char messages[N_ERRORS][MSG_MAX];
static char unknown_prefix[MSG_MAX];
static int  ret;

#define fail(...)            \
    do {                     \
        printf(__VA_ARGS__); \
        printf("\n");        \
        ret = 1;             \
    } while (0)

/* errno must not change when these succeed */
#define MARK EDOM

static int
is_unknown(const char *msg)
{
    return strncmp(msg, unknown_prefix, strlen(unknown_prefix)) == 0;
}

static void
check_errno(const char *what, int errnum)
{
    if (errno != MARK)
        fail("%s(%d) changed errno to %d", what, errnum, errno);
    errno = MARK;
}

static void
check_r(int errnum, const char *name, const char *want)
{
    char   buf[MSG_MAX + 8];
    size_t len = strlen(want);
    size_t n;
    int    r;

    /* Exactly enough room */
    memset(buf, 'x', sizeof(buf));
    r = strerror_r(errnum, buf, len + 1);
    if (r != 0)
        fail("strerror_r(%s) returned %d", name, r);
    else if (strcmp(buf, want) != 0)
        fail("strerror_r(%s) gave \"%s\", strerror \"%s\"", name, buf, want);
    check_errno("strerror_r", errnum);

    /* Too little: ERANGE, and nothing written past buflen */
    for (n = 0; n <= len; n++) {
        memset(buf, 'x', sizeof(buf));
        errno = MARK;
        r = strerror_r(errnum, buf, n);
        if (r != ERANGE)
            fail("strerror_r(%s) with buflen %zu returned %d, not ERANGE", name, n, r);
        if (buf[n] != 'x')
            fail("strerror_r(%s) with buflen %zu wrote past the buffer", name, n);
        errno = MARK;
    }
}

int
main(void)
{
    char    *p, *saved;
    char     copy[MSG_MAX];
    size_t   i, j, len;
    locale_t posix;

    /* What an unknown number looks like, up to any number in it */
    snprintf(unknown_prefix, sizeof(unknown_prefix), "%s", strerror(123456));
    for (len = 0; unknown_prefix[len]; len++)
        if (unknown_prefix[len] == '-'
            || (unknown_prefix[len] >= '0' && unknown_prefix[len] <= '9'))
            break;
    while (len > 0 && unknown_prefix[len - 1] == ' ')
        len--;
    unknown_prefix[len] = '\0';
    if (len == 0)
        fail("strerror(123456) gave \"%s\"", strerror(123456));

    posix = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    if (posix == (locale_t)0) {
        fail("newlocale POSIX");
        return 1;
    }

    errno = MARK;
    for (i = 0; i < N_ERRORS; i++) {
        p = strerror(errors[i].value);
        check_errno("strerror", errors[i].value);
        if (p == NULL) {
            fail("strerror(%s) is NULL", errors[i].name);
            continue;
        }
        /* The message indicates what error occurred */
        if (*p == '\0' || is_unknown(p))
            fail("strerror(%s) is \"%s\"", errors[i].name, p);
        snprintf(messages[i], sizeof(messages[i]), "%s", p);

        /* strerror_l in the POSIX locale gives the same */
        p = strerror_l(errors[i].value, posix);
        check_errno("strerror_l", errors[i].value);
        if (p == NULL || strcmp(p, messages[i]) != 0)
            fail("strerror_l(%s) is \"%s\", strerror \"%s\"", errors[i].name, p ? p : "(null)",
                 messages[i]);
        check_r(errors[i].value, errors[i].name, messages[i]);
    }

    /* Each distinct value has its own message */
    for (i = 0; i < N_ERRORS; i++)
        for (j = i + 1; j < N_ERRORS; j++)
            if (errors[i].value != errors[j].value && strcmp(messages[i], messages[j]) == 0)
                fail("%s and %s share the message \"%s\"", errors[i].name, errors[j].name,
                     messages[i]);

    /* Zero: an empty string, or one saying there was no error */
    p = strerror(0);
    if (p == NULL || is_unknown(p))
        fail("strerror(0) is \"%s\"", p ? p : "(null)");
    else
        for (i = 0; i < N_ERRORS; i++)
            if (strcmp(p, messages[i]) == 0)
                fail("strerror(0) is %s's message", errors[i].name);
    errno = MARK;
    if (strerror_r(0, copy, sizeof(copy)) != 0)
        fail("strerror_r(0) failed");
    check_errno("strerror_r", 0);

    /* Any other int gets a message saying the error is unknown */
    for (i = 0; i < N_UNKNOWN; i++) {
        int r;

        errno = 0;
        p = strerror(unknown[i]);
        if (p == NULL || *p == '\0') {
            fail("strerror(%d) gave no message", unknown[i]);
            continue;
        }
        if (errno != 0 && errno != EINVAL)
            fail("strerror(%d) set errno to %d", unknown[i], errno);
        for (j = 0; j < N_ERRORS; j++)
            if (strcmp(p, messages[j]) == 0)
                fail("strerror(%d) is %s's message", unknown[i], errors[j].name);
        errno = 0;
        p = strerror_l(unknown[i], posix);
        if (p == NULL || *p == '\0')
            fail("strerror_l(%d) gave no message", unknown[i]);
        if (errno != 0 && errno != EINVAL)
            fail("strerror_l(%d) set errno to %d", unknown[i], errno);
        /* strerror_r may fail with EINVAL; either way the buffer holds a message */
        memset(copy, 0, sizeof(copy));
        r = strerror_r(unknown[i], copy, sizeof(copy));
        if (r != 0 && r != EINVAL)
            fail("strerror_r(%d) returned %d", unknown[i], r);
        if (r == 0 && copy[0] == '\0')
            fail("strerror_r(%d) succeeded without a message", unknown[i]);
    }

    /*
     * No other function calls strerror: its string survives calls
     * that report errors
     */
    for (i = 0; i < 2; i++) {
        int errnum = i == 0 ? EINVAL : 100000;

        saved = strerror(errnum);
        snprintf(copy, sizeof(copy), "%s", saved);
        errno = EIO;
        perror("perror, which mustn't call strerror");
        (void)strerror_r(EBADF, messages[0], sizeof(messages[0]));
        (void)strerror_l(ENOENT, posix);
        (void)strsignal(1);
        (void)snprintf(messages[0], sizeof(messages[0]), "%d", 1);
        if (strcmp(saved, copy) != 0)
            fail("strerror(%d)'s string changed to \"%s\"", errnum, saved);
    }

    freelocale(posix);
    if (ret == 0)
        printf("success\n");
    return ret;
}
