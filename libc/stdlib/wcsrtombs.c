/*
Copyright (c) 2002 Thomas Fitzsimmons <fitzsim@redhat.com>
 */
/* Doc in wcsnrtombs.c */

#define _DEFAULT_SOURCE
#include <wchar.h>

size_t
wcsrtombs(char * __restrict dst, const wchar_t ** __restrict src, size_t len,
          mbstate_t * __restrict ps)
{
#ifdef __MB_CAPABLE
    static mbstate_t _wcsrtombs_state;

    if (ps == NULL)
        ps = &_wcsrtombs_state;
#endif
    return wcsnrtombs(dst, src, (size_t)-1, len, ps);
}
