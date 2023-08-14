/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019 Ahmad Fatoum, Pengutronix
 */

#ifndef __EQOS_H_
#define __EQOS_H_

struct eqos;
struct eth_device;

struct eqos_ops {
	int (*init)(struct device *dev, struct eqos *priv);
	int (*get_ethaddr)(struct eth_device *dev, unsigned char *mac);
	int (*set_ethaddr)(struct eth_device *edev, const unsigned char *mac);
	void (*adjust_link)(struct eth_device *edev);
	unsigned long (*get_csr_clk_rate)(struct eqos *);

	bool enh_desc;

#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT		0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK		3
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_NOT_ENABLED	0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB	2
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV	1
	unsigned clk_csr;

#define EQOS_MDIO_ADDR_CR_20_35			2
#define EQOS_MDIO_ADDR_CR_250_300		5
#define EQOS_MDIO_ADDR_SKAP			BIT(4)
#define EQOS_MDIO_ADDR_GOC_SHIFT		2
#define EQOS_MDIO_ADDR_GOC_READ			3
#define EQOS_MDIO_ADDR_GOC_WRITE		1
#define EQOS_MDIO_ADDR_C45E			BIT(1)
	unsigned config_mac;
};

struct eqos_desc;
struct eqos_dma_regs;
struct eqos_mac_regs;
struct eqos_mtl_regs;

#define EQOS_DESCRIPTORS_TX	4
#define EQOS_DESCRIPTORS_RX	64

struct eqos {
	struct eth_device netdev;
	struct mii_bus miibus;

	u8 macaddr[6];

	u32 tx_currdescnum, rx_currdescnum;

	struct eqos_desc *tx_descs, *rx_descs;
	dma_addr_t dma_rx_buf[EQOS_DESCRIPTORS_RX];

	void __iomem *regs;
	struct eqos_mac_regs __iomem *mac_regs;
	struct eqos_dma_regs __iomem *dma_regs;
	struct eqos_mtl_regs __iomem *mtl_regs;

	int phy_addr;
	phy_interface_t interface;

	const struct eqos_ops *ops;
	void *priv;

	bool is_started;
	bool promisc_enabled;
};

struct device;
int eqos_probe(struct device *dev, const struct eqos_ops *ops, void *priv);
void eqos_remove(struct device *dev);

int eqos_get_ethaddr(struct eth_device *edev, unsigned char *mac);
int eqos_set_ethaddr(struct eth_device *edev, const unsigned char *mac);
void eqos_adjust_link(struct eth_device *edev);

#define eqos_dbg(eqos, ...) dev_dbg(&eqos->netdev.dev, __VA_ARGS__)
#define eqos_warn(eqos, ...) dev_warn(&eqos->netdev.dev, __VA_ARGS__)
#define eqos_err(eqos, ...) dev_err(&eqos->netdev.dev, __VA_ARGS__)

#endif
