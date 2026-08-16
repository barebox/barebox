# SPDX-License-Identifier: GPL-2.0-only

obj-y += sleep.o
obj-y += common.o
obj-y += devices.o

obj-$(CONFIG_ARCH_PXA3XX) += mfp-pxa3xx.o pxa3xx.o
obj-$(CONFIG_BAREBOX_UPDATE_PXA_NAND) += bbu.o

pbl-$(CONFIG_ARCH_PXA3XX) += xload-nand.o
