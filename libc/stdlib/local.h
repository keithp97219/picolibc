/*
Copyright (c) 1990 Regents of the University of California.
All rights reserved.
 */
/* Misc. local definitions for libc/stdlib */

#ifndef _LOCAL_H_
#define _LOCAL_H_

#include <stdint.h>
#include <stddef.h>

char         *_gcvt(double, int, char *, char, int);

extern char **environ;

char         *__findenv(const char *name, size_t *offset);
char         *__findenv_n(const char *name, size_t name_len, size_t *offset);
int           __env_put(const char *name, size_t l_name, char *entry, _Bool owned, _Bool overwrite,
                        _Bool *used);
_Bool         __env_disown(const char *entry);

extern uint32_t __environ_sequence;

#include "locale_private.h"

#ifndef __machine_mbstate_t_defined
#include <wchar.h>
#endif

extern __THREAD_LOCAL uint64_t _rand_next;

/* random, srandom, initstate and setstate; these need the libc lock */
extern char                   *__random_state;
int                            __random_type(const char *state);
void                           __random_seed(char *state, int type, unsigned seed);
void                           __random_init(void);

#ifdef __AVR__
typedef unsigned int uwchar_t;
#else
typedef wchar_t uwchar_t;
#endif

mbtowc_f __ascii_mbtowc;
wctomb_f __ascii_wctomb;

#ifdef __MB_CAPABLE

mbtowc_f __utf8_mbtowc;
wctomb_f __utf8_wctomb;

#ifdef __MB_EXTENDED_CHARSETS_ISO
extern const uint16_t __iso_8859_conv[14][0x60];
extern const uint16_t __iso_8859_max[14];
#endif

#ifdef __MB_EXTENDED_CHARSETS_WINDOWS
extern const uint16_t __cp_conv[27][0x80];
extern const uint16_t __cp_max[27];
#endif

#endif

size_t _wcsnrtombs_l(char *, const wchar_t **, size_t, size_t, mbstate_t *, locale_t);

#endif
