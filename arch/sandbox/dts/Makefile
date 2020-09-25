always-$(CONFIG_OFTREE) += \
	sandbox.dtb

# just to build a built-in.o. Otherwise compilation fails when no devicetree is
# created.
obj- += dummy.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts
