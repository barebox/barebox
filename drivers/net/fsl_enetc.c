// SPDX-License-Identifier: GPL-2.0+
/*
 * ENETC ethernet controller driver
 * Copyright 2017-2021 NXP
 */
#include <common.h>
#include <dma.h>
#include <net.h>
#include <linux/phy.h>
#include <linux/pci.h>
#include <io.h>
#include <linux/mdio.h>
#include <asm/system.h>
#include <of_net.h>
#include <asm/unaligned.h>

#include "fsl_enetc.h"

/* MDIO wrappers, we're using these to drive internal MDIO to get to serdes */
static __maybe_unused int enetc_mdio_read(struct enetc_priv *priv, int addr, int devad, int reg)
{
	struct enetc_mdio_priv mdio_priv;

	mdio_priv.regs_base = priv->port_regs + ENETC_PM_IMDIO_BASE;

	return enetc_mdio_read_priv(&mdio_priv, addr, devad, reg);
}

static int enetc_mdio_write(struct enetc_priv *priv, int addr, int devad, int reg,
			    u16 val)
{
	struct enetc_mdio_priv mdio_priv;
	int ret;

	mdio_priv.regs_base = priv->port_regs + ENETC_PM_IMDIO_BASE;

	ret = enetc_mdio_write_priv(&mdio_priv, addr, devad, reg, val);

	return ret;
}

/* only interfaces that can pin out through serdes have internal MDIO */
static bool enetc_has_imdio(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;

	return !!(enetc_read_port(priv, ENETC_PCAPR0) & ENETC_PCAPRO_MDIO);
}

/* set up serdes for SGMII */
static int enetc_init_sgmii(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;
	bool is2500 = false;
	u16 reg;

	if (!enetc_has_imdio(edev))
		return 0;

	if (priv->uclass_id == PHY_INTERFACE_MODE_2500BASEX)
		is2500 = true;

	/*
	 * Set to SGMII mode, for 1Gbps enable AN, for 2.5Gbps set fixed speed.
	 * Although fixed speed is 1Gbps, we could be running at 2.5Gbps based
	 * on PLL configuration.  Setting 1G for 2.5G here is counter intuitive
	 * but intentional.
	 */
	reg = ENETC_PCS_IF_MODE_SGMII;
	reg |= is2500 ? ENETC_PCS_IF_MODE_SPEED_1G : ENETC_PCS_IF_MODE_SGMII_AN;
	enetc_mdio_write(priv, ENETC_PCS_PHY_ADDR, MDIO_DEVAD_NONE,
			 ENETC_PCS_IF_MODE, reg);

	/* Dev ability - SGMII */
	enetc_mdio_write(priv, ENETC_PCS_PHY_ADDR, MDIO_DEVAD_NONE,
			 ENETC_PCS_DEV_ABILITY, ENETC_PCS_DEV_ABILITY_SGMII);

	/* Adjust link timer for SGMII */
	enetc_mdio_write(priv, ENETC_PCS_PHY_ADDR, MDIO_DEVAD_NONE,
			 ENETC_PCS_LINK_TIMER1, ENETC_PCS_LINK_TIMER1_VAL);
	enetc_mdio_write(priv, ENETC_PCS_PHY_ADDR, MDIO_DEVAD_NONE,
			 ENETC_PCS_LINK_TIMER2, ENETC_PCS_LINK_TIMER2_VAL);

	reg = ENETC_PCS_CR_DEF_VAL;
	reg |= is2500 ? ENETC_PCS_CR_RST : ENETC_PCS_CR_RESET_AN;
	/* restart PCS AN */
	enetc_mdio_write(priv, ENETC_PCS_PHY_ADDR, MDIO_DEVAD_NONE,
			 ENETC_PCS_CR, reg);

	return 0;
}

/* set up MAC for RGMII */
static void enetc_init_rgmii(struct eth_device *edev, struct phy_device *phydev)
{
	struct enetc_priv *priv = edev->priv;
	u32 old_val, val;

	old_val = val = enetc_read_port(priv, ENETC_PM_IF_MODE);

	/* disable unreliable RGMII in-band signaling and force the MAC into
	 * the speed negotiated by the PHY.
	 */
	val &= ~ENETC_PM_IF_MODE_AN_ENA;

	if (phydev->speed == SPEED_1000) {
		val &= ~ENETC_PM_IFM_SSP_MASK;
		val |= ENETC_PM_IFM_SSP_1000;
	} else if (phydev->speed == SPEED_100) {
		val &= ~ENETC_PM_IFM_SSP_MASK;
		val |= ENETC_PM_IFM_SSP_100;
	} else if (phydev->speed == SPEED_10) {
		val &= ~ENETC_PM_IFM_SSP_MASK;
		val |= ENETC_PM_IFM_SSP_10;
	}

	if (phydev->duplex == DUPLEX_FULL)
		val |= ENETC_PM_IFM_FULL_DPX;
	else
		val &= ~ENETC_PM_IFM_FULL_DPX;

	if (val == old_val)
		return;

	enetc_write_port(priv, ENETC_PM_IF_MODE, val);
}

/* set up MAC configuration for the given interface type */
static void enetc_setup_mac_iface(struct eth_device *edev,
				  struct phy_device *phydev)
{
	struct enetc_priv *priv = edev->priv;
	u32 if_mode;

	switch (priv->uclass_id) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		enetc_init_rgmii(edev, phydev);
		break;
	case PHY_INTERFACE_MODE_USXGMII:
	case PHY_INTERFACE_MODE_10GBASER:
		/* set ifmode to (US)XGMII */
		if_mode = enetc_read_port(priv, ENETC_PM_IF_MODE);
		if_mode &= ~ENETC_PM_IF_IFMODE_MASK;
		enetc_write_port(priv, ENETC_PM_IF_MODE, if_mode);
		break;
	};
}

/* set up serdes for SXGMII */
static int enetc_init_sxgmii(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;

	if (!enetc_has_imdio(edev))
		return 0;

	/* Dev ability - SXGMII */
	enetc_mdio_write(priv, ENETC_PCS_PHY_ADDR, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_DEV_ABILITY, ENETC_PCS_DEV_ABILITY_SXGMII);

	/* Restart PCS AN */
	enetc_mdio_write(priv, ENETC_PCS_PHY_ADDR, ENETC_PCS_DEVAD_REPL,
			 ENETC_PCS_CR,
			 ENETC_PCS_CR_RST | ENETC_PCS_CR_RESET_AN);

	return 0;
}

/* Apply protocol specific configuration to MAC, serdes as needed */
static void enetc_start_pcs(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;

	priv->uclass_id = of_get_phy_mode(priv->dev->of_node);
	if (priv->uclass_id == PHY_INTERFACE_MODE_NA) {
		dev_dbg(&edev->dev,
			  "phy-mode property not found, defaulting to SGMII\n");
		priv->uclass_id = PHY_INTERFACE_MODE_SGMII;
	}

	switch (priv->uclass_id) {
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
		enetc_init_sgmii(edev);
		break;
	case PHY_INTERFACE_MODE_USXGMII:
	case PHY_INTERFACE_MODE_10GBASER:
		enetc_init_sxgmii(edev);
		break;
	};
}

/*
 * LS1028A is the only part with IERB at this time and there are plans to
 * change its structure, keep this LS1028A specific for now.
 */
#define LS1028A_IERB_BASE		0x1f0800000ULL
#define LS1028A_IERB_PSIPMAR0(pf, vf)	(LS1028A_IERB_BASE + 0x8000 \
					 + (pf) * 0x100 + (vf) * 8)
#define LS1028A_IERB_PSIPMAR1(pf, vf)	(LS1028A_IERB_PSIPMAR0(pf, vf) + 4)

static int enetc_get_hwaddr(struct eth_device *edev, unsigned char *mac)
{
        return -EOPNOTSUPP;
}

static int enetc_ls1028a_write_hwaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct enetc_priv *priv = edev->priv;
	struct pci_dev *pdev = to_pci_dev(priv->dev);
	const int devfn_to_pf[] = {0, 1, 2, -1, -1, -1, 3};
	int devfn = PCI_FUNC(pdev->devfn);
	u32 lower, upper;
	int pf;

	if (devfn >= ARRAY_SIZE(devfn_to_pf))
		return 0;

	pf = devfn_to_pf[devfn];
	if (pf < 0)
		return 0;

	lower = get_unaligned_le16(mac + 4);
	upper = get_unaligned_le32(mac);

	out_le32(LS1028A_IERB_PSIPMAR0(pf, 0), upper);
	out_le32(LS1028A_IERB_PSIPMAR1(pf, 0), lower);

	return 0;
}

static int enetc_write_hwaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct enetc_priv *priv = edev->priv;

	u16 lower = get_unaligned_le16(mac + 4);
	u32 upper = get_unaligned_le32(mac);

	enetc_write_port(priv, ENETC_PSIPMAR0, upper);
	enetc_write_port(priv, ENETC_PSIPMAR1, lower);

	return 0;
}

/* Configure port parameters (# of rings, frame size, enable port) */
static void enetc_enable_si_port(struct enetc_priv *priv)
{
	u32 val;

	/* set Rx/Tx BDR count */
	val = ENETC_PSICFGR_SET_TXBDR(ENETC_TX_BDR_CNT);
	val |= ENETC_PSICFGR_SET_RXBDR(ENETC_RX_BDR_CNT);
	enetc_write_port(priv, ENETC_PSICFGR(0), val);
	/* set Rx max frame size */
	enetc_write_port(priv, ENETC_PM_MAXFRM, ENETC_RX_MAXFRM_SIZE);
	/* enable MAC port */
	enetc_write_port(priv, ENETC_PM_CC, ENETC_PM_CC_RX_TX_EN);
	/* enable port */
	enetc_write_port(priv, ENETC_PMR, ENETC_PMR_SI0_EN);
	/* set SI cache policy */
	enetc_write(priv, ENETC_SICAR0,
		    ENETC_SICAR_RD_CFG | ENETC_SICAR_WR_CFG);
	/* enable SI */
	enetc_write(priv, ENETC_SIMR, ENETC_SIMR_EN);
}

/* returns DMA address for a given buffer index */
static inline dma_addr_t enetc_rxb_address(struct enetc_priv *priv, int i)
{
	return priv->rx_pkg_phys[i];
}

/*
 * Setup a single Tx BD Ring (ID = 0):
 * - set Tx buffer descriptor address
 * - set the BD count
 * - initialize the producer and consumer index
 */
static void enetc_setup_tx_bdr(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;
	struct bd_ring *tx_bdr = &priv->tx_bdr;
	u64 tx_bd_add = (u64)priv->enetc_txbd_phys;

	/* used later to advance to the next Tx BD */
	tx_bdr->bd_count = ENETC_BD_CNT;
	tx_bdr->next_prod_idx = 0;
	tx_bdr->next_cons_idx = 0;
	tx_bdr->cons_idx = priv->regs_base +
				ENETC_BDR(TX, ENETC_TX_BDR_ID, ENETC_TBCIR);
	tx_bdr->prod_idx = priv->regs_base +
				ENETC_BDR(TX, ENETC_TX_BDR_ID, ENETC_TBPIR);

	/* set Tx BD address */
	enetc_bdr_write(priv, TX, ENETC_TX_BDR_ID, ENETC_TBBAR0,
			lower_32_bits(tx_bd_add));
	enetc_bdr_write(priv, TX, ENETC_TX_BDR_ID, ENETC_TBBAR1,
			upper_32_bits(tx_bd_add));
	/* set Tx 8 BD count */
	enetc_bdr_write(priv, TX, ENETC_TX_BDR_ID, ENETC_TBLENR,
			tx_bdr->bd_count);

	/* reset both producer/consumer indexes */
	enetc_write_reg(tx_bdr->cons_idx, tx_bdr->next_cons_idx);
	enetc_write_reg(tx_bdr->prod_idx, tx_bdr->next_prod_idx);

	/* enable TX ring */
	enetc_bdr_write(priv, TX, ENETC_TX_BDR_ID, ENETC_TBMR, ENETC_TBMR_EN);
}

/*
 * Setup a single Rx BD Ring (ID = 0):
 * - set Rx buffer descriptors address (one descriptor per buffer)
 * - set buffer size as max frame size
 * - enable Rx ring
 * - reset consumer and producer indexes
 * - set buffer for each descriptor
 */
static void enetc_setup_rx_bdr(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;
	struct bd_ring *rx_bdr = &priv->rx_bdr;
	u64 rx_bd_add = (u64)priv->enetc_rxbd_phys;
	int i;

	/* used later to advance to the next BD produced by ENETC HW */
	rx_bdr->bd_count = ENETC_BD_CNT;
	rx_bdr->next_prod_idx = 0;
	rx_bdr->next_cons_idx = 0;
	rx_bdr->cons_idx = priv->regs_base +
				ENETC_BDR(RX, ENETC_RX_BDR_ID, ENETC_RBCIR);
	rx_bdr->prod_idx = priv->regs_base +
				ENETC_BDR(RX, ENETC_RX_BDR_ID, ENETC_RBPIR);

	/* set Rx BD address */
	enetc_bdr_write(priv, RX, ENETC_RX_BDR_ID, ENETC_RBBAR0,
			lower_32_bits(rx_bd_add));
	enetc_bdr_write(priv, RX, ENETC_RX_BDR_ID, ENETC_RBBAR1,
			upper_32_bits(rx_bd_add));
	/* set Rx BD count (multiple of 8) */
	enetc_bdr_write(priv, RX, ENETC_RX_BDR_ID, ENETC_RBLENR,
			rx_bdr->bd_count);
	/* set Rx buffer  size */
	enetc_bdr_write(priv, RX, ENETC_RX_BDR_ID, ENETC_RBBSR, PKTSIZE);

	/* fill Rx BD */
	memset_io(priv->enetc_rxbd, 0,
	       rx_bdr->bd_count * sizeof(union enetc_rx_bd));

	for (i = 0; i < rx_bdr->bd_count; i++) {
		priv->rx_pkg[i] = dma_alloc(PKTSIZE);
		priv->rx_pkg_phys[i] = dma_map_single(priv->dev, priv->rx_pkg[i],
						      PKTSIZE, DMA_FROM_DEVICE);
		priv->enetc_rxbd[i].w.addr = priv->rx_pkg_phys[i];
	}

	/* reset producer (ENETC owned) and consumer (SW owned) index */
	enetc_write_reg(rx_bdr->cons_idx, rx_bdr->next_cons_idx);
	enetc_write_reg(rx_bdr->prod_idx, rx_bdr->next_prod_idx);

	/* enable Rx ring */
	enetc_bdr_write(priv, RX, ENETC_RX_BDR_ID, ENETC_RBMR, ENETC_RBMR_EN);
}

/*
 * Start ENETC interface:
 * - perform FLR
 * - enable access to port and SI registers
 * - set mac address
 * - setup TX/RX buffer descriptors
 * - enable Tx/Rx rings
 */
static int enetc_start(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;
	struct pci_dev *pdev = to_pci_dev(priv->dev);
	u32 t;
	int ret, interface;

	/* reset and enable the PCI device */
	pci_flr(pdev);

	pci_read_config_dword(pdev, PCI_COMMAND, &t);
	pci_write_config_dword(pdev, PCI_COMMAND, t | PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);

	interface = of_get_phy_mode(priv->dev->of_node);

	ret = phy_device_connect(edev, NULL, 0, NULL, 0, interface);
	if (ret)
		return ret;

	enetc_enable_si_port(priv);

	/* setup Tx/Rx buffer descriptors */
	enetc_setup_tx_bdr(edev);
	enetc_setup_rx_bdr(edev);

	enetc_setup_mac_iface(edev, priv->phy);

	return 0;
}

/*
 * Stop the network interface:
 * - just quiesce it, we can wipe all configuration as _start starts from
 * scratch each time
 */
static void enetc_stop(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;
	struct pci_dev *pdev = to_pci_dev(priv->dev);
	u32 t;

	/* FLR is sufficient to quiesce the device */
	pci_flr(pdev);

	/* leave the BARs accessible after we stop, this is needed to use
	 * internal MDIO in command line.
	 */
	pci_read_config_dword(pdev, PCI_COMMAND, &t);
	pci_write_config_dword(pdev, PCI_COMMAND, t | PCI_COMMAND_MEMORY);
}

/*
 * ENETC transmit packet:
 * - check if Tx BD ring is full
 * - set buffer/packet address (dma address)
 * - set final fragment flag
 * - try while producer index equals consumer index or timeout
 */
static int enetc_send(struct eth_device *edev, void *packet, int length)
{
	struct enetc_priv *priv = edev->priv;
	struct bd_ring *txr = &priv->tx_bdr;
	int ret;
	u32 pi, ci;
	dma_addr_t dma;
	u64 start;

	pi = txr->next_prod_idx;
	ci = enetc_read_reg(txr->cons_idx) & ENETC_BDR_IDX_MASK;
	/* Tx ring is full when */
	if (((pi + 1) % txr->bd_count) == ci) {
		dev_err(&edev->dev, "Tx BDR full\n");
		return -ETIMEDOUT;
	}

	dev_vdbg(&edev->dev, "TxBD[%d]send: pkt_len=%d, buff @0x%x%08x\n", pi, length,
		  upper_32_bits((u64)packet), lower_32_bits((u64)packet));

	dma = dma_map_single(priv->dev, packet, length, DMA_TO_DEVICE);

	/* prepare Tx BD */
	writeq(dma, &priv->enetc_txbd[pi].addr);
	writew(length, &priv->enetc_txbd[pi].buf_len);
	writew(length, &priv->enetc_txbd[pi].frm_len);
	writew(ENETC_TXBD_FLAGS_F, &priv->enetc_txbd[pi].flags);

	/* send frame: increment producer index */
	pi = (pi + 1) % txr->bd_count;
	txr->next_prod_idx = pi;
	enetc_write_reg(txr->prod_idx, pi);

	start = get_time_ns();

	while (1) {
		if (is_timeout(start, 100 * USECOND)) {
			ret = -ETIMEDOUT;
			break;
		}

		if (pi == (enetc_read_reg(txr->cons_idx) & ENETC_BDR_IDX_MASK)) {
			ret = 0;
			break;
		}
	}

	dma_unmap_single(priv->dev, dma, length, DMA_TO_DEVICE);

	return ret;
}

/*
 * Receive frame:
 * - wait for the next BD to get ready bit set
 * - clean up the descriptor
 * - move on and indicate to HW that the cleaned BD is available for Rx
 */
static int enetc_recv(struct eth_device *edev)
{
	struct enetc_priv *priv = edev->priv;
	struct bd_ring *rxr = &priv->rx_bdr;
	int pi = rxr->next_prod_idx;
	int ci = rxr->next_cons_idx;
	u32 status;
	void *pkg;
	int len;

	status = le32_to_cpu(priv->enetc_rxbd[pi].r.lstatus);

	/* check if current BD is ready to be consumed */
	if (!ENETC_RXBD_STATUS_R(status))
		return 0;

	len = readw(&priv->enetc_rxbd[pi].r.buf_len);

	dev_dbg(&edev->dev, "RxBD[%d]: len=%d err=%d pkt=0x%p\n", pi, len,
		  ENETC_RXBD_STATUS_ERRORS(status), pkg);

	dma_sync_single_for_cpu(priv->dev, priv->rx_pkg_phys[pi], PKTSIZE, DMA_FROM_DEVICE);
	net_receive(edev, priv->rx_pkg[pi], len);
	dma_sync_single_for_device(priv->dev, priv->rx_pkg_phys[pi], PKTSIZE, DMA_FROM_DEVICE);

	/* BD clean up and advance to next in ring */
	memset_io(&priv->enetc_rxbd[pi], 0, sizeof(union enetc_rx_bd));
	writeq(priv->rx_pkg_phys[pi], &priv->enetc_rxbd[pi].w.addr);
	rxr->next_prod_idx = (pi + 1) % rxr->bd_count;
	ci = (ci + 1) % rxr->bd_count;
	rxr->next_cons_idx = ci;
	dmb();
	/* free up the slot in the ring for HW */
	enetc_write_reg(rxr->cons_idx, ci);

	return 0;
}

static int enetc_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device *dev = &pdev->dev;
	struct enetc_priv *priv;
	struct eth_device *edev;

	pci_enable_device(pdev);
	pci_set_master(pdev);

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;

	priv->enetc_txbd = dma_alloc_coherent(sizeof(struct enetc_tx_bd) * ENETC_BD_CNT,
					      &priv->enetc_txbd_phys);
	priv->enetc_rxbd = dma_alloc_coherent(sizeof(union enetc_rx_bd) * ENETC_BD_CNT,
					      &priv->enetc_rxbd_phys);

	if (!priv->enetc_txbd || !priv->enetc_rxbd)
		return -ENOMEM;

	/* initialize register */
	priv->regs_base = pci_iomap(pdev, 0);
	if (!priv->regs_base) {
		dev_err(dev, "failed to map BAR0\n");
		return -EINVAL;
	}

	edev = &priv->edev;
	dev->priv = priv;
	edev->priv = priv;
	edev->open = enetc_start;
	edev->send = enetc_send;
	edev->recv = enetc_recv;
	edev->halt = enetc_stop;
	edev->get_ethaddr = enetc_get_hwaddr;

	if (of_machine_is_compatible("fsl,ls1028a"))
		edev->set_ethaddr = enetc_ls1028a_write_hwaddr;
	else
		edev->set_ethaddr = enetc_write_hwaddr;

	edev->parent = dev;

	priv->port_regs = priv->regs_base + ENETC_PORT_REGS_OFF;

	enetc_start_pcs(&priv->edev);

	return eth_register(edev);
}

static void enetc_remove(struct pci_dev *pdev)
{
	struct enetc_priv *priv = pdev->dev.priv;

	enetc_stop(&priv->edev);
}

static DEFINE_PCI_DEVICE_TABLE(enetc_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_ENETC_ETH) },
        { },
};

static struct pci_driver enetc_eth_driver = {
        .name = "fsl_enetc",
        .id_table = enetc_pci_tbl,
        .probe = enetc_probe,
        .remove = enetc_remove,
};
device_pci_driver(enetc_eth_driver);
