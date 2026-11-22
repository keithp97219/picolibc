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

# In claim-all mode (doc/testing.md), test-flags.h claims everything.
# Tests that then run code the library's real claims leave out are
# expected to fail, and claim-all.py, as the compile and link launcher,
# swaps in a failing stand-in for a test that doesn't build.
if(TESTS_CLAIM_ALL)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(PICOLIBC_CLAIM_ALL ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/claim-all.py)

  set(claim_all_args ${PICOLIBC_TEST_COMPILE_OPTIONS} -D_TEST_LIBC)
  list(REMOVE_ITEM claim_all_args -D_LIBC)
  foreach(dir IN LISTS PICOLIBC_INCLUDE_DIRECTORIES)
    list(APPEND claim_all_args -I${dir})
  endforeach()
  file(GLOB claim_all_sources ${CMAKE_CURRENT_SOURCE_DIR}/test-*/test-*.c)
  execute_process(
    COMMAND ${PICOLIBC_CLAIM_ALL} expect ${CMAKE_CURRENT_SOURCE_DIR}/test-flags.h
            ${CMAKE_CURRENT_BINARY_DIR}/claim-all-expect.txt ${claim_all_sources}
            -- ${CMAKE_C_COMPILER} ${claim_all_args}
    OUTPUT_VARIABLE claim_all_expect
    RESULT_VARIABLE claim_all_result)
  if(NOT claim_all_result EQUAL 0)
    message(FATAL_ERROR "claim-all.py expect failed")
  endif()

  # The first word of each line names a test expected to fail
  set(PICOLIBC_CLAIM_ALL_EXPECT_FAIL)
  string(REPLACE "\n" ";" claim_all_expect "${claim_all_expect}")
  foreach(line IN LISTS claim_all_expect)
    string(REGEX REPLACE " .*" "" name "${line}")
    if(name)
      list(APPEND PICOLIBC_CLAIM_ALL_EXPECT_FAIL ${name})
    endif()
  endforeach()
  list(LENGTH PICOLIBC_CLAIM_ALL_EXPECT_FAIL claim_all_count)
  message(STATUS "claim-all: ${claim_all_count} POSIX tests expected to fail")

  # picolibc_test applies the mode to the tests below here
  set(PICOLIBC_CLAIM_ALL_POSIX ON)
endif()

add_subdirectory(test-aio)
add_subdirectory(test-arpa-inet)
add_subdirectory(test-assert)
add_subdirectory(test-complex)
add_subdirectory(test-ctype)
add_subdirectory(test-devctl)
add_subdirectory(test-dirent)
add_subdirectory(test-dlfcn)
add_subdirectory(test-endian)
add_subdirectory(test-errno)
add_subdirectory(test-error)
add_subdirectory(test-fcntl)
add_subdirectory(test-fenv)
add_subdirectory(test-fmtmsg)
add_subdirectory(test-fnmatch)
add_subdirectory(test-ftw)
add_subdirectory(test-glob)
add_subdirectory(test-grp)
add_subdirectory(test-iconv)
add_subdirectory(test-integration)
add_subdirectory(test-inttypes)
add_subdirectory(test-langinfo)
add_subdirectory(test-libgen)
add_subdirectory(test-libintl)
add_subdirectory(test-locale)
add_subdirectory(test-math)
add_subdirectory(test-mntent)
add_subdirectory(test-monetary)
add_subdirectory(test-mqueue)
add_subdirectory(test-ndbm)
add_subdirectory(test-net-if)
add_subdirectory(test-netdb)
add_subdirectory(test-netinet-in)
add_subdirectory(test-nl_types)
add_subdirectory(test-poll)
add_subdirectory(test-pthread)
add_subdirectory(test-pwd)
add_subdirectory(test-regex)
add_subdirectory(test-sched)
add_subdirectory(test-search)
add_subdirectory(test-semaphore)
add_subdirectory(test-setjmp)
add_subdirectory(test-signal)
add_subdirectory(test-spawn)
add_subdirectory(test-stdatomic)
add_subdirectory(test-stdio)
add_subdirectory(test-stdlib)
add_subdirectory(test-string)
add_subdirectory(test-strings)
add_subdirectory(test-sys-ipc)
add_subdirectory(test-sys-mman)
add_subdirectory(test-sys-msg)
add_subdirectory(test-sys-random)
add_subdirectory(test-sys-resource)
add_subdirectory(test-sys-select)
add_subdirectory(test-sys-sem)
add_subdirectory(test-sys-shm)
add_subdirectory(test-sys-socket)
add_subdirectory(test-sys-stat)
add_subdirectory(test-sys-statvfs)
add_subdirectory(test-sys-sysinfo)
add_subdirectory(test-sys-time)
add_subdirectory(test-sys-times)
add_subdirectory(test-sys-uio)
add_subdirectory(test-sys-utsname)
add_subdirectory(test-sys-wait)
add_subdirectory(test-syslog)
add_subdirectory(test-termios)
add_subdirectory(test-threads)
add_subdirectory(test-time)
add_subdirectory(test-uchar)
add_subdirectory(test-unistd)
add_subdirectory(test-utime)
add_subdirectory(test-utmpx)
add_subdirectory(test-wchar)
add_subdirectory(test-wctype)
add_subdirectory(test-wordexp)
