# SPDX-License-Identifier: GPL-2.0-only

#
# only unsed by the pbl
#
pbl-$(CONFIG_PBL_CONSTRUCTORS) += ctors.o
pbl-y += misc.o
pbl-y += string.o
pbl-y += malloc.o
pbl-y += sha256.o
pbl-$(CONFIG_HAVE_IMAGE_COMPRESSION) += decomp.o
pbl-$(CONFIG_LIBFDT) += fdt.o
pbl-$(CONFIG_PBL_CONSOLE) += console.o
obj-pbl-y += handoff-data.o
obj-pbl-$(CONFIG_MMU) += mmu.o
