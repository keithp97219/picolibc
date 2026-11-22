#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2026 Keith Packard
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#

srcs_os_fallback = [
  'fpathconf.c',
  'getentropy.c',
  'getpid.c',
  'kill.c',
  'pathconf.c',
  'raise.c',
  'sbrk.c',
  'sigaction.c',
  'signal.c',
  'sigprocmask.c',
  'sysconf.c',
  'uname.c',
  ]

src_os_fallback = files(srcs_os_fallback)

if has_os_fallback

  has_os_library = true

  foreach params : targets
    target = params['name']
    target_dir = params['dir']
    target_c_args = params['c_args']

    instdir = join_paths(lib_dir, target_dir)

    if meson.version().version_compare('>=1.10')
      _lib = static_library(libfallback_name,
                            src_os_fallback,
                            build_subdir : target_dir,
                            install : really_install,
                            install_dir : instdir,
                            include_directories : inc,
                            c_args : target_c_args + c_args)
    else
      target_lib_prefix = params['lib_prefix']
      _lib = static_library(join_paths(target_dir,
                                       target_lib_prefix + libfallback_name),
                            src_os_fallback,
                            install : really_install,
                            install_dir : instdir,
                            include_directories : inc,
                            c_args : target_c_args + c_args)
    endif

    set_variable(libfallback_var + target, _lib)

  endforeach
endif

# With os-in-libc, the fallback sits in libc.a underneath the OS layer,
# which provides every OS interface itself (doc/os.md) but may call
# the fallback's helpers (__fallback_fpathconf, ...). Built with
# __FALLBACK_HELPERS_ONLY, these objects define only __fallback_* and
# __baremetal_* names, none of the public ones: in a single archive a
# second definition of a public name, weak or not, could be chosen
# over the OS layer's. kill.c and getpid.c have no helpers.
srcs_os_fallback_helpers = []
foreach file : srcs_os_fallback
  if file not in ['getpid.c', 'kill.c']
    srcs_os_fallback_helpers += file
  endif
endforeach

if os_in_libc
  foreach params : targets
    target = params['name']
    target_dir = params['dir']
    target_c_args = params['c_args']

    if meson.version().version_compare('>=1.10')
      _lib = static_library(libfallback_name + '-helpers',
                            files(srcs_os_fallback_helpers),
                            build_subdir : target_dir,
                            install : false,
                            include_directories : inc,
                            c_args : target_c_args + c_args + ['-D__FALLBACK_HELPERS_ONLY'])
    else
      target_lib_prefix = params['lib_prefix']
      _lib = static_library(join_paths(target_dir,
                                       target_lib_prefix + libfallback_name + '-helpers'),
                            files(srcs_os_fallback_helpers),
                            install : false,
                            include_directories : inc,
                            c_args : target_c_args + c_args + ['-D__FALLBACK_HELPERS_ONLY'])
    endif

    set_variable('libos_fallback_lib' + target, _lib)

  endforeach
endif
