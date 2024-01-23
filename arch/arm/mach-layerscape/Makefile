# SPDX-License-Identifier: GPL-2.0-only

obj- := __dummy__.o
lwl-y += errata.o
lwl-$(CONFIG_ARCH_LS1046) += lowlevel.o lowlevel-ls1046a.o
obj-$(CONFIG_ARCH_LS1046) += icid.o
obj-pbl-y += boot.o soc.o
pbl-y += xload-qspi.o xload.o
pbl-$(CONFIG_ARCH_LS1028) += tzc400.o
obj-$(CONFIG_ARCH_LAYERSCAPE_PPA) += ppa.o ppa-entry.o
obj-$(CONFIG_BOOTM) += pblimage.o

lwl-$(CONFIG_ARCH_LS1021) += lowlevel-ls102xa.o
obj-$(CONFIG_ARCH_LS1021) += restart.o ls102xa_stream_id.o

lwl-$(CONFIG_ARCH_LS1028) += lowlevel-ls1028a.o
