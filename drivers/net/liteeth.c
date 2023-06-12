// SPDX-License-Identifier: GPL-2.0-only
/*
 * LiteX Liteeth Ethernet
 *
 * Copyright 2017 Joel Stanley <joel@jms.id.au>
 *
 * Ported to barebox from linux kernel
 *   Copyright (C) 2019-2021 Antony Pavlov <antonynpavlov@gmail.com>
 *   Copyright (C) 2021 Marek Czerski <m.czerski@ap-tech.pl>
 *
 */

#include <common.h>
#include <io.h>
#include <linux/iopoll.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <of_net.h>
#include <linux/phy.h>
#include <linux/mdio-bitbang.h>

#define DRV_NAME	"liteeth"

#define LITEETH_WRITER_SLOT		0x00
#define LITEETH_WRITER_LENGTH		0x04
#define LITEETH_WRITER_ERRORS		0x08
#define LITEETH_WRITER_EV_STATUS	0x0c
#define LITEETH_WRITER_EV_PENDING	0x10
#define LITEETH_WRITER_EV_ENABLE	0x14
#define LITEETH_READER_START		0x18
#define LITEETH_READER_READY		0x1c
#define LITEETH_READER_LEVEL		0x20
#define LITEETH_READER_SLOT		0x24
#define LITEETH_READER_LENGTH		0x28
#define LITEETH_READER_EV_STATUS	0x2c
#define LITEETH_READER_EV_PENDING	0x30
#define LITEETH_READER_EV_ENABLE	0x34
#define LITEETH_PREAMBLE_CRC		0x38
#define LITEETH_PREAMBLE_ERRORS		0x3c
#define LITEETH_CRC_ERRORS		0x40

#define LITEETH_PHY_CRG_RESET		0x00
#define LITEETH_MDIO_W			0x04
#define  MDIO_W_CLK			BIT(0)
#define  MDIO_W_OE			BIT(1)
#define  MDIO_W_DO			BIT(2)

#define LITEETH_MDIO_R			0x08
#define  MDIO_R_DI			BIT(0)

#define LITEETH_BUFFER_SIZE		0x800
#define MAX_PKT_SIZE			LITEETH_BUFFER_SIZE

struct liteeth {
	struct device *dev;
	struct eth_device edev;
	void __iomem *base;
	void __iomem *mdio_base;
	struct mii_bus *mii_bus;
	struct mdiobb_ctrl mdiobb;

	/* Link management */
	int cur_duplex;
	int cur_speed;

	/* Tx */
	int tx_slot;
	int num_tx_slots;
	void __iomem *tx_base;

	/* Rx */
	int rx_slot;
	int num_rx_slots;
	void __iomem *rx_base;

	void *rx_buf;
};

static inline void litex_write8(void __iomem *addr, u8 val)
{
	writeb(val, addr);
}

static inline void litex_write16(void __iomem *addr, u16 val)
{
	writew(val, addr);
}

static inline u8 litex_read8(void __iomem *addr)
{
	return readb(addr);
}

static inline u32 litex_read32(void __iomem *addr)
{
	return readl(addr);
}

static void liteeth_mdio_w_modify(struct liteeth *priv, u8 clear, u8 set)
{
	void __iomem *mdio_w = priv->mdio_base + LITEETH_MDIO_W;

	litex_write8(mdio_w, (litex_read8(mdio_w) & ~clear) | set);
}

static void liteeth_mdio_ctrl(struct mdiobb_ctrl *ctrl, u8 mask, int set)
{
	struct liteeth *priv = container_of(ctrl, struct liteeth, mdiobb);

	liteeth_mdio_w_modify(priv, mask, set ? mask : 0);
}

/* MDC pin control */
static void liteeth_set_mdc(struct mdiobb_ctrl *ctrl, int level)
{
	liteeth_mdio_ctrl(ctrl, MDIO_W_CLK, level);
}

/* Data I/O pin control */
static void liteeth_set_mdio_dir(struct mdiobb_ctrl *ctrl, int output)
{
	liteeth_mdio_ctrl(ctrl, MDIO_W_OE, output);
}

/* Set data bit */
static void liteeth_set_mdio_data(struct mdiobb_ctrl *ctrl, int value)
{
	liteeth_mdio_ctrl(ctrl, MDIO_W_DO, value);
}

/* Get data bit */
static int liteeth_get_mdio_data(struct mdiobb_ctrl *ctrl)
{
	struct liteeth *priv = container_of(ctrl, struct liteeth, mdiobb);

	return (litex_read8(priv->mdio_base + LITEETH_MDIO_R) & MDIO_R_DI) != 0;
}

/* MDIO bus control struct */
static struct mdiobb_ops bb_ops = {
	.set_mdc = liteeth_set_mdc,
	.set_mdio_dir = liteeth_set_mdio_dir,
	.set_mdio_data = liteeth_set_mdio_data,
	.get_mdio_data = liteeth_get_mdio_data,
};

static int liteeth_init_dev(struct eth_device *edev)
{
	return 0;
}

static int liteeth_eth_open(struct eth_device *edev)
{
	struct liteeth *priv = edev->priv;
	int ret;

	/* Disable events */
	litex_write8(priv->base + LITEETH_WRITER_EV_ENABLE, 0);
	litex_write8(priv->base + LITEETH_READER_EV_ENABLE, 0);

	/* Clear pending events? */
	litex_write8(priv->base + LITEETH_WRITER_EV_PENDING, 1);
	litex_write8(priv->base + LITEETH_READER_EV_PENDING, 1);

	ret = phy_device_connect(edev, priv->mii_bus, -1, NULL, 0, -1);
	if (ret)
		return ret;

	return 0;
}

static int liteeth_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct liteeth *priv = edev->priv;
	void *txbuffer;
	int ret;
	u8 val;
	u8 reg;

	reg = litex_read8(priv->base + LITEETH_READER_EV_PENDING);
	if (reg) {
		litex_write8(priv->base + LITEETH_READER_EV_PENDING, reg);
	}

	/* Reject oversize packets */
	if (unlikely(packet_length > MAX_PKT_SIZE)) {
		dev_err(priv->dev, "tx packet too big\n");
		goto drop;
	}

	txbuffer = priv->tx_base + priv->tx_slot * LITEETH_BUFFER_SIZE;
	memcpy(txbuffer, packet, packet_length);
	litex_write8(priv->base + LITEETH_READER_SLOT, priv->tx_slot);
	litex_write16(priv->base + LITEETH_READER_LENGTH, packet_length);

	ret = readb_poll_timeout(priv->base + LITEETH_READER_READY,
			val, val, 1000);
	if (ret == -ETIMEDOUT) {
		dev_err(priv->dev, "LITEETH_READER_READY timed out\n");
		goto drop;
	}

	litex_write8(priv->base + LITEETH_READER_START, 1);

	priv->tx_slot = (priv->tx_slot + 1) % priv->num_tx_slots;

drop:
	return 0;
}

static int liteeth_eth_rx(struct eth_device *edev)
{
	struct liteeth *priv = edev->priv;
	u8 rx_slot;
	int len = 0;
	u8 reg;

	reg = litex_read8(priv->base + LITEETH_WRITER_EV_PENDING);
	if (!reg) {
		goto done;
	}

	len = litex_read32(priv->base + LITEETH_WRITER_LENGTH);
	if (len == 0 || len > 2048) {
		len = 0;
		dev_err(priv->dev, "%s: invalid len %d\n", __func__, len);
		litex_write8(priv->base + LITEETH_WRITER_EV_PENDING, reg);
		goto done;
	}

	rx_slot = litex_read8(priv->base + LITEETH_WRITER_SLOT);

	memcpy(priv->rx_buf, priv->rx_base + rx_slot * LITEETH_BUFFER_SIZE, len);

	net_receive(edev, priv->rx_buf, len);

	litex_write8(priv->base + LITEETH_WRITER_EV_PENDING, reg);

done:
	return len;
}

static void liteeth_eth_halt(struct eth_device *edev)
{
	struct liteeth *priv = edev->priv;

	litex_write8(priv->base + LITEETH_WRITER_EV_ENABLE, 0);
	litex_write8(priv->base + LITEETH_READER_EV_ENABLE, 0);
}

static void liteeth_reset_hw(struct liteeth *priv)
{
	/* Reset, twice */
	litex_write8(priv->base + LITEETH_PHY_CRG_RESET, 0);
	udelay(10);
	litex_write8(priv->base + LITEETH_PHY_CRG_RESET, 1);
	udelay(10);
	litex_write8(priv->base + LITEETH_PHY_CRG_RESET, 0);
	udelay(10);
}

static int liteeth_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	return 0;
}

static int liteeth_set_ethaddr(struct eth_device *edev,
					const unsigned char *mac_addr)
{
	return 0;
}

static int liteeth_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct eth_device *edev;
	void __iomem *buf_base;
	struct liteeth *priv;
	int err;

	priv = xzalloc(sizeof(struct liteeth));
	edev = &priv->edev;
	edev->priv = priv;
	priv->dev = dev;

	priv->base = dev_request_mem_region(dev, 0);
	if (IS_ERR(priv->base)) {
		err = PTR_ERR(priv->base);
		goto err;
	}

	priv->mdio_base = dev_request_mem_region(dev, 1);
	if (IS_ERR(priv->mdio_base)) {
		err = PTR_ERR(priv->mdio_base);
		goto err;
	}

	buf_base = dev_request_mem_region(dev, 2);
	if (IS_ERR(buf_base)) {
		err = PTR_ERR(buf_base);
		goto err;
	}

	err = of_property_read_u32(np, "rx-fifo-depth",
			&priv->num_rx_slots);
	if (err) {
		dev_err(dev, "unable to get rx-fifo-depth\n");
		goto err;
	}

	err = of_property_read_u32(np, "tx-fifo-depth",
			&priv->num_tx_slots);
	if (err) {
		dev_err(dev, "unable to get tx-fifo-depth\n");
		goto err;
	}

	/* Rx slots */
	priv->rx_base = buf_base;
	priv->rx_slot = 0;

	/* Tx slots come after Rx slots */
	priv->tx_base = buf_base + priv->num_rx_slots * LITEETH_BUFFER_SIZE;
	priv->tx_slot = 0;

	priv->rx_buf = xmalloc(PKTSIZE);

	edev->init = liteeth_init_dev;
	edev->open = liteeth_eth_open;
	edev->send = liteeth_eth_send;
	edev->recv = liteeth_eth_rx;
	edev->get_ethaddr = liteeth_get_ethaddr;
	edev->set_ethaddr = liteeth_set_ethaddr;
	edev->halt = liteeth_eth_halt;
	edev->parent = dev;

	priv->mdiobb.ops = &bb_ops;

	priv->mii_bus = alloc_mdio_bitbang(&priv->mdiobb);
	priv->mii_bus->parent = dev;

	liteeth_reset_hw(priv);

	err = eth_register(edev);
	if (err) {
		dev_err(dev, "failed to register edev\n");
		goto err;
	}

	err = mdiobus_register(priv->mii_bus);
	if (err) {
		dev_err(dev, "failed to register mii_bus\n");
		goto err;
	}

	dev_info(dev, DRV_NAME " driver registered\n");

	return 0;

err:
	return err;
}

static const struct of_device_id liteeth_dt_ids[] = {
	{
		.compatible = "litex,liteeth"
	}, {
	}
};
MODULE_DEVICE_TABLE(of, liteeth_dt_ids);

static struct driver liteeth_driver = {
	.name = DRV_NAME,
	.probe = liteeth_probe,
	.of_compatible = DRV_OF_COMPAT(liteeth_dt_ids),
};
device_platform_driver(liteeth_driver);

MODULE_AUTHOR("Joel Stanley <joel@jms.id.au>");
MODULE_LICENSE("GPL");
