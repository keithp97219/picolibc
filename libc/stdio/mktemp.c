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
/* This is file MKTEMP.C */
/* This file may have been modified by DJ Delorie (Jan 1991).  If so,
** these modifications are Copyright (C) 1991 DJ Delorie.
*/

/*
FUNCTION
<<mktemp>>, <<mkstemp>>, <<mkstemps>>, <<mkostemp>>,
<<mkostemps>>---generate unused file name

INDEX
        mktemp
INDEX
        mkstemp
INDEX
        mkstemps
INDEX
        mkostemp
INDEX
        mkostemps

SYNOPSIS
        #include <stdlib.h>
        char *mktemp(char *<[path]>);
        int mkstemp(char *<[path]>);
        int mkstemps(char *<[path]>, int suffixlen);
        int mkostemp(char *<[path]>, int flags);
        int mkostemps(char *<[path]>, int suffixlen, int flags);

DESCRIPTION
<<mktemp>>, <<mkstemp>>, <<mkstemps>>, <<mkostemp>> and <<mkostemps>> attempt to
generate a file name that is not yet in use for any existing file.
<<mkstemp>>, <<mkstemps>> and <mkostemps>> create the file and open it
for reading and writing; <<mktemp>> simply generates the file name
(making <<mktemp>> a security risk). <<mkostemp>> and <<mkostemps>> allow the addition
of other <<open>> flags, such as <<O_CLOEXEC>>, <<O_APPEND>>, or
<<O_SYNC>>.  On platforms with a separate text mode, <<mkstemp>>
forces <<O_BINARY>>, while <<mkostemp>> allows the choice between
<<O_BINARY>>, <<O_TEXT>>, or 0 for default.

You supply a simple pattern for the generated file name, as the string
at <[path]>.  The pattern should be a valid filename (including path
information if you wish) ending with at least six `<<X>>' characters.
The generated filename will match the leading part of the name you
supply, with the trailing `<<X>>' characters replaced by some
combination of digits and letters.  With <<mkstemps>> and
<mkostemps>>, the `<<X>>' characters end <[suffixlen]> bytes before
the end of the string.

RETURNS
<<mktemp>> returns the pointer <[path]> to the modified string
representing an unused filename, unless it could not generate one, or
the pattern you provided is not suitable for a filename; in that case,
it returns <<NULL>>.  Be aware that there is an inherent race between
generating the name and attempting to create a file by that name;
you are advised to use <<O_EXCL|O_CREAT>>.

<<mkstemp>>, <<mkstemps>> and <<mkostemps>> return a file descriptor
to the newly created file, unless it could not generate an unused
filename, or the pattern you provided is not suitable for a filename;
in that case, it returns <<-1>>.

NOTES
Never use <<mktemp>>.  The generated filenames are easy to guess and
there's a race between the test if the file exists and the creation
of the file.  In combination this makes <<mktemp>> prone to attacks
and using it is a security risk.  Whenever possible use <<mkstemp>>
instead.  It doesn't suffer the race condition.

PORTABILITY
ANSI C does not require either <<mktemp>> or <<mkstemp>>; the System V
Interface Definition requires <<mktemp>> as of Issue 2.  POSIX 2001
requires <<mkstemp>> while deprecating <<mktemp>>.  <<mkstemps>>, and
<<mkostemps>> are not standardized.

Supporting OS subroutines required: <<open>>
*/

#include "local-stdio.h"

static int
_createfile(const char *path, int flags)
{
    return open(path, flags | O_CREAT | O_EXCL | O_RDWR, 0600);
}

int
mkstemp(char *template)
{
    return __gettemp(template, 0, _createfile, 0);
}

char *
mktemp(char *template)
{
    return __gettemp(template, 0, NULL, 0) == 0 ? template : (char *)NULL;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_TEXT
#define O_TEXT 0
#endif

int
mkstemps(char *template, int suffixlen)
{
    return __gettemp(template, suffixlen, _createfile, O_BINARY);
}

int
mkostemps(char *template, int suffixlen, int flags)
{
    flags &= (O_APPEND | O_CLOEXEC | O_SYNC | O_BINARY | O_TEXT);
    return __gettemp(template, suffixlen, _createfile, flags);
}

int
mkostemp(char *template, int flags)
{
    return mkostemps(template, 0, flags);
}
