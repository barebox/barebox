// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2010-2011 Calxeda, Inc.
 */

#include <common.h>
#include <dma.h>
#include <net.h>
#include <clock.h>
#include <malloc.h>
#include <xfuncs.h>
#include <init.h>
#include <errno.h>
#include <io.h>
#include <linux/err.h>

#define TX_NUM_DESC			1
#define RX_NUM_DESC			32

#define ETH_BUF_SZ			2048
#define TX_BUF_SZ			(ETH_BUF_SZ * TX_NUM_DESC)
#define RX_BUF_SZ			(ETH_BUF_SZ * RX_NUM_DESC)

/* XGMAC Register definitions */
#define XGMAC_CONTROL		0x00000000	/* MAC Configuration */
#define XGMAC_FRAME_FILTER	0x00000004	/* MAC Frame Filter */
#define XGMAC_FLOW_CTRL		0x00000018	/* MAC Flow Control */
#define XGMAC_VLAN_TAG		0x0000001C	/* VLAN Tags */
#define XGMAC_VERSION		0x00000020	/* Version */
#define XGMAC_VLAN_INCL		0x00000024	/* VLAN tag for tx frames */
#define XGMAC_LPI_CTRL		0x00000028	/* LPI Control and Status */
#define XGMAC_LPI_TIMER		0x0000002C	/* LPI Timers Control */
#define XGMAC_TX_PACE		0x00000030	/* Transmit Pace and Stretch */
#define XGMAC_VLAN_HASH		0x00000034	/* VLAN Hash Table */
#define XGMAC_DEBUG		0x00000038	/* Debug */
#define XGMAC_INT_STAT		0x0000003C	/* Interrupt and Control */
#define XGMAC_ADDR_HIGH(reg)	(0x00000040 + ((reg) * 8))
#define XGMAC_ADDR_LOW(reg)	(0x00000044 + ((reg) * 8))
#define XGMAC_HASH(n)		(0x00000300 + (n) * 4) /* HASH table regs */
#define XGMAC_NUM_HASH		16
#define XGMAC_OMR		0x00000400
#define XGMAC_REMOTE_WAKE	0x00000700	/* Remote Wake-Up Frm Filter */
#define XGMAC_PMT		0x00000704	/* PMT Control and Status */
#define XGMAC_MMC_CTRL		0x00000800	/* XGMAC MMC Control */
#define XGMAC_MMC_INTR_RX	0x00000804	/* Recieve Interrupt */
#define XGMAC_MMC_INTR_TX	0x00000808	/* Transmit Interrupt */
#define XGMAC_MMC_INTR_MASK_RX	0x0000080c	/* Recieve Interrupt Mask */
#define XGMAC_MMC_INTR_MASK_TX	0x00000810	/* Transmit Interrupt Mask */


/* Hardware TX Statistics Counters */
#define XGMAC_MMC_TXOCTET_GB_LO	0x00000814
#define XGMAC_MMC_TXOCTET_GB_HI	0x00000818
#define XGMAC_MMC_TXFRAME_GB_LO	0x0000081C
#define XGMAC_MMC_TXFRAME_GB_HI	0x00000820
#define XGMAC_MMC_TXBCFRAME_G	0x00000824
#define XGMAC_MMC_TXMCFRAME_G	0x0000082C
#define XGMAC_MMC_TXUCFRAME_GB	0x00000864
#define XGMAC_MMC_TXMCFRAME_GB	0x0000086C
#define XGMAC_MMC_TXBCFRAME_GB	0x00000874
#define XGMAC_MMC_TXUNDERFLOW	0x0000087C
#define XGMAC_MMC_TXOCTET_G_LO	0x00000884
#define XGMAC_MMC_TXOCTET_G_HI	0x00000888
#define XGMAC_MMC_TXFRAME_G_LO	0x0000088C
#define XGMAC_MMC_TXFRAME_G_HI	0x00000890
#define XGMAC_MMC_TXPAUSEFRAME	0x00000894
#define XGMAC_MMC_TXVLANFRAME	0x0000089C

/* Hardware RX Statistics Counters */
#define XGMAC_MMC_RXFRAME_GB_LO	0x00000900
#define XGMAC_MMC_RXFRAME_GB_HI	0x00000904
#define XGMAC_MMC_RXOCTET_GB_LO	0x00000908
#define XGMAC_MMC_RXOCTET_GB_HI	0x0000090C
#define XGMAC_MMC_RXOCTET_G_LO	0x00000910
#define XGMAC_MMC_RXOCTET_G_HI	0x00000914
#define XGMAC_MMC_RXBCFRAME_G	0x00000918
#define XGMAC_MMC_RXMCFRAME_G	0x00000920
#define XGMAC_MMC_RXCRCERR	0x00000928
#define XGMAC_MMC_RXRUNT	0x00000930
#define XGMAC_MMC_RXJABBER	0x00000934
#define XGMAC_MMC_RXUCFRAME_G	0x00000970
#define XGMAC_MMC_RXLENGTHERR	0x00000978
#define XGMAC_MMC_RXPAUSEFRAME	0x00000988
#define XGMAC_MMC_RXOVERFLOW	0x00000990
#define XGMAC_MMC_RXVLANFRAME	0x00000998
#define XGMAC_MMC_RXWATCHDOG	0x000009a0

/* DMA Control and Status Registers */
#define XGMAC_DMA_BUS_MODE	0x00000f00	/* Bus Mode */
#define XGMAC_DMA_TX_POLL	0x00000f04	/* Transmit Poll Demand */
#define XGMAC_DMA_RX_POLL	0x00000f08	/* Received Poll Demand */
#define XGMAC_DMA_RX_BASE_ADDR	0x00000f0c	/* Receive List Base */
#define XGMAC_DMA_TX_BASE_ADDR	0x00000f10	/* Transmit List Base */
#define XGMAC_DMA_STATUS	0x00000f14	/* Status Register */
#define XGMAC_DMA_CONTROL	0x00000f18	/* Ctrl (Operational Mode) */
#define XGMAC_DMA_INTR_ENA	0x00000f1c	/* Interrupt Enable */
#define XGMAC_DMA_MISS_FRAME_CTR 0x00000f20	/* Missed Frame Counter */
#define XGMAC_DMA_RI_WDOG_TIMER	0x00000f24	/* RX Intr Watchdog Timer */
#define XGMAC_DMA_AXI_BUS	0x00000f28	/* AXI Bus Mode */
#define XGMAC_DMA_AXI_STATUS	0x00000f2C	/* AXI Status */
#define XGMAC_DMA_HW_FEATURE	0x00000f58	/* Enabled Hardware Features */

#define XGMAC_ADDR_AE		0x80000000
#define XGMAC_MAX_FILTER_ADDR	31

/* PMT Control and Status */
#define XGMAC_PMT_POINTER_RESET	0x80000000
#define XGMAC_PMT_GLBL_UNICAST	0x00000200
#define XGMAC_PMT_WAKEUP_RX_FRM	0x00000040
#define XGMAC_PMT_MAGIC_PKT	0x00000020
#define XGMAC_PMT_WAKEUP_FRM_EN	0x00000004
#define XGMAC_PMT_MAGIC_PKT_EN	0x00000002
#define XGMAC_PMT_POWERDOWN	0x00000001

#define XGMAC_CONTROL_SPD	0x40000000	/* Speed control */
#define XGMAC_CONTROL_SPD_MASK	0x60000000
#define XGMAC_CONTROL_SPD_1G	0x60000000
#define XGMAC_CONTROL_SPD_2_5G	0x40000000
#define XGMAC_CONTROL_SPD_10G	0x00000000
#define XGMAC_CONTROL_SARC	0x10000000	/* Source Addr Insert/Replace */
#define XGMAC_CONTROL_SARK_MASK	0x18000000
#define XGMAC_CONTROL_CAR	0x04000000	/* CRC Addition/Replacement */
#define XGMAC_CONTROL_CAR_MASK	0x06000000
#define XGMAC_CONTROL_DP	0x01000000	/* Disable Padding */
#define XGMAC_CONTROL_WD	0x00800000	/* Disable Watchdog on rx */
#define XGMAC_CONTROL_JD	0x00400000	/* Jabber disable */
#define XGMAC_CONTROL_JE	0x00100000	/* Jumbo frame */
#define XGMAC_CONTROL_LM	0x00001000	/* Loop-back mode */
#define XGMAC_CONTROL_IPC	0x00000400	/* Checksum Offload */
#define XGMAC_CONTROL_ACS	0x00000080	/* Automatic Pad/FCS Strip */
#define XGMAC_CONTROL_DDIC	0x00000010	/* Disable Deficit Idle Count */
#define XGMAC_CONTROL_TE	0x00000008	/* Transmitter Enable */
#define XGMAC_CONTROL_RE	0x00000004	/* Receiver Enable */

/* XGMAC Frame Filter defines */
#define XGMAC_FRAME_FILTER_PR	0x00000001	/* Promiscuous Mode */
#define XGMAC_FRAME_FILTER_HUC	0x00000002	/* Hash Unicast */
#define XGMAC_FRAME_FILTER_HMC	0x00000004	/* Hash Multicast */
#define XGMAC_FRAME_FILTER_DAIF	0x00000008	/* DA Inverse Filtering */
#define XGMAC_FRAME_FILTER_PM	0x00000010	/* Pass all multicast */
#define XGMAC_FRAME_FILTER_DBF	0x00000020	/* Disable Broadcast frames */
#define XGMAC_FRAME_FILTER_SAIF	0x00000100	/* Inverse Filtering */
#define XGMAC_FRAME_FILTER_SAF	0x00000200	/* Source Address Filter */
#define XGMAC_FRAME_FILTER_HPF	0x00000400	/* Hash or perfect Filter */
#define XGMAC_FRAME_FILTER_VHF	0x00000800	/* VLAN Hash Filter */
#define XGMAC_FRAME_FILTER_VPF	0x00001000	/* VLAN Perfect Filter */
#define XGMAC_FRAME_FILTER_RA	0x80000000	/* Receive all mode */

#define FIFO_MINUS_1K			0x0
#define FIFO_MINUS_2K			0x1
#define FIFO_MINUS_3K			0x2
#define FIFO_MINUS_4K			0x3
#define FIFO_MINUS_6K			0x4
#define FIFO_MINUS_8K			0x5
#define FIFO_MINUS_12K			0x6
#define FIFO_MINUS_16K			0x7

/* XGMAC FLOW CTRL defines */
#define XGMAC_FLOW_CTRL_PT_MASK	0xffff0000	/* Pause Time Mask */
#define XGMAC_FLOW_CTRL_PT_SHIFT	16
#define XGMAC_FLOW_CTRL_DZQP	0x00000080	/* Disable Zero-Quanta Phase */
#define XGMAC_FLOW_CTRL_PLT	0x00000020	/* Pause Low Threshhold */
#define XGMAC_FLOW_CTRL_PLT_SHIFT	4
#define XGMAC_FLOW_CTRL_PLT_MASK 0x00000030	/* PLT MASK */
#define XGMAC_FLOW_CTRL_UP	0x00000008	/* Unicast Pause Frame Detect */
#define XGMAC_FLOW_CTRL_RFE	0x00000004	/* Rx Flow Control Enable */
#define XGMAC_FLOW_CTRL_TFE	0x00000002	/* Tx Flow Control Enable */
#define XGMAC_FLOW_CTRL_FCB_BPA	0x00000001	/* Flow Control Busy ... */

/* XGMAC_INT_STAT reg */
#define XGMAC_INT_STAT_PMT	0x0080		/* PMT Interrupt Status */
#define XGMAC_INT_STAT_LPI	0x0040		/* LPI Interrupt Status */

/* DMA Bus Mode register defines */
#define DMA_BUS_MODE_SFT_RESET	0x00000001	/* Software Reset */
#define DMA_BUS_MODE_DSL_MASK	0x0000007c	/* Descriptor Skip Length */
#define DMA_BUS_MODE_DSL_SHIFT	2		/* (in DWORDS) */
#define DMA_BUS_MODE_ATDS	0x00000080	/* Alternate Descriptor Size */

/* Programmable burst length */
#define DMA_BUS_MODE_PBL_MASK	0x00003f00	/* Programmable Burst Len */
#define DMA_BUS_MODE_PBL_SHIFT	8
#define DMA_BUS_MODE_FB		0x00010000	/* Fixed burst */
#define DMA_BUS_MODE_RPBL_MASK	0x003e0000	/* Rx-Programmable Burst Len */
#define DMA_BUS_MODE_RPBL_SHIFT	17
#define DMA_BUS_MODE_USP	0x00800000
#define DMA_BUS_MODE_8PBL	0x01000000
#define DMA_BUS_MODE_AAL	0x02000000

#define DMA_AXIMODE_ENLPI	0x80000000
#define DMA_AXIMODE_MGK		0x40000000
#define DMA_AXIMODE_WROSR	0x00100000
#define DMA_AXIMODE_WROSR_MASK	0x00F00000
#define DMA_AXIMODE_WROSR_SHIFT	20
#define DMA_AXIMODE_RDOSR	0x00010000
#define DMA_AXIMODE_RDOSR_MASK	0x000F0000
#define DMA_AXIMODE_RDOSR_SHIFT	16
#define DMA_AXIMODE_AAL		0x00001000
#define DMA_AXIMODE_BLEN256	0x00000080
#define DMA_AXIMODE_BLEN128	0x00000040
#define DMA_AXIMODE_BLEN64	0x00000020
#define DMA_AXIMODE_BLEN32	0x00000010
#define DMA_AXIMODE_BLEN16	0x00000008
#define DMA_AXIMODE_BLEN8	0x00000004
#define DMA_AXIMODE_BLEN4	0x00000002
#define DMA_AXIMODE_UNDEF	0x00000001

/* DMA Bus Mode register defines */
#define DMA_BUS_PR_RATIO_MASK	0x0000c000	/* Rx/Tx priority ratio */
#define DMA_BUS_PR_RATIO_SHIFT	14
#define DMA_BUS_FB		0x00010000	/* Fixed Burst */

/* DMA Control register defines */
#define DMA_CONTROL_ST		0x00002000	/* Start/Stop Transmission */
#define DMA_CONTROL_SR		0x00000002	/* Start/Stop Receive */
#define DMA_CONTROL_DFF		0x01000000	/* Disable flush of rx frames */

/* DMA Normal interrupt */
#define DMA_INTR_ENA_NIE	0x00010000	/* Normal Summary */
#define DMA_INTR_ENA_AIE	0x00008000	/* Abnormal Summary */
#define DMA_INTR_ENA_ERE	0x00004000	/* Early Receive */
#define DMA_INTR_ENA_FBE	0x00002000	/* Fatal Bus Error */
#define DMA_INTR_ENA_ETE	0x00000400	/* Early Transmit */
#define DMA_INTR_ENA_RWE	0x00000200	/* Receive Watchdog */
#define DMA_INTR_ENA_RSE	0x00000100	/* Receive Stopped */
#define DMA_INTR_ENA_RUE	0x00000080	/* Receive Buffer Unavailable */
#define DMA_INTR_ENA_RIE	0x00000040	/* Receive Interrupt */
#define DMA_INTR_ENA_UNE	0x00000020	/* Tx Underflow */
#define DMA_INTR_ENA_OVE	0x00000010	/* Receive Overflow */
#define DMA_INTR_ENA_TJE	0x00000008	/* Transmit Jabber */
#define DMA_INTR_ENA_TUE	0x00000004	/* Transmit Buffer Unavail */
#define DMA_INTR_ENA_TSE	0x00000002	/* Transmit Stopped */
#define DMA_INTR_ENA_TIE	0x00000001	/* Transmit Interrupt */

#define DMA_INTR_NORMAL		(DMA_INTR_ENA_NIE | DMA_INTR_ENA_RIE | \
				 DMA_INTR_ENA_TUE)

#define DMA_INTR_ABNORMAL	(DMA_INTR_ENA_AIE | DMA_INTR_ENA_FBE | \
				 DMA_INTR_ENA_RWE | DMA_INTR_ENA_RSE | \
				 DMA_INTR_ENA_RUE | DMA_INTR_ENA_UNE | \
				 DMA_INTR_ENA_OVE | DMA_INTR_ENA_TJE | \
				 DMA_INTR_ENA_TSE)

/* DMA default interrupt mask */
#define DMA_INTR_DEFAULT_MASK	(DMA_INTR_NORMAL | DMA_INTR_ABNORMAL)

/* DMA Status register defines */
#define DMA_STATUS_GMI		0x08000000	/* MMC interrupt */
#define DMA_STATUS_GLI		0x04000000	/* GMAC Line interface int */
#define DMA_STATUS_EB_MASK	0x00380000	/* Error Bits Mask */
#define DMA_STATUS_EB_TX_ABORT	0x00080000	/* Error Bits - TX Abort */
#define DMA_STATUS_EB_RX_ABORT	0x00100000	/* Error Bits - RX Abort */
#define DMA_STATUS_TS_MASK	0x00700000	/* Transmit Process State */
#define DMA_STATUS_TS_SHIFT	20
#define DMA_STATUS_RS_MASK	0x000e0000	/* Receive Process State */
#define DMA_STATUS_RS_SHIFT	17
#define DMA_STATUS_NIS		0x00010000	/* Normal Interrupt Summary */
#define DMA_STATUS_AIS		0x00008000	/* Abnormal Interrupt Summary */
#define DMA_STATUS_ERI		0x00004000	/* Early Receive Interrupt */
#define DMA_STATUS_FBI		0x00002000	/* Fatal Bus Error Interrupt */
#define DMA_STATUS_ETI		0x00000400	/* Early Transmit Interrupt */
#define DMA_STATUS_RWT		0x00000200	/* Receive Watchdog Timeout */
#define DMA_STATUS_RPS		0x00000100	/* Receive Process Stopped */
#define DMA_STATUS_RU		0x00000080	/* Receive Buffer Unavailable */
#define DMA_STATUS_RI		0x00000040	/* Receive Interrupt */
#define DMA_STATUS_UNF		0x00000020	/* Transmit Underflow */
#define DMA_STATUS_OVF		0x00000010	/* Receive Overflow */
#define DMA_STATUS_TJT		0x00000008	/* Transmit Jabber Timeout */
#define DMA_STATUS_TU		0x00000004	/* Transmit Buffer Unavail */
#define DMA_STATUS_TPS		0x00000002	/* Transmit Process Stopped */
#define DMA_STATUS_TI		0x00000001	/* Transmit Interrupt */

/* Common MAC defines */
#define MAC_ENABLE_TX		0x00000008	/* Transmitter Enable */
#define MAC_ENABLE_RX		0x00000004	/* Receiver Enable */

/* XGMAC Operation Mode Register */
#define XGMAC_OMR_TSF		0x00200000	/* TX FIFO Store and Forward */
#define XGMAC_OMR_FTF		0x00100000	/* Flush Transmit FIFO */
#define XGMAC_OMR_TTC		0x00020000	/* Transmit Threshhold Ctrl */
#define XGMAC_OMR_TTC_SHIFT	16
#define XGMAC_OMR_TTC_MASK	0x00030000
#define XGMAC_OMR_RFD		0x00006000	/* FC Deactivation Threshhold */
#define XGMAC_OMR_RFD_SHIFT	12
#define XGMAC_OMR_RFD_MASK	0x00007000	/* FC Deact Threshhold MASK */
#define XGMAC_OMR_RFA		0x00000600	/* FC Activation Threshhold */
#define XGMAC_OMR_RFA_SHIFT	9
#define XGMAC_OMR_RFA_MASK	0x00000E00	/* FC Act Threshhold MASK */
#define XGMAC_OMR_EFC		0x00000100	/* Enable Hardware FC */
#define XGMAC_OMR_FEF		0x00000080	/* Forward Error Frames */
#define XGMAC_OMR_DT		0x00000040	/* Drop TCP/IP csum Errors */
#define XGMAC_OMR_RSF		0x00000020	/* RX FIFO Store and Forward */
#define XGMAC_OMR_RTC_256	0x00000018	/* RX Threshhold Ctrl */
#define XGMAC_OMR_RTC_MASK	0x00000018	/* RX Threshhold Ctrl MASK */

/* XGMAC HW Features Register */
#define DMA_HW_FEAT_TXCOESEL	0x00010000	/* TX Checksum offload */

#define XGMAC_MMC_CTRL_CNT_FRZ	0x00000008

/* XGMAC Descriptor Defines */
#define MAX_DESC_BUF_SZ		(0x2000 - 8)

#define RXDESC_EXT_STATUS	0x00000001
#define RXDESC_CRC_ERR		0x00000002
#define RXDESC_RX_ERR		0x00000008
#define RXDESC_RX_WDOG		0x00000010
#define RXDESC_FRAME_TYPE	0x00000020
#define RXDESC_GIANT_FRAME	0x00000080
#define RXDESC_LAST_SEG		0x00000100
#define RXDESC_FIRST_SEG	0x00000200
#define RXDESC_VLAN_FRAME	0x00000400
#define RXDESC_OVERFLOW_ERR	0x00000800
#define RXDESC_LENGTH_ERR	0x00001000
#define RXDESC_SA_FILTER_FAIL	0x00002000
#define RXDESC_DESCRIPTOR_ERR	0x00004000
#define RXDESC_ERROR_SUMMARY	0x00008000
#define RXDESC_FRAME_LEN_OFFSET	16
#define RXDESC_FRAME_LEN_MASK	0x3fff0000
#define RXDESC_DA_FILTER_FAIL	0x40000000

#define RXDESC1_END_RING	0x00008000

#define RXDESC_IP_PAYLOAD_MASK	0x00000003
#define RXDESC_IP_PAYLOAD_UDP	0x00000001
#define RXDESC_IP_PAYLOAD_TCP	0x00000002
#define RXDESC_IP_PAYLOAD_ICMP	0x00000003
#define RXDESC_IP_HEADER_ERR	0x00000008
#define RXDESC_IP_PAYLOAD_ERR	0x00000010
#define RXDESC_IPV4_PACKET	0x00000040
#define RXDESC_IPV6_PACKET	0x00000080
#define TXDESC_UNDERFLOW_ERR	0x00000001
#define TXDESC_JABBER_TIMEOUT	0x00000002
#define TXDESC_LOCAL_FAULT	0x00000004
#define TXDESC_REMOTE_FAULT	0x00000008
#define TXDESC_VLAN_FRAME	0x00000010
#define TXDESC_FRAME_FLUSHED	0x00000020
#define TXDESC_IP_HEADER_ERR	0x00000040
#define TXDESC_PAYLOAD_CSUM_ERR	0x00000080
#define TXDESC_ERROR_SUMMARY	0x00008000
#define TXDESC_SA_CTRL_INSERT	0x00040000
#define TXDESC_SA_CTRL_REPLACE	0x00080000
#define TXDESC_2ND_ADDR_CHAINED	0x00100000
#define TXDESC_END_RING		0x00200000
#define TXDESC_CSUM_IP		0x00400000
#define TXDESC_CSUM_IP_PAYLD	0x00800000
#define TXDESC_CSUM_ALL		0x00C00000
#define TXDESC_CRC_EN_REPLACE	0x01000000
#define TXDESC_CRC_EN_APPEND	0x02000000
#define TXDESC_DISABLE_PAD	0x04000000
#define TXDESC_FIRST_SEG	0x10000000
#define TXDESC_LAST_SEG		0x20000000
#define TXDESC_INTERRUPT	0x40000000

#define DESC_OWN		0x80000000
#define DESC_BUFFER1_SZ_MASK	0x00001fff
#define DESC_BUFFER2_SZ_MASK	0x1fff0000
#define DESC_BUFFER2_SZ_OFFSET	16

struct xgmac_dma_desc {
	__le32 flags;
	__le32 buf_size;
	__le32 buf1_addr;		/* Buffer 1 Address Pointer */
	__le32 buf2_addr;		/* Buffer 2 Address Pointer */
	__le32 ext_status;
	__le32 res[3];
};

struct xgmac_priv {
	struct xgmac_dma_desc *rx_chain;
	struct xgmac_dma_desc *tx_chain;
	char *rxbuffer;

	u32 tx_currdesc;
	u32 rx_currdesc;

	void __iomem *base;

	struct eth_device edev;
	struct device_d *dev;
};

/* XGMAC Descriptor Access Helpers */
static inline void desc_set_buf_len(struct xgmac_dma_desc *p, u32 buf_sz)
{
	if (buf_sz > MAX_DESC_BUF_SZ)
		p->buf_size = cpu_to_le32(MAX_DESC_BUF_SZ |
			(buf_sz - MAX_DESC_BUF_SZ) << DESC_BUFFER2_SZ_OFFSET);
	else
		p->buf_size = cpu_to_le32(buf_sz);
}

static inline int desc_get_buf_len(struct xgmac_dma_desc *p)
{
	u32 len = le32_to_cpu(p->buf_size);
	return (len & DESC_BUFFER1_SZ_MASK) +
		((len & DESC_BUFFER2_SZ_MASK) >> DESC_BUFFER2_SZ_OFFSET);
}

static inline void desc_init_rx_desc(struct xgmac_dma_desc *p, int ring_size,
				     int buf_sz)
{
	struct xgmac_dma_desc *end = p + ring_size - 1;

	memset(p, 0, sizeof(*p) * ring_size);

	for (; p <= end; p++)
		desc_set_buf_len(p, buf_sz);

	end->buf_size |= cpu_to_le32(RXDESC1_END_RING);
}

static inline void desc_init_tx_desc(struct xgmac_dma_desc *p, u32 ring_size)
{
	memset(p, 0, sizeof(*p) * ring_size);
	p[ring_size - 1].flags = cpu_to_le32(TXDESC_END_RING);
}

static inline int desc_get_owner(struct xgmac_dma_desc *p)
{
	return le32_to_cpu(p->flags) & DESC_OWN;
}

static inline void desc_set_rx_owner(struct xgmac_dma_desc *p)
{
	/* Clear all fields and set the owner */
	p->flags = cpu_to_le32(DESC_OWN);
}

static inline void desc_set_tx_owner(struct xgmac_dma_desc *p, u32 flags)
{
	u32 tmpflags = le32_to_cpu(p->flags);
	tmpflags &= TXDESC_END_RING;
	tmpflags |= flags | DESC_OWN;
	p->flags = cpu_to_le32(tmpflags);
}

static inline void *desc_get_buf_addr(struct xgmac_dma_desc *p)
{
	return (void *)le32_to_cpu(p->buf1_addr);
}

static inline void desc_set_buf_addr(struct xgmac_dma_desc *p,
				     void *paddr, int len)
{
	p->buf1_addr = cpu_to_le32(paddr);
	if (len > MAX_DESC_BUF_SZ)
		p->buf2_addr = cpu_to_le32(paddr + MAX_DESC_BUF_SZ);
}

static inline void desc_set_buf_addr_and_size(struct xgmac_dma_desc *p,
					      void *paddr, int len)
{
	desc_set_buf_len(p, len);
	desc_set_buf_addr(p, paddr, len);
}

static inline int desc_get_rx_frame_len(struct xgmac_dma_desc *p)
{
	u32 data = le32_to_cpu(p->flags);
	u32 len = (data & RXDESC_FRAME_LEN_MASK) >> RXDESC_FRAME_LEN_OFFSET;
	if (data & RXDESC_FRAME_TYPE)
		len -= 4;

	return len;
}

/*
 * Initialize a descriptor ring.  Calxeda XGMAC is configured to use
 * advanced descriptors.
 */

static void init_rx_desc(struct xgmac_priv *priv)
{
	struct xgmac_dma_desc *rxdesc = priv->rx_chain;
	void *rxbuffer = priv->rxbuffer;
	int i;

	desc_init_rx_desc(rxdesc, RX_NUM_DESC, ETH_BUF_SZ);
	writel((ulong)rxdesc, priv->base + XGMAC_DMA_RX_BASE_ADDR);

	for (i = 0; i < RX_NUM_DESC; i++) {
		desc_set_buf_addr(rxdesc + i, rxbuffer + (i * ETH_BUF_SZ),
				  ETH_BUF_SZ);
		desc_set_rx_owner(rxdesc + i);
	}
}

static void init_tx_desc(struct xgmac_priv *priv)
{
	desc_init_tx_desc(priv->tx_chain, TX_NUM_DESC);
	writel((ulong)priv->tx_chain, priv->base + XGMAC_DMA_TX_BASE_ADDR);
}

static int xgmac_reset(struct eth_device *dev)
{
	struct xgmac_priv *priv = dev->priv;
	int ret;
	u32 value;

	value = readl(priv->base + XGMAC_CONTROL) & XGMAC_CONTROL_SPD_MASK;

	writel(DMA_BUS_MODE_SFT_RESET, priv->base + XGMAC_DMA_BUS_MODE);

	ret = wait_on_timeout(100 * MSECOND,
		!(readl(priv->base + XGMAC_DMA_BUS_MODE) & DMA_BUS_MODE_SFT_RESET));

	writel(value, priv->base + XGMAC_CONTROL);

	return ret;
}

static int xgmac_open(struct eth_device *edev)
{
	struct xgmac_priv *priv = edev->priv;
	int value;
	int ret;

	ret = xgmac_reset(edev);
	if (ret)
		return ret;

	/* set the AXI bus modes */
	value = DMA_BUS_MODE_ATDS |
		(16 << DMA_BUS_MODE_PBL_SHIFT) |
		DMA_BUS_MODE_FB | DMA_BUS_MODE_AAL;
	writel(value, priv->base + XGMAC_DMA_BUS_MODE);

	value = DMA_AXIMODE_AAL | DMA_AXIMODE_BLEN16 |
		DMA_AXIMODE_BLEN8 | DMA_AXIMODE_BLEN4;
	writel(value, priv->base + XGMAC_DMA_AXI_BUS);

	/* set flow control parameters and store and forward mode */
	value = (FIFO_MINUS_12K << XGMAC_OMR_RFD_SHIFT) |
		(FIFO_MINUS_4K << XGMAC_OMR_RFA_SHIFT) |
		XGMAC_OMR_EFC | XGMAC_OMR_TSF | XGMAC_OMR_RSF;
	writel(value, priv->base + XGMAC_OMR);

	/* enable pause frames */
	value = (1024 << XGMAC_FLOW_CTRL_PT_SHIFT) |
		(1 << XGMAC_FLOW_CTRL_PLT_SHIFT) |
		XGMAC_FLOW_CTRL_UP | XGMAC_FLOW_CTRL_RFE | XGMAC_FLOW_CTRL_TFE;
	writel(value, priv->base + XGMAC_FLOW_CTRL);

	/* Initialize the descriptor chains */
	init_rx_desc(priv);
	init_tx_desc(priv);

	/* must set to 0, or when started up will cause issues */
	priv->tx_currdesc = 0;
	priv->rx_currdesc = 0;

	/* set default core values */
	value = readl(priv->base + XGMAC_CONTROL);
	value &= XGMAC_CONTROL_SPD_MASK;
	value |= XGMAC_CONTROL_DDIC | XGMAC_CONTROL_ACS |
		XGMAC_CONTROL_IPC | XGMAC_CONTROL_CAR;

	/* Everything is ready enable both mac and DMA */
	value |= XGMAC_CONTROL_RE | XGMAC_CONTROL_TE;
	writel(value, priv->base + XGMAC_CONTROL);

	value = readl(priv->base + XGMAC_DMA_CONTROL);
	value |= DMA_CONTROL_SR | DMA_CONTROL_ST;
	writel(value, priv->base + XGMAC_DMA_CONTROL);

	return 0;
}

static int xgmac_send(struct eth_device *edev, void *packet, int length)
{
	struct xgmac_priv *priv = edev->priv;
	u32 currdesc = priv->tx_currdesc;
	struct xgmac_dma_desc *txdesc = &priv->tx_chain[currdesc];
	int ret;

	dma_sync_single_for_device((unsigned long)packet, length, DMA_TO_DEVICE);
	desc_set_buf_addr_and_size(txdesc, packet, length);
	desc_set_tx_owner(txdesc, TXDESC_FIRST_SEG |
		TXDESC_LAST_SEG | TXDESC_CRC_EN_APPEND);

	/* write poll demand */
	writel(1, priv->base + XGMAC_DMA_TX_POLL);

	ret = wait_on_timeout(1 * SECOND, !desc_get_owner(txdesc));
	dma_sync_single_for_cpu((unsigned long)packet, length, DMA_TO_DEVICE);
	if (ret) {
		dev_err(priv->dev, "TX timeout\n");
		return ret;
	}

	priv->tx_currdesc = (currdesc + 1) & (TX_NUM_DESC - 1);
	return 0;
}

static int xgmac_recv(struct eth_device *edev)
{
	struct xgmac_priv *priv = edev->priv;
	u32 currdesc = priv->rx_currdesc;
	struct xgmac_dma_desc *rxdesc = &priv->rx_chain[currdesc];
	int length = 0;
	void *buf_addr;

	/* check if the host has the desc */
	if (desc_get_owner(rxdesc))
		return -1; /* something bad happened */

	length = desc_get_rx_frame_len(rxdesc);
	buf_addr = desc_get_buf_addr(rxdesc);

	dma_sync_single_for_cpu((unsigned long)buf_addr, length, DMA_FROM_DEVICE);
	net_receive(edev, buf_addr, length);
	dma_sync_single_for_device((unsigned long)buf_addr, length,
				   DMA_FROM_DEVICE);

	/* set descriptor back to owned by XGMAC */
	desc_set_rx_owner(rxdesc);
	writel(1, priv->base + XGMAC_DMA_RX_POLL);

	priv->rx_currdesc = (currdesc + 1) & (RX_NUM_DESC - 1);

	return length;
}

static void xgmac_halt(struct eth_device *edev)
{
	struct xgmac_priv *priv = edev->priv;
	int value;

	/* Disable TX/RX */
	value = readl(priv->base + XGMAC_CONTROL);
	value &= ~(XGMAC_CONTROL_RE | XGMAC_CONTROL_TE);
	writel(value, priv->base + XGMAC_CONTROL);

	/* Disable DMA */
	value = readl(priv->base + XGMAC_DMA_CONTROL);
	value &= ~(DMA_CONTROL_SR | DMA_CONTROL_ST);
	writel(value, priv->base + XGMAC_DMA_CONTROL);

	/* must set to 0, or when started up will cause issues */
	priv->tx_currdesc = 0;
	priv->rx_currdesc = 0;
}

static int xgmac_get_ethaddr(struct eth_device *edev, unsigned char *addr)
{
	struct xgmac_priv *priv = edev->priv;
	u32 hi_addr, lo_addr;

	/* Read the MAC address from the hardware */
	hi_addr = readl(priv->base + XGMAC_ADDR_HIGH(0));
	lo_addr = readl(priv->base + XGMAC_ADDR_LOW(0));

	/* Extract the MAC address from the high and low words */
	addr[0] = lo_addr & 0xff;
	addr[1] = (lo_addr >> 8) & 0xff;
	addr[2] = (lo_addr >> 16) & 0xff;
	addr[3] = (lo_addr >> 24) & 0xff;
	addr[4] = hi_addr & 0xff;
	addr[5] = (hi_addr >> 8) & 0xff;

	return 0;
}

static int xgmac_set_ethaddr(struct eth_device *dev, const unsigned char *addr)
{
	struct xgmac_priv *priv = dev->priv;
	u32 data;

	data = (addr[5] << 8) | addr[4];
	writel(data, priv->base + XGMAC_ADDR_HIGH(0));
	data = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	writel(data, priv->base + XGMAC_ADDR_LOW(0));

	return 0;
}

static int hb_xgmac_probe(struct device_d *dev)
{
	struct resource *iores;
	struct eth_device *edev;
	struct xgmac_priv *priv;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	/* check hardware version */
	if (readl(base + XGMAC_VERSION) != 0x1012)
		return -EINVAL;

	priv = xzalloc(sizeof(*priv));

	priv->dev = dev;
	priv->base = base;

	priv->rxbuffer = dma_alloc_coherent(RX_BUF_SZ, DMA_ADDRESS_BROKEN);
	priv->rx_chain = dma_alloc_coherent(RX_NUM_DESC * sizeof(struct xgmac_dma_desc),
					    DMA_ADDRESS_BROKEN);
	priv->tx_chain = dma_alloc_coherent(TX_NUM_DESC * sizeof(struct xgmac_dma_desc),
					    DMA_ADDRESS_BROKEN);

	edev = &priv->edev;
	edev->priv = priv;

	edev->open = xgmac_open;
	edev->send = xgmac_send;
	edev->recv = xgmac_recv;
	edev->halt = xgmac_halt;
	edev->get_ethaddr = xgmac_get_ethaddr;
	edev->set_ethaddr = xgmac_set_ethaddr;
	edev->parent = dev;

	eth_register(edev);

	return 0;
}

static __maybe_unused struct of_device_id xgmac_dt_ids[] = {
	{
		.compatible = "calxeda,hb-xgmac",
	}, {
		/* sentinel */
	}
};

static struct driver_d hb_xgmac_driver = {
	.name  = "hb-xgmac",
	.probe = hb_xgmac_probe,
	.of_compatible = DRV_OF_COMPAT(xgmac_dt_ids),
};
device_platform_driver(hb_xgmac_driver);
