# SPDX-License-Identifier: GPL-2.0-only

###
# scripts contains sources for various helper programs used throughout
# barebox for the build process.

hostprogs-always-y					+= bin2c
hostprogs-always-y					+= fix_size
hostprogs-always-y					+= bareboxenv
hostprogs-always-y					+= bareboxcrc32
hostprogs-always-y					+= kernel-install
hostprogs-always-$(CONFIG_QOICONV)			+= qoiconv
hostprogs-always-$(CONFIG_RSATOC)			+= rsatoc
HOSTCFLAGS_rsatoc.o = `$(PKG_CONFIG) --cflags openssl`
HOSTLDLIBS_rsatoc = `$(PKG_CONFIG) --libs openssl`
hostprogs-always-$(CONFIG_IMD)				+= bareboximd
hostprogs-always-$(CONFIG_KALLSYMS)			+= kallsyms
hostprogs-always-$(CONFIG_MIPS)				+= mips-relocs
hostprogs-always-$(CONFIG_MVEBU_HOSTTOOLS)		+= kwbimage kwboot mvebuimg
hostprogs-always-$(CONFIG_OMAP_IMAGE)			+= omap_signGP mk-omap-image
HOSTCFLAGS_zynq_mkimage.o = -I$(srctree) -I$(srctree)/include/mach
hostprogs-always-$(CONFIG_ZYNQ_MKIMAGE)			+= zynq_mkimage
hostprogs-always-$(CONFIG_SOCFPGA_MKIMAGE)		+= socfpga_mkimage
hostprogs-always-$(CONFIG_MXS_HOSTTOOLS)		+= mxsimage mxsboot
hostprogs-always-$(CONFIG_LAYERSCAPE_PBLIMAGE)		+= pblimage
hostprogs-always-$(CONFIG_STM32_IMAGE)			+= stm32image
hostprogs-always-$(CONFIG_IMX9_IMAGE)			+= imx9image
hostprogs-always-$(CONFIG_RISCV)			+= prelink-riscv
hostprogs-always-$(CONFIG_RK_IMAGE)			+= rkimage
HOSTCFLAGS_rkimage.o = `$(PKG_CONFIG) --cflags openssl`
HOSTLDLIBS_rkimage = `$(PKG_CONFIG) --libs openssl`
KBUILD_HOSTCFLAGS += -I$(srctree)/scripts/include/
HOSTCFLAGS_mxsimage.o = `$(PKG_CONFIG) --cflags openssl`
HOSTLDLIBS_mxsimage  = `$(PKG_CONFIG) --libs openssl`
HOSTCFLAGS_omap3-usb-loader.o = `$(PKG_CONFIG) --cflags libusb-1.0`
HOSTLDLIBS_omap3-usb-loader  = `$(PKG_CONFIG) --libs libusb-1.0`
hostprogs-always-$(CONFIG_OMAP3_USB_LOADER)		+= omap3-usb-loader
HOSTCFLAGS_omap4_usbboot.o = `$(PKG_CONFIG) --cflags libusb-1.0`
HOSTLDLIBS_omap4_usbboot = -lpthread `$(PKG_CONFIG) --libs libusb-1.0`
hostprogs-always-$(CONFIG_OMAP4_HOSTTOOL_USBBOOT)	+= omap4_usbboot
HOSTCFLAGS_rk-usb-loader.o = `$(PKG_CONFIG) --cflags libusb-1.0`
HOSTLDLIBS_rk-usb-loader  = `$(PKG_CONFIG) --libs libusb-1.0`
hostprogs-always-$(CONFIG_RK_USB_LOADER)		+= rk-usb-loader

userprogs-always-$(CONFIG_BAREBOXENV_TARGET)		+= bareboxenv-target
userprogs-always-$(CONFIG_KERNEL_INSTALL_TARGET)	+= kernel-install-target
userprogs-always-$(CONFIG_BAREBOXCRC32_TARGET)		+= bareboxcrc32-target
userprogs-always-$(CONFIG_IMD_TARGET)			+= bareboximd-target
userprogs-always-$(CONFIG_OMAP3_USB_LOADER_TARGET)	+= omap3-usb-loader-target
userprogs-always-$(CONFIG_OMAP4_USBBOOT_TARGET)		+= omap4_usbboot-target
userprogs-always-$(CONFIG_MVEBU_KWBOOT_TARGET)		+= kwboot-target
userprogs-always-$(CONFIG_RK_USB_LOADER_TARGET)		+= rk-usb-loader-target

omap3-usb-loader-target-userccflags += `$(CROSS_PKG_CONFIG) --cflags libusb-1.0`
omap3-usb-loader-target-userldlibs += `$(CROSS_PKG_CONFIG) --libs libusb-1.0`
omap4_usbboot-target-userccflags += `$(CROSS_PKG_CONFIG) --cflags libusb-1.0`
omap4_usbboot-target-userldlibs += -lpthread `$(CROSS_PKG_CONFIG) --libs libusb-1.0`
rk-usb-loader-target-userccflags += `$(CROSS_PKG_CONFIG) --cflags libusb-1.0`
rk-usb-loader-target-userldlibs += `$(CROSS_PKG_CONFIG) --libs libusb-1.0`

userccflags += -I $(srctree)/$(src)/include -isystem $(srctree)/scripts/include

subdir-y			+= mod
subdir-y			+= imx
subdir-$(CONFIG_DTC)		+= dtc
subdir-$(CONFIG_ARCH_TEGRA)	+= tegra

# Let clean descend into subdirs
subdir-	+= basic kconfig
