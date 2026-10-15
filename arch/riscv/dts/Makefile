# SPDX-License-Identifier: GPL-2.0

ifdef CONFIG_EFI_PAYLOAD
obj-pbl-$(CONFIG_OFDEVICE) += fallback.dtb.o
endif

lwl-$(CONFIG_BOARD_ERIZO_GENERIC) += erizo-generic.dtb.o
lwl-$(CONFIG_BOARD_HIFIVE) += hifive-unmatched-a00.dtb.o \
                              hifive-unleashed-a00.dtb.o
lwl-$(CONFIG_BOARD_BEAGLEV) += jh7100-beaglev-starlight.dtb.o
lwl-$(CONFIG_BOARD_LITEX_LINUX) += litex-linux.dtb.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.z
clean-files += *.dtbo *.dtbo.S .*.dtso
