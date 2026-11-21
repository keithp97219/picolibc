#include <picolibc.h>

#ifdef __x86_64
#if defined(CRT0_FREEBSD)
#include "crt0-64-freebsd.S"
#elif defined(CRT0_LINUX)
#include "crt0-64-linux.S"
#else
#include "crt0-64.S"
#endif
#else
#ifdef CRT0_LINUX
#include "crt0-32-linux.S"
#else
#include "crt0-32.S"
#endif
#endif

