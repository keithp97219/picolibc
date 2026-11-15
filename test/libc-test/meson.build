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
#
# Thread, semaphore and TLS tests from musl's libc-test; see
# README.picolibc
#

libc_test_tests = [
  'functional/pthread_cancel',
  'functional/pthread_cancel-points',
  'functional/pthread_cond',
  'functional/pthread_mutex',
  'functional/pthread_mutex_pi',
  'functional/pthread_tsd',
  'functional/sem_init',
  'functional/sem_open',
  'functional/tls_init',
  'functional/tls_local_exec',
  'regression/pthread_cancel-sem_wait',
  'regression/pthread_cond-smasher',
  'regression/pthread_cond_wait-cancel_ignored',
  'regression/pthread_create-oom',
  'regression/pthread_exit-cancel',
  'regression/pthread_exit-dtor',
  'regression/pthread_once-deadlock',
  'regression/pthread_rwlock-ebusy',
  'regression/sem_close-unmap',
]

# These need features picolibc doesn't have yet
libc_test_picolibc_skip = [
  'functional/pthread_robust',                  # robust mutexes
  'regression/pthread-robust-detach',           # robust mutexes
  'regression/pthread_atfork-errno-clobber',    # RLIMIT_NPROC
  'regression/pthread_condattr_setclock',       # CPU-time clocks
  'regression/raise-race',                      # real-time signals
]

# glibc fails this one: it forks in a signal handler while raise runs
libc_test_native_skip = [
  'regression/raise-race',
]

libc_test_common = files(['common/print.c', 'common/setrlim.c', 'common/memfill.c', 'common/vmfill.c'])
libc_test_inc = include_directories('common')

# The upstream build flags, less those meson already supplies
libc_test_c_args = cc.get_supported_arguments(['-D_POSIX_C_SOURCE=200809L', '-fno-builtin',
                                              '-Wno-unused-function', '-Wno-unused',
                                              '-Wno-overflow', '-Wno-unknown-pragmas',
                                              '-Wno-missing-prototypes',
                                              '-Wno-missing-declarations',
                                              '-Wno-sign-compare',
                                              '-Wno-old-style-definition',
                                              '-Wno-deprecated-declarations'])

if posix_threads
  foreach params : targets
    target = params['name']
    target_c_args = params['c_args']

    _libs = [get_variable('lib_c_link' + target)]
    if is_variable(oslib_test + target)
      _libs += [get_variable(oslib_test + target)]
    endif
    if is_variable(libfallback_var + target)
      _libs += [get_variable(libfallback_var + target)]
    endif

    if is_variable(crt0_test + target)
      _objs = [get_variable(crt0_test + target)]
    else
      _objs = []
    endif

    _lib_files = []
    foreach _lib : _libs
      _lib_files += _lib.full_path()
    endforeach

    _c_args = target_c_args + get_variable('test_c_args' + target, test_c_args) + libc_test_c_args
    _link_args = target_c_args + _lib_files + get_variable('test_link_args' + target, test_link_args)
    _link_depends = get_variable('test_link_depends' + target, test_link_depends) + _libs

    foreach t1 : libc_test_tests
      t1_name = 'libc-test-' + t1.replace('/', '-') + target
      test(t1_name,
           executable(t1_name, [t1 + '.c'] + libc_test_common,
                      c_args: printf_compile_args_d + _c_args,
                      link_args: printf_link_args_d + _link_args,
                      objects: _objs,
                      link_depends: _link_depends,
                      include_directories: [inc, libc_test_inc]),
           depends: bios_bin,
           timeout: 60,
           suite: 'libc-test',
           env: test_env)
    endforeach
  endforeach
endif

if enable_native_tests
  foreach t1 : libc_test_tests + libc_test_picolibc_skip
    if t1 in libc_test_native_skip
      continue
    endif
    t1_name = 'libc-test-' + t1.replace('/', '-') + '-native'
    test(t1_name,
         executable(t1_name, [t1 + '.c'] + libc_test_common,
                    c_args: native_c_args + libc_test_c_args,
                    link_args: native_c_args,
                    include_directories: libc_test_inc,
                    dependencies: native_thread_dep),
         timeout: 60,
         suite: 'libc-test',
         env: test_env)
  endforeach
endif
