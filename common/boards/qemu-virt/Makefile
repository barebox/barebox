# SPDX-License-Identifier: GPL-2.0-only

obj-y += board.o commandline.o
obj-y += qemu-virt-flash.dtbo.o fitimage-pubkey.dtb.o
ifeq ($(CONFIG_RISCV),y)
DTC_CPP_FLAGS_qemu-virt-flash.dtbo := -DCONFIG_RISCV
endif
ifeq ($(CONFIG_ARM),y)
DTC_CPP_FLAGS_qemu-virt-flash.dtbo := -DCONFIG_ARM
endif

policy-y += qemu-virt-factory.sconfig
policy-y += qemu-virt-lockdown.sconfig

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.z
clean-files += *.dtbo *.dtbo.S .*.dtso
