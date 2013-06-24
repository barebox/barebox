dtb-$(CONFIG_ARCH_IMX51) += imx51-babbage.dtb \
	imx51-genesi-efika-sb.dtb
dtb-$(CONFIG_ARCH_IMX53) += imx53-qsb.dtb
dtb-$(CONFIG_ARCH_IMX6) += imx6q-dmo-realq7.dtb \
	imx6q-sabrelite.dtb \
	imx6q-sabresd.dtb

BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o

pbl-$(CONFIG_MACH_EFIKA_MX_SMARTBOOK) += imx51-genesi-efika-sb.dtb.o
pbl-$(CONFIG_MACH_FREESCALE_MX51_PDK) += imx51-babbage.dtb.o
pbl-$(CONFIG_MACH_FREESCALE_MX53_LOCO) += imx53-qsb.dtb.o
pbl-$(CONFIG_MACH_REALQ7) += imx6q-dmo-realq7.dtb.o

.SECONDARY: $(obj)/$(BUILTIN_DTB).dtb.S
.SECONDARY: $(patsubst %,$(obj)/%.S,$(dtb-y))

targets += dtbs
targets += $(dtb-y)

extra-y += $(dtb-y)

clean-files := *.dtb *.dtb.S
