obj- := __dummy__.o
lwl-y += lowlevel.o errata.o
lwl-$(CONFIG_ARCH_LS1046) += lowlevel-ls1046a.o
obj-y += icid.o
obj-pbl-y += boot.o
pbl-y += xload-qspi.o xload.o
obj-$(CONFIG_ARCH_LAYERSCAPE_PPA) += ppa.o ppa-entry.o
obj-$(CONFIG_BOOTM) += pblimage.o
