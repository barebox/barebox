# SPDX-License-Identifier: GPL-2.0-only

obj-y += board.o
obj-y += overlay-of-flash.dtb.o
ifeq ($(CONFIG_RISCV),y)
DTC_CPP_FLAGS_overlay-of-flash.dtb := -DRISCV_VIRT=1
endif
