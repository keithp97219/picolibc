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

#ifndef _SYS__POSIX_H_
#define _SYS__POSIX_H_

/*
 * What libos/freebsd provides, in the __PICOLIBC_OS_ terms doc/os.md
 * defines; <unistd.h> derives the public POSIX names from these. Only
 * what is complete is defined. Nothing here may change picolibc's
 * ABI; see <sys/unistd.h>.
 */

/* clang-format off */

/* Options POSIX.1-2024 requires, provided by the kernel */
#define __PICOLIBC_OS_CHOWN_RESTRICTED             1
#define __PICOLIBC_OS_JOB_CONTROL                  1
#define __PICOLIBC_OS_NO_TRUNC                     1
#define __PICOLIBC_OS_SAVED_IDS                    1
#define __PICOLIBC_OS_SHELL                        1

/* Options */
#define __PICOLIBC_OS_ADVISORY_INFO                202405L
#define __PICOLIBC_OS_FSYNC                        202405L
#define __PICOLIBC_OS_MONOTONIC_CLOCK              202405L
#define __PICOLIBC_OS_PRIORITY_SCHEDULING          202405L
#define __PICOLIBC_OS_REALTIME_SIGNALS             202405L
#define __PICOLIBC_OS_TIMERS                       202405L
#define __PICOLIBC_OS_SYNCHRONIZED_IO              202405L

/* Option groups */
#define __PICOLIBC_OS_CLOFORK            202405L
#define __PICOLIBC_OS_DEVICE_IO          202405L
/* POSIX.1-2008: FreeBSD has no open file description locks */
#define __PICOLIBC_OS_FD_MGMT            200809L
#define __PICOLIBC_OS_FIFO               202405L
#define __PICOLIBC_OS_FIFO_FD            202405L
#define __PICOLIBC_OS_FILE_ATTRIBUTES    202405L
#define __PICOLIBC_OS_FILE_ATTRIBUTES_FD 202405L
#define __PICOLIBC_OS_FILE_SYSTEM        202405L
#define __PICOLIBC_OS_FILE_SYSTEM_EXT    202405L
#define __PICOLIBC_OS_FILE_SYSTEM_FD     202405L
#define __PICOLIBC_OS_MULTI_PROCESS      202405L
#define __PICOLIBC_OS_MULTI_PROCESS_FD   202405L
#define __PICOLIBC_OS_PIPE               202405L
#define __PICOLIBC_OS_SIGNALS            202405L
#define __PICOLIBC_OS_SINGLE_PROCESS     202405L
#define __PICOLIBC_OS_SYMBOLIC_LINKS     202405L
#define __PICOLIBC_OS_SYMBOLIC_LINKS_FD  202405L
#define __PICOLIBC_OS_USER_GROUPS        202405L

/* XSI option groups */
#define __PICOLIBC_OS_XSI_DEVICE_IO            202405L
#define __PICOLIBC_OS_XSI_FILE_SYSTEM          202405L
#define __PICOLIBC_OS_XSI_MULTI_PROCESS        202405L
#define __PICOLIBC_OS_XSI_SIGNALS              202405L
#define __PICOLIBC_OS_XSI_USER_GROUPS          202405L

/*
 * FreeBSD has the process CPU-time clocks (clock_getcpuclockid), but
 * ktimer_create rejects them, a known variance (doc/os.md), so
 * __PICOLIBC_OS_CPUTIME stays undefined.
 *
 * FreeBSD delivers 62 real-time signals; picolibc numbers 31, so
 * __PICOLIBC_OS_RTSIG_MAX stays undefined
 */

/*
 * Options built on libos/pthread with thr_new and _umtx_op. Clock
 * selection and timeouts include pthread_condattr_setclock and the
 * timed locks, so they need the threads too. Priority inheritance and
 * robust mutexes use the kernel's struct umutex, allocated beside the
 * mutex.
 */
#ifdef __PICOLIBC_PTHREADS
#define __PICOLIBC_OS_BARRIERS                     202405L
#define __PICOLIBC_OS_CLOCK_SELECTION              202405L
#define __PICOLIBC_OS_READER_WRITER_LOCKS          202405L
#define __PICOLIBC_OS_SEMAPHORES                   202405L
#define __PICOLIBC_OS_SPIN_LOCKS                   202405L
#define __PICOLIBC_OS_THREADS                      202405L
#define __PICOLIBC_OS_THREAD_ATTR_STACKADDR        202405L
#define __PICOLIBC_OS_THREAD_ATTR_STACKSIZE        202405L
#define __PICOLIBC_OS_THREAD_CPUTIME               202405L
#define __PICOLIBC_OS_THREAD_PRIO_INHERIT          202405L
#define __PICOLIBC_OS_THREAD_PRIORITY_SCHEDULING   202405L
#define __PICOLIBC_OS_THREAD_PRIO_PROTECT          202405L
#define __PICOLIBC_OS_THREAD_PROCESS_SHARED        202405L
#define __PICOLIBC_OS_THREAD_ROBUST_PRIO_INHERIT   202405L
#define __PICOLIBC_OS_THREAD_ROBUST_PRIO_PROTECT   202405L
#define __PICOLIBC_OS_TIMEOUTS                     202405L
#endif

/* clang-format on */

/* A terminal special character of 0xff is turned off */
#define __PICOLIBC_OS_VDISABLE 0xff

/*
 * Options. Message passing (mq_*) is there too, with the mqueuefs
 * module loaded, but FreeBSD notifies for every message sent while no
 * one waits to receive, and at once when registering on a non-empty
 * queue, where POSIX notifies only when a message reaches an empty one
 */
#define __PICOLIBC_OS_MAPPED_FILES          202405L
#define __PICOLIBC_OS_MEMLOCK               202405L
#define __PICOLIBC_OS_MEMLOCK_RANGE         202405L
#define __PICOLIBC_OS_MEMORY_PROTECTION     202405L
#define __PICOLIBC_OS_SHARED_MEMORY_OBJECTS 202405L

/*
 * Option groups. The terminal functions are all there, and read()
 * makes up for canonical input ending a read at a NUL byte (read.c),
 * but the kernel ignores ECHOK, and opening a terminal doesn't make it
 * the controlling one without TIOCSCTTY, so DEVICE_SPECIFIC isn't
 * claimed (doc/os.md)
 */
#define __PICOLIBC_OS_DEVICE_SPECIFIC_R 202405L

/*
 * XSI option groups. The System V IPC functions are there, and
 * shmget checks the permissions of a segment it finds (shmget.c).
 * shm_segsz counts whole pages, and only shmat sets shm_lpid, not
 * shmdt, known variances (doc/os.md)
 */
#define __PICOLIBC_OS_XSI_DEVICE_SPECIFIC    202405L
#define __PICOLIBC_OS_XSI_GENERAL_TERMINAL_R 202405L
#define __PICOLIBC_OS_XSI_IPC                202405L

/* Sockets, with picolibc built for the FreeBSD network interfaces */
#ifdef __PICOLIBC_NETWORK_FREEBSD
#define __PICOLIBC_OS_IPV6        202405L
#define __PICOLIBC_OS_NETWORKING  202405L
#define __PICOLIBC_OS_RAW_SOCKETS 202405L
#endif

#endif /* _SYS__POSIX_H_ */
