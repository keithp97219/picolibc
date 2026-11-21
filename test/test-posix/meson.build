#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2025 Keith Packard
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

# One directory per POSIX header, named test- plus the header with '/'
# replaced by '-' and without '.h'. Each directory's meson.build lists
# its tests in dir_tests, or dir_thread_tests for those needing POSIX
# threads; its tests form a suite named after the directory.
posix_test_dirs = [
  'test-aio',
  'test-arpa-inet',
  'test-dirent',
  'test-fcntl',
  'test-fmtmsg',
  'test-fnmatch',
  'test-ftw',
  'test-glob',
  'test-grp',
  'test-integration',
  'test-libgen',
  'test-libintl',
  'test-monetary',
  'test-mqueue',
  'test-net-if',
  'test-netdb',
  'test-netinet-in',
  'test-nl_types',
  'test-poll',
  'test-pthread',
  'test-pwd',
  'test-regex',
  'test-sched',
  'test-search',
  'test-semaphore',
  'test-setjmp',
  'test-signal',
  'test-spawn',
  'test-stdatomic',
  'test-stdio',
  'test-stdlib',
  'test-sys-ipc',
  'test-sys-mman',
  'test-sys-msg',
  'test-sys-resource',
  'test-sys-select',
  'test-sys-sem',
  'test-sys-shm',
  'test-sys-socket',
  'test-sys-stat',
  'test-sys-statvfs',
  'test-sys-time',
  'test-sys-times',
  'test-sys-uio',
  'test-sys-utsname',
  'test-sys-wait',
  'test-syslog',
  'test-termios',
  'test-threads',
  'test-time',
  'test-unistd',
  'test-utmpx',
  'test-wchar',
  'test-wordexp',
]

test_skip = []
test_skip_native = []

# Thread tests use the library's own locks, not lock-valid.c
test_skip_lock_valid = []

# Per-test compiler arguments
test_extra_c_args = {}

# Each entry is [directory, test name]
posix_tests = []
# Tests that skip without operating system support, found by their check
posix_os_tests = []
foreach d : posix_test_dirs
  dir_tests = []
  dir_thread_tests = []
  subdir(d)
  foreach t : dir_tests + dir_thread_tests
    posix_tests += [[d, t]]
    if fs.read(d / t + '.c').contains('Without the system support this test needs')
      posix_os_tests += t
    endif
  endforeach
  test_skip_lock_valid += dir_thread_tests
endforeach

test_extra_libs = {}

args_test_posix = []
if tests_enable_posix_io
  args_test_posix += ['-DTESTS_ENABLE_POSIX_IO']
endif

test_args = {}

foreach params : targets
  target = params['name']
  target_dir = params['dir']
  target_c_args = params['c_args']

  _libs = [get_variable('lib_c_link' + target)]
  if is_variable(oslib_test + target)
    _libs += [get_variable(oslib_test + target)]
  endif
  if is_variable(libfallback_var + target)
    _libs += [get_variable(libfallback_var + target)]
  endif

  _lib_files=[]
  foreach _lib : _libs
    _lib_files += _lib.full_path()
  endforeach

  if is_variable(crt0_test + target)
    _objs = [get_variable(crt0_test + target)]
  else
    _objs = []
  endif

  _c_args = target_c_args + get_variable('test_c_args' + target, test_c_args) + args_test_posix
  _link_args = target_c_args + _lib_files + get_variable('test_link_args' + target, test_link_args)
  _link_depends = get_variable('test_link_depends' + target, test_link_depends) + _libs
  foreach entry : posix_tests
    t1_dir = entry[0]
    t1 = entry[1]
    if test_skip.contains(t1)
      continue
    endif
    # Without an OS they only skip, so building them for one target is enough
    if not has_os_posix and target != targets[0]['name'] and t1 in posix_os_tests
      continue
    endif

    t1_src = t1_dir / t1 + '.c'
    test_file_name_arg=['-DTEST_FILE_NAME="' + t1 + target + '.txt"']

    if test_extra_libs.has_key(t1) or t1 in test_skip_lock_valid
      lock_source = []
    else
      lock_source = test_lock_valid
    endif

    # Tests may share a name with their directory, so give the files a suffix
    test(t1 + target,
	 executable(t1 + target, [t1_src] + lock_source,
		    name_suffix: 'elf',
		    c_args: printf_compile_args_d + test_file_name_arg + test_extra_c_args.get(t1, []) + _c_args,
		    link_args: printf_link_args_d + _link_args,
		    objects: _objs,
		    link_depends:  _link_depends,
                    link_with: test_extra_libs.get(t1, []),
		    include_directories: inc),
         depends: bios_bin,
         args: test_args.get(t1, []),
	 timeout: 60,
         suite: [t1_dir, 'test-posix'],
	 env: test_env)
  endforeach

endforeach

if enable_native_tests

  # Tell the tests when they're built against glibc, which some need to
  # treat specially
  native_glibc_args = []
  if cc.get_define('__GLIBC__', prefix: '#include <stdio.h>', args: native_c_args) != ''
    native_glibc_args = ['-DTEST_GLIBC']
  endif

  foreach entry : posix_tests
    t1_dir = entry[0]
    t1 = entry[1]
    if t1 in test_skip_native
      continue
    endif
    t1_src = t1_dir / t1 + '.c'
    t1_name = t1 + '-native'
    test_file_name_arg=['-DTEST_FILE_NAME="' + t1_name + '.txt"']

    test(t1_name,
	 executable(t1_name, t1_src,
		    name_suffix: 'elf',
		    c_args: native_c_args + test_file_name_arg + ctype_c_args + args_test_posix + native_glibc_args,
		    link_args: native_c_args,
                    dependencies: [native_lib_m, native_thread_dep]),
         suite: [t1_dir, 'test-linux'],
         env: test_env
        )
  endforeach

endif
