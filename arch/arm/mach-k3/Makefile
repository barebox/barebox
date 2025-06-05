obj-y += common.o
obj-$(CONFIG_MACH_AM62X) += am62x.o
obj-$(CONFIG_MACH_AM62LX) += am62lx.o
obj-pbl-$(CONFIG_MACH_K3_CORTEX_R5) += r5.o
obj-pbl-y += ddrss.o
obj-$(CONFIG_BAREBOX_UPDATE) += bbu.o

extra-$(CONFIG_MACH_K3_CORTEX_R5) += combined-dm-cfg-am62x.k3cfg combined-sysfw-cfg-am62x.k3cfg

$(obj)/combined-dm-cfg-am62x.k3cfg: $(srctree)/arch/arm/mach-k3/schema.yaml \
				    $(obj)/pm-cfg-am62x.yaml \
				    $(obj)/rm-cfg-am62x.yaml \
				    FORCE
	$(call if_changed,k3_cfg)

$(obj)/combined-sysfw-cfg-am62x.k3cfg: $(srctree)/arch/arm/mach-k3/schema.yaml \
				       $(obj)/board-cfg-am62x.yaml \
				       $(obj)/sec-cfg-am62x.yaml \
				       $(obj)/pm-cfg-am62x.yaml \
				       $(obj)/rm-cfg-am62x.yaml \
				       FORCE
	$(call if_changed,k3_cfg)
