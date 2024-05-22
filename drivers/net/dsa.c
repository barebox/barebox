// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <dma.h>
#include <dsa.h>
#include <of_net.h>

u32 dsa_user_ports(struct dsa_switch *ds)
{
	u32 mask = 0;
	int i;

	for (i = 0; i < ds->num_ports; i++) {
		if (ds->dp[i])
			mask |= BIT(ds->dp[i]->index);
	}

	return mask;
}

static int dsa_slave_phy_read(struct mii_bus *bus, int addr, int reg)
{
	struct dsa_switch *ds = bus->priv;

	if (ds->phys_mii_mask & BIT(addr))
		return ds->ops->phy_read(ds, addr, reg);

	return 0xffff;
}

static int dsa_slave_phy_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	struct dsa_switch *ds = bus->priv;

	if (ds->phys_mii_mask & BIT(addr))
		return ds->ops->phy_write(ds, addr, reg, val);

	return 0;
}

static int dsa_slave_mii_bus_init(struct dsa_switch *ds)
{
	ds->slave_mii_bus = xzalloc(sizeof(*ds->slave_mii_bus));
	ds->slave_mii_bus->priv = (void *)ds;
	ds->slave_mii_bus->read = dsa_slave_phy_read;
	ds->slave_mii_bus->write = dsa_slave_phy_write;
	ds->slave_mii_bus->parent = ds->dev;
	ds->slave_mii_bus->phy_mask = ~ds->phys_mii_mask;

	return mdiobus_register(ds->slave_mii_bus);
}

static int dsa_port_probe(struct eth_device *edev)
{
	struct dsa_port *dp = edev->priv;
	struct dsa_switch *ds = dp->ds;
	const struct dsa_switch_ops *ops = ds->ops;
	phy_interface_t interface;
	int ret;

	if (ops->port_probe) {
		interface = of_get_phy_mode(dp->dev->of_node);
		ret = ops->port_probe(dp, dp->index, interface);
		if (ret)
			return ret;
	}

	return 0;
}

static void dsa_port_set_ethaddr(struct eth_device *edev)
{
	struct dsa_port *dp = edev->priv;
	struct dsa_switch *ds = dp->ds;

	if (is_valid_ether_addr(edev->ethaddr))
		return;

	if (!is_valid_ether_addr(ds->edev_master->ethaddr))
		return;

	eth_set_ethaddr(edev, ds->edev_master->ethaddr);
}

static int dsa_port_start(struct eth_device *edev)
{
	struct dsa_port *dp = edev->priv;
	struct dsa_switch *ds = dp->ds;
	const struct dsa_switch_ops *ops = ds->ops;
	phy_interface_t interface;
	int ret;

	if (dp->enabled)
		return -EBUSY;

	interface = of_get_phy_mode(dp->dev->of_node);

	if (ops->port_pre_enable) {
		/* In case of RMII interface we need to enable RMII clock
		 * before talking to the PHY.
		 */
		ret = ops->port_pre_enable(dp, dp->index, interface);
		if (ret)
			return ret;
	}

	ret = phy_device_connect(edev, ds->slave_mii_bus, dp->index,
				 ops->adjust_link, 0, interface);
	if (ret)
		return ret;

	dsa_port_set_ethaddr(edev);

	if (ops->port_enable) {
		ret = ops->port_enable(dp, dp->index, dp->edev.phydev);
		if (ret)
			return ret;
	}

	dp->enabled = true;

	if (!ds->cpu_port_users) {
		struct dsa_port *dpc = ds->dp[ds->cpu_port];

		if (ops->port_pre_enable) {
			/* In case of RMII interface we need to enable RMII clock
			 * before talking to the PHY.
			 */
			ret = ops->port_pre_enable(dpc, ds->cpu_port,
						   ds->cpu_port_fixed_phy->interface);
			if (ret)
				return ret;
		}

		if (ops->port_enable) {
			ret = ops->port_enable(dpc, ds->cpu_port,
					       ds->cpu_port_fixed_phy);
			if (ret)
				return ret;
		}

		ret = eth_set_promisc(ds->edev_master, true);
		if (ret)
			dev_warn(ds->dev, "Failed to set promisc mode. Using different eth addresses may not work. %pe\n",
				 ERR_PTR(ret));

		eth_open(ds->edev_master);
	}

	ds->cpu_port_users++;

	return 0;
}

/* Stop the desired port, the CPU port and the master Eth interface */
static void dsa_port_stop(struct eth_device *edev)
{
	struct dsa_port *dp = edev->priv;
	struct dsa_switch *ds = dp->ds;
	const struct dsa_switch_ops *ops = ds->ops;

	if (!dp->enabled)
		return;

	if (ops->port_disable)
		ops->port_disable(dp, dp->index, dp->edev.phydev);

	dp->enabled = false;
	ds->cpu_port_users--;

	if (!ds->cpu_port_users) {
		struct dsa_port *dpc = ds->dp[ds->cpu_port];

		if (ops->port_disable)
			ops->port_disable(dpc, ds->cpu_port,
					ds->cpu_port_fixed_phy);

		eth_set_promisc(ds->edev_master, false);
		eth_close(ds->edev_master);
	}
}

static int dsa_port_send(struct eth_device *edev, void *packet, int length)
{
	struct dsa_port *dp = edev->priv;
	struct dsa_switch *ds = dp->ds;
	const struct dsa_switch_ops *ops = ds->ops;
	void *tx_buf = ds->tx_buf;
	size_t full_length, stuff = 0;
	int ret;

	if (length < 64)
		stuff = 64 - length;

	full_length = length + ds->needed_headroom + ds->needed_tx_tailroom +
		      stuff;

	if (full_length > DSA_PKTSIZE)
		return -ENOMEM;

	memset(tx_buf + full_length - stuff, 0, stuff);
	memcpy(tx_buf + ds->needed_headroom, packet, length);
	ret = ops->xmit(dp, dp->index, tx_buf, full_length);
	if (ret)
		return ret;

	return eth_send_raw(ds->edev_master, tx_buf, full_length);
}

static int dsa_port_recv(struct eth_device *edev)
{
	struct dsa_port *dp = edev->priv;
	int length;

	if (!dp->rx_buf_length)
		return 0;

	net_receive(edev, dp->rx_buf, dp->rx_buf_length);
	length = dp->rx_buf_length;
	dp->rx_buf_length = 0;

	return length;
}

static int dsa_ether_set_ethaddr(struct eth_device *edev,
				 const unsigned char *adr)
{
	struct dsa_port *dp = edev->priv;
	struct dsa_switch *ds = dp->ds;
	struct eth_device *edev_master;

	edev_master = ds->edev_master;

	return edev_master->set_ethaddr(edev_master, adr);
}

static int dsa_ether_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct dsa_port *dp = edev->priv;
	struct dsa_switch *ds = dp->ds;
	struct eth_device *edev_master;

	edev_master = ds->edev_master;

	return edev_master->get_ethaddr(edev_master, adr);
}

static struct dsa_port *dsa_port_alloc(struct dsa_switch *ds,
				       struct device_node *dn, int port)
{
	struct device *dev;
	struct dsa_port *dp;

	ds->dp[port] = xzalloc(sizeof(*dp));
	dp = ds->dp[port];

	dev = of_platform_device_create(dn, ds->dev);
	of_platform_device_dummy_drv(dev);
	dp->dev = dev;
	dp->ds = ds;
	dp->index = port;

	return dp;
}

static int dsa_switch_register_edev(struct dsa_switch *ds,
				    struct device_node *dn, int port)
{
	struct eth_device *edev;
	struct dsa_port *dp;

	dp = dsa_port_alloc(ds, dn, port);

	/* DMA is done on buffer in receive ring allocated by network
	 * driver. This is then copied into this buffer, so we don't
	 * strictly need to use dma_alloc() here, unlike ds->tx_buf.
	 * We do it anyway as we don't want DSA buffers to be subtly
	 * different to that of a directly used network interface.
	 */
	dp->rx_buf = dma_alloc(DSA_PKTSIZE);

	edev = &dp->edev;
	edev->priv = dp;
	edev->parent = dp->dev;
	edev->init = dsa_port_probe;
	edev->open = dsa_port_start;
	edev->send = dsa_port_send;
	edev->recv = dsa_port_recv;
	edev->halt = dsa_port_stop;
	edev->get_ethaddr = dsa_ether_get_ethaddr;
	edev->set_ethaddr = dsa_ether_set_ethaddr;

	return eth_register(edev);
}

static int dsa_rx_preprocessor(struct eth_device *edev, unsigned char **packet,
			       int *length)
{
	struct dsa_switch *ds = edev->rx_preprocessor_priv;
	const struct dsa_switch_ops *ops = ds->ops;
	struct dsa_port *dp;
	int ret, port;

	ret = ops->rcv(ds, &port, *packet, *length);
	if (ret)
		return ret;

	*length -= ds->needed_headroom;
	*packet += ds->needed_headroom;
	*length -= ds->needed_rx_tailroom;

	if (port > DSA_MAX_PORTS)
		return -ERANGE;

	dp = ds->dp[port];
	if (!dp)
		return 0;

	if (*length > DSA_PKTSIZE)
		return -ENOMEM;

	if (dp->rx_buf_length)
		return -EIO;

	memcpy(dp->rx_buf, *packet, *length);
	dp->rx_buf_length = *length;

	return -ENOMSG;
}

static int dsa_switch_register_master(struct dsa_switch *ds,
				      struct device_node *np,
				      struct device_node *master, int port)
{
	struct device_node *phy_node;
	struct phy_device *phydev;
	int ret;

	if (ds->edev_master) {
		dev_err(ds->dev, "master was already registered!\n");
		return -EINVAL;
	}

	ds->edev_master = of_find_eth_device_by_node(master);
	if (!ds->edev_master) {
		dev_err(ds->dev, "can't find ethernet master device\n");
		return -ENODEV;
	}

	ds->edev_master->rx_preprocessor = dsa_rx_preprocessor;
	ds->edev_master->rx_preprocessor_priv = ds;

	ret = dev_set_param(&ds->edev_master->dev, "mode", "disabled");
	if (ret)
		dev_warn(ds->dev, "Can't set disable master Ethernet device\n");

	phy_node = of_get_child_by_name(np, "fixed-link");
	if (!phy_node)
		return -ENODEV;

	phydev = of_phy_register_fixed_link(phy_node, ds->edev_master);
	if (!phydev)
		return -ENODEV;

	phydev->interface = of_get_phy_mode(np);

	dsa_port_alloc(ds, np, port);

	ds->cpu_port = port;
	ds->cpu_port_fixed_phy = phydev;

	return 0;
}

static int dsa_switch_parse_ports_of(struct dsa_switch *ds,
				     struct device_node *dn)
{
	struct device_node *ports, *port;
	int ret = 0;
	u32 reg;

	ports = of_get_child_by_name(dn, "ports");
	if (!ports) {
		/* The second possibility is "ethernet-ports" */
		ports = of_get_child_by_name(dn, "ethernet-ports");
		if (!ports) {
			dev_err(ds->dev, "no ports child node found\n");
			return -EINVAL;
		}
	}

	/* At first step, find and register master/CPU interface */
	for_each_available_child_of_node(ports, port) {
		struct device_node *master;

		ret = of_property_read_u32(port, "reg", &reg);
		if (ret) {
			dev_err(ds->dev, "No or too many ports are configured\n");
			goto out_put_node;
		}

		if (reg >= ds->num_ports) {
			dev_err(ds->dev, "port %pOF index %u exceeds num_ports (%zu)\n",
				port, reg, ds->num_ports);
			ret = -EINVAL;
			goto out_put_node;
		}

		master = of_parse_phandle(port, "ethernet", 0);
		if (master) {
			ret = dsa_switch_register_master(ds, port, master, reg);
			if (ret)
				return ret;
		}
	}

	/* Now we can register regular switch ports  */
	for_each_available_child_of_node(ports, port) {
		of_property_read_u32(port, "reg", &reg);

		if (of_parse_phandle(port, "ethernet", 0))
			continue;

		ret = dsa_switch_register_edev(ds, port, reg);
		if (ret) {
			dev_err(ds->dev, "Can't create edev for port %i\n",
				reg);
			return ret;
		}
	}

out_put_node:
	return ret;
}

int dsa_register_switch(struct dsa_switch *ds)
{
	int ret;

	if (!ds->dev) {
		pr_err("No dev is set\n");
		return -ENODEV;
	}

	if (!ds->dev->of_node)
		return -ENODEV;

	if (!ds->num_ports || ds->num_ports > DSA_MAX_PORTS) {
		dev_err(ds->dev, "No or too many ports are configured\n");
		return -EINVAL;
	}

	ret = dsa_switch_parse_ports_of(ds, ds->dev->of_node);
	if (ret)
		return ret;

	if (!ds->slave_mii_bus && ds->ops->phy_read) {
		ret = dsa_slave_mii_bus_init(ds);
		if (ret)
			return ret;
	}

	ds->tx_buf = dma_alloc(DSA_PKTSIZE);

	return 0;
}
EXPORT_SYMBOL_GPL(dsa_register_switch);

