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
/* <ndbm.h> is an XSI header */
#define _XOPEN_SOURCE 800
#include <ndbm.h>

/*
 * <ndbm.h>: datum, DBM, the store modes and the prototypes, checked
 * before any other header, so size_t and mode_t have to come from
 * <ndbm.h> itself, and the names it mustn't define in strict mode
 */

static size_t size_check __attribute__((unused));
static mode_t mode_check __attribute__((unused));

/* datum has void *dptr and size_t dsize */
static void **
dptr_check(datum *d)
{
    return &d->dptr;
}
static size_t *
dsize_check(datum *d)
{
    return &d->dsize;
}

/* DBM is a type */
static DBM *dbm_check __attribute__((unused));

/* The store modes differ */
_Static_assert(DBM_INSERT != DBM_REPLACE, "DBM_INSERT and DBM_REPLACE differ");

/* The prototypes */
typedef int             dbm_clearerr_fn(DBM *);
typedef void            dbm_close_fn(DBM *);
typedef int             dbm_delete_fn(DBM *, datum);
typedef int             dbm_error_fn(DBM *);
typedef datum           dbm_fetch_fn(DBM *, datum);
typedef datum           dbm_firstkey_fn(DBM *);
typedef datum           dbm_nextkey_fn(DBM *);
typedef DBM            *dbm_open_fn(const char *, int, mode_t);
typedef int             dbm_store_fn(DBM *, datum, datum, int);

static dbm_clearerr_fn *clearerr_check = dbm_clearerr;
static dbm_close_fn    *close_check = dbm_close;
static dbm_delete_fn   *delete_check = dbm_delete;
static dbm_error_fn    *error_check = dbm_error;
static dbm_fetch_fn    *fetch_check = dbm_fetch;
static dbm_firstkey_fn *firstkey_check = dbm_firstkey;
static dbm_nextkey_fn  *nextkey_check = dbm_nextkey;
static dbm_open_fn     *open_check = dbm_open;
static dbm_store_fn    *store_check = dbm_store;

/* <ndbm.h> reserves only dbm_ and DBM_, so DB is the application's */
static int              DB __attribute__((unused));

/* Nor may it bring in <stdint.h> or the rest of <sys/types.h> */
#ifdef INT8_MAX
#error "<ndbm.h> defines INT8_MAX"
#endif
#ifdef SIZE_MAX
#error "<ndbm.h> defines SIZE_MAX"
#endif
#ifdef FD_SETSIZE
#error "<ndbm.h> defines FD_SETSIZE"
#endif

#include <stdio.h>

int
main(void)
{
    int   errors = 0;
    datum d;

    if (dptr_check(&d) != &d.dptr || dsize_check(&d) != &d.dsize)
        errors++;
    if (!clearerr_check || !close_check || !delete_check || !error_check || !fetch_check
        || !firstkey_check || !nextkey_check || !open_check || !store_check)
        errors++;

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}
