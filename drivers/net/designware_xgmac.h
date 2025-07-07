/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023 Intel Coporation.
 */

#include <net.h>
#include <linux/bitops.h>
#include <linux/phy.h>

/* Core registers */

#define XGMAC_MAC_REGS_BASE 0x000

struct xgmac_mac_regs {
	u32 tx_configuration;			/* 0x000 */
	u32 rx_configuration;			/* 0x004 */
	u32 mac_packet_filter;			/* 0x008 */
	u32 unused_00c[(0x070 - 0x00c) / 4];	/* 0x00c */
	u32 q0_tx_flow_ctrl;			/* 0x070 */
	u32 unused_070[(0x090 - 0x074) / 4];	/* 0x074 */
	u32 rx_flow_ctrl;			/* 0x090 */
	u32 unused_094[(0x0a0 - 0x094) / 4];	/* 0x094 */
	u32 rxq_ctrl0;				/* 0x0a0 */
	u32 rxq_ctrl1;				/* 0x0a4 */
	u32 rxq_ctrl2;				/* 0x0a8 */
	u32 unused_0ac[(0x0dc - 0x0ac) / 4];	/* 0x0ac */
	u32 us_tic_counter;			/* 0x0dc */
	u32 unused_0e0[(0x11c - 0x0e0) / 4];	/* 0x0e0 */
	u32 hw_feature0;			/* 0x11c */
	u32 hw_feature1;			/* 0x120 */
	u32 hw_feature2;			/* 0x124 */
	u32 hw_feature3;			/* 0x128 */
	u32 hw_feature4;			/* 0x12c */
	u32 unused_130[(0x140 - 0x130) / 4];	/* 0x130 */
	u32 mac_extended_conf;			/* 0x140 */
	u32 unused_144[(0x200 - 0x144) / 4];	/* 0x144 */
	u32 mdio_address;			/* 0x200 */
	u32 mdio_data;				/* 0x204 */
	u32 mdio_cont_write_addr;		/* 0x208 */
	u32 mdio_cont_write_data;		/* 0x20c */
	u32 mdio_cont_scan_port_enable;		/* 0x210 */
	u32 mdio_intr_status;			/* 0x214 */
	u32 mdio_intr_enable;			/* 0x218 */
	u32 mdio_port_cnct_dsnct_status;	/* 0x21c */
	u32 mdio_clause_22_port;		/* 0x220 */
	u32 unused_224[(0x300 - 0x224)	/ 4];	/* 0x224 */
	u32 address0_high;			/* 0x300 */
	u32 address0_low;			/* 0x304 */
};

#define XGMAC_TIMEOUT_100MS			100000
#define XGMAC_MAC_CONF_SS_SHIFT			29
#define XGMAC_MAC_CONF_SS_SHIFT_MASK		GENMASK(31, 29)
#define XGMAC_MAC_CONF_SS_10G_XGMII		0
#define XGMAC_MAC_CONF_SS_2_5G_GMII		2
#define XGMAC_MAC_CONF_SS_1G_GMII		3
#define XGMAC_MAC_CONF_SS_100M_MII		4
#define XGMAC_MAC_CONF_SS_5G_XGMII		5
#define XGMAC_MAC_CONF_SS_2_5G_XGMII		6
#define XGMAC_MAC_CONF_SS_2_10M_MII		7

#define XGMAC_MAC_CONF_JD			BIT(16)
#define XGMAC_MAC_CONF_JE			BIT(8)
#define XGMAC_MAC_CONF_WD			BIT(7)
#define XGMAC_MAC_CONF_GPSLCE			BIT(6)
#define XGMAC_MAC_CONF_CST			BIT(2)
#define XGMAC_MAC_CONF_ACS			BIT(1)
#define XGMAC_MAC_CONF_TE			BIT(0)
#define XGMAC_MAC_CONF_RE			BIT(0)

#define XGMAC_MAC_EXT_CONF_HD			BIT(24)

#define XGMAC_MAC_PACKET_FILTER_RA		BIT(31)
#define XGMAC_MAC_PACKET_FILTER_PR		BIT(0)

#define XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT	16
#define XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_MASK	GENMASK(15, 0)
#define XGMAC_MAC_Q0_TX_FLOW_CTRL_TFE		BIT(1)

#define XGMAC_MAC_RX_FLOW_CTRL_RFE		BIT(0)
#define XGMAC_MAC_RXQ_CTRL0_RXQ0EN_SHIFT	0
#define XGMAC_MAC_RXQ_CTRL0_RXQ0EN_MASK		GENMASK(1, 0)
#define XGMAC_MAC_RXQ_CTRL0_RXQ0EN_NOT_ENABLED	0
#define XGMAC_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB	2
#define XGMAC_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV	1

#define XGMAC_MAC_RXQ_CTRL1_MCBCQEN		BIT(15)

#define XGMAC_MAC_RXQ_CTRL2_PSRQ0_SHIFT		0
#define XGMAC_MAC_RXQ_CTRL2_PSRQ0_MASK		GENMASK(7, 0)

#define XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT	6
#define XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_MASK	GENMASK(4, 0)
#define XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT	0
#define XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_MASK	GENMASK(4, 0)

#define XGMAC_MDIO_SINGLE_CMD_SHIFT		16
#define XGMAC_MDIO_SINGLE_CMD_ADDR_CMD_READ	3 << XGMAC_MDIO_SINGLE_CMD_SHIFT
#define XGMAC_MDIO_SINGLE_CMD_ADDR_CMD_WRITE	BIT(16)
#define XGMAC_MAC_MDIO_ADDRESS_PA_SHIFT		16
#define XGMAC_MAC_MDIO_ADDRESS_PA_MASK		GENMASK(15, 0)
#define XGMAC_MAC_MDIO_ADDRESS_DA_SHIFT		21
#define XGMAC_MAC_MDIO_ADDRESS_CR_SHIFT		19
#define XGMAC_MAC_MDIO_ADDRESS_CR_100_150	0
#define XGMAC_MAC_MDIO_ADDRESS_CR_150_250	1
#define XGMAC_MAC_MDIO_ADDRESS_CR_250_300	2
#define XGMAC_MAC_MDIO_ADDRESS_CR_300_350	3
#define XGMAC_MAC_MDIO_ADDRESS_CR_350_400	4
#define XGMAC_MAC_MDIO_ADDRESS_CR_400_500	5
#define XGMAC_MAC_MDIO_ADDRESS_SADDR		BIT(18)
#define XGMAC_MAC_MDIO_ADDRESS_SBUSY		BIT(22)
#define XGMAC_MAC_MDIO_REG_ADDR_C22P_MASK	GENMASK(4, 0)
#define XGMAC_MAC_MDIO_DATA_GD_MASK		GENMASK(15, 0)

/* MTL Registers */

#define XGMAC_MTL_REGS_BASE 0x1000

struct xgmac_mtl_regs {
	u32 mtl_operation_mode;			/* 0x1000 */
	u32 unused_1004[(0x1030 - 0x1004) / 4];	/* 0x1004 */
	u32 mtl_rxq_dma_map0;			/* 0x1030 */
	u32 mtl_rxq_dma_map1;			/* 0x1034 */
	u32 mtl_rxq_dma_map2;			/* 0x1038 */
	u32 mtl_rxq_dma_map3;			/* 0x103c */
	u32 mtl_tc_prty_map0;			/* 0x1040 */
	u32 mtl_tc_prty_map1;			/* 0x1044 */
	u32 unused_1048[(0x1100 - 0x1048) / 4]; /* 0x1048 */
	u32 txq0_operation_mode;		/* 0x1100 */
	u32 unused_1104;			/* 0x1104 */
	u32 txq0_debug;				/* 0x1108 */
	u32 unused_100c[(0x1118 - 0x110c) / 4];	/* 0x110c */
	u32 txq0_quantum_weight;		/* 0x1118 */
	u32 unused_111c[(0x1140 - 0x111c) / 4];	/* 0x111c */
	u32 rxq0_operation_mode;		/* 0x1140 */
	u32 unused_1144;			/* 0x1144 */
	u32 rxq0_debug;				/* 0x1148 */
};

#define XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT		16
#define XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_MASK		GENMASK(8, 0)
#define XGMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT	2
#define XGMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED	2
#define XGMAC_MTL_TXQ0_OPERATION_MODE_TSF		BIT(1)
#define XGMAC_MTL_TXQ0_OPERATION_MODE_FTQ		BIT(0)

#define XGMAC_MTL_TXQ0_DEBUG_TXQSTS			BIT(4)
#define XGMAC_MTL_TXQ0_DEBUG_TRCSTS_SHIFT		1
#define XGMAC_MTL_TXQ0_DEBUG_TRCSTS_MASK		GENMASK(2, 0)
#define XGMAC_MTL_TXQ0_DEBUG_TRCSTS_READ_STATE		0x1

#define XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT		16
#define XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_MASK		GENMASK(9, 0)
#define XGMAC_MTL_RXQ0_OPERATION_MODE_EHFC		BIT(7)
#define XGMAC_MTL_RXQ0_OPERATION_MODE_RSF		BIT(5)

#define XGMAC_MTL_RXQ0_DEBUG_PRXQ_SHIFT			16
#define XGMAC_MTL_RXQ0_DEBUG_PRXQ_MASK			GENMASK(14, 0)
#define XGMAC_MTL_RXQ0_DEBUG_RXQSTS_SHIFT		4
#define XGMAC_MTL_RXQ0_DEBUG_RXQSTS_MASK		GENMASK(1, 0)

/* DMA Registers */

#define XGMAC_DMA_REGS_BASE 0x3000

struct xgmac_dma_regs {
	u32 mode;					/* 0x3000 */
	u32 sysbus_mode;				/* 0x3004 */
	u32 unused_3008[(0x3100 - 0x3008) / 4];		/* 0x3008 */
	u32 ch0_control;				/* 0x3100 */
	u32 ch0_tx_control;				/* 0x3104 */
	u32 ch0_rx_control;				/* 0x3108 */
	u32 slot_func_control_status;			/* 0x310c */
	u32 ch0_txdesc_list_haddress;			/* 0x3110 */
	u32 ch0_txdesc_list_address;			/* 0x3114 */
	u32 ch0_rxdesc_list_haddress;			/* 0x3118 */
	u32 ch0_rxdesc_list_address;			/* 0x311c */
	/* 0x3120: register undocumented; maybe high address of ch0_txdesc_tail_pointer? */
	u32 unused_3120;
	u32 ch0_txdesc_tail_pointer;			/* 0x3124 */
	/* 0x3128: register undocumented; maybe high address of ch0_rxdesc_tail_pointer? */
	u32 unused_3128;
	u32 ch0_rxdesc_tail_pointer;			/* 0x312c */
	u32 ch0_txdesc_ring_length;			/* 0x3130 */
	u32 ch0_rxdesc_ring_length;			/* 0x3134 */
	u32 unused_3138[(0x3160 - 0x3138) / 4];		/* 0x3138 */
	u32 ch0_status;					/* 0x3160 */
};

#define XGMAC_DMA_MODE_SWR				BIT(0)
#define XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_SHIFT		24
#define XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_MASK		GENMASK(4, 0)
#define XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT		16
#define XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK		GENMASK(4, 0)
#define XGMAC_DMA_SYSBUS_MODE_AAL			BIT(12)
#define XGMAC_DMA_SYSBUS_MODE_EAME			BIT(11)
#define XGMAC_DMA_SYSBUS_MODE_BLEN32			BIT(4)
#define XGMAC_DMA_SYSBUS_MODE_BLEN16			BIT(3)
#define XGMAC_DMA_SYSBUS_MODE_BLEN8			BIT(2)
#define XGMAC_DMA_SYSBUS_MODE_BLEN4			BIT(1)
#define XGMAC_DMA_SYSBUS_MODE_UNDEF			BIT(0)

#define XGMAC_DMA_CH0_CONTROL_DSL_SHIFT			18
#define XGMAC_DMA_CH0_CONTROL_PBLX8			BIT(16)

#define XGMAC_DMA_CH0_TX_CONTROL_TXPBL_SHIFT		16
#define XGMAC_DMA_CH0_TX_CONTROL_TXPBL_MASK		GENMASK(5, 0)
#define XGMAC_DMA_CH0_TX_CONTROL_OSP			BIT(4)
#define XGMAC_DMA_CH0_TX_CONTROL_ST			BIT(0)

#define XGMAC_DMA_CH0_RX_CONTROL_RXPBL_SHIFT		16
#define XGMAC_DMA_CH0_RX_CONTROL_RXPBL_MASK		GENMASK(5, 0)
#define XGMAC_DMA_CH0_RX_CONTROL_RBSZ_SHIFT		4
#define XGMAC_DMA_CH0_RX_CONTROL_RBSZ_MASK		GENMASK(10, 0)
#define XGMAC_DMA_CH0_RX_CONTROL_SR			BIT(0)

/* Descriptors */
#define XGMAC_DESCRIPTOR_WORDS		4
#define XGMAC_DESCRIPTOR_SIZE		(XGMAC_DESCRIPTOR_WORDS * 4)
#define XGMAC_DESCRIPTORS_NUM		8
#define XGMAC_DESCRIPTOR_ALIGN		64
#define XGMAC_DESCRIPTORS_SIZE		ALIGN(XGMAC_DESCRIPTORS_NUM * \
					      XGMAC_DESCRIPTOR_SIZE, XGMAC_DESCRIPTOR_ALIGN)
#define XGMAC_BUFFER_ALIGN		XGMAC_DESCRIPTOR_ALIGN
#define XGMAC_MAX_PACKET_SIZE		ALIGN(1568, XGMAC_DESCRIPTOR_ALIGN)
#define XGMAC_RX_BUFFER_SIZE		(XGMAC_DESCRIPTORS_NUM * XGMAC_MAX_PACKET_SIZE)

#define XGMAC_RDES3_PKT_LENGTH_MASK	GENMASK(13, 0)

struct xgmac_desc {
	u32 des0;
	u32 des1;
	u32 des2;
	u32 des3;
};

#define XGMAC_DESC3_OWN		BIT(31)
#define XGMAC_DESC3_FD		BIT(29)
#define XGMAC_DESC3_LD		BIT(28)

#define XGMAC_AXI_WIDTH_32	4
#define XGMAC_AXI_WIDTH_64	8
#define XGMAC_AXI_WIDTH_128	16

struct xgmac_config {
	bool reg_access_always_ok;
	int swr_wait;
	int config_mac;
	int config_mac_mdio;
	unsigned int axi_bus_width;
	phy_interface_t interface;
	struct xgmac_ops *ops;
};

struct xgmac_ops {
	int (*xgmac_probe_resources)(struct device *dev);
	int (*xgmac_start_resets)(struct device *dev);
	int (*xgmac_get_enetaddr)(struct device *dev);
};

struct xgmac_priv {
	struct eth_device netdev;
	struct device *dev;
	const struct xgmac_config *config;
	void __iomem *regs;
	struct xgmac_mac_regs *mac_regs;
	struct xgmac_mtl_regs *mtl_regs;
	struct xgmac_dma_regs *dma_regs;
	struct reset_control *rst;
	struct reset_control *rst_ocp;
	struct mii_bus miibus;
	int phy_addr;
	phy_interface_t interface;

	u8 macaddr[6];

	u32 reg_offset, reg_shift;
	struct xgmac_desc *tx_descs, *rx_descs;
	dma_addr_t tx_descs_phys, rx_descs_phys;
	dma_addr_t dma_rx_buf_phys[XGMAC_DESCRIPTORS_NUM];
	void *dma_rx_buf_virt[XGMAC_DESCRIPTORS_NUM];
	int tx_desc_idx, rx_desc_idx;
	unsigned int desc_size;
	unsigned int desc_per_cacheline;
	bool started;
	bool reg_access_ok;
	bool clk_ck_enabled;
};

int xgmac_probe(struct device *dev);
void xgmac_remove(struct device *dev);
void xgmac_flush_desc_generic(void *desc);
void xgmac_flush_buffer_generic(void *buf, size_t size);
int xgmac_null_ops(struct device *dev);

extern struct xgmac_config xgmac_socfpga_config;
