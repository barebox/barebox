# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_USB_DWC3)			+= dwc3.o

dwc3-y					:= core.o

ifneq ($(filter y,$(CONFIG_USB_DWC3_HOST)),)
	dwc3-y				+= host.o
endif

