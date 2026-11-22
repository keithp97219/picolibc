/*
Copyright (c) 1991, 1993
The Regents of the University of California.  All rights reserved.
All or some portions of this file are derived from material licensed
to the University of California by American Telephone and Telegraph
Co. or Unix System Laboratories, Inc. and are reproduced herein with
the permission of UNIX System Laboratories, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the University nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
 */
#ifndef _CTYPE_H_
#define _CTYPE_H_

#include <sys/cdefs.h>

#if __POSIX_VISIBLE >= 200809 || __MISC_VISIBLE
#include <sys/_locale.h>
#endif

_BEGIN_STD_C

/*
 * The small ctype code does not support extended character sets. It also breaks
 * libstdc++'s ctype implementation, so just skip it for c++
 */
#if defined(__MB_EXTENDED_CHARSETS_NON_UNICODE) || defined(__cplusplus)
#undef _PICOLIBC_CTYPE_SMALL
#define _PICOLIBC_CTYPE_SMALL 0
#endif

/*
 * The default ctype style depends upon whether we are optimizing for
 * size and whether the library supports charset-specific ctype data
 */
#if !defined(_PICOLIBC_CTYPE_SMALL)
#ifdef __OPTIMIZE_SIZE__
#define _PICOLIBC_CTYPE_SMALL 1
#else
#define _PICOLIBC_CTYPE_SMALL 0
#endif
#endif

int isalnum(int __c) __picolibc_export;
int isalpha(int __c) __picolibc_export;
int iscntrl(int __c) __picolibc_export;
int isdigit(int __c) __picolibc_export;
int isgraph(int __c) __picolibc_export;
int islower(int __c) __picolibc_export;
int isprint(int __c) __picolibc_export;
int ispunct(int __c) __picolibc_export;
int isspace(int __c) __picolibc_export;
int isupper(int __c) __picolibc_export;
int isxdigit(int __c) __picolibc_export;
int tolower(int __c) __picolibc_export;
int toupper(int __c) __picolibc_export;

#if __ISO_C_VISIBLE >= 1999
int isblank(int __c) __picolibc_export;
#endif

#if __MISC_VISIBLE || __XSI_VISIBLE
int isascii(int __c) __picolibc_export;
int toascii(int __c) __picolibc_export;
/* POSIX.1-2024 removed _tolower and _toupper */
#if __MISC_VISIBLE || __XSI_VISIBLE < 800
#define _tolower(__c) ((__c) + ('a' - 'A'))
#define _toupper(__c) ((__c) - ('a' - 'A'))
#endif
#define isascii(__c) ((unsigned)(__c) <= 0177)
#define toascii(__c) ((__c) & 0177)
#endif

#if __POSIX_VISIBLE >= 200809
int isalnum_l(int __c, locale_t __l) __picolibc_export;
int isalpha_l(int __c, locale_t __l) __picolibc_export;
int isblank_l(int __c, locale_t __l) __picolibc_export;
int iscntrl_l(int __c, locale_t __l) __picolibc_export;
int isdigit_l(int __c, locale_t __l) __picolibc_export;
int isgraph_l(int __c, locale_t __l) __picolibc_export;
int islower_l(int __c, locale_t __l) __picolibc_export;
int isprint_l(int __c, locale_t __l) __picolibc_export;
int ispunct_l(int __c, locale_t __l) __picolibc_export;
int isspace_l(int __c, locale_t __l) __picolibc_export;
int isupper_l(int __c, locale_t __l) __picolibc_export;
int isxdigit_l(int __c, locale_t __l) __picolibc_export;
int tolower_l(int __c, locale_t __l) __picolibc_export;
int toupper_l(int __c, locale_t __l) __picolibc_export;
#endif

#if __MISC_VISIBLE
int isascii_l(int __c, locale_t __l) __picolibc_export;
int toascii_l(int __c, locale_t __l) __picolibc_export;
#define isascii_l(__c, __l) ((void)(__l), (unsigned)(__c) <= 0177)
#define toascii_l(__c, __l) ((void)(__l), (__c) & 0177)
#endif

#if _PICOLIBC_CTYPE_SMALL

#ifdef __declare_extern_inline

__declare_extern_inline(int)
isblank(int __c)
{
    return __c == ' ' || __c == '\t';
}

__declare_extern_inline(int)
iscntrl(int __c)
{
    return (0x00 <= __c && __c <= 0x1f) || __c == 0x7f;
}

__declare_extern_inline(int)
isdigit(int __c)
{
    return '0' <= __c && __c <= '9';
}

__declare_extern_inline(int)
isgraph(int __c)
{
    return '!' <= __c && __c <= '~';
}

__declare_extern_inline(int)
islower(int __c)
{
    return 'a' <= __c && __c <= 'z';
}

__declare_extern_inline(int)
isprint(int __c)
{
    return ' ' <= __c && __c <= '~';
}

__declare_extern_inline(int)
ispunct(int __c)
{
    return (('!' <= __c && __c <= '/') || (':' <= __c && __c <= '@') || ('[' <= __c && __c <= '`')
            || ('{' <= __c && __c <= '~'));
}

__declare_extern_inline(int)
isspace(int __c)
{
    return __c == ' ' || ('\t' <= __c && __c <= '\r');
}

__declare_extern_inline(int)
isupper(int __c)
{
    return 'A' <= __c && __c <= 'Z';
}

__declare_extern_inline(int)
isxdigit(int __c)
{
    return (isdigit(__c) || ('A' <= __c && __c <= 'F') || ('a' <= __c && __c <= 'f'));
}

__declare_extern_inline(int)
isalpha(int __c)
{
    return isupper(__c) || islower(__c);
}

__declare_extern_inline(int)
isalnum(int __c)
{
    return isalpha(__c) || isdigit(__c);
}

__declare_extern_inline(int)
tolower(int __c)
{
    if (isupper(__c))
        __c = __c - 'A' + 'a';
    return __c;
}

__declare_extern_inline(int)
toupper(int __c)
{
    if (islower(__c))
        __c = __c - 'a' + 'A';
    return __c;
}

#if __POSIX_VISIBLE >= 200809
__declare_extern_inline(int)
isalnum_l(int __c, locale_t __l)
{
    (void)__l;
    return isalnum(__c);
}
__declare_extern_inline(int)
isalpha_l(int __c, locale_t __l)
{
    (void)__l;
    return isalpha(__c);
}
__declare_extern_inline(int)
isblank_l(int __c, locale_t __l)
{
    (void)__l;
    return isblank(__c);
}
__declare_extern_inline(int)
iscntrl_l(int __c, locale_t __l)
{
    (void)__l;
    return iscntrl(__c);
}
__declare_extern_inline(int)
isdigit_l(int __c, locale_t __l)
{
    (void)__l;
    return isdigit(__c);
}
__declare_extern_inline(int)
isgraph_l(int __c, locale_t __l)
{
    (void)__l;
    return isgraph(__c);
}
__declare_extern_inline(int)
islower_l(int __c, locale_t __l)
{
    (void)__l;
    return islower(__c);
}
__declare_extern_inline(int)
isprint_l(int __c, locale_t __l)
{
    (void)__l;
    return isprint(__c);
}
__declare_extern_inline(int)
ispunct_l(int __c, locale_t __l)
{
    (void)__l;
    return ispunct(__c);
}
__declare_extern_inline(int)
isspace_l(int __c, locale_t __l)
{
    (void)__l;
    return isspace(__c);
}
__declare_extern_inline(int)
isupper_l(int __c, locale_t __l)
{
    (void)__l;
    return isupper(__c);
}
__declare_extern_inline(int)
isxdigit_l(int __c, locale_t __l)
{
    (void)__l;
    return isxdigit(__c);
}
__declare_extern_inline(int)
tolower_l(int __c, locale_t __l)
{
    (void)__l;
    return tolower(__c);
}
__declare_extern_inline(int)
toupper_l(int __c, locale_t __l)
{
    (void)__l;
    return toupper(__c);
}
#endif

#endif /* __declare_extern_inline */

#else /* _PICOLIBC_CTYPE_SMALL */

#define _CTYPE_OFFSET 127

/* These must be exported: the isalpha()/isctype() macros dereference
 * them directly from user code, so they must be visible in the DSO. */
extern __picolibc_export const char  _ctype_b[];
extern __picolibc_export const short _ctype_wide[];

#define __CTYPE_TABLE (_ctype_b + _CTYPE_OFFSET)

/* The old name, which libstdc++ uses */
#if __MISC_VISIBLE || defined(__cplusplus)
#define _ctype_ __CTYPE_TABLE
#endif

#define __CTYPE_UPPER 0x001 /* upper */
#define __CTYPE_LOWER 0x002 /* lower */
#define __CTYPE_DIGIT 0x004 /* digit */
#define __CTYPE_SPACE 0x008 /* space */
#define __CTYPE_PUNCT 0x010 /* punct */
#define __CTYPE_CNTRL 0x020 /* control */
#define __CTYPE_HEX   0x040 /* hex */
#define __CTYPE_BLANK 0x080 /* blank (but not tab) */
#define __CTYPE_TAB   0x100 /* tab (only in wide table) */

#ifdef __cplusplus
/* We need these legacy symbols to build libstdc++ */
#define _U __CTYPE_UPPER
#define _L __CTYPE_LOWER
#define _N __CTYPE_DIGIT
#define _S __CTYPE_SPACE
#define _P __CTYPE_PUNCT
#define _C __CTYPE_CNTRL
#define _X __CTYPE_HEX
#define _B __CTYPE_BLANK
#endif

#ifdef __MB_EXTENDED_CHARSETS_NON_UNICODE
const char *__locale_ctype_ptr(void) __picolibc_export;
#define __CTYPE_PTR __locale_ctype_ptr()
#else
#define __CTYPE_PTR __CTYPE_TABLE
#endif

#ifndef __cplusplus

#define __ctype_lookup(__c) (__CTYPE_PTR + 1)[(int)(__c)]

#define isalpha(__c)        (__ctype_lookup(__c) & (__CTYPE_UPPER | __CTYPE_LOWER))
#define isupper(__c)        ((__ctype_lookup(__c) & (__CTYPE_UPPER | __CTYPE_LOWER)) == __CTYPE_UPPER)
#define islower(__c)        ((__ctype_lookup(__c) & (__CTYPE_UPPER | __CTYPE_LOWER)) == __CTYPE_LOWER)
#define isdigit(__c)        (__ctype_lookup(__c) & __CTYPE_DIGIT)
#define isxdigit(__c)       (__ctype_lookup(__c) & (__CTYPE_HEX | __CTYPE_DIGIT))
#define isspace(__c)        (__ctype_lookup(__c) & __CTYPE_SPACE)
#define ispunct(__c)        (__ctype_lookup(__c) & __CTYPE_PUNCT)
#define isalnum(__c)        (__ctype_lookup(__c) & (__CTYPE_UPPER | __CTYPE_LOWER | __CTYPE_DIGIT))
#define isprint(__c)                                                                    \
    (__ctype_lookup(__c)                                                                \
     & (__CTYPE_PUNCT | __CTYPE_UPPER | __CTYPE_LOWER | __CTYPE_DIGIT | __CTYPE_BLANK))
#define isgraph(__c)                                                                        \
    (__ctype_lookup(__c) & (__CTYPE_PUNCT | __CTYPE_UPPER | __CTYPE_LOWER | __CTYPE_DIGIT))
#define iscntrl(__c) (__ctype_lookup(__c) & __CTYPE_CNTRL)

#if __ISO_C_VISIBLE >= 1999 && defined(__declare_extern_inline)
__declare_extern_inline(int)
isblank(int __c)
{
    return __c == '\t' || __ctype_lookup(__c) & __CTYPE_BLANK;
}
#endif

#if __POSIX_VISIBLE >= 200809

#ifdef __MB_EXTENDED_CHARSETS_NON_UNICODE
const char *__locale_ctype_ptr_l(locale_t) __picolibc_export;
#define __CTYPE_PTR_L(__l) __locale_ctype_ptr_l(__l)
#else
#define __CTYPE_PTR_L(__l) ((void)(__l), __CTYPE_TABLE)
#endif

#define __ctype_lookup_l(__c, __l) ((__CTYPE_PTR_L(__l) + 1)[(int)(__c)])

#define isalpha_l(__c, __l)        (__ctype_lookup_l(__c, __l) & (__CTYPE_UPPER | __CTYPE_LOWER))
#define isupper_l(__c, __l)                                                           \
    ((__ctype_lookup_l(__c, __l) & (__CTYPE_UPPER | __CTYPE_LOWER)) == __CTYPE_UPPER)
#define islower_l(__c, __l)                                                           \
    ((__ctype_lookup_l(__c, __l) & (__CTYPE_UPPER | __CTYPE_LOWER)) == __CTYPE_LOWER)
#define isdigit_l(__c, __l)  (__ctype_lookup_l(__c, __l) & __CTYPE_DIGIT)
#define isxdigit_l(__c, __l) (__ctype_lookup_l(__c, __l) & (__CTYPE_HEX | __CTYPE_DIGIT))
#define isspace_l(__c, __l)  (__ctype_lookup_l(__c, __l) & __CTYPE_SPACE)
#define ispunct_l(__c, __l)  (__ctype_lookup_l(__c, __l) & __CTYPE_PUNCT)
#define isalnum_l(__c, __l)                                                        \
    (__ctype_lookup_l(__c, __l) & (__CTYPE_UPPER | __CTYPE_LOWER | __CTYPE_DIGIT))
#define isprint_l(__c, __l)                                                             \
    (__ctype_lookup_l(__c, __l)                                                         \
     & (__CTYPE_PUNCT | __CTYPE_UPPER | __CTYPE_LOWER | __CTYPE_DIGIT | __CTYPE_BLANK))
#define isgraph_l(__c, __l)                                                                        \
    (__ctype_lookup_l(__c, __l) & (__CTYPE_PUNCT | __CTYPE_UPPER | __CTYPE_LOWER | __CTYPE_DIGIT))
#define iscntrl_l(__c, __l) (__ctype_lookup_l(__c, __l) & __CTYPE_CNTRL)

#ifdef __declare_extern_inline
__declare_extern_inline(int)
isblank_l(int __c, locale_t __l)
{
    return __c == '\t' || (__ctype_lookup_l(__c, __l) & __CTYPE_BLANK);
}
#endif

#endif /* __POSIX_VISIBLE >= 200809 */

#endif /* !__cplusplus */

#endif /* else _PICOLIBC_CTYPE_SMALL */

_END_STD_C

#endif /* _CTYPE_H_ */
