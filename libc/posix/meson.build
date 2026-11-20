#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2020 Keith Packard
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
srcs_posix = [
  'alphasort.c',
  'basename.c',
  'creat.c',
  'collcmp.c',
  'confstr.c',
  'ctermid.c',
  'dirfd.c',
  'dirname.c',
  'endgrent.c',
  'endpwent.c',
  'execl.c',
  'execle.c',
  'execlp.c',
  'execv.c',
  'execvp.c',
  'execvpe.c',
  'fgetgrent.c',
  'fgetgrent_r.c',
  'fgetpwent.c',
  'fgetpwent_r.c',
  'fnmatch.c',
  'getgrent.c',
  'getgrent_r.c',
  'getgrgid.c',
  'getgrgid_r.c',
  'getgrnam.c',
  'getgrnam_r.c',
  'getlogin.c',
  'getlogin_r.c',
  'getpass.c',
  'getpwent.c',
  'getpwent_r.c',
  'getpwnam.c',
  'getpwnam_r.c',
  'getpwuid.c',
  'getpwuid_r.c',
  'glob.c',
  'grdata.c',
  'grfile.c',
  'pwdata.c',
  'pwfile.c',
  'posix_close.c',
  'readdir_r.c',
  'realpath.c',
  'regcomp.c',
  'regerror.c',
  'regexec.c',
  'regfree.c',
  'scandir.c',
  'setgrent.c',
  'setpwent.c',
  'sleep.c',
  'strfmon.c',
  'strfmon_l.c',
  'vstrfmon.c',
  'wait.c',
]

# One directory for each header, holding the functions it declares
posix_dirs = [
  'aio',
  'spawn',
]

foreach d : posix_dirs
  dir_srcs = []
  subdir(d)
  foreach file : dir_srcs
    srcs_posix += d / file
  endforeach
endforeach

srcs_posix_use = []
foreach path : srcs_posix
  file = fs.name(path)
  s_file = fs.replace_suffix(file, '.S')
  if file in srcs_machine
    message('libc/posix/' + path + ': machine overrides generic')
  elif s_file in srcs_machine
    message('libc/posix/' + fs.replace_suffix(path, '.S') + ': machine overrides generic')
  elif file in srcs_libos
    message('libc/posix/' + path + ': libos overrides generic')
  else
    srcs_posix_use += path
  endif
endforeach

src_posix = files(srcs_posix_use)
  
