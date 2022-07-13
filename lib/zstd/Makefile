# SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
# ################################################################
# Copyright (c) Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under both the BSD-style license (found in the
# LICENSE file in the root directory of this source tree) and the GPLv2 (found
# in the COPYING file in the root directory of this source tree).
# You may select, at your option, one of the above-listed licenses.
# ################################################################
obj-$(CONFIG_ZSTD_DECOMPRESS) += zstd_decompress.o

zstd_decompress-y := \
		zstd_decompress_module.o \
		common/debug.o \
		common/entropy_common.o \
		common/error_private.o \
		common/fse_decompress.o \
		common/zstd_common.o \
		decompress/huf_decompress.o \
		decompress/zstd_ddict.o \
		decompress/zstd_decompress.o \
		decompress/zstd_decompress_block.o \
