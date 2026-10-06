/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/usb/gadget/function/u_ether.h?id=e584cb58a2ea7ff4d3a4bc43d5ca512ed3ecb77d */
/*
 * u_ether.h -- interface to USB gadget "ethernet link" utilities
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2003-2004 Robert Schwebel, Benedikt Spranger
 * Copyright (C) 2008 Nokia Corporation
 */

#ifndef __U_ETHER_H
#define __U_ETHER_H

#include <linux/err.h>
#include <net.h>
#include <linux/usb/composite.h>
#include <linux/usb/cdc.h>

#define QMULT_DEFAULT 5

#define NET_IP_ALIGN	0

struct eth_dev;
struct sk_buff;

/*
 * This represents the USB side of an "ethernet" link, managed by a USB
 * function which provides control and (maybe) framing.  Two functions
 * in different configurations could share the same ethernet link/netdev,
 * using different host interaction models.
 *
 * There is a current limitation that only one instance of this link may
 * be present in any given configuration.  When that's a problem, network
 * layer facilities can be used to package multiple logical links on this
 * single "physical" one.
 */
struct gether {
	struct usb_function		func;

	/* updated by gether_{connect,disconnect} */
	struct eth_dev			*ioport;

	/* endpoints handle full and/or high speeds */
	struct usb_ep			*in_ep;
	struct usb_ep			*out_ep;

	bool				is_zlp_ok;

	u16				cdc_filter;

	/* hooks for added framing, as needed for RNDIS and EEM. */
	u32				header_len;
	/* room reserved behind each TX frame for wrap() to use */
	u32				tailroom;
	/* NCM requires fixed size bundles */
	bool				is_fixed;
	u32				fixed_out_len;
	u32				fixed_in_len;
	bool				supports_multi_frame;
	struct sk_buff			*(*wrap)(struct gether *port,
						struct sk_buff *skb);
	int				(*unwrap)(struct gether *port,
						struct sk_buff *skb,
						struct sk_buff_head *list);

	/* called on network open/close */
	void				(*open)(struct gether *);
	void				(*close)(struct gether *);
};

#define	DEFAULT_FILTER	(USB_CDC_PACKET_TYPE_BROADCAST \
			|USB_CDC_PACKET_TYPE_ALL_MULTICAST \
			|USB_CDC_PACKET_TYPE_PROMISCUOUS \
			|USB_CDC_PACKET_TYPE_DIRECTED)

/*
 * gether_register_netdev - register the net device
 * @net: net device to register
 *
 * Registers the net device associated with this ethernet-over-usb link
 *
 */
int gether_register_netdev(struct eth_device *eth);

/* gether_setup_default - initialize one ethernet-over-usb link
 * Context: may sleep
 *
 * This sets up the single network link that may be exported by a
 * gadget driver using this framework.  The host link layer address
 * is set to a random value; the device address is left to the
 * net core, which derives it from the machine's unique id if possible.
 */
struct eth_device *gether_setup_default(void);

/**
 * gether_set_gadget - initialize one ethernet-over-usb link with a gadget
 * @net: device representing this link
 * @g: the gadget to initialize with
 *
 * This associates one ethernet-over-usb link with a gadget.
 */
void gether_set_gadget(struct eth_device *net, struct usb_gadget *g);

/**
 * gether_get_host_addr - get an ethernet-over-usb link host address
 * @net: device representing this link
 * @host_addr: place to store eth address of the host
 * @len: length of the @host_addr buffer
 *
 * This gets the host-side Ethernet address of this ethernet-over-usb link.
 * Returns zero on success, else negative errno.
 */
int gether_get_host_addr(struct eth_device *net, char *host_addr, int len);

/**
 * gether_get_host_addr_cdc - get an ethernet-over-usb link host address
 * @net: device representing this link
 * @host_addr: place to store eth address of the host
 * @len: length of the @host_addr buffer
 *
 * This gets the CDC formatted host-side Ethernet address of this
 * ethernet-over-usb link.
 * Returns zero on success, else negative errno.
 */
int gether_get_host_addr_cdc(struct eth_device *net, char *host_addr, int len);

/**
 * gether_get_host_addr_u8 - get an ethernet-over-usb link host address
 * @net: device representing this link
 * @host_mac: place to store the eth address of the host
 *
 * This gets the binary formatted host-side Ethernet address of this
 * ethernet-over-usb link.
 */
void gether_get_host_addr_u8(struct eth_device *net, u8 host_mac[ETH_ALEN]);

/**
 * gether_set_qmult - initialize an ethernet-over-usb link with a multiplier
 * @net: device representing this link
 * @qmult: queue multiplier
 *
 * This sets the queue length multiplier of this ethernet-over-usb link.
 * For higher speeds use longer queues.
 */
void gether_set_qmult(struct eth_device *net, unsigned qmult);

/**
 * gether_get_qmult - get an ethernet-over-usb link multiplier
 * @net: device representing this link
 *
 * This gets the queue length multiplier of this ethernet-over-usb link.
 */
unsigned gether_get_qmult(struct eth_device *net);

void gether_cleanup(struct eth_dev *dev);

/* connect/disconnect is handled by individual functions */
struct eth_device *gether_connect(struct gether *);
void gether_disconnect(struct gether *);

#endif /* __U_ETHER_H */
