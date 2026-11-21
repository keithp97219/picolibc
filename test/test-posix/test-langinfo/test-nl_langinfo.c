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
/* glibc declares ALTMON_1..ALTMON_12 only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * nl_langinfo and nl_langinfo_l give the POSIX locale's values
 * (XBD 7.3), a string for every item, and an empty string for an
 * invalid item
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The items the POSIX locale defines, with their values */
static const struct {
    nl_item     item;
    const char *value;
} posix_values[] = {
    { D_T_FMT,    "%a %b %e %H:%M:%S %Y" },
    { D_FMT,      "%m/%d/%y"             },
    { T_FMT,      "%H:%M:%S"             },
    { T_FMT_AMPM, "%I:%M:%S %p"          },
    { AM_STR,     "AM"                   },
    { PM_STR,     "PM"                   },
    { DAY_1,      "Sunday"               },
    { DAY_2,      "Monday"               },
    { DAY_3,      "Tuesday"              },
    { DAY_4,      "Wednesday"            },
    { DAY_5,      "Thursday"             },
    { DAY_6,      "Friday"               },
    { DAY_7,      "Saturday"             },
    { ABDAY_1,    "Sun"                  },
    { ABDAY_2,    "Mon"                  },
    { ABDAY_3,    "Tue"                  },
    { ABDAY_4,    "Wed"                  },
    { ABDAY_5,    "Thu"                  },
    { ABDAY_6,    "Fri"                  },
    { ABDAY_7,    "Sat"                  },
    { MON_1,      "January"              },
    { MON_2,      "February"             },
    { MON_3,      "March"                },
    { MON_4,      "April"                },
    { MON_5,      "May"                  },
    { MON_6,      "June"                 },
    { MON_7,      "July"                 },
    { MON_8,      "August"               },
    { MON_9,      "September"            },
    { MON_10,     "October"              },
    { MON_11,     "November"             },
    { MON_12,     "December"             },
    { ABMON_1,    "Jan"                  },
    { ABMON_2,    "Feb"                  },
    { ABMON_3,    "Mar"                  },
    { ABMON_4,    "Apr"                  },
    { ABMON_5,    "May"                  },
    { ABMON_6,    "Jun"                  },
    { ABMON_7,    "Jul"                  },
    { ABMON_8,    "Aug"                  },
    { ABMON_9,    "Sep"                  },
    { ABMON_10,   "Oct"                  },
    { ABMON_11,   "Nov"                  },
    { ABMON_12,   "Dec"                  },
    { RADIXCHAR,  "."                    },
    { YESEXPR,    "^[yY]"                },
    { NOEXPR,     "^[nN]"                },
};

#define NVALUES (sizeof(posix_values) / sizeof(posix_values[0]))

/* Items the POSIX locale doesn't define, which still give strings */
static const nl_item other_items[] = {
    CODESET,
    THOUSEP,
    ERA,
    ERA_D_FMT,
    ERA_D_T_FMT,
    ERA_T_FMT,
    ALT_DIGITS,
    CRNCYSTR,
    ALTMON_1,
    ALTMON_2,
    ALTMON_3,
    ALTMON_4,
    ALTMON_5,
    ALTMON_6,
    ALTMON_7,
    ALTMON_8,
    ALTMON_9,
    ALTMON_10,
    ALTMON_11,
    ALTMON_12,
#ifndef __GLIBC__
    /* glibc 2.43 has no ABALTMON_ items */
    ABALTMON_1,
    ABALTMON_2,
    ABALTMON_3,
    ABALTMON_4,
    ABALTMON_5,
    ABALTMON_6,
    ABALTMON_7,
    ABALTMON_8,
    ABALTMON_9,
    ABALTMON_10,
    ABALTMON_11,
    ABALTMON_12,
#endif
};

#define NOTHER (sizeof(other_items) / sizeof(other_items[0]))

/* Values that are no item */
static const nl_item invalid_items[] = { -1, 99999, 0x7fffffff };

#define NINVALID (sizeof(invalid_items) / sizeof(invalid_items[0]))

static void
check_locale(locale_t l, const char *where)
{
    size_t i;
    char  *s;

    printf("%s\n", where);
    for (i = 0; i < NVALUES; i++) {
        s = l ? nl_langinfo_l(posix_values[i].item, l) : nl_langinfo(posix_values[i].item);
        if (s == NULL || strcmp(s, posix_values[i].value) != 0) {
            printf("item %ld: got \"%s\", expected \"%s\"\n", (long)posix_values[i].item,
                   s ? s : "(null)", posix_values[i].value);
            exit(1);
        }
    }
    for (i = 0; i < NOTHER; i++) {
        s = l ? nl_langinfo_l(other_items[i], l) : nl_langinfo(other_items[i]);
        if (s == NULL) {
            printf("item %ld: null\n", (long)other_items[i]);
            exit(1);
        }
    }
    for (i = 0; i < NINVALID; i++) {
        s = l ? nl_langinfo_l(invalid_items[i], l) : nl_langinfo(invalid_items[i]);
        if (s == NULL || *s != '\0') {
            printf("invalid item %ld: got %s\n", (long)invalid_items[i], s ? s : "(null)");
            exit(1);
        }
    }

    /* The codeset has a name */
    s = l ? nl_langinfo_l(CODESET, l) : nl_langinfo(CODESET);
    check(*s != '\0', "CODESET");
}

int
main(void)
{
    locale_t l;

    check_locale((locale_t)0, "startup");
    check(setlocale(LC_ALL, "POSIX") != NULL, "setlocale");
    check_locale((locale_t)0, "POSIX");
    l = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    check(l != (locale_t)0, "newlocale");
    check_locale(l, "nl_langinfo_l");
    check(uselocale(l) == LC_GLOBAL_LOCALE, "uselocale");
    check_locale((locale_t)0, "thread locale");
    check(uselocale(LC_GLOBAL_LOCALE) == l, "uselocale");
    freelocale(l);

#ifdef __PICOLIBC__
    {
        iconv_t cd;

        /* picolibc's codeset names are ones its iconv_open accepts */
        cd = iconv_open(nl_langinfo(CODESET), nl_langinfo(CODESET));
        check(cd != (iconv_t)-1, "iconv_open of CODESET");
        iconv_close(cd);

        /* With multibyte support, C.UTF-8's codeset is UTF-8 */
        l = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
        if (l != (locale_t)0) {
            check(strcmp(nl_langinfo_l(CODESET, l), "UTF-8") == 0, "C.UTF-8 codeset");
            check(strcmp(nl_langinfo(CODESET), "UTF-8") != 0, "the global locale isn't UTF-8");
            cd = iconv_open(nl_langinfo_l(CODESET, l), nl_langinfo(CODESET));
            check(cd != (iconv_t)-1, "iconv_open of the UTF-8 CODESET");
            iconv_close(cd);
            /* The other categories are still the POSIX locale's */
            check(strcmp(nl_langinfo_l(D_FMT, l), "%m/%d/%y") == 0, "C.UTF-8 D_FMT");
            freelocale(l);
        } else {
            printf("no C.UTF-8 locale, skipping its checks\n");
        }
    }
#endif

    printf("success\n");
    return 0;
}
