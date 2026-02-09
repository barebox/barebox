# SPDX-License-Identifier: GPL-2.0-only

obj-y			+= traps.o
obj-y			+= tlb.o
obj-y			+= cache.o
obj-$(CONFIG_MMU)	+= mmu.o
extra-$(CONFIG_QEMU_PPCE500) += start-qemu.o
ifndef CONFIG_QEMU_PPCE500
extra-y			+= start.o
extra-y			+= resetvec.o
endif
