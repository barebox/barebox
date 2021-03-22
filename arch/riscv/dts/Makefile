# SPDX-License-Identifier: GPL-2.0

# just to build a built-in.o. Otherwise compilation fails when no devicetree is
# created.
obj- += dummy.o

pbl-$(CONFIG_BOARD_ERIZO_GENERIC) += erizo-generic.dtb.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts
