# SPDX-License-Identifier: GPL-2.0-only

lwl-$(CONFIG_OFTREE) += \
	sandbox.dtb.o

# just to build a built-in.o. Otherwise compilation fails when no devicetree is
# created.
obj- += dummy.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.z
clean-files += *.dtbo *.dtbo.S .*.dtso
