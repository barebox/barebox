#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#

obj-y += musb_core.o

obj-$(CONFIG_USB_MUSB_HOST) += musb_host.o musb_barebox.o
obj-$(CONFIG_USB_MUSB_GADGET) += musb_gadget.o musb_gadget_ep0.o

obj-$(CONFIG_USB_MUSB_DSPS)	+= musb_dsps.o
obj-$(CONFIG_USB_MUSB_AM335X)	+= phy-am335x-control.o musb_am335x.o phy-am335x.o
