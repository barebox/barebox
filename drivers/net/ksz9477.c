// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <complete.h>
#include <dsa.h>
#include <linux/gpio/consumer.h>
#include <net.h>
#include <platform_data/ksz9477_reg.h>
#include <spi/spi.h>
#include <i2c/i2c.h>
#include "ksz_common.h"

/* SPI frame opcodes */

#define SPI_ADDR_SHIFT			24
#define SPI_ADDR_ALIGN			3
#define SPI_TURNAROUND_SHIFT		5

#define GBIT_SUPPORT			BIT(0)
#define NEW_XMII			BIT(1)
#define IS_9893				BIT(2)
#define KSZ9477_PHY_ERRATA		BIT(3)

KSZ_REGMAP_TABLE(ksz9477_spi, 32, SPI_ADDR_SHIFT,
		 SPI_TURNAROUND_SHIFT, SPI_ADDR_ALIGN);
KSZ_REGMAP_TABLE(ksz9477_i2c, not_used, 16, 0, 0);

static int ksz9477_phy_read16(struct dsa_switch *ds, int addr, int reg)
{
	struct device *dev = ds->dev;
	struct ksz_switch *priv = dev_get_priv(dev);
	u16 val = 0xffff;

	if (addr >= priv->phy_port_cnt)
		return val;

	ksz_pread16(priv, addr, 0x100 + (reg << 1), &val);

	return val;
}

static int ksz9477_phy_write16(struct dsa_switch *ds, int addr, int reg,
			       u16 val)
{
	struct device *dev = ds->dev;
	struct ksz_switch *priv = dev_get_priv(dev);

	/* No real PHY after this. */
	if (addr >= priv->phy_port_cnt)
		return 0;

	/* No gigabit support.  Do not write to this register. */
	if (!(priv->features & GBIT_SUPPORT) && reg == MII_CTRL1000)
		return 0;
	ksz_pwrite16(priv, addr, 0x100 + (reg << 1), val);

	return 0;
}

static int ksz9477_switch_detect(struct ksz_switch *priv)
{
	u8 id_hi, id_lo;
	u8 data8;
	u32 id32;
	int ret;

	/* read chip id */
	ret = ksz_read32(priv, REG_CHIP_ID0__1, &id32);
	if (ret)
		return ret;

	ret = ksz_read8(priv, REG_GLOBAL_OPTIONS, &data8);
	if (ret)
		return ret;

	priv->chip_id = id32;

	priv->phy_port_cnt = 5;
	priv->features = GBIT_SUPPORT | KSZ9477_PHY_ERRATA;

	id_hi = (u8)(id32 >> 16);
	id_lo = (u8)(id32 >> 8);
	if ((id_lo & 0xf) == 3) {
		/* Chip is from KSZ9893 design. */
		dev_info(priv->dev, "Found KSZ9893 or compatible\n");
		priv->features |= IS_9893;
		priv->features &= ~KSZ9477_PHY_ERRATA;

		/* Chip does not support gigabit. */
		if (data8 & SW_QW_ABLE)
			priv->features &= ~GBIT_SUPPORT;
		priv->phy_port_cnt = 2;
	} else {
		dev_info(priv->dev, "Found KSZ9477 or compatible\n");
		/* Chip uses new XMII register definitions. */
		priv->features |= NEW_XMII;

		/* Chip does not support gigabit. */
		if (!(data8 & SW_GIGABIT_ABLE))
			priv->features &= ~GBIT_SUPPORT;
	}

	return 0;
}

static int ksz_reset_switch(struct ksz_switch *priv)
{
	u8 data8;
	u16 data16;
	u32 data32;

	/* reset switch */
	ksz_cfg(priv, REG_SW_OPERATION, SW_RESET, true);

	/* turn off SPI DO Edge select */
	ksz_read8(priv, REG_SW_GLOBAL_SERIAL_CTRL_0, &data8);
	data8 &= ~SPI_AUTO_EDGE_DETECTION;
	ksz_write8(priv, REG_SW_GLOBAL_SERIAL_CTRL_0, data8);

	/* default configuration */
	ksz_read8(priv, REG_SW_LUE_CTRL_1, &data8);
	data8 = SW_AGING_ENABLE | SW_LINK_AUTO_AGING |
	      SW_SRC_ADDR_FILTER | SW_FLUSH_STP_TABLE | SW_FLUSH_MSTP_TABLE;
	ksz_write8(priv, REG_SW_LUE_CTRL_1, data8);

	/* disable interrupts */
	ksz_write32(priv, REG_SW_INT_MASK__4, SWITCH_INT_MASK);
	ksz_write32(priv, REG_SW_PORT_INT_MASK__4, 0x7F);
	ksz_read32(priv, REG_SW_PORT_INT_STATUS__4, &data32);

	/* set broadcast storm protection 10% rate */
	ksz_read16(priv, REG_SW_MAC_CTRL_2, &data16);
	data16 &= ~BROADCAST_STORM_RATE;
	data16 |= (BROADCAST_STORM_VALUE * BROADCAST_STORM_PROT_RATE) / 100;
	ksz_write16(priv, REG_SW_MAC_CTRL_2, data16);

	return 0;
}

static void ksz9477_cfg_port_member(struct ksz_switch *priv, int port,
				    u8 member)
{
	ksz_pwrite32(priv, port, REG_PORT_VLAN_MEMBERSHIP__4, member);
}

static void ksz9477_port_mmd_write(struct ksz_switch *priv, int port,
				   u8 dev_addr, u16 reg_addr, u16 val)
{
	ksz_pwrite16(priv, port, REG_PORT_PHY_MMD_SETUP,
		     MMD_SETUP(PORT_MMD_OP_INDEX, dev_addr));
	ksz_pwrite16(priv, port, REG_PORT_PHY_MMD_INDEX_DATA, reg_addr);
	ksz_pwrite16(priv, port, REG_PORT_PHY_MMD_SETUP,
		     MMD_SETUP(PORT_MMD_OP_DATA_NO_INCR, dev_addr));
	ksz_pwrite16(priv, port, REG_PORT_PHY_MMD_INDEX_DATA, val);
}

static void ksz9477_phy_errata_setup(struct ksz_switch *priv, int port)
{
	/* Apply PHY settings to address errata listed in
	 * KSZ9477, KSZ9897, KSZ9896, KSZ9567, KSZ8565
	 * Silicon Errata and Data Sheet Clarification documents:
	 *
	 * Register settings are needed to improve PHY receive performance
	 */
	ksz9477_port_mmd_write(priv, port, 0x01, 0x6f, 0xdd0b);
	ksz9477_port_mmd_write(priv, port, 0x01, 0x8f, 0x6032);
	ksz9477_port_mmd_write(priv, port, 0x01, 0x9d, 0x248c);
	ksz9477_port_mmd_write(priv, port, 0x01, 0x75, 0x0060);
	ksz9477_port_mmd_write(priv, port, 0x01, 0xd3, 0x7777);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x06, 0x3008);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x08, 0x2001);

	/* Transmit waveform amplitude can be improved
	 * (1000BASE-T, 100BASE-TX, 10BASE-Te)
	 */
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x04, 0x00d0);

	/* Energy Efficient Ethernet (EEE) feature select must
	 * be manually disabled (except on KSZ8565 which is 100Mbit)
	 */
	if (priv->features & GBIT_SUPPORT)
		ksz9477_port_mmd_write(priv, port, 0x07, 0x3c, 0x0000);

	/* Register settings are required to meet data sheet
	 * supply current specifications
	 */
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x13, 0x6eff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x14, 0xe6ff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x15, 0x6eff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x16, 0xe6ff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x17, 0x00ff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x18, 0x43ff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x19, 0xc3ff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x1a, 0x6fff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x1b, 0x07ff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x1c, 0x0fff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x1d, 0xe7ff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x1e, 0xefff);
	ksz9477_port_mmd_write(priv, port, 0x1c, 0x20, 0xeeee);
}

static void ksz9477_set_xmii(struct ksz_switch *priv, int mode, u8 *data)
{
	u8 xmii;

	if (priv->features & NEW_XMII) {
		switch (mode) {
		case 0:
			xmii = PORT_MII_SEL;
			break;
		case 1:
			xmii = PORT_RMII_SEL;
			break;
		case 2:
			xmii = PORT_GMII_SEL;
			break;
		default:
			xmii = PORT_RGMII_SEL;
			break;
		}
	} else {
		switch (mode) {
		case 0:
			xmii = PORT_MII_SEL_S1;
			break;
		case 1:
			xmii = PORT_RMII_SEL_S1;
			break;
		case 2:
			xmii = PORT_GMII_SEL_S1;
			break;
		default:
			xmii = PORT_RGMII_SEL_S1;
			break;
		}
	}
	*data &= ~PORT_MII_SEL_M;
	*data |= xmii;
}

static void ksz9477_set_gbit(struct ksz_switch *priv, bool gbit, u8 *data)
{
	if (priv->features & NEW_XMII) {
		if (gbit)
			*data &= ~PORT_MII_NOT_1GBIT;
		else
			*data |= PORT_MII_NOT_1GBIT;
	} else {
		if (gbit)
			*data |= PORT_MII_1000MBIT_S1;
		else
			*data &= ~PORT_MII_1000MBIT_S1;
	}
}

static int ksz_port_setup(struct ksz_switch *priv, int port,
			  phy_interface_t interface)
{
	u8 data8, member;

	if (port != priv->ds.cpu_port) {
		ksz_pwrite8(priv, port, REG_PORT_CTRL_0, 0);
		member = BIT(priv->ds.cpu_port);
		ksz9477_cfg_port_member(priv, port, member);

		member = dsa_user_ports(&priv->ds);
		ksz9477_cfg_port_member(priv, priv->ds.cpu_port, member);

		if (priv->features & KSZ9477_PHY_ERRATA)
			ksz9477_phy_errata_setup(priv, port);

		ksz_pwrite16(priv, port, 0x100 + (MII_BMCR << 1),
			     BMCR_ANENABLE | BMCR_ANRESTART | BMCR_RESET);
	} else {
		ksz_pwrite8(priv, port, REG_PORT_CTRL_0, PORT_TAIL_TAG_ENABLE);
		/* cpu port: configure MAC interface mode */
		ksz_pread8(priv, port, REG_PORT_XMII_CTRL_1, &data8);
		switch (interface) {
		case PHY_INTERFACE_MODE_MII:
			ksz9477_set_xmii(priv, 0, &data8);
			ksz9477_set_gbit(priv, false, &data8);
			break;
		case PHY_INTERFACE_MODE_RMII:
			ksz9477_set_xmii(priv, 1, &data8);
			ksz9477_set_gbit(priv, false, &data8);
			break;
		case PHY_INTERFACE_MODE_GMII:
			ksz9477_set_xmii(priv, 2, &data8);
			ksz9477_set_gbit(priv, true, &data8);
			break;
		default:
			ksz9477_set_xmii(priv, 3, &data8);
			ksz9477_set_gbit(priv, true, &data8);
			data8 &= ~PORT_RGMII_ID_IG_ENABLE;
			data8 &= ~PORT_RGMII_ID_EG_ENABLE;
			if (interface == PHY_INTERFACE_MODE_RGMII_ID ||
			    interface == PHY_INTERFACE_MODE_RGMII_RXID)
				data8 |= PORT_RGMII_ID_IG_ENABLE;
			if (interface == PHY_INTERFACE_MODE_RGMII_ID ||
			    interface == PHY_INTERFACE_MODE_RGMII_TXID)
				data8 |= PORT_RGMII_ID_EG_ENABLE;
			/* On KSZ9893, disable RGMII in-band status support */
			if (priv->features & IS_9893)
				data8 &= ~PORT_MII_MAC_MODE;
			break;
		}
		ksz_pwrite8(priv, port, REG_PORT_XMII_CTRL_1, data8);
	}

	return 0;
}

static int ksz_port_enable(struct dsa_port *dp, int port,
			   struct phy_device *phy)
{
	struct device *dev = dp->ds->dev;
	struct ksz_switch *priv = dev_get_priv(dev);
	u8 data8;
	int ret;

	/* setup this port */
	ret = ksz_port_setup(priv, port, phy->interface);
	if (ret) {
		dev_err(dev, "port setup failed: %d\n", ret);
		return ret;
	}

	/* enable port forwarding for this port */
	ksz_pread8(priv, port, REG_PORT_LUE_MSTP_STATE, &data8);
	data8 &= ~PORT_LEARN_DISABLE;
	data8 |= (PORT_TX_ENABLE | PORT_RX_ENABLE);
	ksz_pwrite8(priv, port, REG_PORT_LUE_MSTP_STATE, data8);

	/* if cpu master we are done */
	if (port == dp->ds->cpu_port)
		return 0;

	/* start switch */
	ksz_read8(priv, REG_SW_OPERATION, &data8);
	data8 |= SW_START;
	ksz_write8(priv, REG_SW_OPERATION, data8);

	return 0;
}

static int ksz_xmit(struct dsa_port *dp, int port, void *packet, int length)
{
	u16 *tag = packet + length - dp->ds->needed_tx_tailroom;

	*tag = cpu_to_be16(BIT(dp->index));

	return 0;
}

static int ksz_recv(struct dsa_switch *ds, int *port, void *packet, int length)
{
	u8 *tag = packet + length - ds->needed_rx_tailroom;

	*port = *tag & 7;

	return 0;
};

static const struct dsa_switch_ops ksz_dsa_ops = {
	.port_enable = ksz_port_enable,
	.xmit = ksz_xmit,
	.rcv = ksz_recv,
	.phy_read = ksz9477_phy_read16,
	.phy_write = ksz9477_phy_write16,
};

static int ksz_default_setup(struct ksz_switch *priv)
{
	int i;

	for (i = 0; i < priv->ds.num_ports; i++) {
		/* isolate all ports by default */
		ksz9477_cfg_port_member(priv, i, 0);
		/* and suspend ports with integrated PHYs */
		if (i < priv->phy_port_cnt)
			ksz_pwrite16(priv, i, 0x100 + (MII_BMCR << 1),
				     BMCR_PDOWN);
	}

	return 0;
}

static int microchip_switch_regmap_init(struct ksz_switch *priv)
{
	const struct regmap_config *cfg;
	int i;

	cfg = priv->spi ? ksz9477_spi_regmap_config : ksz9477_i2c_regmap_config;

	for (i = 0; i < KSZ_REGMAP_ENTRY_COUNT; i++) {
		if (priv->spi)
			priv->regmap[i] = regmap_init_spi(priv->spi, &cfg[i]);
		else
			priv->regmap[i] = regmap_init_i2c(priv->i2c, &cfg[i]);
		if (IS_ERR(priv->regmap[i]))
			return dev_err_probe(priv->dev, PTR_ERR(priv->regmap[i]),
					     "Failed to initialize regmap%i\n",
					     cfg[i].val_bits);
	}

	return 0;
}

static int microchip_switch_probe(struct device *dev)
{
	struct device *hw_dev;
	struct ksz_switch *priv;
	struct gpio_desc *gpio;
	int ret = 0;
	struct dsa_switch *ds;

	priv = xzalloc(sizeof(*priv));

	dev->priv = priv;
	priv->dev = dev;

	if (dev_bus_is_spi(dev)) {
		priv->spi = (struct spi_device *)dev->type_data;
		priv->spi->mode = SPI_MODE_0;
		priv->spi->bits_per_word = 8;
		hw_dev = &priv->spi->dev;
	} else if (dev_bus_is_i2c(dev)) {
		priv->i2c = dev->type_data;
		hw_dev = &priv->i2c->dev;
	}

	ret = microchip_switch_regmap_init(priv);
	if (ret)
		return ret;

	gpio = gpiod_get_optional(dev, "reset", GPIOF_OUT_INIT_ACTIVE);
	if (IS_ERR(gpio)) {
		dev_warn(dev, "Failed to get 'reset' GPIO (ignored)\n");
	} else if (gpio) {
		mdelay(1);
		gpiod_set_value(gpio, false);
	}

	ksz_reset_switch(dev->priv);

	ret = ksz9477_switch_detect(dev->priv);
	if (ret) {
		dev_err(hw_dev, "error detecting KSZ9477: %pe\n", ERR_PTR(ret));
		return -ENODEV;
	}

	dev_info(dev, "chip id: 0x%08x\n", priv->chip_id);

	ds = &priv->ds;
	ds->dev = dev;
	ds->num_ports = 7;
	ds->ops = &ksz_dsa_ops;
	ds->needed_rx_tailroom = 1;
	ds->needed_tx_tailroom = 2;
	if (priv->phy_port_cnt == 5)
		ds->phys_mii_mask = 0x1f;
	else
		ds->phys_mii_mask = 0x03;

	ksz_default_setup(priv);

	ret = dsa_register_switch(ds);
	if (ret)
		return ret;

	return regmap_multi_register_cdev(priv->regmap[0], priv->regmap[1],
					  priv->regmap[2], NULL);
}

static const struct of_device_id microchip_switch_dt_ids[] = {
	{ .compatible = "microchip,ksz8563" },
	{ .compatible = "microchip,ksz9477" },
	{ .compatible = "microchip,ksz9563" },
	{ }
};
MODULE_DEVICE_TABLE(of, microchip_switch_dt_ids);

static struct driver microchip_switch_spi_driver = {
	.name		= "ksz9477-spi",
	.probe		= microchip_switch_probe,
	.of_compatible	= DRV_OF_COMPAT(microchip_switch_dt_ids),
};
device_spi_driver(microchip_switch_spi_driver);

static struct driver microchip_switch_i2c_driver = {
	.name		= "ksz9477-i2c",
	.probe		= microchip_switch_probe,
	.of_compatible	= DRV_OF_COMPAT(microchip_switch_dt_ids),
};
device_i2c_driver(microchip_switch_i2c_driver);
