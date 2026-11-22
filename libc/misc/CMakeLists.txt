#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2022 Keith Packard
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
picolibc_sources(
  atomic_flag_clear.c
  atomic_flag_clear_explicit.c
  atomic_flag_test_and_set.c
  atomic_flag_test_and_set_explicit.c
  atomic_signal_fence.c
  atomic_thread_fence.c
  dso_handle.c
  getauxval.c
  ffs.c
  fini.c
  init.c
  inittls.c
  libatomic_compare_exchange.c
  libatomic_compare_exchange_1.c
  libatomic_compare_exchange_16.c
  libatomic_compare_exchange_2.c
  libatomic_compare_exchange_4.c
  libatomic_compare_exchange_8.c
  libatomic_exchange.c
  libatomic_exchange_1.c
  libatomic_exchange_16.c
  libatomic_exchange_2.c
  libatomic_exchange_4.c
  libatomic_exchange_8.c
  libatomic_fetch_add_1.c
  libatomic_fetch_add_16.c
  libatomic_fetch_add_2.c
  libatomic_fetch_add_4.c
  libatomic_fetch_add_8.c
  libatomic_fetch_and_1.c
  libatomic_fetch_and_16.c
  libatomic_fetch_and_2.c
  libatomic_fetch_and_4.c
  libatomic_fetch_and_8.c
  libatomic_fetch_nand_1.c
  libatomic_fetch_nand_16.c
  libatomic_fetch_nand_2.c
  libatomic_fetch_nand_4.c
  libatomic_fetch_nand_8.c
  libatomic_fetch_or_1.c
  libatomic_fetch_or_16.c
  libatomic_fetch_or_2.c
  libatomic_fetch_or_4.c
  libatomic_fetch_or_8.c
  libatomic_fetch_sub_1.c
  libatomic_fetch_sub_16.c
  libatomic_fetch_sub_2.c
  libatomic_fetch_sub_4.c
  libatomic_fetch_sub_8.c
  libatomic_fetch_xor_1.c
  libatomic_fetch_xor_16.c
  libatomic_fetch_xor_2.c
  libatomic_fetch_xor_4.c
  libatomic_fetch_xor_8.c
  libatomic_is_lock_free.c
  libatomic_load.c
  libatomic_load_1.c
  libatomic_load_16.c
  libatomic_load_2.c
  libatomic_load_4.c
  libatomic_load_8.c
  libatomic_lock.c
  libatomic_store.c
  libatomic_store_1.c
  libatomic_store_16.c
  libatomic_store_2.c
  libatomic_store_4.c
  libatomic_store_8.c
  libatomic_test_and_set.c
  lock.c
  unctrl.c
  )
