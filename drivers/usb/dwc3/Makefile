# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_USB_DWC3)			+= dwc3.o

dwc3-y					:= core.o

ifneq ($(filter y,$(CONFIG_USB_DWC3_HOST) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= host.o
endif

ifneq ($(filter y,$(CONFIG_USB_DWC3_GADGET) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= gadget.o ep0.o
endif

obj-$(CONFIG_USB_DWC3_OF_SIMPLE)	+= dwc3-of-simple.o
