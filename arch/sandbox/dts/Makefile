ifeq ($(CONFIG_OFTREE),y)
dtb-y += \
	sandbox.dtb
endif

# just to build a built-in.o. Otherwise compilation fails when no devicetree is
# created.
obj- += dummy.o

always := $(dtb-y)
clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts
