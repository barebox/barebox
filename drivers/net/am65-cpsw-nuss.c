// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments K3 AM65 Ethernet Switch SubSystem Driver
 *
 * Copyright (C) 2019, Texas Instruments, Incorporated
 *
 */
#include <driver.h>
#include <net.h>
#include <soc/ti/ti-udma.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <dma-devices.h>
#include <of_net.h>
#include <stdio.h>

#define AM65_CPSW_CPSWNU_MAX_PORTS 9

#define AM65_CPSW_SS_BASE		0x0
#define AM65_CPSW_SGMII_BASE	0x100
#define AM65_CPSW_MDIO_BASE	0xf00
#define AM65_CPSW_XGMII_BASE	0x2100
#define AM65_CPSW_CPSW_NU_BASE	0x20000
#define AM65_CPSW_CPSW_NU_ALE_BASE 0x1e000

#define AM65_CPSW_CPSW_NU_PORTS_OFFSET	0x1000
#define AM65_CPSW_CPSW_NU_PORT_MACSL_OFFSET	0x330

#define AM65_CPSW_MDIO_BUS_FREQ_DEF 1000000

#define AM65_CPSW_CTL_REG			0x4
#define AM65_CPSW_STAT_PORT_EN_REG	0x14
#define AM65_CPSW_PTYPE_REG		0x18

#define AM65_CPSW_CTL_REG_P0_ENABLE			BIT(2)
#define AM65_CPSW_CTL_REG_P0_TX_CRC_REMOVE		BIT(13)
#define AM65_CPSW_CTL_REG_P0_RX_PAD			BIT(14)

#define AM65_CPSW_P0_FLOW_ID_REG			0x8
#define AM65_CPSW_PN_RX_MAXLEN_REG		0x24
#define AM65_CPSW_PN_REG_SA_L			0x308
#define AM65_CPSW_PN_REG_SA_H			0x30c

#define AM65_CPSW_SGMII_CONTROL_REG             0x010
#define AM65_CPSW_SGMII_MR_ADV_ABILITY_REG      0x018
#define AM65_CPSW_SGMII_CONTROL_MR_AN_ENABLE    BIT(0)

#define ADVERTISE_SGMII				0x1

#define AM65_CPSW_ALE_CTL_REG			0x8
#define AM65_CPSW_ALE_CTL_REG_ENABLE		BIT(31)
#define AM65_CPSW_ALE_CTL_REG_RESET_TBL		BIT(30)
#define AM65_CPSW_ALE_CTL_REG_BYPASS		BIT(4)
#define AM65_CPSW_ALE_PN_CTL_REG(x)		(0x40 + (x) * 4)
#define AM65_CPSW_ALE_PN_CTL_REG_MODE_FORWARD	0x3
#define AM65_CPSW_ALE_PN_CTL_REG_MAC_ONLY	BIT(11)

#define AM65_CPSW_ALE_THREADMAPDEF_REG		0x134
#define AM65_CPSW_ALE_DEFTHREAD_EN		BIT(15)

#define AM65_CPSW_MACSL_CTL_REG			0x0
#define AM65_CPSW_MACSL_CTL_REG_IFCTL_A		BIT(15)
#define AM65_CPSW_MACSL_CTL_EXT_EN		BIT(18)
#define AM65_CPSW_MACSL_CTL_REG_GIG		BIT(7)
#define AM65_CPSW_MACSL_CTL_REG_GMII_EN		BIT(5)
#define AM65_CPSW_MACSL_CTL_REG_LOOPBACK	BIT(1)
#define AM65_CPSW_MACSL_CTL_REG_FULL_DUPLEX	BIT(0)
#define AM65_CPSW_MACSL_RESET_REG		0x8
#define AM65_CPSW_MACSL_RESET_REG_RESET		BIT(0)
#define AM65_CPSW_MACSL_STATUS_REG		0x4
#define AM65_CPSW_MACSL_RESET_REG_PN_IDLE	BIT(31)
#define AM65_CPSW_MACSL_RESET_REG_PN_E_IDLE	BIT(30)
#define AM65_CPSW_MACSL_RESET_REG_PN_P_IDLE	BIT(29)
#define AM65_CPSW_MACSL_RESET_REG_PN_TX_IDLE	BIT(28)
#define AM65_CPSW_MACSL_RESET_REG_IDLE_MASK \
	(AM65_CPSW_MACSL_RESET_REG_PN_IDLE | \
	 AM65_CPSW_MACSL_RESET_REG_PN_E_IDLE | \
	 AM65_CPSW_MACSL_RESET_REG_PN_P_IDLE | \
	 AM65_CPSW_MACSL_RESET_REG_PN_TX_IDLE)

#define AM65_CPSW_CPPI_PKT_TYPE			0x7

#define DEFAULT_GPIO_RESET_DELAY		10

#define UDMA_RX_DESC_NUM PKTBUFSRX

struct am65_cpsw_port {
	void __iomem		*port_base;
	void __iomem		*port_sgmii_base;
	void __iomem		*macsl_base;
	bool			enabled;
	u32			mac_control;
	u32			port_id;
	struct device_node	*device_node;
	struct eth_device	edev;
	struct device		dev;
	struct am65_cpsw_common	*cpsw_common;
	struct phy_device	*phydev;
	phy_interface_t		interface;
};

struct am65_cpsw_common {
	struct device		*dev;
	void __iomem		*ss_base;
	void __iomem		*cpsw_base;
	void __iomem		*ale_base;

	struct clk		*fclk;
	struct pm_domain	*pwrdmn;

	void			*rx_buffer[UDMA_RX_DESC_NUM];

	u32			port_num;
	struct am65_cpsw_port	ports[AM65_CPSW_CPSWNU_MAX_PORTS];

	u32			bus_freq;

	struct dma		*dma_tx;
	struct dma		*dma_rx;
	int			started;
};

static int am65_cpsw_macsl_reset(struct am65_cpsw_port *slave)
{
	u32 i = 100;

	/* Set the soft reset bit */
	writel(AM65_CPSW_MACSL_RESET_REG_RESET,
	       slave->macsl_base + AM65_CPSW_MACSL_RESET_REG);

	while ((readl(slave->macsl_base + AM65_CPSW_MACSL_RESET_REG) &
		AM65_CPSW_MACSL_RESET_REG_RESET) && i--);

	/* Timeout on the reset */
	return i;
}

static int am65_cpsw_macsl_wait_for_idle(struct am65_cpsw_port *slave)
{
	u32 i = 100;

	while ((readl(slave->macsl_base + AM65_CPSW_MACSL_STATUS_REG) &
		AM65_CPSW_MACSL_RESET_REG_IDLE_MASK) && i--);

	return i;
}

static struct am65_cpsw_port *edev_to_port(struct eth_device *edev)
{
	return container_of(edev, struct am65_cpsw_port, edev);
}

static void am65_cpsw_update_link(struct eth_device *edev)
{
	struct am65_cpsw_port *port = edev_to_port(edev);
	struct phy_device *phy = port->edev.phydev;

	u32 mac_control = 0;

	if (phy->interface == PHY_INTERFACE_MODE_SGMII) {
		writel(ADVERTISE_SGMII,
		       port->port_sgmii_base + AM65_CPSW_SGMII_MR_ADV_ABILITY_REG);
		writel(AM65_CPSW_SGMII_CONTROL_MR_AN_ENABLE,
		       port->port_sgmii_base + AM65_CPSW_SGMII_CONTROL_REG);
	}

	if (phy->link) { /* link up */
		mac_control = /*AM65_CPSW_MACSL_CTL_REG_LOOPBACK |*/
			      AM65_CPSW_MACSL_CTL_REG_GMII_EN;
		if (phy->speed == 1000)
			mac_control |= AM65_CPSW_MACSL_CTL_REG_GIG;
		if (phy->speed == 10 && phy_interface_is_rgmii(phy))
			/* Can be used with in band mode only */
			mac_control |= AM65_CPSW_MACSL_CTL_EXT_EN;
		if (phy->duplex == DUPLEX_FULL)
			mac_control |= AM65_CPSW_MACSL_CTL_REG_FULL_DUPLEX;
		if (phy->speed == 100)
			mac_control |= AM65_CPSW_MACSL_CTL_REG_IFCTL_A;
		if (phy->interface == PHY_INTERFACE_MODE_SGMII)
			mac_control |= AM65_CPSW_MACSL_CTL_EXT_EN;
	}

	if (mac_control == port->mac_control)
		return;

	writel(mac_control, port->macsl_base + AM65_CPSW_MACSL_CTL_REG);
	port->mac_control = mac_control;
}

#define AM65_GMII_SEL_PORT_OFFS(x)	(0x4 * ((x) - 1))

#define AM65_GMII_SEL_MODE_MII		0
#define AM65_GMII_SEL_MODE_RMII		1
#define AM65_GMII_SEL_MODE_RGMII	2
#define AM65_GMII_SEL_MODE_SGMII	3

#define AM65_GMII_SEL_RGMII_IDMODE	BIT(4)

static int am65_cpsw_gmii_sel_k3(struct am65_cpsw_port *port,
				 phy_interface_t phy_mode)
{
	struct device *dev = &port->dev;
	u32 offset, reg;
	bool rgmii_id = false;
	struct regmap *gmii_sel;
	u32 mode = 0;
	int ret;

	gmii_sel = syscon_regmap_lookup_by_phandle(port->device_node, "phys");
	if (IS_ERR(gmii_sel))
		return PTR_ERR(gmii_sel);

	ret = of_property_read_u32_index(port->device_node, "phys", 1, &offset);
	if (ret)
		return ret;

	regmap_read(gmii_sel, AM65_GMII_SEL_PORT_OFFS(offset), &reg);

	dev_dbg(dev, "old gmii_sel: %08x\n", reg);

	switch (phy_mode) {
	case PHY_INTERFACE_MODE_RMII:
		mode = AM65_GMII_SEL_MODE_RMII;
		break;

	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_RXID:
		mode = AM65_GMII_SEL_MODE_RGMII;
		break;

	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		mode = AM65_GMII_SEL_MODE_RGMII;
		rgmii_id = true;
		break;

	case PHY_INTERFACE_MODE_SGMII:
		mode = AM65_GMII_SEL_MODE_SGMII;
		break;

	default:
		dev_warn(dev,
			 "Unsupported PHY mode: %u. Defaulting to MII.\n",
			 phy_mode);
		fallthrough;
	case PHY_INTERFACE_MODE_MII:
		mode = AM65_GMII_SEL_MODE_MII;
		break;
	};

	if (rgmii_id)
		mode |= AM65_GMII_SEL_RGMII_IDMODE;

	reg = mode;
	dev_dbg(dev, "gmii_sel PHY mode: %u, new gmii_sel: %08x\n",
		phy_mode, reg);
	regmap_write(gmii_sel, AM65_GMII_SEL_PORT_OFFS(offset), reg);

	regmap_read(gmii_sel, AM65_GMII_SEL_PORT_OFFS(offset), &reg);
	if (reg != mode) {
		dev_err(dev,
			"gmii_sel PHY mode NOT SET!: requested: %08x, gmii_sel: %08x\n",
			mode, reg);
		return 0;
	}

	return 0;
}

static int am65_cpsw_common_start(struct am65_cpsw_common *common)
{
	struct ti_udma_drv_chan_cfg_data *dma_rx_cfg_data;
	struct am65_cpsw_port *port0 = &common->ports[0];
	int ret, i;

	if (common->started) {
		common->started++;
		return 0;
	}

	ret = clk_enable(common->fclk);
	if (ret) {
		dev_err(common->dev, "clk enabled failed %d\n", ret);
		goto err_off_pwrdm;
	}

	common->dma_tx = dma_get_by_name(common->dev, "tx0");
	if (IS_ERR(common->dma_tx)) {
		ret = PTR_ERR(common->dma_tx);
		dev_err(common->dev, "TX dma get failed: %pe\n", common->dma_tx);
		goto err_off_clk;
	}

	common->dma_rx = dma_get_by_name(common->dev, "rx");
	if (IS_ERR(common->dma_rx)) {
		ret = PTR_ERR(common->dma_rx);
		dev_err(common->dev, "RX dma get failed: %pe\n", common->dma_rx);
		goto err_free_tx;
	}

	ret = net_alloc_packets(common->rx_buffer, UDMA_RX_DESC_NUM);
	if (ret)
		return ret;

	for (i = 0; i < UDMA_RX_DESC_NUM; i++) {
		dma_addr_t dma;

		dma = dma_map_single(common->dev, common->rx_buffer[i], PKTSIZE,
				     DMA_FROM_DEVICE);
		if (dma_mapping_error(common->dev, dma))
			return -EFAULT;

		ret = dma_prepare_rcv_buf(common->dma_rx,
					  dma,
					  PKTSIZE);
		if (ret) {
			dev_err(common->dev, "RX dma add buf failed %d\n", ret);
			goto err_free_rx;
		}
	}

	ret = dma_enable(common->dma_tx);
	if (ret) {
		dev_err(common->dev, "TX dma_enable failed %d\n", ret);
		goto err_free_rx;
	}

	ret = dma_enable(common->dma_rx);
	if (ret) {
		dev_err(common->dev, "RX dma_enable failed %d\n", ret);
		goto err_dis_tx;
	}

	/* Control register */
	writel(AM65_CPSW_CTL_REG_P0_ENABLE |
	       AM65_CPSW_CTL_REG_P0_TX_CRC_REMOVE |
	       AM65_CPSW_CTL_REG_P0_RX_PAD,
	       common->cpsw_base + AM65_CPSW_CTL_REG);

	/* disable priority elevation */
	writel(0, common->cpsw_base + AM65_CPSW_PTYPE_REG);

	/* Port 0  length register */
	writel(PKTSIZE, port0->port_base + AM65_CPSW_PN_RX_MAXLEN_REG);

	/* set base flow_id */
	dma_get_cfg(common->dma_rx, 0, (void **)&dma_rx_cfg_data);
	writel(dma_rx_cfg_data->flow_id_base,
	       port0->port_base + AM65_CPSW_P0_FLOW_ID_REG);
	dev_dbg(common->dev, "K3 CPSW: rflow_id_base: %u\n",
		 dma_rx_cfg_data->flow_id_base);

	/* Reset and enable the ALE */
	writel(AM65_CPSW_ALE_CTL_REG_ENABLE | AM65_CPSW_ALE_CTL_REG_RESET_TBL |
	       AM65_CPSW_ALE_CTL_REG_BYPASS,
	       common->ale_base + AM65_CPSW_ALE_CTL_REG);

	/* port 0 put into forward mode */
	writel(AM65_CPSW_ALE_PN_CTL_REG_MODE_FORWARD,
	       common->ale_base + AM65_CPSW_ALE_PN_CTL_REG(0));

	writel(AM65_CPSW_ALE_DEFTHREAD_EN,
	       common->ale_base + AM65_CPSW_ALE_THREADMAPDEF_REG);

	common->started++;

	return 0;

err_dis_tx:
	dma_disable(common->dma_tx);
err_free_rx:
	dma_free(common->dma_rx);
err_free_tx:
	dma_free(common->dma_tx);
err_off_clk:
	clk_disable(common->fclk);
err_off_pwrdm:
	dev_err(common->dev, "%s end error\n", __func__);

	return ret;
}

static int am65_cpsw_start(struct eth_device *edev)
{
	struct am65_cpsw_port *port = edev_to_port(edev);
	struct device *dev = &port->dev;
	struct am65_cpsw_common	*common = port->cpsw_common;
	int ret;

	ret = am65_cpsw_common_start(common);
	if (ret)
		return ret;

	ret = phy_device_connect(edev, NULL, -1,
				 am65_cpsw_update_link, 0, port->interface);
	if (ret)
		return ret;

	/* enable statistics */
	writel(BIT(0) | BIT(port->port_id),
	       common->cpsw_base + AM65_CPSW_STAT_PORT_EN_REG);

	/* Port x Max length register */
	writel(PKTSIZE, port->port_base + AM65_CPSW_PN_RX_MAXLEN_REG);

	/* Port x ALE: mac_only, Forwarding */
	writel(AM65_CPSW_ALE_PN_CTL_REG_MAC_ONLY |
	       AM65_CPSW_ALE_PN_CTL_REG_MODE_FORWARD,
	       common->ale_base + AM65_CPSW_ALE_PN_CTL_REG(port->port_id));

	port->mac_control = 0;
	if (!am65_cpsw_macsl_reset(port)) {
		dev_err(dev, "mac_sl reset failed\n");
		ret = -EFAULT;
		goto err;
	}

	return 0;
err:
	writel(0, port->macsl_base + AM65_CPSW_MACSL_CTL_REG);
	/* disable ports */
	writel(0, common->ale_base + AM65_CPSW_ALE_PN_CTL_REG(port->port_id));
	if (!am65_cpsw_macsl_wait_for_idle(port))
		dev_err(common->dev, "mac_sl idle timeout\n");

	return ret;
}

static int am65_cpsw_send(struct eth_device *edev, void *packet, int length)
{
	struct am65_cpsw_port *port = edev_to_port(edev);
	struct am65_cpsw_common	*common = port->cpsw_common;
	struct ti_udma_drv_packet_data packet_data;
	dma_addr_t dma;
	int ret;

	if (!common->started)
		return -ENETDOWN;

	packet_data.pkt_type = AM65_CPSW_CPPI_PKT_TYPE;
	packet_data.dest_tag = port->port_id;

	dma = dma_map_single(common->dev, packet, length, DMA_TO_DEVICE);
	if (dma_mapping_error(common->dev, dma))
		return -EFAULT;

	ret = dma_send(common->dma_tx, dma, length, &packet_data);
	if (ret) {
		dev_err(&port->dev, "TX dma_send failed %d\n", ret);
		return ret;
	}

	dma_unmap_single(common->dev, dma, length, DMA_TO_DEVICE);

	return 0;
}

static void am65_cpsw_recv(struct eth_device *edev)
{
	struct am65_cpsw_port *port = edev_to_port(edev);
	struct am65_cpsw_common	*common = port->cpsw_common;
	struct ti_udma_drv_packet_data packet_data;
	dma_addr_t pkt;
	int ret;
	u32 port_id;

	if (!common->started)
		return;

	ret = dma_receive(common->dma_rx, &pkt, &packet_data);
	if (!ret)
		return;
	if (ret < 0) {
		dev_err(common->dev, "dma failed with %d\n", ret);
		return;
	}

	dma_sync_single_for_cpu(common->dev, pkt, ret, DMA_FROM_DEVICE);

	/*
	 * We have multiple ports (with an ethernet device per port), pick
	 * the right ethernet devices based on the incoming tag id.
	 */

	port_id = packet_data.src_tag;
	if (port_id >= AM65_CPSW_CPSWNU_MAX_PORTS) {
		dev_err(common->dev, "received pkt on invalid port_id %d\n", port_id);
		goto out;
	}

	port = &common->ports[port_id];
	if (!port->enabled) {
		dev_err(common->dev, "received pkt on disabled port_id %d\n", port_id);
		goto out;
	}

	net_receive(&port->edev, (void *)pkt, ret);
out:
	dma_sync_single_for_device(common->dev, pkt, ret, DMA_FROM_DEVICE);

	ret = dma_prepare_rcv_buf(common->dma_rx, pkt, PKTSIZE);
	if (ret)
		dev_err(common->dev, "RX dma free_pkt failed %d\n", ret);
}

static void am65_cpsw_common_stop(struct am65_cpsw_common *common)
{
	common->started--;

	if (common->started)
		return;

	writel(0, common->ale_base + AM65_CPSW_ALE_PN_CTL_REG(0));
	writel(0, common->ale_base + AM65_CPSW_ALE_CTL_REG);
	writel(0, common->cpsw_base + AM65_CPSW_CTL_REG);

	dma_disable(common->dma_tx);
	dma_release(common->dma_tx);

	dma_disable(common->dma_rx);
	dma_release(common->dma_rx);
}

static void am65_cpsw_stop(struct eth_device *edev)
{
	struct am65_cpsw_port *port = edev_to_port(edev);
	struct am65_cpsw_common *common = port->cpsw_common;

	if (!am65_cpsw_macsl_wait_for_idle(port))
		dev_err(&port->dev, "mac_sl idle timeout\n");

	writel(0, common->ale_base + AM65_CPSW_ALE_PN_CTL_REG(port->port_id));
	writel(0, port->macsl_base + AM65_CPSW_MACSL_CTL_REG);

	am65_cpsw_common_stop(common);
}

static int am65_cpsw_am654_get_efuse_macid(struct am65_cpsw_port *port,
					   u8 *mac_addr)
{
	u32 mac_lo, mac_hi, offset;
	struct regmap *syscon;
	int ret;

	syscon = syscon_regmap_lookup_by_phandle(port->device_node, "ti,syscon-efuse");
	if (IS_ERR(syscon)) {
		if (PTR_ERR(syscon) == -ENODEV)
			return 0;
		return PTR_ERR(syscon);
	}

	ret = of_property_read_u32_index(port->device_node, "ti,syscon-efuse", 1, &offset);
	if (ret)
		return ret;

	regmap_read(syscon, offset, &mac_lo);
	regmap_read(syscon, offset + 4, &mac_hi);

	mac_addr[0] = (mac_hi >> 8) & 0xff;
	mac_addr[1] = mac_hi & 0xff;
	mac_addr[2] = (mac_lo >> 24) & 0xff;
	mac_addr[3] = (mac_lo >> 16) & 0xff;
	mac_addr[4] = (mac_lo >> 8) & 0xff;
	mac_addr[5] = mac_lo & 0xff;

	return 0;
}

static int am65_cpsw_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	struct am65_cpsw_port *port = edev_to_port(edev);

	am65_cpsw_am654_get_efuse_macid(port, mac);

	return 0;
}

#define mac_hi(mac)	(((mac)[0] << 0) | ((mac)[1] << 8) |    \
			 ((mac)[2] << 16) | ((mac)[3] << 24))
#define mac_lo(mac)	(((mac)[4] << 0) | ((mac)[5] << 8))

static int am65_cpsw_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct am65_cpsw_port *port = edev_to_port(edev);

	writel(mac_hi(mac), port->port_base + AM65_CPSW_PN_REG_SA_H);
	writel(mac_lo(mac), port->port_base + AM65_CPSW_PN_REG_SA_L);

	return 0;
}

static int am65_cpsw_ofdata_parse_phy(struct am65_cpsw_port *port)
{
	int ret;

	ret = of_get_phy_mode(port->device_node);
	if (ret < 0)
		port->interface = PHY_INTERFACE_MODE_MII;
	else
		port->interface = ret;

	return 0;
}

static int am65_cpsw_port_probe(struct am65_cpsw_common *cpsw_common,
				struct am65_cpsw_port *port)
{
	struct eth_device *edev;
	struct device *dev;
	int ret;

	if (!port->enabled)
		return 0;

	dev = &port->dev;

	dev_set_name(dev, "cpsw-slave");
	dev->id = port->port_id;
	dev->parent = cpsw_common->dev;
	dev->of_node = port->device_node;
	ret = register_device(dev);
	if (ret)
		return ret;

	port->cpsw_common = cpsw_common;

	ret = am65_cpsw_ofdata_parse_phy(port);
	if (ret)
		goto out;

	ret = am65_cpsw_gmii_sel_k3(port, port->interface);
	if (ret)
		goto out;

	edev = &port->edev;
	edev->parent = dev;

	edev->open = am65_cpsw_start;
	edev->halt = am65_cpsw_stop;
	edev->send = am65_cpsw_send;
	edev->recv = am65_cpsw_recv;
	edev->get_ethaddr = am65_cpsw_get_ethaddr;
	edev->set_ethaddr = am65_cpsw_set_ethaddr;

	ret = eth_register(edev);
	if (!ret)
		return 0;
out:
	return ret;
}

static int am65_cpsw_probe_nuss(struct device *dev)
{
	struct am65_cpsw_common *cpsw_common;
	struct device_node *ports_np, *node;
	int ret, i;

	cpsw_common = xzalloc(sizeof(*cpsw_common));

	cpsw_common->dev = dev;
	cpsw_common->ss_base = dev_request_mem_region(dev, 0);
	if (IS_ERR(cpsw_common->ss_base))
		return -EINVAL;

	cpsw_common->fclk = clk_get(dev, "fck");
	if (IS_ERR(cpsw_common->fclk))
		return dev_err_probe(dev, PTR_ERR(cpsw_common->fclk), "failed to get clock\n");

	cpsw_common->cpsw_base = cpsw_common->ss_base + AM65_CPSW_CPSW_NU_BASE;
	cpsw_common->ale_base = cpsw_common->cpsw_base +
				AM65_CPSW_CPSW_NU_ALE_BASE;

	dev->priv = cpsw_common;

	ports_np = of_get_child_by_name(dev->of_node, "ethernet-ports");
	if (!ports_np) {
		ret = -ENOENT;
		goto out;
	}

	for_each_child_of_node(ports_np, node) {
		const char *node_name;
		u32 port_id;
		bool enabled;
		struct am65_cpsw_port *port;

		node_name = node->name;

		enabled = of_device_is_available(node);

		ret = of_property_read_u32(node, "reg", &port_id);
		if (ret) {
			dev_err(dev, "%s: failed to get port_id (%d)\n",
				node_name, ret);
			goto out;
		}

		if (port_id >= AM65_CPSW_CPSWNU_MAX_PORTS) {
			dev_err(dev, "%s: invalid port_id (%d)\n",
				node_name, port_id);
			ret = -EINVAL;
			goto out;
		}
		cpsw_common->port_num++;

		if (!port_id)
			continue;

		port = &cpsw_common->ports[port_id];

		port->enabled = true;
		port->device_node = node;
	}

	for (i = 0; i < AM65_CPSW_CPSWNU_MAX_PORTS; i++) {
		struct am65_cpsw_port *port = &cpsw_common->ports[i];

		port->port_id = i;
		port->port_base = cpsw_common->cpsw_base +
				  AM65_CPSW_CPSW_NU_PORTS_OFFSET +
				  (i * AM65_CPSW_CPSW_NU_PORTS_OFFSET);
		port->port_sgmii_base = cpsw_common->ss_base +
					(i * AM65_CPSW_SGMII_BASE);
		port->macsl_base = port->port_base +
				   AM65_CPSW_CPSW_NU_PORT_MACSL_OFFSET;

		ret = am65_cpsw_port_probe(cpsw_common, port);

		if (ret)
			dev_err(dev, "Failed to probe port %d\n", port->port_id);

	}

	cpsw_common->bus_freq = AM65_CPSW_MDIO_BUS_FREQ_DEF;
	of_property_read_u32(dev->of_node, "bus_freq", &cpsw_common->bus_freq);

	dev_dbg(dev, "K3 CPSW: nuss_ver: 0x%08X cpsw_ver: 0x%08X ale_ver: 0x%08X Ports:%u\n",
		 readl(cpsw_common->ss_base),
		 readl(cpsw_common->cpsw_base),
		 readl(cpsw_common->ale_base),
		 cpsw_common->port_num);

	of_platform_populate(dev->of_node, NULL, dev);
out:
	return ret;
}

static void am65_cpsw_remove_nuss(struct device *dev)
{
	struct am65_cpsw_common *cpsw_common = dev->priv;
	int i;

	for (i = 0; i < AM65_CPSW_CPSWNU_MAX_PORTS; i++) {
		struct am65_cpsw_port *port = &cpsw_common->ports[i];

		if (!port->enabled)
			continue;

		am65_cpsw_stop(&port->edev);
	}
}

static const struct of_device_id am65_cpsw_nuss_ids[] = {
	{ .compatible = "ti,am654-cpsw-nuss" },
	{ .compatible = "ti,j721e-cpsw-nuss" },
	{ .compatible = "ti,am642-cpsw-nuss" },
	{ }
};

static struct driver am65_cpsw_nuss = {
	.probe = am65_cpsw_probe_nuss,
	.remove = am65_cpsw_remove_nuss,
	.name = "am65_cpsw_nuss",
	.of_compatible = am65_cpsw_nuss_ids,
};

device_platform_driver(am65_cpsw_nuss);
