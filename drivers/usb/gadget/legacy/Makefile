# SPDX-License-Identifier: GPL-2.0

ccflags-y := -I$(srctree)/drivers/usb/gadget/
ccflags-y += -I$(srctree)/drivers/usb/gadget/udc/
ccflags-y += -I$(srctree)/drivers/usb/gadget/function/

obj-$(CONFIG_USB_GADGET_SERIAL) += serial.o
obj-$(CONFIG_USB_GADGET) += multi.o
