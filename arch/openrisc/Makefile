# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := generic_defconfig

KBUILD_CPPFLAGS += -D__OR1K__ -ffixed-r10 -mhard-mul -mhard-div

board-$(CONFIG_GENERIC) := generic

KALLSYMS += --symbol-prefix=_

archprepare: maketools

PHONY += maketools

ifneq ($(board-y),)
core-y += arch/openrisc/boards/$(board-y)/
endif

lds-y += arch/openrisc/cpu/barebox.lds

dts := arch/openrisc/dts

%.dtb: scripts
	$(Q)$(MAKE) $(build)=$(dts) $(dts)/$@

TEXT_BASE = $(CONFIG_ARCH_TEXT_BASE)

KBUILD_CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE)

KBUILD_IMAGE := barebox
