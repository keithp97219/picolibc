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
 * The current text domain, and each domain's directory and codeset
 * bindings. picolibc has no message catalogs, so these are only
 * recorded and reported back.
 */

#define _POSIX_C_SOURCE 202405L
#include <errno.h>
#include <libintl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/lock.h>

#define DEFAULT_DOMAIN    "messages"
#define DEFAULT_DIRECTORY "/usr/share/locale"

struct binding {
    struct binding *next;
    char           *dirname;
    char           *codeset;
    char            domainname[];
};

static char           *current_domain;
static struct binding *bindings;

/*
 * malloc shares the libc lock, so allocate before taking it and free
 * after releasing it.
 */

char *
textdomain(const char *domainname)
{
    char *copy = NULL;
    char *old = NULL;
    char *ret;

    if (domainname != NULL && *domainname != '\0' && strcmp(domainname, DEFAULT_DOMAIN) != 0) {
        copy = strdup(domainname);
        if (copy == NULL)
            return NULL;
    }

    __LIBC_LOCK();
    if (domainname != NULL) {
        old = current_domain;
        current_domain = copy;
    }
    ret = current_domain ? current_domain : (char *)DEFAULT_DOMAIN;
    __LIBC_UNLOCK();

    free(old);
    return ret;
}

static struct binding *
find_binding(const char *domainname)
{
    struct binding *b;

    for (b = bindings; b; b = b->next)
        if (strcmp(b->domainname, domainname) == 0)
            return b;
    return NULL;
}

/* Set or report one of the domain's bindings */
static char *
bind_value(const char *domainname, const char *value, size_t offset, const char *dflt)
{
    struct binding *b, *fresh = NULL;
    char           *copy = NULL;
    char           *old = NULL;
    char           *ret;

    /* No domain: no change, and errno is left alone */
    if (domainname == NULL || *domainname == '\0')
        return NULL;

    /* On failure, the existing binding stays */
    if (value) {
        size_t len = strlen(domainname) + 1;

        copy = strdup(value);
        fresh = calloc(1, sizeof(*fresh) + len);
        if (copy == NULL || fresh == NULL) {
            free(copy);
            free(fresh);
            return NULL;
        }
        memcpy(fresh->domainname, domainname, len);
    }

    __LIBC_LOCK();
    b = find_binding(domainname);
    if (b == NULL && fresh != NULL) {
        b = fresh;
        fresh = NULL;
        b->next = bindings;
        bindings = b;
    }
    if (b == NULL) {
        ret = (char *)dflt;
    } else {
        char **slot = (char **)((char *)b + offset);

        if (copy) {
            old = *slot;
            *slot = copy;
        }
        ret = *slot ? *slot : (char *)dflt;
    }
    __LIBC_UNLOCK();

    free(old);
    free(fresh);
    return ret;
}

char *
bindtextdomain(const char *domainname, const char *dirname)
{
    return bind_value(domainname, dirname, offsetof(struct binding, dirname), DEFAULT_DIRECTORY);
}

char *
bind_textdomain_codeset(const char *domainname, const char *codeset)
{
    /* With no codeset bound, report none */
    return bind_value(domainname, codeset, offsetof(struct binding, codeset), NULL);
}
