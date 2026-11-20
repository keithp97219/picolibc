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

/*
 * POSIX options and option groups provided by libos/linux, included
 * at the end of <unistd.h>. Only what is complete is defined;
 * anything undefined is not provided.
 */

#ifndef _SYS__POSIX_H_
#define _SYS__POSIX_H_

/* clang-format off */

/* Options POSIX.1-2024 requires, provided by the kernel */
#define _POSIX_CHOWN_RESTRICTED             1
#define _POSIX_NO_TRUNC                     1
#define _POSIX_SAVED_IDS                    1
#define _POSIX_SHELL                        1
#define _POSIX_VDISABLE                     '\0'

/* Options */
#define _POSIX_CLOCK_SELECTION              202405L
#define _POSIX_CPUTIME                      202405L
#define _POSIX_FSYNC                        202405L
#define _POSIX_MAPPED_FILES                 202405L
#define _POSIX_MEMLOCK                      202405L
#define _POSIX_MEMLOCK_RANGE                202405L
#define _POSIX_MEMORY_PROTECTION            202405L
#define _POSIX_MESSAGE_PASSING              202405L
#define _POSIX_MONOTONIC_CLOCK              202405L
#define _POSIX_REALTIME_SIGNALS             202405L
#define _POSIX_SHARED_MEMORY_OBJECTS        202405L
#define _POSIX_SYNCHRONIZED_IO              202405L
#define _POSIX_TIMEOUTS                     202405L
#define _POSIX_TIMERS                       202405L

/* Options built on the libos/linux threads */
#ifdef __PICOLIBC_PTHREADS
#define _POSIX_BARRIERS                     202405L
#define _POSIX_READER_WRITER_LOCKS          202405L
#define _POSIX_SEMAPHORES                   202405L
#define _POSIX_SPIN_LOCKS                   202405L
#define _POSIX_THREADS                      202405L
#define _POSIX_THREAD_ATTR_STACKADDR        202405L
#define _POSIX_THREAD_ATTR_STACKSIZE        202405L
#define _POSIX_THREAD_CPUTIME               202405L
#define _POSIX_THREAD_PRIO_INHERIT          202405L
#define _POSIX_THREAD_PRIO_PROTECT          202405L
#define _POSIX_THREAD_PRIORITY_SCHEDULING   202405L
#define _POSIX_THREAD_PROCESS_SHARED        202405L
#endif

/* Option groups */
#define __PICOLIBC_POSIX_C_LIB_EXT          202405L
#define __PICOLIBC_POSIX_DEVICE_IO          202405L
#define __PICOLIBC_POSIX_DEVICE_SPECIFIC_R  202405L
#define __PICOLIBC_POSIX_FD_MGMT            202405L
#define __PICOLIBC_POSIX_FILE_SYSTEM        202405L
#define __PICOLIBC_POSIX_FILE_SYSTEM_EXT    202405L
#define __PICOLIBC_POSIX_PIPE               200809L /* no pipe2 */
#define __PICOLIBC_POSIX_SIGNALS            202405L
#define __PICOLIBC_POSIX_SINGLE_PROCESS     202405L
#define __PICOLIBC_POSIX_USER_GROUPS        202405L

/* clang-format on */

#endif /* _SYS__POSIX_H_ */
