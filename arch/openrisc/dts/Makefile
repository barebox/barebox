# SPDX-License-Identifier: GPL-2.0-only

BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
lwl-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.z
clean-files += *.dtbo *.dtbo.S .*.dtso
