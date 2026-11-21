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
 * picolibc's uname without an OS: applications without <sys/_posix.h>
 * reach it as __baremetal_uname. It's also the weak uname an OS
 * overrides.
 */
#include <picolibc.h>
#include <string.h>
#include <sys/utsname.h>

#if defined(__aarch64__)
#define MACHINE "aarch64"
#elif defined(__arm__)
#define MACHINE "arm"
#elif defined(__x86_64__)
#define MACHINE "x86_64"
#elif defined(__i386__)
#define MACHINE "i386"
#elif defined(__riscv) && __riscv_xlen == 64
#define MACHINE "riscv64"
#elif defined(__riscv)
#define MACHINE "riscv32"
#elif defined(__hexagon__)
#define MACHINE "hexagon"
#elif defined(__mips__)
#define MACHINE "mips"
#elif defined(__powerpc64__)
#define MACHINE "powerpc64"
#elif defined(__powerpc__)
#define MACHINE "powerpc"
#elif defined(__m68k__)
#define MACHINE "m68k"
#elif defined(__sparc__)
#define MACHINE "sparc"
#elif defined(__xtensa__)
#define MACHINE "xtensa"
#elif defined(__AVR__)
#define MACHINE "avr"
#elif defined(__MSP430__)
#define MACHINE "msp430"
#elif defined(__loongarch__)
#define MACHINE "loongarch"
#elif defined(__or1k__)
#define MACHINE "or1k"
#elif defined(__microblaze__)
#define MACHINE "microblaze"
#elif defined(__sh__)
#define MACHINE "sh"
#elif defined(__nios2__)
#define MACHINE "nios2"
#elif defined(__arc__)
#define MACHINE "arc"
#else
#define MACHINE "unknown"
#endif

int
__baremetal_uname(struct utsname *name)
{
    memset(name, 0, sizeof(*name));
    strcpy(name->sysname, "picolibc");
    strcpy(name->release, __PICOLIBC_VERSION__);
    strcpy(name->version, "bare metal");
    strcpy(name->machine, MACHINE);
    return 0;
}

#ifdef __weak_reference
__weak_reference(__baremetal_uname, uname);
#else
int
uname(struct utsname *name)
{
    return __baremetal_uname(name);
}
#endif
