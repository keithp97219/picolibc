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

# libos/pthread over a stub OS backend which places TLS itself and
# keeps running after the last thread exits, as an RTOS backend does.
# The core is built here against the stub's local-pthread-os.h and
# <sys/_posix.h>, so this runs where picolibc has no OS layer of its
# own; libc.a is the same either way.

if oslib_test_variant == 'semihost' and thread_local_storage and not get_option('single-thread')

  test_pthread_os_inc = [include_directories('.', 'include', '../../libos/fallback'),
                         pthread_inc, pthread_sys_inc, inc]

  foreach params : targets
    target = params['name']
    target_c_args = params['c_args']

    _core = static_library('pthread-os-stub' + target,
                           src_os_pthread_all,
                           install : false,
                           include_directories : test_pthread_os_inc,
                           c_args : target_c_args + c_args)

    _libs = [get_variable('lib_c_link' + target)]
    if is_variable(oslib_test + target)
      _libs += [get_variable(oslib_test + target)]
    endif
    if is_variable(libfallback_var + target)
      _libs += [get_variable(libfallback_var + target)]
    endif

    # The core ahead of libc, which it uses
    _lib_files = [_core.full_path()]
    foreach _lib : _libs
      _lib_files += _lib.full_path()
    endforeach

    if is_variable(crt0_test + target)
      _objs = [get_variable(crt0_test + target)]
    else
      _objs = []
    endif

    _c_args = target_c_args + get_variable('test_c_args' + target, test_c_args)
    _link_args = target_c_args + _lib_files + get_variable('test_link_args' + target, test_link_args)
    _link_depends = get_variable('test_link_depends' + target, test_link_depends) + _libs + [_core]

    t1 = 'test-pthread-os'
    test(t1 + target,
         executable(t1 + target, [t1 + '.c'],
                    c_args : printf_compile_args_d + _c_args,
                    link_args : printf_link_args_d + _link_args,
                    objects : _objs,
                    link_depends : _link_depends,
                    include_directories : test_pthread_os_inc),
         depends : bios_bin,
         timeout : 60,
         suite : 'test-pthread-os',
         env : test_env)
  endforeach
endif
