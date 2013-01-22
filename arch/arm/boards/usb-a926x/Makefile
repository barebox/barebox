obj-y += init.o

lowlevel_init-$(CONFIG_MACH_USB_A9263) = usb_a9263_lowlevel_init.o

obj-$(CONFIG_MACH_DO_LOWLEVEL_INIT) += $(lowlevel_init-y)

pbl-$(CONFIG_MACH_DO_LOWLEVEL_INIT) += $(lowlevel_init-y)
