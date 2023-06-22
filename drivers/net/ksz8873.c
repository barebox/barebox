// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <complete.h>
#include <dsa.h>
#include <linux/gpio/consumer.h>
#include <linux/mii.h>
#include <linux/mdio.h>
#include <net.h>
#include <of_device.h>
#include <regmap.h>

#define KSZ8873_CHIP_ID0		0x00
#define KSZ8873_CHIP_ID1		0x01
#define KSZ88_CHIP_ID_M			GENMASK(7, 4)
#define KSZ88_REV_ID_M			GENMASK(3, 1)

#define KSZ8873_GLOBAL_CTRL_1		0x03
#define KSZ8873_PASS_ALL_FRAMES		BIT(7)
#define KSZ8873_P3_TAIL_TAG_EN		BIT(6)

/*
 * port specific registers. Should be used with ksz_pwrite/ksz_pread functions
 */
#define KSZ8873_PORTx_CTRL_1		0x01
#define KSZ8873_PORTx_CTRL_12		0x0c

#define PORT_AUTO_NEG_ENABLE		BIT(7)
#define PORT_FORCE_100_MBIT		BIT(6)
#define PORT_FORCE_FULL_DUPLEX		BIT(5)
#define PORT_AUTO_NEG_100BTX_FD		BIT(3)
#define PORT_AUTO_NEG_100BTX		BIT(2)
#define PORT_AUTO_NEG_10BT_FD		BIT(1)
#define PORT_AUTO_NEG_10BT		BIT(0)

#define KSZ8873_PORTx_CTRL_13		0x0d

#define PORT_AUTO_NEG_RESTART		BIT(5)
#define PORT_POWER_DOWN			BIT(3)

#define KSZ8873_PORTx_STATUS_0		0x0e

#define PORT_AUTO_NEG_COMPLETE		BIT(6)
#define PORT_STAT_LINK_GOOD		BIT(5)
#define PORT_REMOTE_100BTX_FD		BIT(3)
#define PORT_REMOTE_100BTX		BIT(2)
#define PORT_REMOTE_10BT_FD		BIT(1)
#define PORT_REMOTE_10BT		BIT(0)

#define KSZ8873_PORTx_STATUS_1		0x0f

#define KSZ8795_ID_HI			0x0022
#define KSZ8863_ID_LO			0x1430

#define PORT_CTRL_ADDR(port, addr)	((addr) + 0x10 + (port) * 0x10)

struct ksz8873_dcfg {
	unsigned int num_ports;
	unsigned int phy_port_cnt;
	u8 id0;
	u8 id1;
};

struct ksz8873_switch {
	struct phy_device *mdiodev;
	struct dsa_switch ds;
	struct device *dev;
	const struct ksz8873_dcfg *dcfg;
	struct regmap *regmap;
};

/* Serial Management Interface (SMI) uses the following frame format:
 *
 *       preamble|start|Read/Write|  PHY   |  REG  |TA|   Data bits      | Idle
 *               |frame| OP code  |address |address|  |                  |
 * read | 32x1´s | 01  |    00    | 1xRRR  | RRRRR |Z0| 00000000DDDDDDDD |  Z
 * write| 32x1´s | 01  |    00    | 0xRRR  | RRRRR |10| xxxxxxxxDDDDDDDD |  Z
 *
 */

#define SMI_KSZ88XX_READ_PHY	BIT(4)

static int ksz8873_mdio_read(void *ctx, unsigned int reg, unsigned int *val)
{
	struct ksz8873_switch *priv = ctx;
	struct phy_device *mdiodev = priv->mdiodev;
	int ret;

	ret = mdiobus_read(mdiodev->bus, ((reg & 0xE0) >> 5) |
			     SMI_KSZ88XX_READ_PHY, reg);
	if (ret < 0)
		return ret;

	*val = ret;

	return 0;
}

static int ksz8873_mdio_write(void *ctx, unsigned int reg, unsigned int val)
{
	struct ksz8873_switch *priv = ctx;
	struct phy_device *mdiodev = priv->mdiodev;

	return mdiobus_write(mdiodev->bus, ((reg & 0xE0) >> 5), reg, val);
}

static const struct regmap_bus ksz8873_regmap_smi = {
	.reg_read = ksz8873_mdio_read,
	.reg_write = ksz8873_mdio_write,
};

static const struct regmap_config ksz8873_regmap_config = {
	.name = "#8",
	.reg_bits = 8,
	.pad_bits = 24,
	.val_bits = 8,
};

static int ksz_read8(struct ksz8873_switch *priv, u32 reg, u8 *val)
{
	unsigned int value;
	int ret = regmap_read(priv->regmap, reg, &value);

	*val = value & 0xff;

	return ret;
}

static int ksz_write8(struct ksz8873_switch *priv, u32 reg, u8 value)
{
	return regmap_write(priv->regmap, reg, value);
}

static int ksz_pread8(struct ksz8873_switch *priv, int port, int reg, u8 *val)
{
	return ksz_read8(priv, PORT_CTRL_ADDR(port, reg), val);
}

static int ksz_pwrite8(struct ksz8873_switch *priv, int port, int reg, u8 val)
{
	return ksz_write8(priv, PORT_CTRL_ADDR(port, reg), val);
}

static void ksz8_r_phy(struct ksz8873_switch *priv, u16 phy, u16 reg, u16 *val)
{
	u8 restart, ctrl, link;
	int processed = true;
	u16 data = 0;
	u8 p = phy;

	switch (reg) {
	case MII_BMCR:
		ksz_pread8(priv, p, KSZ8873_PORTx_CTRL_13, &restart);
		ksz_pread8(priv, p, KSZ8873_PORTx_CTRL_12, &ctrl);
		if (ctrl & PORT_FORCE_100_MBIT)
			data |= BMCR_SPEED100;
		if ((ctrl & PORT_AUTO_NEG_ENABLE))
			data |= BMCR_ANENABLE;
		if (restart & PORT_POWER_DOWN)
			data |= BMCR_PDOWN;
		if (restart & PORT_AUTO_NEG_RESTART)
			data |= BMCR_ANRESTART;
		if (ctrl & PORT_FORCE_FULL_DUPLEX)
			data |= BMCR_FULLDPLX;
		break;
	case MII_BMSR:
		ksz_pread8(priv, p, KSZ8873_PORTx_STATUS_0, &link);
		data = BMSR_100FULL |
		       BMSR_100HALF |
		       BMSR_10FULL |
		       BMSR_10HALF |
		       BMSR_ANEGCAPABLE;
		if (link & PORT_AUTO_NEG_COMPLETE)
			data |= BMSR_ANEGCOMPLETE;
		if (link & PORT_STAT_LINK_GOOD)
			data |= BMSR_LSTATUS;
		break;
	case MII_PHYSID1:
		data = KSZ8795_ID_HI;
		break;
	case MII_PHYSID2:
		data = KSZ8863_ID_LO;
		break;
	case MII_ADVERTISE:
		ksz_pread8(priv, p, KSZ8873_PORTx_CTRL_12, &ctrl);
		data = ADVERTISE_CSMA;
		if (ctrl & PORT_AUTO_NEG_100BTX_FD)
			data |= ADVERTISE_100FULL;
		if (ctrl & PORT_AUTO_NEG_100BTX)
			data |= ADVERTISE_100HALF;
		if (ctrl & PORT_AUTO_NEG_10BT_FD)
			data |= ADVERTISE_10FULL;
		if (ctrl & PORT_AUTO_NEG_10BT)
			data |= ADVERTISE_10HALF;
		break;
	case MII_LPA:
		ksz_pread8(priv, p, KSZ8873_PORTx_STATUS_0, &link);
		data = LPA_SLCT;
		if (link & PORT_REMOTE_100BTX_FD)
			data |= LPA_100FULL;
		if (link & PORT_REMOTE_100BTX)
			data |= LPA_100HALF;
		if (link & PORT_REMOTE_10BT_FD)
			data |= LPA_10FULL;
		if (link & PORT_REMOTE_10BT)
			data |= LPA_10HALF;
		if (data & ~LPA_SLCT)
			data |= LPA_LPACK;
		break;
	default:
		processed = false;
		break;
	}
	if (processed)
		*val = data;
}

static void ksz8_w_phy(struct ksz8873_switch *priv, u16 phy, u16 reg, u16 val)
{
	u8 restart, ctrl, data;
	u8 p = phy;

	switch (reg) {
	case MII_BMCR:
		ksz_pread8(priv, p, KSZ8873_PORTx_CTRL_12, &ctrl);
		data = ctrl;
		if ((val & BMCR_ANENABLE))
			data |= PORT_AUTO_NEG_ENABLE;
		else
			data &= ~PORT_AUTO_NEG_ENABLE;

		if (val & BMCR_SPEED100)
			data |= PORT_FORCE_100_MBIT;
		else
			data &= ~PORT_FORCE_100_MBIT;
		if (val & BMCR_FULLDPLX)
			data |= PORT_FORCE_FULL_DUPLEX;
		else
			data &= ~PORT_FORCE_FULL_DUPLEX;
		if (data != ctrl)
			ksz_pwrite8(priv, p, KSZ8873_PORTx_CTRL_12, data);
		ksz_pread8(priv, p, KSZ8873_PORTx_CTRL_13, &restart);
		data = restart;
		if (val & BMCR_ANRESTART)
			data |= PORT_AUTO_NEG_RESTART;
		else
			data &= ~(PORT_AUTO_NEG_RESTART);
		if (val & BMCR_PDOWN)
			data |= PORT_POWER_DOWN;
		else
			data &= ~PORT_POWER_DOWN;
		if (data != restart)
			ksz_pwrite8(priv, p, KSZ8873_PORTx_CTRL_13, data);
		break;
	case MII_ADVERTISE:
		ksz_pread8(priv, p, KSZ8873_PORTx_CTRL_12, &ctrl);
		data = ctrl;
		data &= ~(PORT_AUTO_NEG_100BTX_FD |
			  PORT_AUTO_NEG_100BTX |
			  PORT_AUTO_NEG_10BT_FD |
			  PORT_AUTO_NEG_10BT);
		if (val & ADVERTISE_100FULL)
			data |= PORT_AUTO_NEG_100BTX_FD;
		if (val & ADVERTISE_100HALF)
			data |= PORT_AUTO_NEG_100BTX;
		if (val & ADVERTISE_10FULL)
			data |= PORT_AUTO_NEG_10BT_FD;
		if (val & ADVERTISE_10HALF)
			data |= PORT_AUTO_NEG_10BT;
		if (data != ctrl)
			ksz_pwrite8(priv, p, KSZ8873_PORTx_CTRL_12, data);
		break;
	default:
		break;
	}
}

static int ksz8873_phy_read16(struct dsa_switch *ds, int addr, int reg)
{
	struct device *dev = ds->dev;
	struct ksz8873_switch *priv = dev_get_priv(dev);
	u16 val = 0xffff;

	if (addr >= priv->dcfg->phy_port_cnt)
		return val;

	ksz8_r_phy(priv, addr, reg, &val);

	return val;
}

static int ksz8873_phy_write16(struct dsa_switch *ds, int addr, int reg,
			       u16 val)
{
	struct device *dev = ds->dev;
	struct ksz8873_switch *priv = dev_get_priv(dev);

	/* No real PHY after this. */
	if (addr >= priv->dcfg->phy_port_cnt)
		return 0;

	ksz8_w_phy(priv, addr, reg, val);

	return 0;
}

static void ksz8873_cfg_port_member(struct ksz8873_switch *priv, int port,
				    u8 member)
{
	ksz_pwrite8(priv, port, KSZ8873_PORTx_CTRL_1, member);
}

static int ksz8873_port_enable(struct dsa_port *dp, int port,
			   struct phy_device *phy)
{
	return 0;
}

static int ksz8873_xmit(struct dsa_port *dp, int port, void *packet, int length)
{
	u8 *tag = packet + length - dp->ds->needed_tx_tailroom;

	*tag = BIT(dp->index);

	return 0;
}

static int ksz8873_recv(struct dsa_switch *ds, int *port, void *packet,
			int length)
{
	u8 *tag = packet + length - ds->needed_rx_tailroom;

	*port = *tag & 7;

	return 0;
};

static const struct dsa_switch_ops ksz8873_dsa_ops = {
	.port_enable = ksz8873_port_enable,
	.xmit = ksz8873_xmit,
	.rcv = ksz8873_recv,
	.phy_read = ksz8873_phy_read16,
	.phy_write = ksz8873_phy_write16,
};

static int ksz8873_default_setup(struct ksz8873_switch *priv)
{
	int i;

	ksz_write8(priv, KSZ8873_GLOBAL_CTRL_1, KSZ8873_PASS_ALL_FRAMES |
		   KSZ8873_P3_TAIL_TAG_EN);

	for (i = 0; i < priv->ds.num_ports; i++) {
		u8 member;
		/* isolate all ports by default */
		member = BIT(priv->ds.cpu_port);
		ksz8873_cfg_port_member(priv, i, member);

		member = dsa_user_ports(&priv->ds);
		ksz8873_cfg_port_member(priv, priv->ds.cpu_port, member);
	}

	return 0;
}

static int ksz8873_probe_mdio(struct phy_device *mdiodev)
{
	struct device *dev = &mdiodev->dev;
	const struct ksz8873_dcfg *dcfg;
	struct ksz8873_switch *priv;
	struct dsa_switch *ds;
	struct gpio_desc *gpio;
	int ret;
	u8 id0, id1;

	priv = xzalloc(sizeof(*priv));

	dcfg = of_device_get_match_data(dev);
	if (!dcfg)
		return -EINVAL;

	dev->priv = priv;
	priv->dev = dev;
	priv->dcfg = dcfg;
	priv->mdiodev = mdiodev;

	priv->regmap = regmap_init(dev, &ksz8873_regmap_smi, priv,
				   &ksz8873_regmap_config);
	if (IS_ERR(priv->regmap))
		return dev_err_probe(dev, PTR_ERR(priv->regmap),
				     "Failed to initialize regmap.\n");

	gpio = gpiod_get_optional(dev, "reset", GPIOF_OUT_INIT_ACTIVE);
	if (IS_ERR(gpio)) {
		dev_warn(dev, "Failed to get 'reset' GPIO (ignored)\n");
	} else if (gpio) {
		mdelay(1);
		gpiod_set_value(gpio, false);
	}

	ret = ksz_read8(priv, KSZ8873_CHIP_ID0, &id0);
	if (ret)
		return ret;

	ret = ksz_read8(priv, KSZ8873_CHIP_ID1, &id1);
	if (ret)
		return ret;

	if (id0 != dcfg->id0 ||
	    (id1 & (KSZ88_CHIP_ID_M | KSZ88_REV_ID_M)) != dcfg->id1)
		return -ENODEV;

	ds = &priv->ds;
	ds->dev = dev;
	ds->num_ports = dcfg->num_ports;
	ds->ops = &ksz8873_dsa_ops;
	ds->needed_rx_tailroom = 1;
	ds->needed_tx_tailroom = 1;
	ds->phys_mii_mask = 0x3;

	ret = dsa_register_switch(ds);
	if (ret)
		return ret;

	ksz8873_default_setup(priv);

	return 0;
}

static const struct ksz8873_dcfg ksz8873_dcfg = {
	.num_ports = 3,
	.phy_port_cnt = 2,
	.id0 = 0x88,
	.id1 = 0x30,
};

static const struct of_device_id ksz8873_dt_ids[] = {
	{ .compatible = "microchip,ksz8873", .data = &ksz8873_dcfg },
	{ }
};
MODULE_DEVICE_TABLE(of, ksz8873_dt_ids);

static struct phy_driver ksz8873_driver_mdio = {
	.drv = {
		.name = "KSZ8873 MDIO",
		.of_compatible = DRV_OF_COMPAT(ksz8873_dt_ids),
	},
	.probe = ksz8873_probe_mdio,
};
device_mdio_driver(ksz8873_driver_mdio);
