# SPDX-License-Identifier: GPL-2.0-only

obj-y += board.o
obj-y += qemu-virt-flash.dtbo.o
ifeq ($(CONFIG_RISCV),y)
DTC_CPP_FLAGS_qemu-virt-flash.dtbo := -DCONFIG_RISCV
endif
ifeq ($(CONFIG_ARM),y)
DTC_CPP_FLAGS_qemu-virt-flash.dtbo := -DCONFIG_ARM
endif

clean-files += *.dtbo *.dtbo.S .*.dtso
