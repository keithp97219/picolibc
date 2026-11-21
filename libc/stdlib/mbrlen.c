/*
Copyright (c) 2002 Thomas Fitzsimmons <fitzsim@redhat.com>
 */
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

size_t
mbrlen(const char * __restrict s, size_t n, mbstate_t * __restrict ps)
{
#ifdef __MB_CAPABLE
    static mbstate_t _mbrlen_state;

    if (ps == NULL)
        ps = &_mbrlen_state;
#endif
    return mbrtowc(NULL, s, n, ps);
}
