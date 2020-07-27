###
# scripts contains sources for various helper programs used throughout
# barebox for the build process.
# ---------------------------------------------------------------------------
# kallsyms:      Find all symbols in barebox

hostprogs-y                      += bin2c
hostprogs-y                      += mkimage
hostprogs-y                      += fix_size
hostprogs-y                      += bareboxenv
hostprogs-y                      += bareboxcrc32
hostprogs-y                      += kernel-install
hostprogs-$(CONFIG_CRYPTO_RSA_BUILTIN_KEYS) += rsatoc
HOSTCFLAGS_rsatoc = `pkg-config --cflags openssl`
HOSTLDLIBS_rsatoc = `pkg-config --libs openssl`
hostprogs-$(CONFIG_IMD)          += bareboximd
hostprogs-$(CONFIG_KALLSYMS)     += kallsyms
hostprogs-$(CONFIG_MIPS)         += mips-relocs
hostprogs-$(CONFIG_MVEBU_HOSTTOOLS) += kwbimage kwboot mvebuimg
hostprogs-$(CONFIG_ARCH_OMAP)    += omap_signGP mk-omap-image
hostprogs-$(CONFIG_ARCH_S5PCxx)  += s5p_cksum
hostprogs-$(CONFIG_ARCH_DAVINCI) += mkublheader
HOSTCFLAGS_zynq_mkimage.o = -I$(srctree) -I$(srctree)/arch/arm/mach-zynq/include
hostprogs-$(CONFIG_ARCH_ZYNQ)	 += zynq_mkimage
hostprogs-$(CONFIG_ARCH_SOCFPGA) += socfpga_mkimage
hostprogs-$(CONFIG_MXS_HOSTTOOLS)+= mxsimage mxsboot
hostprogs-$(CONFIG_ARCH_LAYERSCAPE) += pblimage
hostprogs-$(CONFIG_ARCH_STM32MP) += stm32image
KBUILD_HOSTCFLAGS += -I$(srctree)/scripts/include/
HOSTLDLIBS_mxsimage  = `pkg-config --libs openssl`
HOSTCFLAGS_omap3-usb-loader.o = `pkg-config --cflags libusb-1.0`
HOSTLDLIBS_omap3-usb-loader  = `pkg-config --libs libusb-1.0`
hostprogs-$(CONFIG_OMAP3_USB_LOADER)  += omap3-usb-loader
HOSTCFLAGS_omap4_usbboot.o = `pkg-config --cflags libusb-1.0`
HOSTLDLIBS_omap4_usbboot = -lpthread `pkg-config --libs libusb-1.0`
hostprogs-$(CONFIG_OMAP4_HOSTTOOL_USBBOOT) += omap4_usbboot

userprogs-$(CONFIG_BAREBOXENV_TARGET)		+= bareboxenv-target
userprogs-$(CONFIG_KERNEL_INSTALL_TARGET)	+= kernel-install-target
userprogs-$(CONFIG_BAREBOXCRC32_TARGET)		+= bareboxcrc32-target
userprogs-$(CONFIG_IMD_TARGET)			+= bareboximd-target

userccflags += -I $(srctree)/$(src)/include

userprogs	:= $(userprogs-y)
always-y	:= $(hostprogs-y) $(hostprogs-m) $(userprogs-y)

subdir-y			+= mod
subdir-y			+= imx
subdir-$(CONFIG_X86)		+= setupmbr
subdir-$(CONFIG_DTC)		+= dtc
subdir-$(CONFIG_ARCH_TEGRA)	+= tegra

# Let clean descend into subdirs
subdir-	+= basic kconfig setupmbr
