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
 * Time zones from TZif files (RFC 8536, and RFC 9636 for version 4)
 *
 * tzset loads one when TZ is unset (/etc/localtime), ":name" or not a
 * POSIX rule string, from $TZDIR/name, by default
 * /usr/share/zoneinfo/name. The transitions and time types give local
 * time up to the last transition, and the footer, a POSIX rule string
 * kept in __tzinfo, after it. Leap seconds and the standard/wall and
 * UT/local indicators are ignored.
 */

#include "local.h"

#ifdef __TZIF

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TZDIR     "/usr/share/zoneinfo"
#define TZDEFAULT "/etc/localtime"

/* Limits on what a file may hold, well beyond any real zone */
#define TZIF_MAX_TIMES 65535
#define TZIF_MAX_TYPES 256
#define TZIF_MAX_CHARS 1024
#define TZIF_MAX_TZ    128

struct __tzif *__tzif_zone;

/* Whether /etc/localtime failed to load, so it isn't tried each time */
static bool    no_default;

/* Buffered reads from the file */
struct reader {
    int      fd;
    unsigned pos;
    unsigned len;
    uint8_t  buf[256];
};

/* Read n bytes to dst, or skip them when dst is NULL */
static bool
get(struct reader *r, void *dst, uint64_t n)
{
    uint8_t *d = dst;

    while (n) {
        unsigned c;

        if (r->pos == r->len) {
            ssize_t got = read(r->fd, r->buf, sizeof(r->buf));
            if (got <= 0)
                return false;
            r->pos = 0;
            r->len = (unsigned)got;
        }
        c = r->len - r->pos;
        if (c > n)
            c = (unsigned)n;
        if (d) {
            memcpy(d, r->buf + r->pos, c);
            d += c;
        }
        r->pos += c;
        n -= c;
    }
    return true;
}

static uint32_t
be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/* The header: "TZif", the version, 15 reserved bytes and six counts */
#define HDR_SIZE 44

enum { ISUTCNT, ISSTDCNT, LEAPCNT, TIMECNT, TYPECNT, CHARCNT };

static bool
get_header(struct reader *r, uint8_t *version, uint32_t cnt[6])
{
    uint8_t h[HDR_SIZE];
    int     i;

    if (!get(r, h, HDR_SIZE) || memcmp(h, "TZif", 4) != 0)
        return false;
    *version = h[4];
    for (i = 0; i < 6; i++)
        cnt[i] = be32(h + 20 + 4 * i);
    return (*version == '\0' || (*version >= '2' && *version <= '4')) && cnt[TYPECNT] != 0
        && cnt[CHARCNT] != 0 && (cnt[ISUTCNT] == 0 || cnt[ISUTCNT] == cnt[TYPECNT])
        && (cnt[ISSTDCNT] == 0 || cnt[ISSTDCNT] == cnt[TYPECNT]);
}

/* The size of a data block with w-byte times */
static uint64_t
data_size(const uint32_t cnt[6], unsigned w)
{
    return (uint64_t)cnt[TIMECNT] * (w + 1) + (uint64_t)cnt[TYPECNT] * 6 + cnt[CHARCNT]
        + (uint64_t)cnt[LEAPCNT] * (w + 4) + cnt[ISSTDCNT] + cnt[ISUTCNT];
}

/*
 * The most recent standard and daylight saving time types, which set
 * tzname, timezone and daylight as other implementations do; dst is
 * -1 without one.
 */
static void
recent_types(const struct __tzif *z, int *std, int *dst)
{
    uint32_t i = z->timecnt;
    int      s = -1, d = -1;

    while (i-- > 0 && (s < 0 || d < 0)) {
        int t = z->idx[i];
        if (z->types[t].isdst) {
            if (d < 0)
                d = t;
        } else if (s < 0)
            s = t;
    }
    for (i = 0; s < 0 && i < z->typecnt; i++)
        if (!z->types[i].isdst)
            s = (int)i;
    if (s < 0)
        s = 0;
    *std = s;
    *dst = d;
}

static void
copy_name(char *to, const char *from)
{
    strncpy(to, from, TZNAME_MAX);
    to[TZNAME_MAX] = '\0';
}

/* Set tzname, timezone and daylight for zone; call with the lock held */
void
__tzif_set_names(const struct __tzif *zone)
{
    int std, dst;

    recent_types(zone, &std, &dst);
    copy_name(__tzname_std, zone->abbr + zone->types[std].abbrind);
    copy_name(__tzname_dst, zone->abbr + zone->types[dst < 0 ? std : dst].abbrind);
    timezone = -(long)zone->types[std].utoff;
    daylight = dst >= 0;
}

/*
 * Abbreviations, kept for the life of the process so that tm_zone
 * stays valid when the zone it came from is replaced. Each distinct
 * block is stored once, so the pool grows only with the zones used.
 */
struct abbrs {
    struct abbrs *next;
    size_t        len;
    char          data[];
};

static struct abbrs *abbrs;

/* Look for the block s, len bytes, in the pool; call with the lock held */
static struct abbrs *
find(const char *s, size_t len)
{
    struct abbrs *a;

    for (a = abbrs; a; a = a->next)
        if (a->len == len && memcmp(a->data, s, len) == 0)
            break;
    return a;
}

/* The pool's copy of the block s, len bytes, adding it when it's new */
static const char *
intern(const char *s, size_t len)
{
    struct abbrs *a, *n;

    __LIBC_LOCK();
    a = find(s, len);
    __LIBC_UNLOCK();
    if (a)
        return a->data;

    /* malloc takes the lock too */
    n = malloc(sizeof(*n) + len);
    if (!n)
        return NULL;
    n->len = len;
    memcpy(n->data, s, len);

    __LIBC_LOCK();
    a = find(s, len);
    if (!a) {
        n->next = abbrs;
        abbrs = n;
        a = n;
        n = NULL;
    }
    __LIBC_UNLOCK();
    free(n);
    return a->data;
}

#define ALIGN(x, a) (((x) + (a) - 1) & ~(size_t)((a) - 1))

/*
 * Read a TZif file into one allocation. The footer's names and rules,
 * or without a footer ones made from the most recent types, go to
 * new_tzname and new_tzrule.
 */
static struct __tzif *
tzif_read(struct reader *r, const char *name, char new_tzname[2][TZNAME_MAX + 2],
          tzrule_t new_tzrule[2])
{
    uint8_t        version;
    uint32_t       cnt[6];
    unsigned       w = 4;
    struct __tzif *z;
    size_t         off_trans, off_types, off_idx, off_name, namelen;
    char           abbr[TZIF_MAX_CHARS + 1];
    uint32_t       i, timecnt, typecnt, charcnt;
    int            std, dst;

    if (!get_header(r, &version, cnt))
        return NULL;
    if (version != '\0') {
        /* Skip the version 1 data for the 64-bit data after it */
        if (!get(r, NULL, data_size(cnt, 4)) || !get_header(r, &version, cnt))
            return NULL;
        w = 8;
    }
    timecnt = cnt[TIMECNT];
    typecnt = cnt[TYPECNT];
    charcnt = cnt[CHARCNT];
    if (timecnt > TZIF_MAX_TIMES || typecnt > TZIF_MAX_TYPES || charcnt > TZIF_MAX_CHARS)
        return NULL;

    namelen = strlen(name) + 1;
    off_trans = ALIGN(sizeof(*z), sizeof(int64_t));
    off_types = off_trans + timecnt * sizeof(int64_t);
    off_idx = off_types + typecnt * sizeof(struct __tzif_type);
    off_name = off_idx + timecnt;
    z = malloc(off_name + namelen);
    if (!z)
        return NULL;
    z->trans = (int64_t *)((char *)z + off_trans);
    z->types = (struct __tzif_type *)((char *)z + off_types);
    z->idx = (uint8_t *)z + off_idx;
    z->name = (char *)z + off_name;
    z->timecnt = timecnt;
    z->typecnt = (uint16_t)typecnt;
    z->footer = 0;
    memcpy(z->name, name, namelen);

    /* Transition times, in ascending order */
    for (i = 0; i < timecnt; i++) {
        uint8_t b[8];

        if (!get(r, b, w))
            goto fail;
        if (w == 8)
            z->trans[i] = (int64_t)(((uint64_t)be32(b) << 32) | be32(b + 4));
        else
            z->trans[i] = (int32_t)be32(b);
        if (i && z->trans[i] <= z->trans[i - 1])
            goto fail;
    }

    /* The type of each */
    if (!get(r, z->idx, timecnt))
        goto fail;
    for (i = 0; i < timecnt; i++)
        if (z->idx[i] >= typecnt)
            goto fail;

    /* Time types: UT offset, isdst and abbreviation index */
    for (i = 0; i < typecnt; i++) {
        uint8_t b[6];
        int32_t utoff;

        if (!get(r, b, 6))
            goto fail;
        utoff = (int32_t)be32(b);
        if (utoff <= -SECSPERDAY || utoff >= SECSPERDAY || b[4] > 1 || b[5] >= charcnt)
            goto fail;
        z->types[i].utoff = utoff;
        z->types[i].isdst = b[4];
        z->types[i].abbrind = b[5];
    }

    /* Abbreviations, which outlive the zone */
    if (!get(r, abbr, charcnt))
        goto fail;
    abbr[charcnt] = '\0';
    z->abbr = intern(abbr, charcnt + 1);
    if (!z->abbr)
        goto fail;

    if (w == 8) {
        /* The footer, a rule string between newlines, after the leap seconds and indicators */
        char tz[TZIF_MAX_TZ];

        if (!get(r, NULL, (uint64_t)cnt[LEAPCNT] * 12 + cnt[ISSTDCNT] + cnt[ISUTCNT])
            || !get(r, tz, 1) || tz[0] != '\n')
            goto fail;
        for (i = 0;; i++) {
            if (i == TZIF_MAX_TZ || !get(r, tz + i, 1))
                goto fail;
            if (tz[i] == '\n')
                break;
        }
        tz[i] = '\0';

        /* An empty footer, or one we can't parse, leaves the last type in effect */
        if (i > 0 && __tzparse(tz, new_tzname, new_tzrule)) {
            z->footer = 1;
            for (i = 0; i < 2; i++) {
                z->footer_name[i] = intern(new_tzname[i], strlen(new_tzname[i]) + 1);
                if (!z->footer_name[i])
                    goto fail;
            }
            return z;
        }
    }

    /* Without a footer, the rules hold the most recent types */
    recent_types(z, &std, &dst);
    if (dst < 0)
        dst = std;
    memset(new_tzrule, 0, 2 * sizeof(tzrule_t));
    new_tzrule[0].ch = new_tzrule[1].ch = 'J';
    new_tzrule[0].offset = -z->types[std].utoff;
    new_tzrule[1].offset = -z->types[dst].utoff;
    copy_name(new_tzname[0], z->abbr + z->types[std].abbrind);
    copy_name(new_tzname[1], z->abbr + z->types[dst].abbrind);
    return z;

fail:
    free(z);
    return NULL;
}

/* Whether name is a file name under TZDIR: not empty and without ".." */
static bool
valid_name(const char *name)
{
    const char *c = name;

    if (*c == '\0')
        return false;
    for (;;) {
        if (c[0] == '.' && c[1] == '.' && (c[2] == '/' || c[2] == '\0'))
            return false;
        c = strchr(c, '/');
        if (!c)
            return true;
        c++;
    }
}

/*
 * Make the zone in the TZif file tzenv names, or /etc/localtime when
 * tzenv is NULL, the current time zone, unless it already is. Returns
 * false, changing nothing, when the file can't be read.
 */
bool
__tzif_load(const char *tzenv)
{
    const char    *name = tzenv ? tzenv : "";
    const char    *file;
    char          *path = NULL;
    struct reader  r;
    struct __tzif *z;
    char           new_tzname[2][TZNAME_MAX + 2];
    tzrule_t       new_tzrule[2];
    bool           same;

    __LIBC_LOCK();
    same = __tzif_zone && __tzif_zone->deflt == !tzenv && strcmp(__tzif_zone->name, name) == 0;
    __LIBC_UNLOCK();
    if (same)
        return true;

    if (!tzenv) {
        if (no_default)
            return false;
        file = TZDEFAULT;
    } else {
        if (*tzenv == ':')
            tzenv++;
        if (*tzenv == '/')
            file = tzenv;
        else {
            const char *dir = secure_getenv("TZDIR");

            if (!valid_name(tzenv))
                return false;
            if (!dir || !*dir)
                dir = TZDIR;
            path = malloc(strlen(dir) + strlen(tzenv) + 2);
            if (!path)
                return false;
            strcpy(path, dir);
            strcat(path, "/");
            strcat(path, tzenv);
            file = path;
        }
    }

    r.fd = open(file, O_RDONLY | O_CLOEXEC);
    free(path);
    z = NULL;
    if (r.fd >= 0) {
        r.pos = r.len = 0;
        z = tzif_read(&r, name, new_tzname, new_tzrule);
        close(r.fd);
        if (z)
            z->deflt = !tzenv;
    }
    if (!z) {
        if (!tzenv)
            no_default = true;
        return false;
    }
    __tzset_install(new_tzname, new_tzrule, z);
    return true;
}

/*
 * The time type in effect at t, or NULL when the footer rule applies;
 * call with the lock held. Before the first transition, that's type 0
 * (RFC 8536 section 3.2).
 */
const struct __tzif_type *
__tzif_type_at(const struct __tzif *z, time_t t)
{
    uint32_t lo = 0, hi = z->timecnt;

    if (hi == 0)
        return z->footer ? NULL : &z->types[0];
    if (t < z->trans[0])
        return &z->types[0];

    /* The last transition at or before t */
    while (hi - lo > 1) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (z->trans[mid] <= t)
            lo = mid;
        else
            hi = mid;
    }
    if (lo == z->timecnt - 1 && z->footer)
        return NULL;
    return &z->types[z->idx[lo]];
}

#endif /* __TZIF */
