// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * DaVinci MDIO Module driver
 */
#include <driver.h>
#include <linux/phy.h>
#include <clock.h>
#include <xfuncs.h>
#include <io.h>
#include <linux/clk.h>

#define PHY_REG_MASK		0x1f
#define PHY_ID_MASK		0x1f

struct cpsw_mdio_regs {
	u32	version;
	u32	control;
#define CONTROL_IDLE		(1 << 31)
#define CONTROL_ENABLE		(1 << 30)

	u32	alive;
	u32	link;
	u32	linkintraw;
	u32	linkintmasked;
	u32	__reserved_0[2];
	u32	userintraw;
	u32	userintmasked;
	u32	userintmaskset;
	u32	userintmaskclr;
	u32	__reserved_1[20];

	struct {
		u32 access;
		u32 physel;
#define USERACCESS_GO		(1 << 31)
#define USERACCESS_WRITE	(1 << 30)
#define USERACCESS_ACK		(1 << 29)
#define USERACCESS_READ		(0)
#define USERACCESS_DATA		(0xffff)
	} user[0];
};

struct cpsw_mdio_priv {
	struct device			*dev;
	struct mii_bus			miibus;
	struct cpsw_mdio_regs		*mdio_regs;
	struct clk			*clk;
};

/* wait until hardware is ready for another user access */
static u32 wait_for_user_access(struct cpsw_mdio_priv *priv)
{
	struct cpsw_mdio_regs *mdio_regs = priv->mdio_regs;
	u32 tmp;
	uint64_t start = get_time_ns();

	do {
		tmp = readl(&mdio_regs->user[0].access);

		if (!(tmp & USERACCESS_GO))
			break;

		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(priv->dev, "timeout waiting for user access\n");
			break;
		}
	} while (1);

	return tmp;
}

static int cpsw_mdio_read(struct mii_bus *bus, int phy_id, int phy_reg)
{
	struct cpsw_mdio_priv *priv = bus->priv;
	struct cpsw_mdio_regs *mdio_regs = priv->mdio_regs;

	u32 tmp;

	if (phy_reg & ~PHY_REG_MASK || phy_id & ~PHY_ID_MASK)
		return -EINVAL;

	wait_for_user_access(priv);

	tmp = (USERACCESS_GO | USERACCESS_READ | (phy_reg << 21) |
	       (phy_id << 16));
	writel(tmp, &mdio_regs->user[0].access);

	tmp = wait_for_user_access(priv);

	return (tmp & USERACCESS_ACK) ? (tmp & USERACCESS_DATA) : -1;
}

static int cpsw_mdio_write(struct mii_bus *bus, int phy_id, int phy_reg, u16 value)
{
	struct cpsw_mdio_priv *priv = bus->priv;
	struct cpsw_mdio_regs *mdio_regs = priv->mdio_regs;
	u32 tmp;

	if (phy_reg & ~PHY_REG_MASK || phy_id & ~PHY_ID_MASK)
		return -EINVAL;

	wait_for_user_access(priv);
	tmp = (USERACCESS_GO | USERACCESS_WRITE | (phy_reg << 21) |
		   (phy_id << 16) | (value & USERACCESS_DATA));
	writel(tmp, &mdio_regs->user[0].access);
	wait_for_user_access(priv);

	return 0;
}

#define CONTROL_MAX_DIV 0xffff

static void cpsw_mdio_init_clk(struct cpsw_mdio_priv *priv)
{
	u32 mdio_in, clk_div;
	u32 bus_freq = 1000000;

	of_property_read_u32(priv->dev->of_node, "bus_freq", &bus_freq);

	priv->clk = clk_get_enabled(priv->dev, "fck");
	if (IS_ERR(priv->clk))
		mdio_in = 256000000;
	else
		mdio_in = clk_get_rate(priv->clk);

	clk_div = (mdio_in / bus_freq) - 1;
	if (clk_div > CONTROL_MAX_DIV)
		clk_div = CONTROL_MAX_DIV;

	writel(clk_div | CONTROL_ENABLE, &priv->mdio_regs->control);
}

static int cpsw_mdio_probe(struct device *dev)
{
	struct resource *iores;
	struct cpsw_mdio_priv *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;

	/* If we can't request I/O memory region, we'll assume parent did
	 * it for us
	 */
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores) && PTR_ERR(iores) == -EBUSY)
		iores = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->mdio_regs = IOMEM(iores->start);
	priv->miibus.read = cpsw_mdio_read;
	priv->miibus.write = cpsw_mdio_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	cpsw_mdio_init_clk(priv);

	ret = mdiobus_register(&priv->miibus);
	if (ret)
		return ret;

	return 0;
}

static __maybe_unused struct of_device_id cpsw_mdio_dt_ids[] = {
	{
		.compatible = "ti,cpsw-mdio",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, cpsw_mdio_dt_ids);

static struct driver cpsw_mdio_driver = {
	.name   = "cpsw-mdio",
	.probe  = cpsw_mdio_probe,
	.of_compatible = DRV_OF_COMPAT(cpsw_mdio_dt_ids),
};
coredevice_platform_driver(cpsw_mdio_driver);
