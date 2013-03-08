/*
 * netconsole.c - network console support
 *
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "netconsole: " fmt

#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <stringlist.h>
#include <net.h>
#include <kfifo.h>
#include <init.h>
#include <linux/err.h>

/**
 * @file
 * @brief Network console support
 */

struct nc_priv {
	struct console_device cdev;
	struct kfifo *fifo;
	int busy;
	struct net_connection *con;

	uint16_t port;
	IPaddr_t ip;
};

static struct nc_priv *g_priv;

static void nc_handler(void *ctx, char *pkt, unsigned len)
{
	struct nc_priv *priv = g_priv;
	unsigned char *packet = net_eth_to_udp_payload(pkt);

	kfifo_put(priv->fifo, packet, net_eth_to_udplen(pkt));
}

static int nc_init(void)
{
	struct nc_priv *priv = g_priv;

	if (priv->con)
		net_unregister(priv->con);

	priv->con = net_udp_new(priv->ip, priv->port, nc_handler, NULL);
	if (IS_ERR(priv->con)) {
		int ret = PTR_ERR(priv->con);
		priv->con = NULL;
		return ret;
	}

	net_udp_bind(priv->con, priv->port);
	priv->cdev.f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
	return 0;
}

static int nc_getc(struct console_device *cdev)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);
	unsigned char c;

	while (!kfifo_len(priv->fifo))
		net_poll();

	kfifo_getc(priv->fifo, &c);

	return c;
}

static int nc_tstc(struct console_device *cdev)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);

	if (priv->busy)
		return kfifo_len(priv->fifo) ? 1 : 0;

	net_poll();

	return kfifo_len(priv->fifo) ? 1 : 0;
}

static void nc_putc(struct console_device *cdev, char c)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);
	unsigned char *packet;

	if (!priv->con)
		return;

	if (priv->busy)
		return;

	packet = net_udp_get_payload(priv->con);
	*packet = c;

	priv->busy = 1;
	net_udp_send(priv->con, 1);
	priv->busy = 0;
}

static int nc_port_set(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct nc_priv *priv = g_priv;
	char portstr[16];
	int port;

	if (!val)
		dev_param_set_generic(dev, param, NULL);

	port = simple_strtoul(val, NULL, 10);
	if (port > 65535)
		return -EINVAL;

	priv->port = port;
	nc_init();

	sprintf(portstr, "%d", port);
	dev_param_set_generic(dev, param, portstr);

	return 0;
}

static int nc_remoteip_set(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct nc_priv *priv = g_priv;
	IPaddr_t ip;
	int ret;

	if (!val)
		dev_param_set_generic(dev, param, NULL);

	if (string_to_ip(val, &ip))
		return -EINVAL;

	priv->ip = ip;
	ret = nc_init();
	if (ret)
		return ret;

	dev_param_set_generic(dev, param, val);

	return 0;
}

static int netconsole_init(void)
{
	struct nc_priv *priv;
	struct console_device *cdev;
	int ret;

	priv = xzalloc(sizeof(*priv));
	cdev = &priv->cdev;
	cdev->tstc = nc_tstc;
	cdev->putc = nc_putc;
	cdev->getc = nc_getc;

	g_priv = priv;

	priv->fifo = kfifo_alloc(1024);

	ret = console_register(cdev);
	if (ret) {
		pr_err("registering failed with %s\n", strerror(-ret));
		kfree(priv);
		return ret;
	}

	dev_add_param(&cdev->class_dev, "ip", nc_remoteip_set, NULL, 0);
	dev_add_param(&cdev->class_dev, "port", nc_port_set, NULL, 0);
	dev_set_param(&cdev->class_dev, "port", "6666");

	pr_info("registered as %s%d\n", cdev->class_dev.name, cdev->class_dev.id);

	return 0;
}

device_initcall(netconsole_init);

/** @page net_netconsole Network console

@section net_netconsole Using an UDP based network console

If enabled barebox supports a console via udp networking. There is only
one network console supported registered during init time. It is deactivated
by default because it opens great security holes, so use with care.

To use the network console you have to configure the remote ip and the local
and remote ports. Assuming the network console is registered as cs1, it can be
configured with:

@code
cs1.ip=<remotehost>
cs1.port=<port>
cs1.active=ioe
@endcode

On the remote host call scripts/netconsole with bareboxes ip and port as
parameters. port is initialized to 6666 by default.

*/
