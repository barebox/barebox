# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := generic_defconfig

KBUILD_CPPFLAGS += -fno-strict-aliasing

KALLSYMS += --symbol-prefix=_

ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE    := kvx-elf-
endif

DEFAULT_CFLAGS := -fno-builtin -fstrict-align -g
DEFAULT_CFLAGS += -DTEXT_BASE=$(CONFIG_TEXT_BASE)

LIBGCC_PATH = $(dir $(shell $(CC) $(KBUILD_CFLAGS) --print-libgcc-file-name))

KVX_BUILD_FLAGS-y = $(DEFAULT_CFLAGS)
KVX_BUILD_FLAGS-$(CONFIG_ARCH_COOLIDGE_V1) += -march=kv3-1
KVX_BUILD_FLAGS-$(CONFIG_ARCH_COOLIDGE_V2) += -march=kv3-2
KBUILD_CFLAGS += $(KVX_BUILD_FLAGS-y)
KBUILD_AFLAGS += $(KVX_BUILD_FLAGS-y)

KBUILD_LDFLAGS += -m elf64kvx

archprepare: maketools

PHONY += maketools

common-y += arch/kvx/lib/
common-y += arch/kvx/cpu/
common-$(CONFIG_OFTREE) += arch/kvx/dts/

lds-y += arch/kvx/cpu/barebox.lds

cmd_barebox__ ?= $(LD) $(KBUILD_LDFLAGS) $(LDFLAGS_barebox) -o $@ \
      -T $(BAREBOX_LDS) \
      --whole-archive $(BAREBOX_OBJS) --no-whole-archive \
      -L$(LIBGCC_PATH) -lgcc \
      $(filter-out $(BAREBOX_LDS) $(BAREBOX_OBJS) FORCE ,$^)
