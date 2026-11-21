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

/* Options */
#define __PICOLIBC_OS_ADVISORY_INFO                202405L
#define __PICOLIBC_OS_FSYNC                        202405L
#define __PICOLIBC_OS_MONOTONIC_CLOCK              202405L
#define __PICOLIBC_OS_PRIORITY_SCHEDULING          202405L
#define __PICOLIBC_OS_REALTIME_SIGNALS             202405L
#define __PICOLIBC_OS_TIMERS                       202405L
/* SYNCHRONIZED_IO waits for msync, with the memory mapping functions */

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
 * ktimer_create rejects them, so __PICOLIBC_OS_CPUTIME stays undefined
 * until timers on them are emulated.
 *
 * FreeBSD delivers 62 real-time signals; picolibc numbers 31, so
 * __PICOLIBC_OS_RTSIG_MAX stays undefined
 */

/*
 * Options built on libos/pthread with thr_new and _umtx_op. Clock
 * selection and timeouts include pthread_condattr_setclock and the
 * timed locks, so they need the threads too. Priority inheritance and
 * robust mutexes would need the kernel's umutex layout, which
 * pthread_mutex_t doesn't have. Priority protection works, but no test
 * covers it without priority inheritance yet.
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
#define __PICOLIBC_OS_THREAD_PRIORITY_SCHEDULING   202405L
#define __PICOLIBC_OS_THREAD_PROCESS_SHARED        202405L
#define __PICOLIBC_OS_TIMEOUTS                     202405L
#endif

/* clang-format on */

#endif /* _SYS__POSIX_H_ */
