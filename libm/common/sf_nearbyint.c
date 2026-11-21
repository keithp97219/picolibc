/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#define _GNU_SOURCE
#include <math.h>
#include "fdlibm.h"

float
nearbyintf(float x)
{
    if (isnan(x))
        return x + x;
#if defined(FE_INEXACT)
    fenv_t env;
    fegetenv(&env);
    fedisableexcept(FE_INEXACT);
#endif
    /*
     * Keep rint between the environment changes: the barriers stop the
     * compiler from moving it, which clang does on targets without
     * strict floating point support
     */
    x = opt_barrier_float(rintf(opt_barrier_float(x)));
#if defined(FE_INEXACT)
    fesetenv(&env);
#endif
    return x;
}

_MATH_ALIAS_f_f(nearbyint)
