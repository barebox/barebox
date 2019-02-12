# SPDX-License-Identifier: GPL-2.0

ccflags-y				+= -I$(src)

obj-$(CONFIG_NVME_CORE)			+= nvme-core.o
obj-$(CONFIG_BLK_DEV_NVME)		+= nvme.o

nvme-core-y				:= core.o
nvme-y					+= pci.o
