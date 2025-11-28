# SPDX-License-Identifier: GPL-2.0-only

KBUILD_DEFCONFIG := generic_defconfig

KBUILD_CPPFLAGS += -D__OR1K__ -ffixed-r10 -mhard-mul -mhard-div

board-$(CONFIG_GENERIC) := generic

KALLSYMS += --symbol-prefix=_

archprepare: maketools

PHONY += maketools

ifneq ($(board-y),)
BOARD  := arch/openrisc/boards/$(board-y)/
else
BOARD  :=
endif

common-y += $(BOARD)
common-y += arch/openrisc/lib/
common-y += arch/openrisc/cpu/

lds-y += arch/openrisc/cpu/barebox.lds

common-$(CONFIG_BUILTIN_DTB) += arch/openrisc/dts/

dts := arch/openrisc/dts

%.dtb: scripts
	$(Q)$(MAKE) $(build)=$(dts) $(dts)/$@

TEXT_BASE = $(CONFIG_ARCH_TEXT_BASE)

KBUILD_CPPFLAGS += -DTEXT_BASE=$(TEXT_BASE)

KBUILD_IMAGE := barebox
