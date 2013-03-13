hostprogs-$(CONFIG_ARCH_IMX_IMXIMAGE) += imx-image
hostprogs-$(CONFIG_ARCH_IMX_USBLOADER) += imx-usb-loader

always          := $(hostprogs-y)

HOSTCFLAGS_imx-usb-loader.o = `pkg-config --cflags libusb-1.0`
HOSTLOADLIBES_imx-usb-loader  = `pkg-config --libs libusb-1.0`

imx-usb-loader-objs := imx-usb-loader.o
imx-image-objs := imx-image.o
