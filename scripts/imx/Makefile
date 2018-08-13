hostprogs-$(CONFIG_ARCH_IMX_IMXIMAGE) += imx-image
hostprogs-$(CONFIG_ARCH_IMX_USBLOADER) += imx-usb-loader

always          := $(hostprogs-y)

HOSTCFLAGS_imx-usb-loader.o = `pkg-config --cflags libusb-1.0`
HOSTLOADLIBES_imx-usb-loader  = `pkg-config --libs libusb-1.0`

HOSTCFLAGS_imx.o = -I$(srctree)/arch/arm/mach-imx/include
HOSTCFLAGS_imx-image.o = -I$(srctree) -I$(srctree)/arch/arm/mach-imx/include
HOSTCFLAGS_imx-usb-loader.o += -I$(srctree) -I$(srctree)/arch/arm/mach-imx/include
ifdef CONFIG_ARCH_IMX_IMXIMAGE_SSL_SUPPORT
HOSTCFLAGS_imx-image.o += -DIMXIMAGE_SSL_SUPPORT
HOSTLOADLIBES_imx-image  = `pkg-config --libs openssl`
endif

imx-usb-loader-objs := imx-usb-loader.o imx.o
imx-image-objs := imx-image.o imx.o
