/* Copyright (c) 2007 Jeff Johnston  <jjohnstn@redhat.com> */

/* The private bits behind the open, fcntl and BSD file flags */

#ifndef _SYS__FCNTL_FLAGS_H_
#define _SYS__FCNTL_FLAGS_H_

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

#endif /* _SYS__FCNTL_FLAGS_H_ */
