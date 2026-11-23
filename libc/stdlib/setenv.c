/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution.
 * Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#define _DEFAULT_SOURCE
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/lock.h>
#include "local.h"

extern char **environ;

/*
 * The environ array allocated here, if environ still points at it,
 * and the entries setenv allocated, so that they can be reused or
 * freed when replaced. Everything else, the initial environment, the
 * strings given to putenv and any array the application assigned to
 * environ, belongs to someone else and is left alone.
 */
static char **env_array;
static char **env_owned;
static size_t env_owned_len, env_owned_size;

/* Whether setenv allocated entry; the caller holds the lock */
static bool
__env_owned(const char *entry)
{
    size_t i;

    for (i = 0; i < env_owned_len; i++)
        if (env_owned[i] == entry)
            return true;
    return false;
}

/*
 * Forget an entry setenv allocated, returning whether it was one;
 * the caller holds the lock and frees it after releasing the lock.
 */
bool
__env_disown(const char *entry)
{
    size_t i;

    for (i = 0; i < env_owned_len; i++)
        if (env_owned[i] == entry) {
            env_owned[i] = env_owned[--env_owned_len];
            return true;
        }
    return false;
}

/*
 * Make entry, "name=value" with a name of l_name bytes, the
 * environment's definition of the name, replacing an existing one
 * when overwrite is set. 'owned' says that setenv allocated entry.
 * *used says whether entry went in. Allocation happens without the
 * lock (malloc and free take it), so the work starts over if the
 * environment changes meanwhile.
 */
int
__env_put(const char *name, size_t l_name, char *entry, bool owned, bool overwrite, bool *used)
{
    char **new_environ;
    char **new_owned;
    size_t new_owned_size;
    size_t offset = 0;
    size_t n_environ = 0;
    char  *old_entry = NULL;
    char **old_environ = NULL;
    char **old_owned = NULL;

    *used = false;
    for (;;) {
        uint32_t old_sequence;
        bool     found;
        bool     need_owned;

        new_environ = NULL;
        new_owned = NULL;
        new_owned_size = 0;

        __LIBC_LOCK();
        old_sequence = __environ_sequence;
        found = __findenv_n(name, l_name, &offset) != NULL;
        if (found && !overwrite) {
            __LIBC_UNLOCK();
            return 0;
        }
        need_owned = owned && env_owned_len == env_owned_size;
        if (found && !need_owned)
            break;

        if (!found) {
            char **env;

            n_environ = 0;
            if (environ)
                for (env = environ; *env; ++env)
                    n_environ++;
        }
        if (need_owned)
            new_owned_size = env_owned_size ? env_owned_size * 2 : 8;
        __LIBC_UNLOCK();

        if (!found)
            new_environ = malloc((n_environ + 2) * sizeof(char *));
        if (need_owned)
            new_owned = malloc(new_owned_size * sizeof(char *));
        if ((!found && !new_environ) || (need_owned && !new_owned)) {
            free(new_environ);
            free(new_owned);
            errno = ENOMEM;
            return -1;
        }

        __LIBC_LOCK();
        if (__environ_sequence == old_sequence)
            break;
        __LIBC_UNLOCK();
        free(new_environ);
        free(new_owned);
    }

    /* Holding the lock, with nothing changed since the search */
    __environ_sequence++;

    if (new_owned) {
        if (env_owned_len)
            memcpy(new_owned, env_owned, env_owned_len * sizeof(char *));
        old_owned = env_owned;
        env_owned = new_owned;
        env_owned_size = new_owned_size;
    }

    if (new_environ) {
        if (n_environ)
            memcpy(new_environ, environ, n_environ * sizeof(char *));
        new_environ[n_environ] = entry;
        new_environ[n_environ + 1] = NULL;
        if (env_array && environ == env_array)
            old_environ = environ;
        environ = env_array = new_environ;
    } else {
        old_entry = environ[offset];
        environ[offset] = entry;
        if (!__env_disown(old_entry))
            old_entry = NULL;
    }

    if (owned)
        env_owned[env_owned_len++] = entry;

    __LIBC_UNLOCK();

    /* free takes the lock too, so release the old storage afterwards */
    free(old_environ);
    free(old_owned);
    free(old_entry);

    *used = true;
    return 0;
}

/*
 * setenv --
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */

int
setenv(const char *name, const char *value, int rewrite)
{
    char  *cur_entry;
    char  *new_entry;
    size_t offset;
    size_t l_value, l_name;
    bool   used;

    /* Name cannot be NULL, empty, or contain an equal sign.  */
    if (name == NULL || name[0] == '\0' || strchr(name, '=')) {
        errno = EINVAL;
        return -1;
    }

    l_name = strlen(name);
    l_value = strlen(value);

    /* An existing entry that setenv allocated may have room for the value */
    __LIBC_LOCK();
    cur_entry = __findenv(name, &offset);
    if (cur_entry) {
        if (!rewrite) {
            __LIBC_UNLOCK();
            return 0;
        }
        if (__env_owned(environ[offset]) && strlen(cur_entry) >= l_value) {
            strcpy(cur_entry, value);
            __environ_sequence++;
            __LIBC_UNLOCK();
            return 0;
        }
    }
    __LIBC_UNLOCK();

    new_entry = malloc(l_name + 1 + l_value + 1);
    if (!new_entry)
        return -1;

    char *eq = stpcpy(new_entry, name);
    *eq++ = '=';
    strcpy(eq, value);

    if (__env_put(name, l_name, new_entry, true, rewrite != 0, &used) < 0) {
        free(new_entry);
        return -1;
    }
    if (!used)
        free(new_entry);
    return 0;
}
