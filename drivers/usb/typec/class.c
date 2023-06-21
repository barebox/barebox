// SPDX-License-Identifier: GPL-2.0
/*
 * USB Type-C Connector Class
 *
 * Copyright (C) 2017, Intel Corporation
 * Author: Heikki Krogerus <heikki.krogerus@linux.intel.com>
 */

#include <module.h>
#include <driver.h>
#include <linux/usb/role.h>
#include <linux/usb/typec.h>
#include <linux/usb/typec_altmode.h>
#include <param.h>

enum typec_param_accessory {
	TYPEC_PARAM_ACCESSORY_NONE,
	TYPEC_PARAM_ACCESSORY_AUDIO,
	TYPEC_PARAM_ACCESSORY_DEBUG,
};

struct typec_port {
	struct device dev;
	const struct typec_operations *ops;
	int pwr_role;		/* enum typec_role */
	int usb_role;		/* enum usb_role role */
	int accessory;		/* enum typec_param_accessory */
};

/**
 * typec_set_pwr_role - Report power role change
 * @port: The USB Type-C Port where the role was changed
 * @role: The new data role
 *
 * This routine is used by the port drivers to report power role changes.
 */
void typec_set_pwr_role(struct typec_port *port, enum typec_role role)
{
	port->pwr_role = role;
}
EXPORT_SYMBOL_GPL(typec_set_pwr_role);

static inline enum typec_param_accessory typec_mode_to_accessory(int mode)
{
	switch (mode) {
	case TYPEC_MODE_AUDIO:
		return TYPEC_PARAM_ACCESSORY_AUDIO;
	case TYPEC_MODE_DEBUG:
		return TYPEC_PARAM_ACCESSORY_DEBUG;
	default:
		return TYPEC_PARAM_ACCESSORY_NONE;
	}
}

/**
 * typec_set_mode - Set mode of operation for USB Type-C connector
 * @port: USB Type-C connector
 * @mode: Accessory Mode, USB Operation or Safe State
 *
 * Configure @port for Accessory Mode @mode. This function will configure the
 * muxes needed for @mode.
 */
int typec_set_mode(struct typec_port *port, int mode)
{
	port->accessory = typec_mode_to_accessory(mode);
	return 0;
}
EXPORT_SYMBOL_GPL(typec_set_mode);

/**
 * typec_set_role - Set USB role for a Type-C connector
 * @port: USB Type-C connector
 * @role: USB role to be switched to
 *
 * Set USB role @role for @sw. This is equivalent to Linux
 * usb_role_switch_set_role();
 */
int typec_set_role(struct typec_port *port, enum usb_role role)
{
	port->usb_role = role;
	return 0;
}
EXPORT_SYMBOL_GPL(typec_set_role);

/**
 * typec_get_drvdata - Return private driver data pointer
 * @port: USB Type-C port
 */
void *typec_get_drvdata(struct typec_port *port)
{
	return port->dev.priv;
}
EXPORT_SYMBOL_GPL(typec_get_drvdata);

static int typec_register_port_dev(struct typec_port *port, const char *name, int id)
{
	port->dev.id = id;
	dev_set_name(&port->dev, name);

	return register_device(&port->dev);
}

static const char * const usb_role_names[] = {
	[USB_ROLE_NONE]		= "none",
	[USB_ROLE_HOST]		= "host",
	[USB_ROLE_DEVICE]	= "device",
};

static const char * const pwr_role_names[] = {
	[TYPEC_SINK]		= "sink",
	[TYPEC_SOURCE]		= "source",
};

static const char * const accessory_names[] = {
	[TYPEC_PARAM_ACCESSORY_NONE]	= "none",
	[TYPEC_PARAM_ACCESSORY_AUDIO]	= "audio", /* analog */
	[TYPEC_PARAM_ACCESSORY_DEBUG]	= "debug",
};

static int typec_param_update(struct param_d *p, void *priv)
{
	struct typec_port *port = priv;

	return port->ops->poll(port);
}

/**
 * typec_register_port - Register a USB Type-C Port
 * @parent: Parent device
 * @cap: Description of the port
 *
 * Registers a device for USB Type-C Port described in @cap.
 *
 * Returns handle to the port on success or ERR_PTR on failure.
 */
struct typec_port *typec_register_port(struct device *parent,
				       const struct typec_capability *cap)
{
	struct typec_port *port;
	struct device *dev;
	const char *alias;
	int ret;

	port = kzalloc(sizeof(*port), GFP_KERNEL);
	if (!port)
		return ERR_PTR(-ENOMEM);

	port->ops = cap->ops;
	dev = &port->dev;
	dev->parent = parent;
	dev->of_node = cap->of_node;
	dev->priv = cap->driver_data;

	alias = dev->of_node ? of_alias_get(dev->of_node) : NULL;
	if (alias)
		ret = typec_register_port_dev(port, alias, DEVICE_ID_SINGLE);
	if (!alias || ret)
		ret = typec_register_port_dev(port, "typec", DEVICE_ID_DYNAMIC);

	if (ret)
		return ERR_PTR(ret);

	of_platform_device_dummy_drv(dev);
	if (dev->of_node)
		dev->of_node->dev = dev;

	dev_add_param_enum(dev, "usb_role", param_set_readonly, typec_param_update,
			   &port->usb_role, usb_role_names,
			   ARRAY_SIZE(usb_role_names), port);
	dev_add_param_enum(dev, "pwr_role", param_set_readonly, typec_param_update,
			   &port->pwr_role, pwr_role_names,
			   ARRAY_SIZE(pwr_role_names), port);
	dev_add_param_enum(dev, "accessory", param_set_readonly, typec_param_update,
			   &port->accessory, accessory_names,
			   ARRAY_SIZE(accessory_names), port);

	return port;
}
EXPORT_SYMBOL_GPL(typec_register_port);
