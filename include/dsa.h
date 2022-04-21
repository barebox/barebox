/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright 2019-2021 NXP
 */

#ifndef __DSA_H__
#define __DSA_H__

#include <linux/phy.h>
#include <net.h>

/**
 * DSA stands for Distributed Switch Architecture and it is infrastructure
 * intended to support drivers for Switches that rely on an intermediary
 * Ethernet device for I/O.  These switches may support cascading allowing
 * them to be arranged as a tree.
 * DSA is documented in detail in the Linux kernel documentation under
 * Documentation/networking/dsa/dsa.txt
 * The network layout of such a switch is shown below:
 *
 *                      |------|
 *                      | eth0 | <--- master eth device (regular eth driver)
 *                      |------|
 *                        ^  |
 * tag added by switch -->|  |
 *                        |  |
 *                        |  |<-- tag added by DSA driver
 *                        |  v
 *      |--------------------------------------|
 *      |             | CPU port |             | <-- DSA (switch) device
 *      |             ------------             |     (DSA driver)
 *      | _________  _________       _________ |
 *      | | port0 |  | port1 |  ...  | portn | | <-- ports as eth devices
 *      |-+-------+--+-------+-------+-------+-|     ('dsa-port' eth driver)
 *
 */

#define DSA_MAX_PORTS		12
#define DSA_PKTSIZE		1538

struct dsa_port;
struct dsa_switch;

struct dsa_ops {
	int (*port_probe)(struct dsa_port *dp, int port,
			  phy_interface_t phy_mode);
	int (*port_pre_enable)(struct dsa_port *dp, int port,
			       phy_interface_t phy_mode);
	int (*port_enable)(struct dsa_port *dp, int port,
			   struct phy_device *phy);
	void (*port_disable)(struct dsa_port *dp, int port,
			     struct phy_device *phy);
	int (*xmit)(struct dsa_port *dp, int port, void *packet, int length);
	int (*rcv)(struct dsa_switch *ds, int *portp, void *packet, int length);

	int (*phy_read)(struct dsa_switch *ds, int port, int regnum);
	int (*phy_write)(struct dsa_switch *ds, int port, int regnum, u16 val);
};

struct dsa_port {
	struct device_d dev;
	struct dsa_switch *ds;
	unsigned int index;
	struct eth_device edev;
	unsigned char *rx_buf;
	size_t rx_buf_length;
	bool enabled;
};

struct dsa_switch {
	struct device_d *dev;
	const struct dsa_ops *ops;
	size_t num_ports;
	u32 cpu_port;
	int cpu_port_users;
	struct eth_device *edev_master;
	struct phy_device *cpu_port_fixed_phy;
	struct dsa_port *dp[DSA_MAX_PORTS];
	size_t needed_headroom;
	size_t needed_rx_tailroom;
	size_t needed_tx_tailroom;
	void *tx_buf;
	struct mii_bus *slave_mii_bus;
	u32 phys_mii_mask;
};

int dsa_register_switch(struct dsa_switch *ds);
u32 dsa_user_ports(struct dsa_switch *ds);

#endif /* __DSA_H__ */
