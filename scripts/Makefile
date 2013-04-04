###
# scripts contains sources for various helper programs used throughout
# the kernel for the build process.
# ---------------------------------------------------------------------------
# kallsyms:      Find all symbols in barebox

hostprogs-$(CONFIG_KALLSYMS)     += kallsyms
hostprogs-y                      += bin2c
hostprogs-y                      += mkimage
hostprogs-y                      += bareboxenv
hostprogs-$(CONFIG_ARCH_NETX)    += gen_netx_image
hostprogs-$(CONFIG_ARCH_OMAP)    += omap_signGP mk-am35xx-spi-image
hostprogs-$(CONFIG_ARCH_S5PCxx)  += s5p_cksum
hostprogs-$(CONFIG_ARCH_DAVINCI) += mkublheader
hostprogs-$(CONFIG_ARCH_ZYNQ)	 += zynq_mkimage

HOSTLOADLIBES_omap4_usbboot = -lpthread
omap4_usbboot-objs               := usb_linux.o omap4_usbboot.o
hostprogs-$(CONFIG_OMAP4_USBBOOT)+= omap4_usbboot

always		:= $(hostprogs-y) $(hostprogs-m)

subdir-y                     += mod

subdir-$(CONFIG_X86)             += setupmbr

# Let clean descend into subdirs
subdir-	+= basic kconfig setupmbr

quiet_cmd_csingle	= CC      $@
      cmd_csingle	= $(CC) -Wp,-MD,$(depfile) $(CFLAGS) -o $@ $<

obj-$(CONFIG_BAREBOXENV_TARGET)	+= bareboxenv-target

scripts/bareboxenv-target: scripts/bareboxenv.c FORCE
	$(call if_changed_dep,csingle)

subdir-$(CONFIG_DTC)         += dtc
