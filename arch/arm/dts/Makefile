dtb-$(CONFIG_ARCH_IMX51) += imx51-babbage.dtb \
	imx51-genesi-efika-sb.dtb
dtb-$(CONFIG_ARCH_IMX6) += imx6q-sabrelite.dtb \
	imx6q-sabresd.dtb

BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o

.SECONDARY: $(obj)/$(BUILTIN_DTB).dtb.S

targets += dtbs
targets += $(dtb-y)

extra-y += $(dtb-y)

clean-files := *.dtb *.dtb.S
