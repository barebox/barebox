/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_USB_TYPEC_H
#define __LINUX_USB_TYPEC_H

#include <linux/types.h>
#include <linux/usb/role.h>

struct typec_port;

struct device;
struct device_node;

enum typec_role {
	TYPEC_SINK,
	TYPEC_SOURCE,
};

enum typec_accessory {
	TYPEC_ACCESSORY_NONE,
	TYPEC_ACCESSORY_AUDIO,
	TYPEC_ACCESSORY_DEBUG,
};

struct typec_operations {
	int (*poll)(struct typec_port *port);
};

/*
 * struct typec_capability - USB Type-C Port Capabilities
 * @driver_data: Private pointer for driver specific info
 * @ops: Port operations vector
 *
 * Static capabilities of a single USB Type-C port.
 */
struct typec_capability {
	void			*driver_data;

	const struct typec_operations	*ops;
	struct device_node	*of_node;
};

struct typec_port *typec_register_port(struct device *parent,
				       const struct typec_capability *cap);

void typec_set_pwr_role(struct typec_port *port, enum typec_role role);

int typec_set_mode(struct typec_port *port, int mode);

int typec_set_role(struct typec_port *port, enum usb_role role);

void *typec_get_drvdata(struct typec_port *port);

#endif /* __LINUX_USB_TYPEC_H */
