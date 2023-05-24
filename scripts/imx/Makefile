# SPDX-License-Identifier: GPL-2.0-only

hostprogs-always-$(CONFIG_ARCH_IMX_IMXIMAGE)	+= imx-image
hostprogs-always-$(CONFIG_ARCH_IMX_USBLOADER)	+= imx-usb-loader

HOSTCFLAGS_imx-usb-loader.o = `pkg-config --cflags libusb-1.0` -include $(objtree)/include/generated/utsrelease.h
HOSTLDLIBS_imx-usb-loader  = `pkg-config --libs libusb-1.0`

imx-usb-loader-target-userccflags += `$(CROSS_PKG_CONFIG) --cflags libusb-1.0`
imx-usb-loader-target-userldlibs += `$(CROSS_PKG_CONFIG) --libs libusb-1.0`

HOSTCFLAGS_imx.o = -I$(srctree)/include/mach
imx-target-userccflags += -I$(srctree)/include/mach
HOSTCFLAGS_imx-image.o = -I$(srctree) -I$(srctree)/include/mach
HOSTCFLAGS_imx-usb-loader.o += -I$(srctree) -I$(srctree)/include/mach
imx-usb-loader-target-userccflags += -I$(srctree) -I$(srctree)/include/mach
ifdef CONFIG_ARCH_IMX_IMXIMAGE_SSL_SUPPORT
HOSTCFLAGS_imx-image.o += -DIMXIMAGE_SSL_SUPPORT
HOSTLDLIBS_imx-image  = `pkg-config --libs openssl`
endif

imx-usb-loader-objs := imx-usb-loader.o imx.o
imx-usb-loader-target-objs := imx-usb-loader-target.o imx-target.o
imx-image-objs := imx-image.o imx.o

userprogs-always-$(CONFIG_ARCH_IMX_USBLOADER_TARGET)	+= imx-usb-loader-target

userccflags += -I $(srctree)/$(src)/include -isystem $(srctree)/scripts/include
