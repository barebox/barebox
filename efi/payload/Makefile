# SPDX-License-Identifier: GPL-2.0-only

obj-y += init.o
obj-y += image.o
obj-$(CONFIG_OFTREE) += fdt.o
bbenv-y += env-efi
obj-$(CONFIG_CMD_IOMEM) += iomem.o
obj-pbl-$(CONFIG_EFI_PAYLOAD) += early-mem.o
obj-$(CONFIG_EFI_PAYLOAD) += entry-single.o
