# SPDX-License-Identifier: GPL-2.0-only
#
# just to build a built-in.o. Otherwise compilation fails when no devicetree is
# created.
obj- += dummy.o

lwl-$(CONFIG_BUILTIN_DTB) += $(CONFIG_BUILTIN_DTB_NAME).dtb.o
lwl-$(CONFIG_BOARD_K200) += k200.dtb.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.z
clean-files += *.dtbo *.dtbo.S .*.dtso
