// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SMSC LAN9[12]1[567] Network driver
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>

#include <command.h>
#include <net.h>
#include <malloc.h>
#include <init.h>
#include <xfuncs.h>
#include <errno.h>
#include <clock.h>
#include <io.h>
#include <platform_data/eth-smc911x.h>
#include <linux/phy.h>

#include "smc911x.h"

struct smc911x_priv {
	struct eth_device edev;
	struct mii_bus miibus;
	void __iomem *base;

	u32 shift;
	int generation;
	unsigned int flags;
	unsigned int idrev;
	unsigned int using_extphy;
	unsigned int phy_mask;

	u32 (*reg_read)(struct smc911x_priv *priv, u32 reg);
	void (*reg_write)(struct smc911x_priv *priv, u32 reg, u32 val);
};

#define DRIVERNAME "smc911x"

#define __smc_shift(priv, reg) ((reg) << ((priv)->shift))

static inline u32 smc911x_reg_read(struct smc911x_priv *priv, u32 reg)
{
	return priv->reg_read(priv, reg);
}

static inline u32 __smc911x_reg_readw(struct smc911x_priv *priv, u32 reg)
{
	return ((readw(priv->base + reg) & 0xFFFF) |
		((readw(priv->base + reg + 2) & 0xFFFF) << 16));
}

static inline u32 __smc911x_reg_readl(struct smc911x_priv *priv, u32 reg)
{
	return readl(priv->base + reg);
}

static inline u32
__smc911x_reg_readw_shift(struct smc911x_priv *priv, u32 reg)
{
	return (readw(priv->base +
			__smc_shift(priv, reg)) & 0xFFFF) |
		((readw(priv->base +
		__smc_shift(priv, reg + 2)) & 0xFFFF) << 16);

}

static inline u32
__smc911x_reg_readl_shift(struct smc911x_priv *priv, u32 reg)
{
	return readl(priv->base + __smc_shift(priv, reg));
}

static inline void smc911x_reg_write(struct smc911x_priv *priv, u32 reg,
					u32 val)
{
	priv->reg_write(priv, reg, val);
}

static inline void __smc911x_reg_writew(struct smc911x_priv *priv, u32 reg,
					u32 val)
{
	writew(val & 0xFFFF, priv->base + reg);
	writew((val >> 16) & 0xFFFF, priv->base + reg + 2);
}

static inline void __smc911x_reg_writel(struct smc911x_priv *priv, u32 reg,
					u32 val)
{
	writel(val, priv->base + reg);
}

static inline void
__smc911x_reg_writew_shift(struct smc911x_priv *priv, u32 reg, u32 val)
{
	writew(val & 0xFFFF,
		priv->base + __smc_shift(priv, reg));
	writew((val >> 16) & 0xFFFF,
		priv->base + __smc_shift(priv, reg + 2));
}

static inline void
__smc911x_reg_writel_shift(struct smc911x_priv *priv, u32 reg, u32 val)
{
	writel(val, priv->base + __smc_shift(priv, reg));
}

static int smc911x_mac_wait_busy(struct smc911x_priv *priv)
{
	uint64_t start = get_time_ns();

	while (!is_timeout(start, MSECOND)) {
		if (!(smc911x_reg_read(priv, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY))
			return 0;
	}

	printf("%s: mac timeout\n", __FUNCTION__);
	return -1;
}

static u32 smc911x_get_mac_csr(struct eth_device *edev, u8 reg)
{
	struct smc911x_priv *priv = edev->priv;
	ulong val;

	smc911x_mac_wait_busy(priv);

	smc911x_reg_write(priv, MAC_CSR_CMD, MAC_CSR_CMD_CSR_BUSY |
			  MAC_CSR_CMD_R_NOT_W | reg);

	smc911x_mac_wait_busy(priv);

	val = smc911x_reg_read(priv, MAC_CSR_DATA);

	return val;
}

static void smc911x_set_mac_csr(struct eth_device *edev, u8 reg, u32 data)
{
	struct smc911x_priv *priv = edev->priv;

	smc911x_mac_wait_busy(priv);

	smc911x_reg_write(priv, MAC_CSR_DATA, data);
	smc911x_reg_write(priv, MAC_CSR_CMD, MAC_CSR_CMD_CSR_BUSY | reg);

	smc911x_mac_wait_busy(priv);
}

static int smc911x_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	unsigned long addrh, addrl;

	addrh = smc911x_get_mac_csr(edev, ADDRH);
	addrl = smc911x_get_mac_csr(edev, ADDRL);

	m[0] = (addrl       ) & 0xff;
	m[1] = (addrl >>  8 ) & 0xff;
	m[2] = (addrl >> 16 ) & 0xff;
	m[3] = (addrl >> 24 ) & 0xff;
	m[4] = (addrh       ) & 0xff;
	m[5] = (addrh >>  8 ) & 0xff;

	/* we get 0xff when there is no eeprom connected */
	if ((m[0] & m[1] & m[2] & m[3] & m[4] & m[5]) == 0xff)
		return -1;

	return 0;
}

static int smc911x_set_ethaddr(struct eth_device *edev, const unsigned char *m)
{
	unsigned long addrh, addrl;

	addrl = m[0] | m[1] << 8 | m[2] << 16 | m[3] << 24;
	addrh = m[4] | m[5] << 8;
	smc911x_set_mac_csr(edev, ADDRH, addrh);
	smc911x_set_mac_csr(edev, ADDRL, addrl);

	return 0;
}

static int smc911x_phy_read(struct mii_bus *bus, int phy_addr, int reg)
{
	struct eth_device *edev = bus->priv;

	while (smc911x_get_mac_csr(edev, MII_ACC) & MII_ACC_MII_BUSY);

	smc911x_set_mac_csr(edev, MII_ACC, phy_addr << 11 | reg << 6 |
			MII_ACC_MII_BUSY);

	while (smc911x_get_mac_csr(edev, MII_ACC) & MII_ACC_MII_BUSY);

	return smc911x_get_mac_csr(edev, MII_DATA);
}

static int smc911x_phy_write(struct mii_bus *bus, int phy_addr,
	int reg, u16 val)
{
	struct eth_device *edev = bus->priv;

	while (smc911x_get_mac_csr(edev, MII_ACC) & MII_ACC_MII_BUSY);

	smc911x_set_mac_csr(edev, MII_DATA, val);
	smc911x_set_mac_csr(edev, MII_ACC,
		phy_addr << 11 | reg << 6 | MII_ACC_MII_BUSY |
		MII_ACC_MII_WRITE);

	while (smc911x_get_mac_csr(edev, MII_ACC) & MII_ACC_MII_BUSY);

	return 0;
}

/* Switch to external phy. Assumes tx and rx are stopped. */
static void smsc911x_phy_enable_external(struct eth_device *edev)
{
	struct smc911x_priv *priv = edev->priv;
	u32 hwcfg = smc911x_reg_read(priv, HW_CFG);

	/* Disable phy clocks to the MAC */
	hwcfg &= (~HW_CFG_PHY_CLK_SEL);
	hwcfg |= HW_CFG_PHY_CLK_SEL_CLK_DIS;
	smc911x_reg_write(priv, HW_CFG, hwcfg);
	udelay(10);	/* Enough time for clocks to stop */

	/* Switch to external phy */
	hwcfg |= HW_CFG_EXT_PHY_EN;
	smc911x_reg_write(priv, HW_CFG, hwcfg);

	/* Enable phy clocks to the MAC */
	hwcfg &= (~HW_CFG_PHY_CLK_SEL);
	hwcfg |= HW_CFG_PHY_CLK_SEL_EXT_PHY;
	smc911x_reg_write(priv, HW_CFG, hwcfg);
	udelay(10);	/* Enough time for clocks to restart */

	/* Switch MDIO/MDC to external PHY */
	hwcfg |= HW_CFG_SMI_SEL;
	smc911x_reg_write(priv, HW_CFG, hwcfg);
}

/* Autodetects and enables external phy if present on supported chips.
 * autodetection can be overridden by specifying SMC911X_FORCE_INTERNAL_PHY
 * or SMC911X_FORCE_EXTERNAL_PHY in the platform_data flags. */
static void smsc911x_phy_initialise_external(struct eth_device *edev)
{
	struct smc911x_priv *priv = edev->priv;
	u32 hwcfg = smc911x_reg_read(priv, HW_CFG);

	if (priv->flags & SMC911X_FORCE_INTERNAL_PHY) {
		dev_info(edev->parent, "Forcing internal PHY\n");
		priv->using_extphy = 0;
	} else if (priv->flags & SMC911X_FORCE_EXTERNAL_PHY) {
		dev_info(edev->parent, "Forcing external PHY\n");
		smsc911x_phy_enable_external(edev);
		priv->using_extphy = 1;
	} else if (hwcfg & HW_CFG_EXT_PHY_DET) {
		dev_info(edev->parent,
			"HW_CFG EXT_PHY_DET set, using external PHY\n");
		smsc911x_phy_enable_external(edev);
		priv->using_extphy = 1;
	} else {
		dev_info(edev->parent,
			"HW_CFG EXT_PHY_DET clear, using internal PHY\n");
		priv->using_extphy = 0;
	}
}

static int smc911x_phy_reset(struct eth_device *edev)
{
	struct smc911x_priv *priv = edev->priv;
	u32 reg;

	reg = smc911x_reg_read(priv, PMT_CTRL);
	reg &= 0xfcf;
	reg |= PMT_CTRL_PHY_RST;
	smc911x_reg_write(priv, PMT_CTRL, reg);

	mdelay(100);

	return 0;
}

static void smc911x_reset(struct eth_device *edev)
{
	struct smc911x_priv *priv = edev->priv;
	uint64_t start;

	/* Take out of PM setting first */
	if (smc911x_reg_read(priv, PMT_CTRL) & PMT_CTRL_READY) {
		/* Write to the bytetest will take out of powerdown */
		smc911x_reg_write(priv, BYTE_TEST, 0);

		start = get_time_ns();
		while(1) {
			if ((smc911x_reg_read(priv, PMT_CTRL) & PMT_CTRL_READY))
				break;
			if (is_timeout(start, 100 * USECOND)) {
				dev_err(&edev->dev,
						"timeout waiting for PM restore\n");
				return;
			}
		}
	}

	/* Disable interrupts */
	smc911x_reg_write(priv, INT_EN, 0);

	smc911x_reg_write(priv, HW_CFG, HW_CFG_SRST);

	start = get_time_ns();
	while(1) {
		if (!(smc911x_reg_read(priv, E2P_CMD) & E2P_CMD_EPC_BUSY))
			break;
		if (is_timeout(start, 10 * MSECOND)) {
			dev_err(&edev->dev, "reset timeout\n");
			return;
		}
	}

	/* Reset the FIFO level and flow control settings */
	smc911x_set_mac_csr(edev, FLOW, FLOW_FCPT | FLOW_FCEN);

	smc911x_reg_write(priv, AFC_CFG, 0x0050287F);

	/* Set to LED outputs */
	smc911x_reg_write(priv, GPIO_CFG, 0x70070000);

	/* Select internal/external PHY */
	switch (priv->idrev & 0xFFFF0000) {
	case 0x01170000:
	case 0x01150000:
	case 0x117A0000:
	case 0x115A0000:
		/* External PHY supported, try to autodetect */
		smsc911x_phy_initialise_external(edev);
		break;
	default:
		dev_dbg(edev->parent, "External PHY is not supported, "
			"using internal PHY\n");
		priv->using_extphy = 0;
		break;
	}

	if (!priv->using_extphy) {
		/* Mask all PHYs except ID 1 (internal) */
		priv->miibus.phy_mask = ~(1 << 1);
	} else {
		/* Mask PHYs as requested (or try all if mask is 0) */
		/* Attn.: first probed PHY is used by phy_device_connect */
		priv->miibus.phy_mask = priv->phy_mask;
	}
}

static void smc911x_enable(struct eth_device *edev)
{
	struct smc911x_priv *priv = edev->priv;
	u32 hw_cfg_phy_settings;

	/* Enable TX */
	hw_cfg_phy_settings = smc911x_reg_read(priv, HW_CFG) & (HW_CFG_PHY_CLK_SEL|HW_CFG_SMI_SEL|HW_CFG_EXT_PHY_EN);
	smc911x_reg_write(priv, HW_CFG, 8 << 16 | HW_CFG_SF | hw_cfg_phy_settings);

	smc911x_reg_write(priv, GPT_CFG, GPT_CFG_TIMER_EN | 10000);

	smc911x_reg_write(priv, TX_CFG, TX_CFG_TX_ON);

	/* no padding to start of packets */
	smc911x_reg_write(priv, RX_CFG, RX_CFG_RX_DUMP);
}

static int smc911x_eth_open(struct eth_device *edev)
{
	struct smc911x_priv *priv = (struct smc911x_priv *)edev->priv;
	int ret;

	/* use first probed PHY (see also phy_mask) */
	ret = phy_device_connect(edev, &priv->miibus, -1, NULL,
				 0, PHY_INTERFACE_MODE_NA);
	if (ret)
		return ret;

	/* Turn on Tx + Rx */
	smc911x_enable(edev);
	return 0;
}

static int smc911x_eth_send(struct eth_device *edev, void *packet, int length)
{
	struct smc911x_priv *priv = (struct smc911x_priv *)edev->priv;
	u32 *data = (u32*)packet;
	u32 tmplen;
	u32 status;
	uint64_t start;

	smc911x_reg_write(priv, TX_DATA_FIFO,
		  TX_CMD_A_INT_FIRST_SEG | TX_CMD_A_INT_LAST_SEG | length);
	smc911x_reg_write(priv, TX_DATA_FIFO, length);

	tmplen = (length + 3) / 4;

	while(tmplen--)
		smc911x_reg_write(priv, TX_DATA_FIFO, *data++);

	/* wait for transmission */
	start = get_time_ns();
	while (1) {
		if ((smc911x_reg_read(priv, TX_FIFO_INF) &
					TX_FIFO_INF_TSUSED) >> 16)
			break;
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(&edev->dev, "TX timeout\n");
			return -1;
		}
	}

	/* get status. Ignore 'no carrier' error, it has no meaning for
	 * full duplex operation
	 */
	status = smc911x_reg_read(priv, TX_STATUS_FIFO) & (TX_STS_LOC |
		TX_STS_LATE_COLL | TX_STS_MANY_COLL | TX_STS_MANY_DEFER |
		TX_STS_UNDERRUN);

	if(!status)
		return 0;

	dev_err(&edev->dev, "failed to send packet: %s%s%s%s%s\n",
		status & TX_STS_LOC ? "TX_STS_LOC " : "",
		status & TX_STS_LATE_COLL ? "TX_STS_LATE_COLL " : "",
		status & TX_STS_MANY_COLL ? "TX_STS_MANY_COLL " : "",
		status & TX_STS_MANY_DEFER ? "TX_STS_MANY_DEFER " : "",
		status & TX_STS_UNDERRUN ? "TX_STS_UNDERRUN" : "");

	return -1;
}

static void smc911x_eth_halt(struct eth_device *edev)
{
	struct smc911x_priv *priv = (struct smc911x_priv *)edev->priv;

	/* Disable TX */
	smc911x_reg_write(priv, TX_CFG, TX_CFG_STOP_TX);

//	smc911x_reset(edev);
}

static int smc911x_eth_rx(struct eth_device *edev)
{
	struct smc911x_priv *priv = (struct smc911x_priv *)edev->priv;
	u32 *data = (u32 *)NetRxPackets[0];
	u32 pktlen, tmplen;
	u32 status;

	if((smc911x_reg_read(priv, RX_FIFO_INF) & RX_FIFO_INF_RXSUSED) >> 16) {
		status = smc911x_reg_read(priv, RX_STATUS_FIFO);
		pktlen = (status & RX_STS_PKT_LEN) >> 16;

		smc911x_reg_write(priv, RX_CFG, 0);

		tmplen = (pktlen + 2 + 3) / 4;
		while(tmplen--)
			*data++ = smc911x_reg_read(priv, RX_DATA_FIFO);

		if(status & RX_STS_ES)
			dev_err(&edev->dev, "dropped bad packet. Status: 0x%08x\n",
				status);
		else
			net_receive(edev, NetRxPackets[0], pktlen);
	}

	return 0;
}

static int smc911x_init_dev(struct eth_device *edev)
{
	smc911x_set_mac_csr(edev, MAC_CR, MAC_CR_TXEN | MAC_CR_RXEN |
			MAC_CR_HBDIS);

	return 0;
}

static int smc911x_probe(struct device_d *dev)
{
	struct resource *iores;
	struct eth_device *edev;
	struct smc911x_priv *priv;
	uint32_t val;
	int is_32bit, ret;
	struct smc911x_plat *pdata = dev->platform_data;

	priv = xzalloc(sizeof(*priv));
	is_32bit = dev->resource[0].flags & IORESOURCE_MEM_TYPE_MASK;
	if (!is_32bit)
		is_32bit = 1;
	else
		is_32bit = is_32bit == IORESOURCE_MEM_32BIT;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->base = IOMEM(iores->start);

	if (pdata) {
		priv->shift = pdata->shift;
		priv->flags = pdata->flags;
		priv->phy_mask = pdata->phy_mask;
	} else if (IS_ENABLED(CONFIG_OFDEVICE) && dev->device_node) {
		ret = of_property_read_u32(dev->device_node, "reg-io-width", &val);
		if (ret)
			return ret;
		is_32bit = (val == 4);

		of_property_read_u32(dev->device_node, "reg-shift", &priv->shift);

		if (of_property_read_bool(dev->device_node, "smsc,force-internal-phy"))
			priv->flags |= SMC911X_FORCE_INTERNAL_PHY;

		if (of_property_read_bool(dev->device_node, "smsc,force-external-phy"))
			priv->flags |= SMC911X_FORCE_EXTERNAL_PHY;
	}

	if (is_32bit) {
		if (priv->shift) {
			priv->reg_read = __smc911x_reg_readl_shift;
			priv->reg_write = __smc911x_reg_writel_shift;
		} else {
			priv->reg_read = __smc911x_reg_readl;
			priv->reg_write = __smc911x_reg_writel;
		}
	} else {
		if (priv->shift) {
			priv->reg_read = __smc911x_reg_readw_shift;
			priv->reg_write = __smc911x_reg_writew_shift;
		} else {
			priv->reg_read = __smc911x_reg_readw;
			priv->reg_write = __smc911x_reg_writew;
		}
	}

	/*
	 * poll the READY bit in PMT_CTRL. Any other access to the device is
	 * forbidden while this bit isn't set. Try for 100ms
	 */
	ret = wait_on_timeout(100 * MSECOND, !smc911x_reg_read(priv, PMT_CTRL) & PMT_CTRL_READY);
	if (!ret) {
		dev_err(dev, "Device not READY in 100ms aborting\n");
		return -ENODEV;
	}

	val = smc911x_reg_read(priv, BYTE_TEST);
	if (val == 0x43218765) {
		dev_dbg(dev, "BYTE_TEST looks swapped, "
			   "applying WORD_SWAP");
		smc911x_reg_write(priv, WORD_SWAP, 0xffffffff);

		/* 1 dummy read of BYTE_TEST is needed after a write to
		 * WORD_SWAP before its contents are valid */
		val = smc911x_reg_read(priv, BYTE_TEST);

		val = smc911x_reg_read(priv, BYTE_TEST);
	}

	if (val != 0x87654321) {
		dev_err(dev, "no smc911x found on 0x%p (byte_test=0x%08x)\n",
			priv->base, val);
		if ((((val >> 16) & 0xFFFF) == (val & 0xFFFF)) && is_32bit) {
			/*
			 * This may mean the chip is set
			 * for 32 bit while the bus is reading 16 bit
			 */
			dev_err(dev, "top 16 bits equal to bottom 16 bits\n");
		}
		return -ENODEV;
	}

	priv->idrev = val = smc911x_reg_read(priv, ID_REV);
	switch (val & 0xFFFF0000) {
	case 0x01180000:
	case 0x01170000:
	case 0x01160000:
	case 0x01150000:
	case 0x218A0000:
		/* LAN911[5678] family */
		priv->generation = val & 0x0000FFFF;
		break;

	case 0x118A0000:
	case 0x117A0000:
	case 0x116A0000:
	case 0x115A0000:
		/* LAN921[5678] family */
		priv->generation = 3;
		break;

	case 0x92100000:
	case 0x92110000:
	case 0x92200000:
	case 0x92210000:
		/* LAN9210/LAN9211/LAN9220/LAN9221 */
		priv->generation = 4;
		break;

	default:
		dev_err(dev, "LAN911x not identified, idrev: 0x%08X\n",
			  val);
		return -ENODEV;
	}

	dev_info(dev, "LAN911x identified, idrev: 0x%08X, generation: %d\n",
		   val, priv->generation);

	edev = &priv->edev;
	edev->priv = priv;

	edev->init = smc911x_init_dev;
	edev->open = smc911x_eth_open;
	edev->send = smc911x_eth_send;
	edev->recv = smc911x_eth_rx;
	edev->halt = smc911x_eth_halt;
	edev->get_ethaddr = smc911x_get_ethaddr;
	edev->set_ethaddr = smc911x_set_ethaddr;
	edev->parent = dev;

	priv->miibus.read = smc911x_phy_read;
	priv->miibus.write = smc911x_phy_write;
	priv->miibus.priv = edev;
	priv->miibus.parent = dev;

	smc911x_reset(edev);
	smc911x_phy_reset(edev);

	mdiobus_register(&priv->miibus);
	eth_register(edev);

        return 0;
}

static const struct of_device_id smsc911x_dt_ids[] = {
	{ .compatible = "smsc,lan9115", },
	{ /* sentinel */ }
};

static struct driver_d smc911x_driver = {
        .name  = "smc911x",
        .probe = smc911x_probe,
	.of_compatible = DRV_OF_COMPAT(smsc911x_dt_ids),
};
device_platform_driver(smc911x_driver);
