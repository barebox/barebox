# SPDX-License-Identifier: GPL-2.0

obj-y += core.o time.o
obj-$(CONFIG_HAS_DMA) += dma.o
obj-pbl-$(CONFIG_RISCV_M_MODE) += mtrap.o
obj-pbl-$(CONFIG_RISCV_S_MODE) += strap.o
obj-pbl-y += interrupts.o
