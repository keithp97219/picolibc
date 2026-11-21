/* Copyright (c) 2007 Jeff Johnston  <jjohnstn@redhat.com> */

/*
 * The open flags, shared by <fcntl.h> and <stdlib.h> (for mkostemp)
 */

#ifndef _SYS__OPEN_FLAGS_H_
#define _SYS__OPEN_FLAGS_H_

#include <sys/cdefs.h>

#define _FOPEN      (-1)       /* from sys/file.h, kernel use only */
#define _FREAD      0x0001     /* read enabled */
#define _FWRITE     0x0002     /* write enabled */
#define _FAPPEND    0x0400     /* append (writes guaranteed at the end) */
#define _FMARK      0x0010     /* internal; mark during gc() */
#define _FDEFER     0x0020     /* internal; defer for next gc pass */
#define _FASYNC     0x0008     /* signal pgrp when data ready */
#define _FSHLOCK    0x0080     /* BSD flock() shared lock present */
#define _FEXLOCK    0x0100     /* BSD flock() exclusive lock present */
#define _FCREAT     0x0040     /* open with file create */
#define _FTRUNC     0x0200     /* open with truncation */
#define _FEXCL      0x0800     /* error on open if file exists */
#define _FNBIO      0x1000     /* non blocking I/O (sys5 style) */
#define _FSYNC      0x2000     /* do all writes synchronously */
#define _FNONBLOCK  0x4000     /* non blocking I/O (POSIX style) */
#define _FNDELAY    _FNONBLOCK /* non blocking I/O (4.2 style) */
#define _FNOCTTY    0x8000     /* don't assign a ctty on this open */
#define _FNOINHERIT 0x40000
#define _FDIRECT    0x80000
#define _FNOFOLLOW  0x100000
#define _FDIRECTORY 0x200000
#define _FEXECSRCH  0x400000

/* O_EXEC and O_SEARCH are file access modes too */
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR | _FEXECSRCH)

/*
 * Flag values for open(2) and fcntl(2)
 * The kernel adds 1 to the open modes to turn it into some
 * combination of FREAD and FWRITE.
 */
#define O_RDONLY 0 /* +1 == FREAD */
#define O_WRONLY 1 /* +1 == FWRITE */
#define O_RDWR   2 /* +1 == FREAD|FWRITE */
#define O_APPEND _FAPPEND
#define O_CREAT  _FCREAT
#define O_TRUNC  _FTRUNC
#define O_EXCL   _FEXCL
#define O_SYNC   _FSYNC
#define O_ASYNC  _FASYNC
/*	O_NDELAY	_FNDELAY 	set in include/fcntl.h */
/*	O_NDELAY	_FNBIO 		set in include/fcntl.h */
#define O_NONBLOCK _FNONBLOCK
#define O_NOCTTY   _FNOCTTY

/* POSIX-1.2008 specific flags */
#if __POSIX_VISIBLE >= 200809
#define O_CLOEXEC   _FNOINHERIT
#define O_NOFOLLOW  _FNOFOLLOW
#define O_DIRECTORY _FDIRECTORY
#define O_EXEC      _FEXECSRCH
#define O_SEARCH    _FEXECSRCH
#endif

#if __POSIX_VISIBLE >= 202405
/* Zero, as POSIX allows; terminals start in whatever state the OS gives them */
#define O_TTY_INIT 0
#endif

#if __BSD_VISIBLE
#define O_DIRECT _FDIRECT
#endif

#define O_DSYNC _FSYNC
#define O_RSYNC _FSYNC

#endif /* _SYS__OPEN_FLAGS_H_ */
