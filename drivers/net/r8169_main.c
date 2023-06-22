// SPDX-License-Identifier: GPL-2.0-only
/*
 * r8169.c: RealTek 8169/8168/8101 ethernet driver.
 *
 * Copyright (c) 2002 ShuChen <shuchen@realtek.com.tw>
 * Copyright (c) 2003 - 2007 Francois Romieu <romieu@fr.zoreil.com>
 * Copyright (c) a lot of people too. Please respect their work.
 *
 * See MAINTAINERS file for support contact information.
 */

#include <common.h>
#include <dma.h>
#include <init.h>
#include <net.h>
#include <malloc.h>
#include <linux/pci.h>
#include <linux/sizes.h>
#include <asm/unaligned.h>
#include <asm/system.h>

#include "r8169.h"
#include "r8169_firmware.h"

#define FIRMWARE_8168D_1	"rtl_nic/rtl8168d-1.fw"
#define FIRMWARE_8168D_2	"rtl_nic/rtl8168d-2.fw"
#define FIRMWARE_8168E_1	"rtl_nic/rtl8168e-1.fw"
#define FIRMWARE_8168E_2	"rtl_nic/rtl8168e-2.fw"
#define FIRMWARE_8168E_3	"rtl_nic/rtl8168e-3.fw"
#define FIRMWARE_8168F_1	"rtl_nic/rtl8168f-1.fw"
#define FIRMWARE_8168F_2	"rtl_nic/rtl8168f-2.fw"
#define FIRMWARE_8105E_1	"rtl_nic/rtl8105e-1.fw"
#define FIRMWARE_8402_1		"rtl_nic/rtl8402-1.fw"
#define FIRMWARE_8411_1		"rtl_nic/rtl8411-1.fw"
#define FIRMWARE_8411_2		"rtl_nic/rtl8411-2.fw"
#define FIRMWARE_8106E_1	"rtl_nic/rtl8106e-1.fw"
#define FIRMWARE_8106E_2	"rtl_nic/rtl8106e-2.fw"
#define FIRMWARE_8168G_2	"rtl_nic/rtl8168g-2.fw"
#define FIRMWARE_8168G_3	"rtl_nic/rtl8168g-3.fw"
#define FIRMWARE_8168H_2	"rtl_nic/rtl8168h-2.fw"
#define FIRMWARE_8168FP_3	"rtl_nic/rtl8168fp-3.fw"
#define FIRMWARE_8107E_2	"rtl_nic/rtl8107e-2.fw"
#define FIRMWARE_8125A_3	"rtl_nic/rtl8125a-3.fw"
#define FIRMWARE_8125B_2	"rtl_nic/rtl8125b-2.fw"

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC. */
#define	MC_FILTER_LIMIT	32

#define TX_DMA_BURST	7	/* Maximum PCI burst, '7' is unlimited */
#define InterFrameGap	0x03	/* 3 means InterFrameGap = the shortest one */

#define R8169_REGS_SIZE		256
#define R8169_RX_BUF_SIZE	(SZ_16K - 1)
#define NUM_TX_DESC	256	/* Number of Tx descriptor registers */
#define NUM_RX_DESC	256	/* Number of Rx descriptor registers */
#define R8169_TX_RING_BYTES	(NUM_TX_DESC * sizeof(struct TxDesc))
#define R8169_RX_RING_BYTES	(NUM_RX_DESC * sizeof(struct RxDesc))

#define OCP_STD_PHY_BASE	0xa400

#define RTL_CFG_NO_GBIT	1

/* write/read MMIO register */
#define RTL_W8(tp, reg, val8)	writeb((val8), tp->mmio_addr + (reg))
#define RTL_W16(tp, reg, val16)	writew((val16), tp->mmio_addr + (reg))
#define RTL_W32(tp, reg, val32)	writel((val32), tp->mmio_addr + (reg))
#define RTL_R8(tp, reg)		readb(tp->mmio_addr + (reg))
#define RTL_R16(tp, reg)		readw(tp->mmio_addr + (reg))
#define RTL_R32(tp, reg)		readl(tp->mmio_addr + (reg))

#define JUMBO_4K	(4 * SZ_1K - VLAN_ETH_HLEN - ETH_FCS_LEN)
#define JUMBO_6K	(6 * SZ_1K - VLAN_ETH_HLEN - ETH_FCS_LEN)
#define JUMBO_7K	(7 * SZ_1K - VLAN_ETH_HLEN - ETH_FCS_LEN)
#define JUMBO_9K	(9 * SZ_1K - VLAN_ETH_HLEN - ETH_FCS_LEN)

static const struct {
	const char *name;
	const char *fw_name;
} rtl_chip_infos[] = {
	/* PCI devices. */
	[RTL_GIGA_MAC_VER_02] = {"RTL8169s"				},
	[RTL_GIGA_MAC_VER_03] = {"RTL8110s"				},
	[RTL_GIGA_MAC_VER_04] = {"RTL8169sb/8110sb"			},
	[RTL_GIGA_MAC_VER_05] = {"RTL8169sc/8110sc"			},
	[RTL_GIGA_MAC_VER_06] = {"RTL8169sc/8110sc"			},
	/* PCI-E devices. */
	[RTL_GIGA_MAC_VER_07] = {"RTL8102e"				},
	[RTL_GIGA_MAC_VER_08] = {"RTL8102e"				},
	[RTL_GIGA_MAC_VER_09] = {"RTL8102e/RTL8103e"			},
	[RTL_GIGA_MAC_VER_10] = {"RTL8101e/RTL8100e"			},
	[RTL_GIGA_MAC_VER_11] = {"RTL8168b/8111b"			},
	[RTL_GIGA_MAC_VER_14] = {"RTL8401"				},
	[RTL_GIGA_MAC_VER_17] = {"RTL8168b/8111b"			},
	[RTL_GIGA_MAC_VER_18] = {"RTL8168cp/8111cp"			},
	[RTL_GIGA_MAC_VER_19] = {"RTL8168c/8111c"			},
	[RTL_GIGA_MAC_VER_20] = {"RTL8168c/8111c"			},
	[RTL_GIGA_MAC_VER_21] = {"RTL8168c/8111c"			},
	[RTL_GIGA_MAC_VER_22] = {"RTL8168c/8111c"			},
	[RTL_GIGA_MAC_VER_23] = {"RTL8168cp/8111cp"			},
	[RTL_GIGA_MAC_VER_24] = {"RTL8168cp/8111cp"			},
	[RTL_GIGA_MAC_VER_25] = {"RTL8168d/8111d",	FIRMWARE_8168D_1},
	[RTL_GIGA_MAC_VER_26] = {"RTL8168d/8111d",	FIRMWARE_8168D_2},
	[RTL_GIGA_MAC_VER_28] = {"RTL8168dp/8111dp"			},
	[RTL_GIGA_MAC_VER_29] = {"RTL8105e",		FIRMWARE_8105E_1},
	[RTL_GIGA_MAC_VER_30] = {"RTL8105e",		FIRMWARE_8105E_1},
	[RTL_GIGA_MAC_VER_31] = {"RTL8168dp/8111dp"			},
	[RTL_GIGA_MAC_VER_32] = {"RTL8168e/8111e",	FIRMWARE_8168E_1},
	[RTL_GIGA_MAC_VER_33] = {"RTL8168e/8111e",	FIRMWARE_8168E_2},
	[RTL_GIGA_MAC_VER_34] = {"RTL8168evl/8111evl",	FIRMWARE_8168E_3},
	[RTL_GIGA_MAC_VER_35] = {"RTL8168f/8111f",	FIRMWARE_8168F_1},
	[RTL_GIGA_MAC_VER_36] = {"RTL8168f/8111f",	FIRMWARE_8168F_2},
	[RTL_GIGA_MAC_VER_37] = {"RTL8402",		FIRMWARE_8402_1 },
	[RTL_GIGA_MAC_VER_38] = {"RTL8411",		FIRMWARE_8411_1 },
	[RTL_GIGA_MAC_VER_39] = {"RTL8106e",		FIRMWARE_8106E_1},
	[RTL_GIGA_MAC_VER_40] = {"RTL8168g/8111g",	FIRMWARE_8168G_2},
	[RTL_GIGA_MAC_VER_42] = {"RTL8168gu/8111gu",	FIRMWARE_8168G_3},
	[RTL_GIGA_MAC_VER_43] = {"RTL8106eus",		FIRMWARE_8106E_2},
	[RTL_GIGA_MAC_VER_44] = {"RTL8411b",		FIRMWARE_8411_2 },
	[RTL_GIGA_MAC_VER_46] = {"RTL8168h/8111h",	FIRMWARE_8168H_2},
	[RTL_GIGA_MAC_VER_48] = {"RTL8107e",		FIRMWARE_8107E_2},
	[RTL_GIGA_MAC_VER_51] = {"RTL8168ep/8111ep"			},
	[RTL_GIGA_MAC_VER_52] = {"RTL8168fp/RTL8117",  FIRMWARE_8168FP_3},
	[RTL_GIGA_MAC_VER_53] = {"RTL8168fp/RTL8117",			},
	[RTL_GIGA_MAC_VER_61] = {"RTL8125A",		FIRMWARE_8125A_3},
	/* reserve 62 for CFG_METHOD_4 in the vendor driver */
	[RTL_GIGA_MAC_VER_63] = {"RTL8125B",		FIRMWARE_8125B_2},
};

static const struct pci_device_id rtl8169_pci_tbl[] = {
	{ PCI_VDEVICE(REALTEK,	0x2502) },
	{ PCI_VDEVICE(REALTEK,	0x2600) },
	{ PCI_VDEVICE(REALTEK,	0x8129) },
	{ PCI_VDEVICE(REALTEK,	0x8136), RTL_CFG_NO_GBIT },
	{ PCI_VDEVICE(REALTEK,	0x8161) },
	{ PCI_VDEVICE(REALTEK,	0x8162) },
	{ PCI_VDEVICE(REALTEK,	0x8167) },
	{ PCI_VDEVICE(REALTEK,	0x8168) },
	{ PCI_VDEVICE(NCUBE,	0x8168) },
	{ PCI_VDEVICE(REALTEK,	0x8169) },
	{ PCI_VENDOR_ID_DLINK,	0x4300,
		PCI_VENDOR_ID_DLINK, 0x4b10, 0, 0 },
	{ PCI_VDEVICE(DLINK,	0x4300) },
	{ PCI_VDEVICE(DLINK,	0x4302) },
	{ PCI_VDEVICE(AT,	0xc107) },
	{ PCI_VENDOR_ID_LINKSYS, 0x1032, PCI_ANY_ID, 0x0024 },
	{ 0x0001, 0x8168, PCI_ANY_ID, 0x2410 },
	{ PCI_VDEVICE(REALTEK,	0x8125) },
	{ PCI_VDEVICE(REALTEK,	0x3000) },
	{}
};

MODULE_DEVICE_TABLE(pci, rtl8169_pci_tbl);

enum rtl_registers {
	MAC0		= 0,	/* Ethernet hardware address. */
	MAC4		= 4,
	MAR0		= 8,	/* Multicast filter. */
	CounterAddrLow		= 0x10,
	CounterAddrHigh		= 0x14,
	TxDescStartAddrLow	= 0x20,
	TxDescStartAddrHigh	= 0x24,
	TxHDescStartAddrLow	= 0x28,
	TxHDescStartAddrHigh	= 0x2c,
	FLASH		= 0x30,
	ERSR		= 0x36,
	ChipCmd		= 0x37,
	TxPoll		= 0x38,
	IntrMask	= 0x3c,
	IntrStatus	= 0x3e,

	TxConfig	= 0x40,
#define	TXCFG_AUTO_FIFO			(1 << 7)	/* 8111e-vl */
#define	TXCFG_EMPTY			(1 << 11)	/* 8111e-vl */

	RxConfig	= 0x44,
#define	RX128_INT_EN			(1 << 15)	/* 8111c and later */
#define	RX_MULTI_EN			(1 << 14)	/* 8111c only */
#define	RXCFG_FIFO_SHIFT		13
					/* No threshold before first PCI xfer */
#define	RX_FIFO_THRESH			(7 << RXCFG_FIFO_SHIFT)
#define	RX_EARLY_OFF			(1 << 11)
#define	RXCFG_DMA_SHIFT			8
					/* Unlimited maximum PCI burst. */
#define	RX_DMA_BURST			(7 << RXCFG_DMA_SHIFT)
	RxMissed	= 0x4c,
	Cfg9346		= 0x50,
	Config0		= 0x51,
	Config1		= 0x52,
	Config2		= 0x53,
#define PME_SIGNAL			(1 << 5)	/* 8168c and later */

	Config3		= 0x54,
	Config4		= 0x55,
	Config5		= 0x56,
	PHYAR		= 0x60,
	PHYstatus	= 0x6c,
	RxMaxSize	= 0xda,
	CPlusCmd	= 0xe0,
	IntrMitigate	= 0xe2,

#define RTL_COALESCE_TX_USECS	GENMASK(15, 12)
#define RTL_COALESCE_TX_FRAMES	GENMASK(11, 8)
#define RTL_COALESCE_RX_USECS	GENMASK(7, 4)
#define RTL_COALESCE_RX_FRAMES	GENMASK(3, 0)

#define RTL_COALESCE_T_MAX	0x0fU
#define RTL_COALESCE_FRAME_MAX	(RTL_COALESCE_T_MAX * 4)

	RxDescAddrLow	= 0xe4,
	RxDescAddrHigh	= 0xe8,
	EarlyTxThres	= 0xec,	/* 8169. Unit of 32 bytes. */

#define NoEarlyTx	0x3f	/* Max value : no early transmit. */

	MaxTxPacketSize	= 0xec,	/* 8101/8168. Unit of 128 bytes. */

#define TxPacketMax	(8064 >> 7)
#define EarlySize	0x27

	FuncEvent	= 0xf0,
	FuncEventMask	= 0xf4,
	FuncPresetState	= 0xf8,
	IBCR0           = 0xf8,
	IBCR2           = 0xf9,
	IBIMR0          = 0xfa,
	IBISR0          = 0xfb,
	FuncForceEvent	= 0xfc,
};

enum rtl8168_8101_registers {
	CSIDR			= 0x64,
	CSIAR			= 0x68,
#define	CSIAR_FLAG			0x80000000
#define	CSIAR_WRITE_CMD			0x80000000
#define	CSIAR_BYTE_ENABLE		0x0000f000
#define	CSIAR_ADDR_MASK			0x00000fff
	PMCH			= 0x6f,
#define D3COLD_NO_PLL_DOWN		BIT(7)
#define D3HOT_NO_PLL_DOWN		BIT(6)
#define D3_NO_PLL_DOWN			(BIT(7) | BIT(6))
	EPHYAR			= 0x80,
#define	EPHYAR_FLAG			0x80000000
#define	EPHYAR_WRITE_CMD		0x80000000
#define	EPHYAR_REG_MASK			0x1f
#define	EPHYAR_REG_SHIFT		16
#define	EPHYAR_DATA_MASK		0xffff
	DLLPR			= 0xd0,
#define	PFM_EN				(1 << 6)
#define	TX_10M_PS_EN			(1 << 7)
	DBG_REG			= 0xd1,
#define	FIX_NAK_1			(1 << 4)
#define	FIX_NAK_2			(1 << 3)
	TWSI			= 0xd2,
	MCU			= 0xd3,
#define	NOW_IS_OOB			(1 << 7)
#define	TX_EMPTY			(1 << 5)
#define	RX_EMPTY			(1 << 4)
#define	RXTX_EMPTY			(TX_EMPTY | RX_EMPTY)
#define	EN_NDP				(1 << 3)
#define	EN_OOB_RESET			(1 << 2)
#define	LINK_LIST_RDY			(1 << 1)
	EFUSEAR			= 0xdc,
#define	EFUSEAR_FLAG			0x80000000
#define	EFUSEAR_WRITE_CMD		0x80000000
#define	EFUSEAR_READ_CMD		0x00000000
#define	EFUSEAR_REG_MASK		0x03ff
#define	EFUSEAR_REG_SHIFT		8
#define	EFUSEAR_DATA_MASK		0xff
	MISC_1			= 0xf2,
#define	PFM_D3COLD_EN			(1 << 6)
};

enum rtl8168_registers {
	LED_FREQ		= 0x1a,
	EEE_LED			= 0x1b,
	ERIDR			= 0x70,
	ERIAR			= 0x74,
#define ERIAR_FLAG			0x80000000
#define ERIAR_WRITE_CMD			0x80000000
#define ERIAR_READ_CMD			0x00000000
#define ERIAR_ADDR_BYTE_ALIGN		4
#define ERIAR_TYPE_SHIFT		16
#define ERIAR_EXGMAC			(0x00 << ERIAR_TYPE_SHIFT)
#define ERIAR_MSIX			(0x01 << ERIAR_TYPE_SHIFT)
#define ERIAR_ASF			(0x02 << ERIAR_TYPE_SHIFT)
#define ERIAR_OOB			(0x02 << ERIAR_TYPE_SHIFT)
#define ERIAR_MASK_SHIFT		12
#define ERIAR_MASK_0001			(0x1 << ERIAR_MASK_SHIFT)
#define ERIAR_MASK_0011			(0x3 << ERIAR_MASK_SHIFT)
#define ERIAR_MASK_0100			(0x4 << ERIAR_MASK_SHIFT)
#define ERIAR_MASK_0101			(0x5 << ERIAR_MASK_SHIFT)
#define ERIAR_MASK_1111			(0xf << ERIAR_MASK_SHIFT)
	EPHY_RXER_NUM		= 0x7c,
	OCPDR			= 0xb0,	/* OCP GPHY access */
#define OCPDR_WRITE_CMD			0x80000000
#define OCPDR_READ_CMD			0x00000000
#define OCPDR_REG_MASK			0x7f
#define OCPDR_GPHY_REG_SHIFT		16
#define OCPDR_DATA_MASK			0xffff
	OCPAR			= 0xb4,
#define OCPAR_FLAG			0x80000000
#define OCPAR_GPHY_WRITE_CMD		0x8000f060
#define OCPAR_GPHY_READ_CMD		0x0000f060
	GPHY_OCP		= 0xb8,
	RDSAR1			= 0xd0,	/* 8168c only. Undocumented on 8168dp */
	MISC			= 0xf0,	/* 8168e only. */
#define TXPLA_RST			(1 << 29)
#define DISABLE_LAN_EN			(1 << 23) /* Enable GPIO pin */
#define PWM_EN				(1 << 22)
#define RXDV_GATED_EN			(1 << 19)
#define EARLY_TALLY_EN			(1 << 16)
};

enum rtl8125_registers {
	IntrMask_8125		= 0x38,
	IntrStatus_8125		= 0x3c,
	TxPoll_8125		= 0x90,
	MAC0_BKP		= 0x19e0,
	EEE_TXIDLE_TIMER_8125	= 0x6048,
};

#define RX_VLAN_INNER_8125	BIT(22)
#define RX_VLAN_OUTER_8125	BIT(23)
#define RX_VLAN_8125		(RX_VLAN_INNER_8125 | RX_VLAN_OUTER_8125)

#define RX_FETCH_DFLT_8125	(8 << 27)

enum rtl_register_content {
	/* InterruptStatusBits */
	SYSErr		= 0x8000,
	PCSTimeout	= 0x4000,
	SWInt		= 0x0100,
	TxDescUnavail	= 0x0080,
	RxFIFOOver	= 0x0040,
	LinkChg		= 0x0020,
	RxOverflow	= 0x0010,
	TxErr		= 0x0008,
	TxOK		= 0x0004,
	RxErr		= 0x0002,
	RxOK		= 0x0001,

	/* RxStatusDesc */
	RxRWT	= (1 << 22),
	RxRES	= (1 << 21),
	RxRUNT	= (1 << 20),
	RxCRC	= (1 << 19),

	/* ChipCmdBits */
	StopReq		= 0x80,
	CmdReset	= 0x10,
	CmdRxEnb	= 0x08,
	CmdTxEnb	= 0x04,
	RxBufEmpty	= 0x01,

	/* TXPoll register p.5 */
	HPQ		= 0x80,		/* Poll cmd on the high prio queue */
	NPQ		= 0x40,		/* Poll cmd on the low prio queue */
	FSWInt		= 0x01,		/* Forced software interrupt */

	/* Cfg9346Bits */
	Cfg9346_Lock	= 0x00,
	Cfg9346_Unlock	= 0xc0,

	/* rx_mode_bits */
	AcceptErr	= 0x20,
	AcceptRunt	= 0x10,
#define RX_CONFIG_ACCEPT_ERR_MASK	0x30
	AcceptBroadcast	= 0x08,
	AcceptMulticast	= 0x04,
	AcceptMyPhys	= 0x02,
	AcceptAllPhys	= 0x01,
#define RX_CONFIG_ACCEPT_OK_MASK	0x0f
#define RX_CONFIG_ACCEPT_MASK		0x3f

	/* TxConfigBits */
	TxInterFrameGapShift = 24,
	TxDMAShift = 8,	/* DMA burst value (0-7) is shift this many bits */

	/* Config1 register p.24 */
	LEDS1		= (1 << 7),
	LEDS0		= (1 << 6),
	Speed_down	= (1 << 4),
	MEMMAP		= (1 << 3),
	IOMAP		= (1 << 2),
	VPD		= (1 << 1),
	PMEnable	= (1 << 0),	/* Power Management Enable */

	/* Config2 register p. 25 */
	ClkReqEn	= (1 << 7),	/* Clock Request Enable */
	MSIEnable	= (1 << 5),	/* 8169 only. Reserved in the 8168. */
	PCI_Clock_66MHz = 0x01,
	PCI_Clock_33MHz = 0x00,

	/* Config3 register p.25 */
	MagicPacket	= (1 << 5),	/* Wake up when receives a Magic Packet */
	LinkUp		= (1 << 4),	/* Wake up when the cable connection is re-established */
	Jumbo_En0	= (1 << 2),	/* 8168 only. Reserved in the 8168b */
	Rdy_to_L23	= (1 << 1),	/* L23 Enable */
	Beacon_en	= (1 << 0),	/* 8168 only. Reserved in the 8168b */

	/* Config4 register */
	Jumbo_En1	= (1 << 1),	/* 8168 only. Reserved in the 8168b */

	/* Config5 register p.27 */
	BWF		= (1 << 6),	/* Accept Broadcast wakeup frame */
	MWF		= (1 << 5),	/* Accept Multicast wakeup frame */
	UWF		= (1 << 4),	/* Accept Unicast wakeup frame */
	Spi_en		= (1 << 3),
	LanWake		= (1 << 1),	/* LanWake enable/disable */
	PMEStatus	= (1 << 0),	/* PME status can be reset by PCI RST# */
	ASPM_en		= (1 << 0),	/* ASPM enable */

	/* CPlusCmd p.31 */
	EnableBist	= (1 << 15),	// 8168 8101
	Mac_dbgo_oe	= (1 << 14),	// 8168 8101
	EnAnaPLL	= (1 << 14),	// 8169
	Normal_mode	= (1 << 13),	// unused
	Force_half_dup	= (1 << 12),	// 8168 8101
	Force_rxflow_en	= (1 << 11),	// 8168 8101
	Force_txflow_en	= (1 << 10),	// 8168 8101
	Cxpl_dbg_sel	= (1 << 9),	// 8168 8101
	ASF		= (1 << 8),	// 8168 8101
	PktCntrDisable	= (1 << 7),	// 8168 8101
	Mac_dbgo_sel	= 0x001c,	// 8168
	RxVlan		= (1 << 6),
	RxChkSum	= (1 << 5),
	PCIDAC		= (1 << 4),
	PCIMulRW	= (1 << 3),
#define INTT_MASK	GENMASK(1, 0)
#define CPCMD_MASK	(Normal_mode | RxVlan | RxChkSum | INTT_MASK)

	/* rtl8169_PHYstatus */
	TBI_Enable	= 0x80,
	TxFlowCtrl	= 0x40,
	RxFlowCtrl	= 0x20,
	_1000bpsF	= 0x10,
	_100bps		= 0x08,
	_10bps		= 0x04,
	LinkStatus	= 0x02,
	FullDup		= 0x01,

	/* ResetCounterCommand */
	CounterReset	= 0x1,

	/* DumpCounterCommand */
	CounterDump	= 0x8,

	/* magic enable v2 */
	MagicPacket_v2	= (1 << 16),	/* Wake up when receives a Magic Packet */
};

enum rtl_desc_bit {
	/* First doubleword. */
	DescOwn		= (1 << 31), /* Descriptor is owned by NIC */
	RingEnd		= (1 << 30), /* End of descriptor ring */
	FirstFrag	= (1 << 29), /* First segment of a packet */
	LastFrag	= (1 << 28), /* Final segment of a packet */
};

/* Generic case. */
enum rtl_tx_desc_bit {
	/* First doubleword. */
	TD_LSO		= (1 << 27),		/* Large Send Offload */
#define TD_MSS_MAX			0x07ffu	/* MSS value */

	/* Second doubleword. */
	TxVlanTag	= (1 << 17),		/* Add VLAN tag */
};

/* 8169, 8168b and 810x except 8102e. */
enum rtl_tx_desc_bit_0 {
	/* First doubleword. */
#define TD0_MSS_SHIFT			16	/* MSS position (11 bits) */
	TD0_TCP_CS	= (1 << 16),		/* Calculate TCP/IP checksum */
	TD0_UDP_CS	= (1 << 17),		/* Calculate UDP/IP checksum */
	TD0_IP_CS	= (1 << 18),		/* Calculate IP checksum */
};

/* 8102e, 8168c and beyond. */
enum rtl_tx_desc_bit_1 {
	/* First doubleword. */
	TD1_GTSENV4	= (1 << 26),		/* Giant Send for IPv4 */
	TD1_GTSENV6	= (1 << 25),		/* Giant Send for IPv6 */
#define GTTCPHO_SHIFT			18
#define GTTCPHO_MAX			0x7f

	/* Second doubleword. */
#define TCPHO_SHIFT			18
#define TCPHO_MAX			0x3ff
#define TD1_MSS_SHIFT			18	/* MSS position (11 bits) */
	TD1_IPv6_CS	= (1 << 28),		/* Calculate IPv6 checksum */
	TD1_IPv4_CS	= (1 << 29),		/* Calculate IPv4 checksum */
	TD1_TCP_CS	= (1 << 30),		/* Calculate TCP/IP checksum */
	TD1_UDP_CS	= (1 << 31),		/* Calculate UDP/IP checksum */
};

enum rtl_rx_desc_bit {
	/* Rx private */
	PID1		= (1 << 18), /* Protocol ID bit 1/2 */
	PID0		= (1 << 17), /* Protocol ID bit 0/2 */

#define RxProtoUDP	(PID1)
#define RxProtoTCP	(PID0)
#define RxProtoIP	(PID1 | PID0)
#define RxProtoMask	RxProtoIP

	IPFail		= (1 << 16), /* IP checksum failed */
	UDPFail		= (1 << 15), /* UDP/IP checksum failed */
	TCPFail		= (1 << 14), /* TCP/IP checksum failed */

#define RxCSFailMask	(IPFail | UDPFail | TCPFail)

	RxVlanTag	= (1 << 16), /* VLAN tag available */
};

#define RTL_GSO_MAX_SIZE_V1	32000
#define RTL_GSO_MAX_SEGS_V1	24
#define RTL_GSO_MAX_SIZE_V2	64000
#define RTL_GSO_MAX_SEGS_V2	64

struct TxDesc {
	__le32 opts1;
	__le32 opts2;
	__le64 addr;
};

struct RxDesc {
	__le32 opts1;
	__le32 opts2;
	__le64 addr;
};

struct ring_info {
	struct sk_buff	*skb;
	u32		len;
};

struct rtl8169_counters {
	__le64	tx_packets;
	__le64	rx_packets;
	__le64	tx_errors;
	__le32	rx_errors;
	__le16	rx_missed;
	__le16	align_errors;
	__le32	tx_one_collision;
	__le32	tx_multi_collision;
	__le64	rx_unicast;
	__le64	rx_broadcast;
	__le32	rx_multicast;
	__le16	tx_aborted;
	__le16	tx_underun;
};

struct rtl8169_tc_offsets {
	bool	inited;
	__le64	tx_errors;
	__le32	tx_multi_collision;
	__le16	tx_aborted;
	__le16	rx_missed;
};

enum rtl_flag {
	RTL_FLAG_TASK_ENABLED = 0,
	RTL_FLAG_TASK_RESET_PENDING,
	RTL_FLAG_TASK_TX_TIMEOUT,
	RTL_FLAG_MAX
};

enum rtl_dash_type {
	RTL_DASH_NONE,
	RTL_DASH_DP,
	RTL_DASH_EP,
};

struct rtl8169_private {
	void __iomem *mmio_addr;	/* memory map physical address */
	struct pci_dev *pci_dev;
	struct device *dev;
	struct eth_device edev;
	struct phy_device *phydev;
	enum mac_version mac_version;
	enum rtl_dash_type dash_type;
	u32 cur_rx; /* Index into the Rx descriptor buffer of next Rx pkt. */
	u32 cur_tx; /* Index into the Tx descriptor buffer of next Rx pkt. */
	u32 dirty_tx;
	struct TxDesc *TxDescArray;	/* 256-aligned Tx descriptor ring */
	struct RxDesc *RxDescArray;	/* 256-aligned Rx descriptor ring */
	dma_addr_t TxPhyAddr;
	dma_addr_t RxPhyAddr;
	u16 cp_cmd;
	void			*tx_buf;
	dma_addr_t		tx_buf_phys;
	void			*rx_buf;
	dma_addr_t		rx_buf_phys;

	unsigned supports_gmii:1;
	unsigned aspm_manageable:1;

	const char *fw_name;
	struct rtl_fw *rtl_fw;

	u32 ocp_base;

	struct mii_bus miibus;
};

typedef void (*rtl_generic_fct)(struct rtl8169_private *tp);

static inline struct device *tp_to_dev(struct rtl8169_private *tp)
{
	return &tp->pci_dev->dev;
}

static void rtl_lock_config_regs(struct rtl8169_private *tp)
{
	RTL_W8(tp, Cfg9346, Cfg9346_Lock);
}

static void rtl_unlock_config_regs(struct rtl8169_private *tp)
{
	RTL_W8(tp, Cfg9346, Cfg9346_Unlock);
}

static void rtl_pci_commit(struct rtl8169_private *tp)
{
	/* Read an arbitrary register to commit a preceding PCI write */
	RTL_R8(tp, ChipCmd);
}

static bool rtl_is_8125(struct rtl8169_private *tp)
{
	return tp->mac_version >= RTL_GIGA_MAC_VER_61;
}

static bool rtl_is_8168evl_up(struct rtl8169_private *tp)
{
	return tp->mac_version >= RTL_GIGA_MAC_VER_34 &&
	       tp->mac_version != RTL_GIGA_MAC_VER_39 &&
	       tp->mac_version <= RTL_GIGA_MAC_VER_53;
}

static void rtl_read_mac_from_reg(struct rtl8169_private *tp, u8 *mac, int reg)
{
	int i;

	for (i = 0; i < ETH_ALEN; i++)
		mac[i] = RTL_R8(tp, reg + i);
}

struct rtl_cond {
	bool (*check)(struct rtl8169_private *);
	const char *msg;
};

static bool rtl_loop_wait(struct rtl8169_private *tp, const struct rtl_cond *c,
			  unsigned long usecs, int n, bool high)
{
	int i;

	for (i = 0; i < n; i++) {
		if (c->check(tp) == high)
			return true;
		udelay(usecs);
	}

	dev_err(tp->dev, "%s == %d (loop: %d, delay: %lu).\n",
		   c->msg, !high, n, usecs);
	return false;
}

static bool rtl_loop_wait_high(struct rtl8169_private *tp,
			       const struct rtl_cond *c,
			       unsigned long d, int n)
{
	return rtl_loop_wait(tp, c, d, n, true);
}

static bool rtl_loop_wait_low(struct rtl8169_private *tp,
			      const struct rtl_cond *c,
			      unsigned long d, int n)
{
	return rtl_loop_wait(tp, c, d, n, false);
}

#define DECLARE_RTL_COND(name)				\
static bool name ## _check(struct rtl8169_private *);	\
							\
static const struct rtl_cond name = {			\
	.check	= name ## _check,			\
	.msg	= #name					\
};							\
							\
static bool name ## _check(struct rtl8169_private *tp)

static void r8168fp_adjust_ocp_cmd(struct rtl8169_private *tp, u32 *cmd, int type)
{
	/* based on RTL8168FP_OOBMAC_BASE in vendor driver */
	if (type == ERIAR_OOB &&
	    (tp->mac_version == RTL_GIGA_MAC_VER_52 ||
	     tp->mac_version == RTL_GIGA_MAC_VER_53))
		*cmd |= 0xf70 << 18;
}

DECLARE_RTL_COND(rtl_eriar_cond)
{
	return RTL_R32(tp, ERIAR) & ERIAR_FLAG;
}

static void _rtl_eri_write(struct rtl8169_private *tp, int addr, u32 mask,
			   u32 val, int type)
{
	u32 cmd = ERIAR_WRITE_CMD | type | mask | addr;

	if (WARN(addr & 3 || !mask, "addr: 0x%x, mask: 0x%08x\n", addr, mask))
		return;

	RTL_W32(tp, ERIDR, val);
	r8168fp_adjust_ocp_cmd(tp, &cmd, type);
	RTL_W32(tp, ERIAR, cmd);

	rtl_loop_wait_low(tp, &rtl_eriar_cond, 100, 100);
}

static void rtl_eri_write(struct rtl8169_private *tp, int addr, u32 mask,
			  u32 val)
{
	_rtl_eri_write(tp, addr, mask, val, ERIAR_EXGMAC);
}

static u32 _rtl_eri_read(struct rtl8169_private *tp, int addr, int type)
{
	u32 cmd = ERIAR_READ_CMD | type | ERIAR_MASK_1111 | addr;

	r8168fp_adjust_ocp_cmd(tp, &cmd, type);
	RTL_W32(tp, ERIAR, cmd);

	return rtl_loop_wait_high(tp, &rtl_eriar_cond, 100, 100) ?
		RTL_R32(tp, ERIDR) : ~0;
}

static u32 rtl_eri_read(struct rtl8169_private *tp, int addr)
{
	return _rtl_eri_read(tp, addr, ERIAR_EXGMAC);
}

static void rtl_w0w1_eri(struct rtl8169_private *tp, int addr, u32 p, u32 m)
{
	u32 val = rtl_eri_read(tp, addr);

	rtl_eri_write(tp, addr, ERIAR_MASK_1111, (val & ~m) | p);
}

static void rtl_eri_set_bits(struct rtl8169_private *tp, int addr, u32 p)
{
	rtl_w0w1_eri(tp, addr, p, 0);
}

static void rtl_eri_clear_bits(struct rtl8169_private *tp, int addr, u32 m)
{
	rtl_w0w1_eri(tp, addr, 0, m);
}

static bool rtl_ocp_reg_failure(u32 reg)
{
	return WARN_ONCE(reg & 0xffff0001, "Invalid ocp reg %x!\n", reg);
}

DECLARE_RTL_COND(rtl_ocp_gphy_cond)
{
	return RTL_R32(tp, GPHY_OCP) & OCPAR_FLAG;
}

static void r8168_phy_ocp_write(struct rtl8169_private *tp, u32 reg, u32 data)
{
	if (rtl_ocp_reg_failure(reg))
		return;

	RTL_W32(tp, GPHY_OCP, OCPAR_FLAG | (reg << 15) | data);

	rtl_loop_wait_low(tp, &rtl_ocp_gphy_cond, 25, 10);
}

static int r8168_phy_ocp_read(struct rtl8169_private *tp, u32 reg)
{
	if (rtl_ocp_reg_failure(reg))
		return 0;

	RTL_W32(tp, GPHY_OCP, reg << 15);

	return rtl_loop_wait_high(tp, &rtl_ocp_gphy_cond, 25, 10) ?
		(RTL_R32(tp, GPHY_OCP) & 0xffff) : -ETIMEDOUT;
}

static void r8168_mac_ocp_write(struct rtl8169_private *tp, u32 reg, u32 data)
{
	if (rtl_ocp_reg_failure(reg))
		return;

	RTL_W32(tp, OCPDR, OCPAR_FLAG | (reg << 15) | data);
}

static u16 r8168_mac_ocp_read(struct rtl8169_private *tp, u32 reg)
{
	if (rtl_ocp_reg_failure(reg))
		return 0;

	RTL_W32(tp, OCPDR, reg << 15);

	return RTL_R32(tp, OCPDR);
}

static void r8168_mac_ocp_modify(struct rtl8169_private *tp, u32 reg, u16 mask,
				 u16 set)
{
	u16 data = r8168_mac_ocp_read(tp, reg);

	r8168_mac_ocp_write(tp, reg, (data & ~mask) | set);
}

/* Work around a hw issue with RTL8168g PHY, the quirk disables
 * PHY MCU interrupts before PHY power-down.
 */
static void rtl8168g_phy_suspend_quirk(struct rtl8169_private *tp, int value)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_40:
		if (value & BMCR_RESET || !(value & BMCR_PDOWN))
			rtl_eri_set_bits(tp, 0x1a8, 0xfc000000);
		else
			rtl_eri_clear_bits(tp, 0x1a8, 0xfc000000);
		break;
	default:
		break;
	}
};

static void r8168g_mdio_write(struct rtl8169_private *tp, int reg, int value)
{
	if (reg == 0x1f) {
		tp->ocp_base = value ? value << 4 : OCP_STD_PHY_BASE;
		return;
	}

	if (tp->ocp_base != OCP_STD_PHY_BASE)
		reg -= 0x10;

	if (tp->ocp_base == OCP_STD_PHY_BASE && reg == MII_BMCR)
		rtl8168g_phy_suspend_quirk(tp, value);

	r8168_phy_ocp_write(tp, tp->ocp_base + reg * 2, value);
}

static int r8168g_mdio_read(struct rtl8169_private *tp, int reg)
{
	if (reg == 0x1f)
		return tp->ocp_base == OCP_STD_PHY_BASE ? 0 : tp->ocp_base >> 4;

	if (tp->ocp_base != OCP_STD_PHY_BASE)
		reg -= 0x10;

	return r8168_phy_ocp_read(tp, tp->ocp_base + reg * 2);
}

static void mac_mcu_write(struct rtl8169_private *tp, int reg, int value)
{
	if (reg == 0x1f) {
		tp->ocp_base = value << 4;
		return;
	}

	r8168_mac_ocp_write(tp, tp->ocp_base + reg, value);
}

static int mac_mcu_read(struct rtl8169_private *tp, int reg)
{
	return r8168_mac_ocp_read(tp, tp->ocp_base + reg);
}

DECLARE_RTL_COND(rtl_phyar_cond)
{
	return RTL_R32(tp, PHYAR) & 0x80000000;
}

static void r8169_mdio_write(struct rtl8169_private *tp, int reg, int value)
{
	RTL_W32(tp, PHYAR, 0x80000000 | (reg & 0x1f) << 16 | (value & 0xffff));

	rtl_loop_wait_low(tp, &rtl_phyar_cond, 25, 20);
	/*
	 * According to hardware specs a 20us delay is required after write
	 * complete indication, but before sending next command.
	 */
	udelay(20);
}

static int r8169_mdio_read(struct rtl8169_private *tp, int reg)
{
	int value;

	RTL_W32(tp, PHYAR, 0x0 | (reg & 0x1f) << 16);

	value = rtl_loop_wait_high(tp, &rtl_phyar_cond, 25, 20) ?
		RTL_R32(tp, PHYAR) & 0xffff : -ETIMEDOUT;

	/*
	 * According to hardware specs a 20us delay is required after read
	 * complete indication, but before sending next command.
	 */
	udelay(20);

	return value;
}

DECLARE_RTL_COND(rtl_ocpar_cond)
{
	return RTL_R32(tp, OCPAR) & OCPAR_FLAG;
}

#define R8168DP_1_MDIO_ACCESS_BIT	0x00020000

static void r8168dp_2_mdio_start(struct rtl8169_private *tp)
{
	RTL_W32(tp, 0xd0, RTL_R32(tp, 0xd0) & ~R8168DP_1_MDIO_ACCESS_BIT);
}

static void r8168dp_2_mdio_stop(struct rtl8169_private *tp)
{
	RTL_W32(tp, 0xd0, RTL_R32(tp, 0xd0) | R8168DP_1_MDIO_ACCESS_BIT);
}

static void r8168dp_2_mdio_write(struct rtl8169_private *tp, int reg, int value)
{
	r8168dp_2_mdio_start(tp);

	r8169_mdio_write(tp, reg, value);

	r8168dp_2_mdio_stop(tp);
}

static int r8168dp_2_mdio_read(struct rtl8169_private *tp, int reg)
{
	int value;

	/* Work around issue with chip reporting wrong PHY ID */
	if (reg == MII_PHYSID2)
		return 0xc912;

	r8168dp_2_mdio_start(tp);

	value = r8169_mdio_read(tp, reg);

	r8168dp_2_mdio_stop(tp);

	return value;
}

static void rtl_writephy(struct rtl8169_private *tp, int location, int val)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_28:
	case RTL_GIGA_MAC_VER_31:
		r8168dp_2_mdio_write(tp, location, val);
		break;
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_63:
		r8168g_mdio_write(tp, location, val);
		break;
	default:
		r8169_mdio_write(tp, location, val);
		break;
	}
}

static int rtl_readphy(struct rtl8169_private *tp, int location)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_28:
	case RTL_GIGA_MAC_VER_31:
		return r8168dp_2_mdio_read(tp, location);
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_63:
		return r8168g_mdio_read(tp, location);
	default:
		return r8169_mdio_read(tp, location);
	}
}

DECLARE_RTL_COND(rtl_ephyar_cond)
{
	return RTL_R32(tp, EPHYAR) & EPHYAR_FLAG;
}

static void rtl_ephy_write(struct rtl8169_private *tp, int reg_addr, int value)
{
	RTL_W32(tp, EPHYAR, EPHYAR_WRITE_CMD | (value & EPHYAR_DATA_MASK) |
		(reg_addr & EPHYAR_REG_MASK) << EPHYAR_REG_SHIFT);

	rtl_loop_wait_low(tp, &rtl_ephyar_cond, 10, 100);

	udelay(10);
}

static u16 rtl_ephy_read(struct rtl8169_private *tp, int reg_addr)
{
	RTL_W32(tp, EPHYAR, (reg_addr & EPHYAR_REG_MASK) << EPHYAR_REG_SHIFT);

	return rtl_loop_wait_high(tp, &rtl_ephyar_cond, 10, 100) ?
		RTL_R32(tp, EPHYAR) & EPHYAR_DATA_MASK : ~0;
}

static u32 r8168dp_ocp_read(struct rtl8169_private *tp, u16 reg)
{
	RTL_W32(tp, OCPAR, 0x0fu << 12 | (reg & 0x0fff));
	return rtl_loop_wait_high(tp, &rtl_ocpar_cond, 100, 20) ?
		RTL_R32(tp, OCPDR) : ~0;
}

static u32 r8168ep_ocp_read(struct rtl8169_private *tp, u16 reg)
{
	return _rtl_eri_read(tp, reg, ERIAR_OOB);
}

static void r8168dp_ocp_write(struct rtl8169_private *tp, u8 mask, u16 reg,
			      u32 data)
{
	RTL_W32(tp, OCPDR, data);
	RTL_W32(tp, OCPAR, OCPAR_FLAG | ((u32)mask & 0x0f) << 12 | (reg & 0x0fff));
	rtl_loop_wait_low(tp, &rtl_ocpar_cond, 100, 20);
}

static void r8168ep_ocp_write(struct rtl8169_private *tp, u8 mask, u16 reg,
			      u32 data)
{
	_rtl_eri_write(tp, reg, ((u32)mask & 0x0f) << ERIAR_MASK_SHIFT,
		       data, ERIAR_OOB);
}

static void r8168dp_oob_notify(struct rtl8169_private *tp, u8 cmd)
{
	rtl_eri_write(tp, 0xe8, ERIAR_MASK_0001, cmd);

	r8168dp_ocp_write(tp, 0x1, 0x30, 0x00000001);
}

#define OOB_CMD_RESET		0x00
#define OOB_CMD_DRIVER_START	0x05
#define OOB_CMD_DRIVER_STOP	0x06

static u16 rtl8168_get_ocp_reg(struct rtl8169_private *tp)
{
	return (tp->mac_version == RTL_GIGA_MAC_VER_31) ? 0xb8 : 0x10;
}

DECLARE_RTL_COND(rtl_dp_ocp_read_cond)
{
	u16 reg;

	reg = rtl8168_get_ocp_reg(tp);

	return r8168dp_ocp_read(tp, reg) & 0x00000800;
}

DECLARE_RTL_COND(rtl_ep_ocp_read_cond)
{
	return r8168ep_ocp_read(tp, 0x124) & 0x00000001;
}

DECLARE_RTL_COND(rtl_ocp_tx_cond)
{
	return RTL_R8(tp, IBISR0) & 0x20;
}

static void rtl8168ep_stop_cmac(struct rtl8169_private *tp)
{
	RTL_W8(tp, IBCR2, RTL_R8(tp, IBCR2) & ~0x01);
	rtl_loop_wait_high(tp, &rtl_ocp_tx_cond, 50000, 2000);
	RTL_W8(tp, IBISR0, RTL_R8(tp, IBISR0) | 0x20);
	RTL_W8(tp, IBCR0, RTL_R8(tp, IBCR0) & ~0x01);
}

static void rtl8168dp_driver_start(struct rtl8169_private *tp)
{
	r8168dp_oob_notify(tp, OOB_CMD_DRIVER_START);
	rtl_loop_wait_high(tp, &rtl_dp_ocp_read_cond, 10000, 10);
}

static void rtl8168ep_driver_start(struct rtl8169_private *tp)
{
	r8168ep_ocp_write(tp, 0x01, 0x180, OOB_CMD_DRIVER_START);
	r8168ep_ocp_write(tp, 0x01, 0x30, r8168ep_ocp_read(tp, 0x30) | 0x01);
	rtl_loop_wait_high(tp, &rtl_ep_ocp_read_cond, 10000, 10);
}

static void rtl8168_driver_start(struct rtl8169_private *tp)
{
	if (tp->dash_type == RTL_DASH_DP)
		rtl8168dp_driver_start(tp);
	else
		rtl8168ep_driver_start(tp);
}

static bool r8168dp_check_dash(struct rtl8169_private *tp)
{
	u16 reg = rtl8168_get_ocp_reg(tp);

	return r8168dp_ocp_read(tp, reg) & BIT(15);
}

static bool r8168ep_check_dash(struct rtl8169_private *tp)
{
	return r8168ep_ocp_read(tp, 0x128) & BIT(0);
}

static enum rtl_dash_type rtl_check_dash(struct rtl8169_private *tp)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_28:
	case RTL_GIGA_MAC_VER_31:
		return r8168dp_check_dash(tp) ? RTL_DASH_DP : RTL_DASH_NONE;
	case RTL_GIGA_MAC_VER_51 ... RTL_GIGA_MAC_VER_53:
		return r8168ep_check_dash(tp) ? RTL_DASH_EP : RTL_DASH_NONE;
	default:
		return RTL_DASH_NONE;
	}
}

static void rtl_set_d3_pll_down(struct rtl8169_private *tp, bool enable)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_25 ... RTL_GIGA_MAC_VER_26:
	case RTL_GIGA_MAC_VER_29 ... RTL_GIGA_MAC_VER_30:
	case RTL_GIGA_MAC_VER_32 ... RTL_GIGA_MAC_VER_37:
	case RTL_GIGA_MAC_VER_39 ... RTL_GIGA_MAC_VER_63:
		if (enable)
			RTL_W8(tp, PMCH, RTL_R8(tp, PMCH) & ~D3_NO_PLL_DOWN);
		else
			RTL_W8(tp, PMCH, RTL_R8(tp, PMCH) | D3_NO_PLL_DOWN);
		break;
	default:
		break;
	}
}

static void rtl_reset_packet_filter(struct rtl8169_private *tp)
{
	rtl_eri_clear_bits(tp, 0xdc, BIT(0));
	rtl_eri_set_bits(tp, 0xdc, BIT(0));
}

DECLARE_RTL_COND(rtl_efusear_cond)
{
	return RTL_R32(tp, EFUSEAR) & EFUSEAR_FLAG;
}

u8 rtl8168d_efuse_read(struct rtl8169_private *tp, int reg_addr)
{
	RTL_W32(tp, EFUSEAR, (reg_addr & EFUSEAR_REG_MASK) << EFUSEAR_REG_SHIFT);

	return rtl_loop_wait_high(tp, &rtl_efusear_cond, 100, 300) ?
		RTL_R32(tp, EFUSEAR) & EFUSEAR_DATA_MASK : ~0;
}

static void rtl_link_chg_patch(struct rtl8169_private *tp)
{
	struct phy_device *phydev = tp->phydev;

	if (tp->mac_version == RTL_GIGA_MAC_VER_34 ||
	    tp->mac_version == RTL_GIGA_MAC_VER_38) {
		if (phydev->speed == SPEED_1000) {
			rtl_eri_write(tp, 0x1bc, ERIAR_MASK_1111, 0x00000011);
			rtl_eri_write(tp, 0x1dc, ERIAR_MASK_1111, 0x00000005);
		} else if (phydev->speed == SPEED_100) {
			rtl_eri_write(tp, 0x1bc, ERIAR_MASK_1111, 0x0000001f);
			rtl_eri_write(tp, 0x1dc, ERIAR_MASK_1111, 0x00000005);
		} else {
			rtl_eri_write(tp, 0x1bc, ERIAR_MASK_1111, 0x0000001f);
			rtl_eri_write(tp, 0x1dc, ERIAR_MASK_1111, 0x0000003f);
		}
		rtl_reset_packet_filter(tp);
	} else if (tp->mac_version == RTL_GIGA_MAC_VER_35 ||
		   tp->mac_version == RTL_GIGA_MAC_VER_36) {
		if (phydev->speed == SPEED_1000) {
			rtl_eri_write(tp, 0x1bc, ERIAR_MASK_1111, 0x00000011);
			rtl_eri_write(tp, 0x1dc, ERIAR_MASK_1111, 0x00000005);
		} else {
			rtl_eri_write(tp, 0x1bc, ERIAR_MASK_1111, 0x0000001f);
			rtl_eri_write(tp, 0x1dc, ERIAR_MASK_1111, 0x0000003f);
		}
	} else if (tp->mac_version == RTL_GIGA_MAC_VER_37) {
		if (phydev->speed == SPEED_10) {
			rtl_eri_write(tp, 0x1d0, ERIAR_MASK_0011, 0x4d02);
			rtl_eri_write(tp, 0x1dc, ERIAR_MASK_0011, 0x0060a);
		} else {
			rtl_eri_write(tp, 0x1d0, ERIAR_MASK_0011, 0x0000);
		}
	}
}

static enum mac_version rtl8169_get_mac_version(u16 xid, bool gmii)
{
	/*
	 * The driver currently handles the 8168Bf and the 8168Be identically
	 * but they can be identified more specifically through the test below
	 * if needed:
	 *
	 * (RTL_R32(tp, TxConfig) & 0x700000) == 0x500000 ? 8168Bf : 8168Be
	 *
	 * Same thing for the 8101Eb and the 8101Ec:
	 *
	 * (RTL_R32(tp, TxConfig) & 0x700000) == 0x200000 ? 8101Eb : 8101Ec
	 */
	static const struct rtl_mac_info {
		u16 mask;
		u16 val;
		enum mac_version ver;
	} mac_info[] = {
		/* 8125B family. */
		{ 0x7cf, 0x641,	RTL_GIGA_MAC_VER_63 },

		/* 8125A family. */
		{ 0x7cf, 0x609,	RTL_GIGA_MAC_VER_61 },
		/* It seems only XID 609 made it to the mass market.
		 * { 0x7cf, 0x608,	RTL_GIGA_MAC_VER_60 },
		 * { 0x7c8, 0x608,	RTL_GIGA_MAC_VER_61 },
		 */

		/* RTL8117 */
		{ 0x7cf, 0x54b,	RTL_GIGA_MAC_VER_53 },
		{ 0x7cf, 0x54a,	RTL_GIGA_MAC_VER_52 },

		/* 8168EP family. */
		{ 0x7cf, 0x502,	RTL_GIGA_MAC_VER_51 },
		/* It seems this chip version never made it to
		 * the wild. Let's disable detection.
		 * { 0x7cf, 0x501,      RTL_GIGA_MAC_VER_50 },
		 * { 0x7cf, 0x500,      RTL_GIGA_MAC_VER_49 },
		 */

		/* 8168H family. */
		{ 0x7cf, 0x541,	RTL_GIGA_MAC_VER_46 },
		/* It seems this chip version never made it to
		 * the wild. Let's disable detection.
		 * { 0x7cf, 0x540,	RTL_GIGA_MAC_VER_45 },
		 */

		/* 8168G family. */
		{ 0x7cf, 0x5c8,	RTL_GIGA_MAC_VER_44 },
		{ 0x7cf, 0x509,	RTL_GIGA_MAC_VER_42 },
		/* It seems this chip version never made it to
		 * the wild. Let's disable detection.
		 * { 0x7cf, 0x4c1,	RTL_GIGA_MAC_VER_41 },
		 */
		{ 0x7cf, 0x4c0,	RTL_GIGA_MAC_VER_40 },

		/* 8168F family. */
		{ 0x7c8, 0x488,	RTL_GIGA_MAC_VER_38 },
		{ 0x7cf, 0x481,	RTL_GIGA_MAC_VER_36 },
		{ 0x7cf, 0x480,	RTL_GIGA_MAC_VER_35 },

		/* 8168E family. */
		{ 0x7c8, 0x2c8,	RTL_GIGA_MAC_VER_34 },
		{ 0x7cf, 0x2c1,	RTL_GIGA_MAC_VER_32 },
		{ 0x7c8, 0x2c0,	RTL_GIGA_MAC_VER_33 },

		/* 8168D family. */
		{ 0x7cf, 0x281,	RTL_GIGA_MAC_VER_25 },
		{ 0x7c8, 0x280,	RTL_GIGA_MAC_VER_26 },

		/* 8168DP family. */
		/* It seems this early RTL8168dp version never made it to
		 * the wild. Support has been removed.
		 * { 0x7cf, 0x288,      RTL_GIGA_MAC_VER_27 },
		 */
		{ 0x7cf, 0x28a,	RTL_GIGA_MAC_VER_28 },
		{ 0x7cf, 0x28b,	RTL_GIGA_MAC_VER_31 },

		/* 8168C family. */
		{ 0x7cf, 0x3c9,	RTL_GIGA_MAC_VER_23 },
		{ 0x7cf, 0x3c8,	RTL_GIGA_MAC_VER_18 },
		{ 0x7c8, 0x3c8,	RTL_GIGA_MAC_VER_24 },
		{ 0x7cf, 0x3c0,	RTL_GIGA_MAC_VER_19 },
		{ 0x7cf, 0x3c2,	RTL_GIGA_MAC_VER_20 },
		{ 0x7cf, 0x3c3,	RTL_GIGA_MAC_VER_21 },
		{ 0x7c8, 0x3c0,	RTL_GIGA_MAC_VER_22 },

		/* 8168B family. */
		{ 0x7c8, 0x380,	RTL_GIGA_MAC_VER_17 },
		{ 0x7c8, 0x300,	RTL_GIGA_MAC_VER_11 },

		/* 8101 family. */
		{ 0x7c8, 0x448,	RTL_GIGA_MAC_VER_39 },
		{ 0x7c8, 0x440,	RTL_GIGA_MAC_VER_37 },
		{ 0x7cf, 0x409,	RTL_GIGA_MAC_VER_29 },
		{ 0x7c8, 0x408,	RTL_GIGA_MAC_VER_30 },
		{ 0x7cf, 0x349,	RTL_GIGA_MAC_VER_08 },
		{ 0x7cf, 0x249,	RTL_GIGA_MAC_VER_08 },
		{ 0x7cf, 0x348,	RTL_GIGA_MAC_VER_07 },
		{ 0x7cf, 0x248,	RTL_GIGA_MAC_VER_07 },
		{ 0x7cf, 0x240,	RTL_GIGA_MAC_VER_14 },
		{ 0x7c8, 0x348,	RTL_GIGA_MAC_VER_09 },
		{ 0x7c8, 0x248,	RTL_GIGA_MAC_VER_09 },
		{ 0x7c8, 0x340,	RTL_GIGA_MAC_VER_10 },

		/* 8110 family. */
		{ 0xfc8, 0x980,	RTL_GIGA_MAC_VER_06 },
		{ 0xfc8, 0x180,	RTL_GIGA_MAC_VER_05 },
		{ 0xfc8, 0x100,	RTL_GIGA_MAC_VER_04 },
		{ 0xfc8, 0x040,	RTL_GIGA_MAC_VER_03 },
		{ 0xfc8, 0x008,	RTL_GIGA_MAC_VER_02 },

		/* Catch-all */
		{ 0x000, 0x000,	RTL_GIGA_MAC_NONE   }
	};
	const struct rtl_mac_info *p = mac_info;
	enum mac_version ver;

	while ((xid & p->mask) != p->val)
		p++;
	ver = p->ver;

	if (ver != RTL_GIGA_MAC_NONE && !gmii) {
		if (ver == RTL_GIGA_MAC_VER_42)
			ver = RTL_GIGA_MAC_VER_43;
		else if (ver == RTL_GIGA_MAC_VER_46)
			ver = RTL_GIGA_MAC_VER_48;
	}

	return ver;
}

void r8169_apply_firmware(struct rtl8169_private *tp)
{
	int val;
	u64 start;

	/* TODO: release firmware if rtl_fw_write_firmware signals failure. */
	if (tp->rtl_fw) {
		rtl_fw_write_firmware(tp, tp->rtl_fw);
		/* At least one firmware doesn't reset tp->ocp_base. */
		tp->ocp_base = OCP_STD_PHY_BASE;

		/* PHY soft reset may still be in progress */
		start = get_time_ns();
		while (is_timeout(start, SECOND)) {
			val = phy_read(tp->phydev, MII_BMCR);
			if (!(val & BMCR_RESET))
				return;
		}
	}
}

static void rtl_rar_exgmac_set(struct rtl8169_private *tp, const u8 *addr)
{
	rtl_eri_write(tp, 0xe0, ERIAR_MASK_1111, get_unaligned_le32(addr));
	rtl_eri_write(tp, 0xe4, ERIAR_MASK_1111, get_unaligned_le16(addr + 4));
	rtl_eri_write(tp, 0xf0, ERIAR_MASK_1111, get_unaligned_le16(addr) << 16);
	rtl_eri_write(tp, 0xf4, ERIAR_MASK_1111, get_unaligned_le32(addr + 2));
}

u16 rtl8168h_2_get_adc_bias_ioffset(struct rtl8169_private *tp)
{
	u16 data1, data2, ioffset;

	r8168_mac_ocp_write(tp, 0xdd02, 0x807d);
	data1 = r8168_mac_ocp_read(tp, 0xdd02);
	data2 = r8168_mac_ocp_read(tp, 0xdd00);

	ioffset = (data2 >> 1) & 0x7ff8;
	ioffset |= data2 & 0x0007;
	if (data1 & BIT(7))
		ioffset |= BIT(15);

	return ioffset;
}

static void rtl8169_init_phy(struct rtl8169_private *tp)
{
	r8169_hw_phy_config(tp, tp->phydev, tp->mac_version);

	if (tp->mac_version <= RTL_GIGA_MAC_VER_06) {
		pci_write_config_byte(tp->pci_dev, PCI_LATENCY_TIMER, 0x40);
		pci_write_config_byte(tp->pci_dev, PCI_CACHE_LINE_SIZE, 0x08);
		/* set undocumented MAC Reg C+CR Offset 0x82h */
		RTL_W8(tp, 0x82, 0x01);
	}
}

static void rtl_rar_set(struct rtl8169_private *tp, const u8 *addr)
{
	rtl_unlock_config_regs(tp);

	RTL_W32(tp, MAC4, get_unaligned_le16(addr + 4));
	rtl_pci_commit(tp);

	RTL_W32(tp, MAC0, get_unaligned_le32(addr));
	rtl_pci_commit(tp);

	if (tp->mac_version == RTL_GIGA_MAC_VER_34)
		rtl_rar_exgmac_set(tp, addr);

	rtl_lock_config_regs(tp);
}

static void rtl_init_rxcfg(struct rtl8169_private *tp)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_02 ... RTL_GIGA_MAC_VER_06:
	case RTL_GIGA_MAC_VER_10 ... RTL_GIGA_MAC_VER_17:
		RTL_W32(tp, RxConfig, RX_FIFO_THRESH | RX_DMA_BURST);
		break;
	case RTL_GIGA_MAC_VER_18 ... RTL_GIGA_MAC_VER_24:
	case RTL_GIGA_MAC_VER_34 ... RTL_GIGA_MAC_VER_36:
	case RTL_GIGA_MAC_VER_38:
		RTL_W32(tp, RxConfig, RX128_INT_EN | RX_MULTI_EN | RX_DMA_BURST);
		break;
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_53:
		RTL_W32(tp, RxConfig, RX128_INT_EN | RX_MULTI_EN | RX_DMA_BURST | RX_EARLY_OFF);
		break;
	case RTL_GIGA_MAC_VER_61 ... RTL_GIGA_MAC_VER_63:
		RTL_W32(tp, RxConfig, RX_FETCH_DFLT_8125 | RX_DMA_BURST);
		break;
	default:
		RTL_W32(tp, RxConfig, RX128_INT_EN | RX_DMA_BURST);
		break;
	}
}

DECLARE_RTL_COND(rtl_chipcmd_cond)
{
	return RTL_R8(tp, ChipCmd) & CmdReset;
}

static void rtl_hw_reset(struct rtl8169_private *tp)
{
	RTL_W8(tp, ChipCmd, CmdReset);

	rtl_loop_wait_low(tp, &rtl_chipcmd_cond, 100, 100);
}

static void rtl_request_firmware(struct rtl8169_private *tp)
{
	struct rtl_fw *rtl_fw;

	/* firmware loaded already or no firmware available */
	if (tp->rtl_fw || !tp->fw_name)
		return;

	rtl_fw = kzalloc(sizeof(*rtl_fw), GFP_KERNEL);
	if (!rtl_fw)
		return;

	rtl_fw->phy_write = rtl_writephy;
	rtl_fw->phy_read = rtl_readphy;
	rtl_fw->mac_mcu_write = mac_mcu_write;
	rtl_fw->mac_mcu_read = mac_mcu_read;
	rtl_fw->fw_name = tp->fw_name;
	rtl_fw->dev = tp_to_dev(tp);

	if (rtl_fw_request_firmware(rtl_fw))
		kfree(rtl_fw);
	else
		tp->rtl_fw = rtl_fw;
}

static void rtl_rx_close(struct rtl8169_private *tp)
{
	RTL_W32(tp, RxConfig, RTL_R32(tp, RxConfig) & ~RX_CONFIG_ACCEPT_MASK);
}

DECLARE_RTL_COND(rtl_npq_cond)
{
	return RTL_R8(tp, TxPoll) & NPQ;
}

DECLARE_RTL_COND(rtl_txcfg_empty_cond)
{
	return RTL_R32(tp, TxConfig) & TXCFG_EMPTY;
}

DECLARE_RTL_COND(rtl_rxtx_empty_cond)
{
	return (RTL_R8(tp, MCU) & RXTX_EMPTY) == RXTX_EMPTY;
}

DECLARE_RTL_COND(rtl_rxtx_empty_cond_2)
{
	/* IntrMitigate has new functionality on RTL8125 */
	return (RTL_R16(tp, IntrMitigate) & 0x0103) == 0x0103;
}

static void rtl_wait_txrx_fifo_empty(struct rtl8169_private *tp)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_53:
		rtl_loop_wait_high(tp, &rtl_txcfg_empty_cond, 100, 42);
		rtl_loop_wait_high(tp, &rtl_rxtx_empty_cond, 100, 42);
		break;
	case RTL_GIGA_MAC_VER_61 ... RTL_GIGA_MAC_VER_61:
		rtl_loop_wait_high(tp, &rtl_rxtx_empty_cond, 100, 42);
		break;
	case RTL_GIGA_MAC_VER_63:
		RTL_W8(tp, ChipCmd, RTL_R8(tp, ChipCmd) | StopReq);
		rtl_loop_wait_high(tp, &rtl_rxtx_empty_cond, 100, 42);
		rtl_loop_wait_high(tp, &rtl_rxtx_empty_cond_2, 100, 42);
		break;
	default:
		break;
	}
}

static void rtl_disable_rxdvgate(struct rtl8169_private *tp)
{
	RTL_W32(tp, MISC, RTL_R32(tp, MISC) & ~RXDV_GATED_EN);
}

static void rtl_enable_rxdvgate(struct rtl8169_private *tp)
{
	RTL_W32(tp, MISC, RTL_R32(tp, MISC) | RXDV_GATED_EN);
	udelay(2000);
	rtl_wait_txrx_fifo_empty(tp);
}

static void rtl_set_tx_config_registers(struct rtl8169_private *tp)
{
	u32 val = TX_DMA_BURST << TxDMAShift |
		  InterFrameGap << TxInterFrameGapShift;

	if (rtl_is_8168evl_up(tp))
		val |= TXCFG_AUTO_FIFO;

	RTL_W32(tp, TxConfig, val);
}

static void rtl_set_rx_max_size(struct rtl8169_private *tp)
{
	/* Low hurts. Let's disable the filtering. */
	RTL_W16(tp, RxMaxSize, R8169_RX_BUF_SIZE + 1);
}

static void rtl_set_rx_tx_desc_registers(struct rtl8169_private *tp)
{
	/*
	 * Magic spell: some iop3xx ARM board needs the TxDescAddrHigh
	 * register to be written before TxDescAddrLow to work.
	 * Switching from MMIO to I/O access fixes the issue as well.
	 */
	RTL_W32(tp, TxDescStartAddrHigh, ((u64) tp->TxPhyAddr) >> 32);
	RTL_W32(tp, TxDescStartAddrLow, ((u64) tp->TxPhyAddr) & DMA_BIT_MASK(32));
	RTL_W32(tp, RxDescAddrHigh, ((u64) tp->RxPhyAddr) >> 32);
	RTL_W32(tp, RxDescAddrLow, ((u64) tp->RxPhyAddr) & DMA_BIT_MASK(32));
}

static void rtl8169_set_magic_reg(struct rtl8169_private *tp)
{
	u32 val;

	if (tp->mac_version == RTL_GIGA_MAC_VER_05)
		val = 0x000fff00;
	else if (tp->mac_version == RTL_GIGA_MAC_VER_06)
		val = 0x00ffff00;
	else
		return;

	if (RTL_R8(tp, Config2) & PCI_Clock_66MHz)
		val |= 0xff;

	RTL_W32(tp, 0x7c, val);
}

static void rtl_set_rx_mode(struct  rtl8169_private *tp)
{
	u32 rx_mode = AcceptBroadcast | AcceptMyPhys | AcceptMulticast;
	/* Multicast hash filter */
	u32 mc_filter[2] = { 0xffffffff, 0xffffffff };
	u32 tmp;

	RTL_W32(tp, MAR0 + 4, mc_filter[1]);
	RTL_W32(tp, MAR0 + 0, mc_filter[0]);

	tmp = RTL_R32(tp, RxConfig);
	RTL_W32(tp, RxConfig, (tmp & ~RX_CONFIG_ACCEPT_OK_MASK) | rx_mode);
}

DECLARE_RTL_COND(rtl_csiar_cond)
{
	return RTL_R32(tp, CSIAR) & CSIAR_FLAG;
}

static void rtl_csi_write(struct rtl8169_private *tp, int addr, int value)
{
	u32 func = PCI_FUNC(tp->pci_dev->devfn);

	RTL_W32(tp, CSIDR, value);
	RTL_W32(tp, CSIAR, CSIAR_WRITE_CMD | (addr & CSIAR_ADDR_MASK) |
		CSIAR_BYTE_ENABLE | func << 16);

	rtl_loop_wait_low(tp, &rtl_csiar_cond, 10, 100);
}

static u32 rtl_csi_read(struct rtl8169_private *tp, int addr)
{
	u32 func = PCI_FUNC(tp->pci_dev->devfn);

	RTL_W32(tp, CSIAR, (addr & CSIAR_ADDR_MASK) | func << 16 |
		CSIAR_BYTE_ENABLE);

	return rtl_loop_wait_high(tp, &rtl_csiar_cond, 10, 100) ?
		RTL_R32(tp, CSIDR) : ~0;
}

static void rtl_set_aspm_entry_latency(struct rtl8169_private *tp, u8 val)
{
	struct pci_dev *pdev = tp->pci_dev;
	u32 csi;

	/* According to Realtek the value at config space address 0x070f
	 * controls the L0s/L1 entrance latency. We try standard ECAM access
	 * first and if it fails fall back to CSI.
	 * bit 0..2: L0: 0 = 1us, 1 = 2us .. 6 = 7us, 7 = 7us (no typo)
	 * bit 3..5: L1: 0 = 1us, 1 = 2us .. 6 = 64us, 7 = 64us
	 */
	if (pci_write_config_byte(pdev, 0x070f, val) == PCIBIOS_SUCCESSFUL)
		return;

	dev_dbg(tp->dev,
		"No native access to PCI extended config space, falling back to CSI\n");
	csi = rtl_csi_read(tp, 0x070c) & 0x00ffffff;
	rtl_csi_write(tp, 0x070c, csi | val << 24);
}

static void rtl_set_def_aspm_entry_latency(struct rtl8169_private *tp)
{
	/* L0 7us, L1 16us */
	rtl_set_aspm_entry_latency(tp, 0x27);
}

struct ephy_info {
	unsigned int offset;
	u16 mask;
	u16 bits;
};

static void __rtl_ephy_init(struct rtl8169_private *tp,
			    const struct ephy_info *e, int len)
{
	u16 w;

	while (len-- > 0) {
		w = (rtl_ephy_read(tp, e->offset) & ~e->mask) | e->bits;
		rtl_ephy_write(tp, e->offset, w);
		e++;
	}
}

#define rtl_ephy_init(tp, a) __rtl_ephy_init(tp, a, ARRAY_SIZE(a))

static void rtl_disable_clock_request(struct rtl8169_private *tp)
{
	/* Not yet implemented */
}

static void rtl_enable_clock_request(struct rtl8169_private *tp)
{
	/* Not yet implemented */
}

static void rtl_pcie_state_l2l3_disable(struct rtl8169_private *tp)
{
	/* work around an issue when PCI reset occurs during L2/L3 state */
	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Rdy_to_L23);
}

static void rtl_enable_exit_l1(struct rtl8169_private *tp)
{
	/* Bits control which events trigger ASPM L1 exit:
	 * Bit 12: rxdv
	 * Bit 11: ltr_msg
	 * Bit 10: txdma_poll
	 * Bit  9: xadm
	 * Bit  8: pktavi
	 * Bit  7: txpla
	 */
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_34 ... RTL_GIGA_MAC_VER_36:
		rtl_eri_set_bits(tp, 0xd4, 0x1f00);
		break;
	case RTL_GIGA_MAC_VER_37 ... RTL_GIGA_MAC_VER_38:
		rtl_eri_set_bits(tp, 0xd4, 0x0c00);
		break;
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_63:
		r8168_mac_ocp_modify(tp, 0xc0ac, 0, 0x1f80);
		break;
	default:
		break;
	}
}

static void rtl_disable_exit_l1(struct rtl8169_private *tp)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_34 ... RTL_GIGA_MAC_VER_38:
		rtl_eri_clear_bits(tp, 0xd4, 0x1f00);
		break;
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_63:
		r8168_mac_ocp_modify(tp, 0xc0ac, 0x1f80, 0);
		break;
	default:
		break;
	}
}

static void rtl_hw_aspm_clkreq_enable(struct rtl8169_private *tp, bool enable)
{
	/* Don't enable ASPM in the chip if OS can't control ASPM */
	if (enable && tp->aspm_manageable) {
		RTL_W8(tp, Config5, RTL_R8(tp, Config5) | ASPM_en);
		RTL_W8(tp, Config2, RTL_R8(tp, Config2) | ClkReqEn);

		switch (tp->mac_version) {
		case RTL_GIGA_MAC_VER_46 ... RTL_GIGA_MAC_VER_48:
		case RTL_GIGA_MAC_VER_61 ... RTL_GIGA_MAC_VER_63:
			/* reset ephy tx/rx disable timer */
			r8168_mac_ocp_modify(tp, 0xe094, 0xff00, 0);
			/* chip can trigger L1.2 */
			r8168_mac_ocp_modify(tp, 0xe092, 0x00ff, BIT(2));
			break;
		default:
			break;
		}
	} else {
		switch (tp->mac_version) {
		case RTL_GIGA_MAC_VER_46 ... RTL_GIGA_MAC_VER_48:
		case RTL_GIGA_MAC_VER_61 ... RTL_GIGA_MAC_VER_63:
			r8168_mac_ocp_modify(tp, 0xe092, 0x00ff, 0);
			break;
		default:
			break;
		}

		RTL_W8(tp, Config2, RTL_R8(tp, Config2) & ~ClkReqEn);
		RTL_W8(tp, Config5, RTL_R8(tp, Config5) & ~ASPM_en);
	}

	udelay(10);
}

static void rtl_set_fifo_size(struct rtl8169_private *tp, u16 rx_stat,
			      u16 tx_stat, u16 rx_dyn, u16 tx_dyn)
{
	/* Usage of dynamic vs. static FIFO is controlled by bit
	 * TXCFG_AUTO_FIFO. Exact meaning of FIFO values isn't known.
	 */
	rtl_eri_write(tp, 0xc8, ERIAR_MASK_1111, (rx_stat << 16) | rx_dyn);
	rtl_eri_write(tp, 0xe8, ERIAR_MASK_1111, (tx_stat << 16) | tx_dyn);
}

static void rtl8168g_set_pause_thresholds(struct rtl8169_private *tp,
					  u8 low, u8 high)
{
	/* FIFO thresholds for pause flow control */
	rtl_eri_write(tp, 0xcc, ERIAR_MASK_0001, low);
	rtl_eri_write(tp, 0xd0, ERIAR_MASK_0001, high);
}

static void rtl_hw_start_8168b(struct rtl8169_private *tp)
{
	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Beacon_en);
}

static void __rtl_hw_start_8168cp(struct rtl8169_private *tp)
{
	RTL_W8(tp, Config1, RTL_R8(tp, Config1) | Speed_down);

	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Beacon_en);

	rtl_disable_clock_request(tp);
}

static void rtl_hw_start_8168cp_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168cp[] = {
		{ 0x01, 0,	0x0001 },
		{ 0x02, 0x0800,	0x1000 },
		{ 0x03, 0,	0x0042 },
		{ 0x06, 0x0080,	0x0000 },
		{ 0x07, 0,	0x2000 }
	};

	rtl_set_def_aspm_entry_latency(tp);

	rtl_ephy_init(tp, e_info_8168cp);

	__rtl_hw_start_8168cp(tp);
}

static void rtl_hw_start_8168cp_2(struct rtl8169_private *tp)
{
	rtl_set_def_aspm_entry_latency(tp);

	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Beacon_en);
}

static void rtl_hw_start_8168cp_3(struct rtl8169_private *tp)
{
	rtl_set_def_aspm_entry_latency(tp);

	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Beacon_en);

	/* Magic. */
	RTL_W8(tp, DBG_REG, 0x20);
}

static void rtl_hw_start_8168c_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168c_1[] = {
		{ 0x02, 0x0800,	0x1000 },
		{ 0x03, 0,	0x0002 },
		{ 0x06, 0x0080,	0x0000 }
	};

	rtl_set_def_aspm_entry_latency(tp);

	RTL_W8(tp, DBG_REG, 0x06 | FIX_NAK_1 | FIX_NAK_2);

	rtl_ephy_init(tp, e_info_8168c_1);

	__rtl_hw_start_8168cp(tp);
}

static void rtl_hw_start_8168c_2(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168c_2[] = {
		{ 0x01, 0,	0x0001 },
		{ 0x03, 0x0400,	0x0020 }
	};

	rtl_set_def_aspm_entry_latency(tp);

	rtl_ephy_init(tp, e_info_8168c_2);

	__rtl_hw_start_8168cp(tp);
}

static void rtl_hw_start_8168c_4(struct rtl8169_private *tp)
{
	rtl_set_def_aspm_entry_latency(tp);

	__rtl_hw_start_8168cp(tp);
}

static void rtl_hw_start_8168d(struct rtl8169_private *tp)
{
	rtl_set_def_aspm_entry_latency(tp);

	rtl_disable_clock_request(tp);
}

static void rtl_hw_start_8168d_4(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168d_4[] = {
		{ 0x0b, 0x0000,	0x0048 },
		{ 0x19, 0x0020,	0x0050 },
		{ 0x0c, 0x0100,	0x0020 },
		{ 0x10, 0x0004,	0x0000 },
	};

	rtl_set_def_aspm_entry_latency(tp);

	rtl_ephy_init(tp, e_info_8168d_4);

	rtl_enable_clock_request(tp);
}

static void rtl_hw_start_8168e_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168e_1[] = {
		{ 0x00, 0x0200,	0x0100 },
		{ 0x00, 0x0000,	0x0004 },
		{ 0x06, 0x0002,	0x0001 },
		{ 0x06, 0x0000,	0x0030 },
		{ 0x07, 0x0000,	0x2000 },
		{ 0x00, 0x0000,	0x0020 },
		{ 0x03, 0x5800,	0x2000 },
		{ 0x03, 0x0000,	0x0001 },
		{ 0x01, 0x0800,	0x1000 },
		{ 0x07, 0x0000,	0x4000 },
		{ 0x1e, 0x0000,	0x2000 },
		{ 0x19, 0xffff,	0xfe6c },
		{ 0x0a, 0x0000,	0x0040 }
	};

	rtl_set_def_aspm_entry_latency(tp);

	rtl_ephy_init(tp, e_info_8168e_1);

	rtl_disable_clock_request(tp);

	/* Reset tx FIFO pointer */
	RTL_W32(tp, MISC, RTL_R32(tp, MISC) | TXPLA_RST);
	RTL_W32(tp, MISC, RTL_R32(tp, MISC) & ~TXPLA_RST);

	RTL_W8(tp, Config5, RTL_R8(tp, Config5) & ~Spi_en);
}

static void rtl_hw_start_8168e_2(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168e_2[] = {
		{ 0x09, 0x0000,	0x0080 },
		{ 0x19, 0x0000,	0x0224 },
		{ 0x00, 0x0000,	0x0004 },
		{ 0x0c, 0x3df0,	0x0200 },
	};

	rtl_set_def_aspm_entry_latency(tp);

	rtl_ephy_init(tp, e_info_8168e_2);

	rtl_eri_write(tp, 0xc0, ERIAR_MASK_0011, 0x0000);
	rtl_eri_write(tp, 0xb8, ERIAR_MASK_1111, 0x0000);
	rtl_set_fifo_size(tp, 0x10, 0x10, 0x02, 0x06);
	rtl_eri_set_bits(tp, 0x1d0, BIT(1));
	rtl_reset_packet_filter(tp);
	rtl_eri_set_bits(tp, 0x1b0, BIT(4));
	rtl_eri_write(tp, 0xcc, ERIAR_MASK_1111, 0x00000050);
	rtl_eri_write(tp, 0xd0, ERIAR_MASK_1111, 0x07ff0060);

	rtl_disable_clock_request(tp);

	RTL_W8(tp, MCU, RTL_R8(tp, MCU) & ~NOW_IS_OOB);

	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) | PFM_EN);
	RTL_W32(tp, MISC, RTL_R32(tp, MISC) | PWM_EN);
	RTL_W8(tp, Config5, RTL_R8(tp, Config5) & ~Spi_en);

	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_start_8168f(struct rtl8169_private *tp)
{
	rtl_set_def_aspm_entry_latency(tp);

	rtl_eri_write(tp, 0xc0, ERIAR_MASK_0011, 0x0000);
	rtl_eri_write(tp, 0xb8, ERIAR_MASK_1111, 0x0000);
	rtl_set_fifo_size(tp, 0x10, 0x10, 0x02, 0x06);
	rtl_reset_packet_filter(tp);
	rtl_eri_set_bits(tp, 0x1b0, BIT(4));
	rtl_eri_set_bits(tp, 0x1d0, BIT(4) | BIT(1));
	rtl_eri_write(tp, 0xcc, ERIAR_MASK_1111, 0x00000050);
	rtl_eri_write(tp, 0xd0, ERIAR_MASK_1111, 0x00000060);

	rtl_disable_clock_request(tp);

	RTL_W8(tp, MCU, RTL_R8(tp, MCU) & ~NOW_IS_OOB);
	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) | PFM_EN);
	RTL_W32(tp, MISC, RTL_R32(tp, MISC) | PWM_EN);
	RTL_W8(tp, Config5, RTL_R8(tp, Config5) & ~Spi_en);
}

static void rtl_hw_start_8168f_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168f_1[] = {
		{ 0x06, 0x00c0,	0x0020 },
		{ 0x08, 0x0001,	0x0002 },
		{ 0x09, 0x0000,	0x0080 },
		{ 0x19, 0x0000,	0x0224 },
		{ 0x00, 0x0000,	0x0008 },
		{ 0x0c, 0x3df0,	0x0200 },
	};

	rtl_hw_start_8168f(tp);

	rtl_ephy_init(tp, e_info_8168f_1);
}

static void rtl_hw_start_8411(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168f_1[] = {
		{ 0x06, 0x00c0,	0x0020 },
		{ 0x0f, 0xffff,	0x5200 },
		{ 0x19, 0x0000,	0x0224 },
		{ 0x00, 0x0000,	0x0008 },
		{ 0x0c, 0x3df0,	0x0200 },
	};

	rtl_hw_start_8168f(tp);
	rtl_pcie_state_l2l3_disable(tp);

	rtl_ephy_init(tp, e_info_8168f_1);
}

static void rtl_hw_start_8168g(struct rtl8169_private *tp)
{
	rtl_set_fifo_size(tp, 0x08, 0x10, 0x02, 0x06);
	rtl8168g_set_pause_thresholds(tp, 0x38, 0x48);

	rtl_set_def_aspm_entry_latency(tp);

	rtl_reset_packet_filter(tp);
	rtl_eri_write(tp, 0x2f8, ERIAR_MASK_0011, 0x1d8f);

	rtl_disable_rxdvgate(tp);

	rtl_eri_write(tp, 0xc0, ERIAR_MASK_0011, 0x0000);
	rtl_eri_write(tp, 0xb8, ERIAR_MASK_0011, 0x0000);

	rtl_w0w1_eri(tp, 0x2fc, 0x01, 0x06);
	rtl_eri_clear_bits(tp, 0x1b0, BIT(12));

	rtl_pcie_state_l2l3_disable(tp);
}

static void rtl_hw_start_8168g_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168g_1[] = {
		{ 0x00, 0x0008,	0x0000 },
		{ 0x0c, 0x3ff0,	0x0820 },
		{ 0x1e, 0x0000,	0x0001 },
		{ 0x19, 0x8000,	0x0000 }
	};

	rtl_hw_start_8168g(tp);

	/* disable aspm and clock request before access ephy */
	rtl_hw_aspm_clkreq_enable(tp, false);
	rtl_ephy_init(tp, e_info_8168g_1);
	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_start_8168g_2(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168g_2[] = {
		{ 0x00, 0x0008,	0x0000 },
		{ 0x0c, 0x3ff0,	0x0820 },
		{ 0x19, 0xffff,	0x7c00 },
		{ 0x1e, 0xffff,	0x20eb },
		{ 0x0d, 0xffff,	0x1666 },
		{ 0x00, 0xffff,	0x10a3 },
		{ 0x06, 0xffff,	0xf050 },
		{ 0x04, 0x0000,	0x0010 },
		{ 0x1d, 0x4000,	0x0000 },
	};

	rtl_hw_start_8168g(tp);

	/* disable aspm and clock request before access ephy */
	rtl_hw_aspm_clkreq_enable(tp, false);
	rtl_ephy_init(tp, e_info_8168g_2);
}

static void rtl_hw_start_8411_2(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8411_2[] = {
		{ 0x00, 0x0008,	0x0000 },
		{ 0x0c, 0x37d0,	0x0820 },
		{ 0x1e, 0x0000,	0x0001 },
		{ 0x19, 0x8021,	0x0000 },
		{ 0x1e, 0x0000,	0x2000 },
		{ 0x0d, 0x0100,	0x0200 },
		{ 0x00, 0x0000,	0x0080 },
		{ 0x06, 0x0000,	0x0010 },
		{ 0x04, 0x0000,	0x0010 },
		{ 0x1d, 0x0000,	0x4000 },
	};

	rtl_hw_start_8168g(tp);

	/* disable aspm and clock request before access ephy */
	rtl_hw_aspm_clkreq_enable(tp, false);
	rtl_ephy_init(tp, e_info_8411_2);

	/* The following Realtek-provided magic fixes an issue with the RX unit
	 * getting confused after the PHY having been powered-down.
	 */
	r8168_mac_ocp_write(tp, 0xFC28, 0x0000);
	r8168_mac_ocp_write(tp, 0xFC2A, 0x0000);
	r8168_mac_ocp_write(tp, 0xFC2C, 0x0000);
	r8168_mac_ocp_write(tp, 0xFC2E, 0x0000);
	r8168_mac_ocp_write(tp, 0xFC30, 0x0000);
	r8168_mac_ocp_write(tp, 0xFC32, 0x0000);
	r8168_mac_ocp_write(tp, 0xFC34, 0x0000);
	r8168_mac_ocp_write(tp, 0xFC36, 0x0000);
	mdelay(3);
	r8168_mac_ocp_write(tp, 0xFC26, 0x0000);

	r8168_mac_ocp_write(tp, 0xF800, 0xE008);
	r8168_mac_ocp_write(tp, 0xF802, 0xE00A);
	r8168_mac_ocp_write(tp, 0xF804, 0xE00C);
	r8168_mac_ocp_write(tp, 0xF806, 0xE00E);
	r8168_mac_ocp_write(tp, 0xF808, 0xE027);
	r8168_mac_ocp_write(tp, 0xF80A, 0xE04F);
	r8168_mac_ocp_write(tp, 0xF80C, 0xE05E);
	r8168_mac_ocp_write(tp, 0xF80E, 0xE065);
	r8168_mac_ocp_write(tp, 0xF810, 0xC602);
	r8168_mac_ocp_write(tp, 0xF812, 0xBE00);
	r8168_mac_ocp_write(tp, 0xF814, 0x0000);
	r8168_mac_ocp_write(tp, 0xF816, 0xC502);
	r8168_mac_ocp_write(tp, 0xF818, 0xBD00);
	r8168_mac_ocp_write(tp, 0xF81A, 0x074C);
	r8168_mac_ocp_write(tp, 0xF81C, 0xC302);
	r8168_mac_ocp_write(tp, 0xF81E, 0xBB00);
	r8168_mac_ocp_write(tp, 0xF820, 0x080A);
	r8168_mac_ocp_write(tp, 0xF822, 0x6420);
	r8168_mac_ocp_write(tp, 0xF824, 0x48C2);
	r8168_mac_ocp_write(tp, 0xF826, 0x8C20);
	r8168_mac_ocp_write(tp, 0xF828, 0xC516);
	r8168_mac_ocp_write(tp, 0xF82A, 0x64A4);
	r8168_mac_ocp_write(tp, 0xF82C, 0x49C0);
	r8168_mac_ocp_write(tp, 0xF82E, 0xF009);
	r8168_mac_ocp_write(tp, 0xF830, 0x74A2);
	r8168_mac_ocp_write(tp, 0xF832, 0x8CA5);
	r8168_mac_ocp_write(tp, 0xF834, 0x74A0);
	r8168_mac_ocp_write(tp, 0xF836, 0xC50E);
	r8168_mac_ocp_write(tp, 0xF838, 0x9CA2);
	r8168_mac_ocp_write(tp, 0xF83A, 0x1C11);
	r8168_mac_ocp_write(tp, 0xF83C, 0x9CA0);
	r8168_mac_ocp_write(tp, 0xF83E, 0xE006);
	r8168_mac_ocp_write(tp, 0xF840, 0x74F8);
	r8168_mac_ocp_write(tp, 0xF842, 0x48C4);
	r8168_mac_ocp_write(tp, 0xF844, 0x8CF8);
	r8168_mac_ocp_write(tp, 0xF846, 0xC404);
	r8168_mac_ocp_write(tp, 0xF848, 0xBC00);
	r8168_mac_ocp_write(tp, 0xF84A, 0xC403);
	r8168_mac_ocp_write(tp, 0xF84C, 0xBC00);
	r8168_mac_ocp_write(tp, 0xF84E, 0x0BF2);
	r8168_mac_ocp_write(tp, 0xF850, 0x0C0A);
	r8168_mac_ocp_write(tp, 0xF852, 0xE434);
	r8168_mac_ocp_write(tp, 0xF854, 0xD3C0);
	r8168_mac_ocp_write(tp, 0xF856, 0x49D9);
	r8168_mac_ocp_write(tp, 0xF858, 0xF01F);
	r8168_mac_ocp_write(tp, 0xF85A, 0xC526);
	r8168_mac_ocp_write(tp, 0xF85C, 0x64A5);
	r8168_mac_ocp_write(tp, 0xF85E, 0x1400);
	r8168_mac_ocp_write(tp, 0xF860, 0xF007);
	r8168_mac_ocp_write(tp, 0xF862, 0x0C01);
	r8168_mac_ocp_write(tp, 0xF864, 0x8CA5);
	r8168_mac_ocp_write(tp, 0xF866, 0x1C15);
	r8168_mac_ocp_write(tp, 0xF868, 0xC51B);
	r8168_mac_ocp_write(tp, 0xF86A, 0x9CA0);
	r8168_mac_ocp_write(tp, 0xF86C, 0xE013);
	r8168_mac_ocp_write(tp, 0xF86E, 0xC519);
	r8168_mac_ocp_write(tp, 0xF870, 0x74A0);
	r8168_mac_ocp_write(tp, 0xF872, 0x48C4);
	r8168_mac_ocp_write(tp, 0xF874, 0x8CA0);
	r8168_mac_ocp_write(tp, 0xF876, 0xC516);
	r8168_mac_ocp_write(tp, 0xF878, 0x74A4);
	r8168_mac_ocp_write(tp, 0xF87A, 0x48C8);
	r8168_mac_ocp_write(tp, 0xF87C, 0x48CA);
	r8168_mac_ocp_write(tp, 0xF87E, 0x9CA4);
	r8168_mac_ocp_write(tp, 0xF880, 0xC512);
	r8168_mac_ocp_write(tp, 0xF882, 0x1B00);
	r8168_mac_ocp_write(tp, 0xF884, 0x9BA0);
	r8168_mac_ocp_write(tp, 0xF886, 0x1B1C);
	r8168_mac_ocp_write(tp, 0xF888, 0x483F);
	r8168_mac_ocp_write(tp, 0xF88A, 0x9BA2);
	r8168_mac_ocp_write(tp, 0xF88C, 0x1B04);
	r8168_mac_ocp_write(tp, 0xF88E, 0xC508);
	r8168_mac_ocp_write(tp, 0xF890, 0x9BA0);
	r8168_mac_ocp_write(tp, 0xF892, 0xC505);
	r8168_mac_ocp_write(tp, 0xF894, 0xBD00);
	r8168_mac_ocp_write(tp, 0xF896, 0xC502);
	r8168_mac_ocp_write(tp, 0xF898, 0xBD00);
	r8168_mac_ocp_write(tp, 0xF89A, 0x0300);
	r8168_mac_ocp_write(tp, 0xF89C, 0x051E);
	r8168_mac_ocp_write(tp, 0xF89E, 0xE434);
	r8168_mac_ocp_write(tp, 0xF8A0, 0xE018);
	r8168_mac_ocp_write(tp, 0xF8A2, 0xE092);
	r8168_mac_ocp_write(tp, 0xF8A4, 0xDE20);
	r8168_mac_ocp_write(tp, 0xF8A6, 0xD3C0);
	r8168_mac_ocp_write(tp, 0xF8A8, 0xC50F);
	r8168_mac_ocp_write(tp, 0xF8AA, 0x76A4);
	r8168_mac_ocp_write(tp, 0xF8AC, 0x49E3);
	r8168_mac_ocp_write(tp, 0xF8AE, 0xF007);
	r8168_mac_ocp_write(tp, 0xF8B0, 0x49C0);
	r8168_mac_ocp_write(tp, 0xF8B2, 0xF103);
	r8168_mac_ocp_write(tp, 0xF8B4, 0xC607);
	r8168_mac_ocp_write(tp, 0xF8B6, 0xBE00);
	r8168_mac_ocp_write(tp, 0xF8B8, 0xC606);
	r8168_mac_ocp_write(tp, 0xF8BA, 0xBE00);
	r8168_mac_ocp_write(tp, 0xF8BC, 0xC602);
	r8168_mac_ocp_write(tp, 0xF8BE, 0xBE00);
	r8168_mac_ocp_write(tp, 0xF8C0, 0x0C4C);
	r8168_mac_ocp_write(tp, 0xF8C2, 0x0C28);
	r8168_mac_ocp_write(tp, 0xF8C4, 0x0C2C);
	r8168_mac_ocp_write(tp, 0xF8C6, 0xDC00);
	r8168_mac_ocp_write(tp, 0xF8C8, 0xC707);
	r8168_mac_ocp_write(tp, 0xF8CA, 0x1D00);
	r8168_mac_ocp_write(tp, 0xF8CC, 0x8DE2);
	r8168_mac_ocp_write(tp, 0xF8CE, 0x48C1);
	r8168_mac_ocp_write(tp, 0xF8D0, 0xC502);
	r8168_mac_ocp_write(tp, 0xF8D2, 0xBD00);
	r8168_mac_ocp_write(tp, 0xF8D4, 0x00AA);
	r8168_mac_ocp_write(tp, 0xF8D6, 0xE0C0);
	r8168_mac_ocp_write(tp, 0xF8D8, 0xC502);
	r8168_mac_ocp_write(tp, 0xF8DA, 0xBD00);
	r8168_mac_ocp_write(tp, 0xF8DC, 0x0132);

	r8168_mac_ocp_write(tp, 0xFC26, 0x8000);

	r8168_mac_ocp_write(tp, 0xFC2A, 0x0743);
	r8168_mac_ocp_write(tp, 0xFC2C, 0x0801);
	r8168_mac_ocp_write(tp, 0xFC2E, 0x0BE9);
	r8168_mac_ocp_write(tp, 0xFC30, 0x02FD);
	r8168_mac_ocp_write(tp, 0xFC32, 0x0C25);
	r8168_mac_ocp_write(tp, 0xFC34, 0x00A9);
	r8168_mac_ocp_write(tp, 0xFC36, 0x012D);

	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_start_8168h_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168h_1[] = {
		{ 0x1e, 0x0800,	0x0001 },
		{ 0x1d, 0x0000,	0x0800 },
		{ 0x05, 0xffff,	0x2089 },
		{ 0x06, 0xffff,	0x5881 },
		{ 0x04, 0xffff,	0x854a },
		{ 0x01, 0xffff,	0x068b }
	};
	int rg_saw_cnt;

	/* disable aspm and clock request before access ephy */
	rtl_hw_aspm_clkreq_enable(tp, false);
	rtl_ephy_init(tp, e_info_8168h_1);

	rtl_set_fifo_size(tp, 0x08, 0x10, 0x02, 0x06);
	rtl8168g_set_pause_thresholds(tp, 0x38, 0x48);

	rtl_set_def_aspm_entry_latency(tp);

	rtl_reset_packet_filter(tp);

	rtl_eri_set_bits(tp, 0xdc, 0x001c);

	rtl_eri_write(tp, 0x5f0, ERIAR_MASK_0011, 0x4f87);

	rtl_disable_rxdvgate(tp);

	rtl_eri_write(tp, 0xc0, ERIAR_MASK_0011, 0x0000);
	rtl_eri_write(tp, 0xb8, ERIAR_MASK_0011, 0x0000);

	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) & ~PFM_EN);
	RTL_W8(tp, MISC_1, RTL_R8(tp, MISC_1) & ~PFM_D3COLD_EN);

	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) & ~TX_10M_PS_EN);

	rtl_eri_clear_bits(tp, 0x1b0, BIT(12));

	rtl_pcie_state_l2l3_disable(tp);

	rg_saw_cnt = phy_read_paged(tp->phydev, 0x0c42, 0x13) & 0x3fff;
	if (rg_saw_cnt > 0) {
		u16 sw_cnt_1ms_ini;

		sw_cnt_1ms_ini = 16000000/rg_saw_cnt;
		sw_cnt_1ms_ini &= 0x0fff;
		r8168_mac_ocp_modify(tp, 0xd412, 0x0fff, sw_cnt_1ms_ini);
	}

	r8168_mac_ocp_modify(tp, 0xe056, 0x00f0, 0x0070);
	r8168_mac_ocp_modify(tp, 0xe052, 0x6000, 0x8008);
	r8168_mac_ocp_modify(tp, 0xe0d6, 0x01ff, 0x017f);
	r8168_mac_ocp_modify(tp, 0xd420, 0x0fff, 0x047f);

	r8168_mac_ocp_write(tp, 0xe63e, 0x0001);
	r8168_mac_ocp_write(tp, 0xe63e, 0x0000);
	r8168_mac_ocp_write(tp, 0xc094, 0x0000);
	r8168_mac_ocp_write(tp, 0xc09e, 0x0000);

	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_start_8168ep(struct rtl8169_private *tp)
{
	rtl8168ep_stop_cmac(tp);

	rtl_set_fifo_size(tp, 0x08, 0x10, 0x02, 0x06);
	rtl8168g_set_pause_thresholds(tp, 0x2f, 0x5f);

	rtl_set_def_aspm_entry_latency(tp);

	rtl_reset_packet_filter(tp);

	rtl_eri_write(tp, 0x5f0, ERIAR_MASK_0011, 0x4f87);

	rtl_disable_rxdvgate(tp);

	rtl_eri_write(tp, 0xc0, ERIAR_MASK_0011, 0x0000);
	rtl_eri_write(tp, 0xb8, ERIAR_MASK_0011, 0x0000);

	rtl_w0w1_eri(tp, 0x2fc, 0x01, 0x06);

	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) & ~TX_10M_PS_EN);

	rtl_pcie_state_l2l3_disable(tp);
}

static void rtl_hw_start_8168ep_3(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8168ep_3[] = {
		{ 0x00, 0x0000,	0x0080 },
		{ 0x0d, 0x0100,	0x0200 },
		{ 0x19, 0x8021,	0x0000 },
		{ 0x1e, 0x0000,	0x2000 },
	};

	/* disable aspm and clock request before access ephy */
	rtl_hw_aspm_clkreq_enable(tp, false);
	rtl_ephy_init(tp, e_info_8168ep_3);

	rtl_hw_start_8168ep(tp);

	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) & ~PFM_EN);
	RTL_W8(tp, MISC_1, RTL_R8(tp, MISC_1) & ~PFM_D3COLD_EN);

	r8168_mac_ocp_modify(tp, 0xd3e2, 0x0fff, 0x0271);
	r8168_mac_ocp_modify(tp, 0xd3e4, 0x00ff, 0x0000);
	r8168_mac_ocp_modify(tp, 0xe860, 0x0000, 0x0080);

	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_start_8117(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8117[] = {
		{ 0x19, 0x0040,	0x1100 },
		{ 0x59, 0x0040,	0x1100 },
	};
	int rg_saw_cnt;

	rtl8168ep_stop_cmac(tp);

	/* disable aspm and clock request before access ephy */
	rtl_hw_aspm_clkreq_enable(tp, false);
	rtl_ephy_init(tp, e_info_8117);

	rtl_set_fifo_size(tp, 0x08, 0x10, 0x02, 0x06);
	rtl8168g_set_pause_thresholds(tp, 0x2f, 0x5f);

	rtl_set_def_aspm_entry_latency(tp);

	rtl_reset_packet_filter(tp);

	rtl_eri_set_bits(tp, 0xd4, 0x0010);

	rtl_eri_write(tp, 0x5f0, ERIAR_MASK_0011, 0x4f87);

	rtl_disable_rxdvgate(tp);

	rtl_eri_write(tp, 0xc0, ERIAR_MASK_0011, 0x0000);
	rtl_eri_write(tp, 0xb8, ERIAR_MASK_0011, 0x0000);

	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) & ~PFM_EN);
	RTL_W8(tp, MISC_1, RTL_R8(tp, MISC_1) & ~PFM_D3COLD_EN);

	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) & ~TX_10M_PS_EN);

	rtl_eri_clear_bits(tp, 0x1b0, BIT(12));

	rtl_pcie_state_l2l3_disable(tp);

	rg_saw_cnt = phy_read_paged(tp->phydev, 0x0c42, 0x13) & 0x3fff;
	if (rg_saw_cnt > 0) {
		u16 sw_cnt_1ms_ini;

		sw_cnt_1ms_ini = (16000000 / rg_saw_cnt) & 0x0fff;
		r8168_mac_ocp_modify(tp, 0xd412, 0x0fff, sw_cnt_1ms_ini);
	}

	r8168_mac_ocp_modify(tp, 0xe056, 0x00f0, 0x0070);
	r8168_mac_ocp_write(tp, 0xea80, 0x0003);
	r8168_mac_ocp_modify(tp, 0xe052, 0x0000, 0x0009);
	r8168_mac_ocp_modify(tp, 0xd420, 0x0fff, 0x047f);

	r8168_mac_ocp_write(tp, 0xe63e, 0x0001);
	r8168_mac_ocp_write(tp, 0xe63e, 0x0000);
	r8168_mac_ocp_write(tp, 0xc094, 0x0000);
	r8168_mac_ocp_write(tp, 0xc09e, 0x0000);

	/* firmware is for MAC only */
	r8169_apply_firmware(tp);

	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_start_8102e_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8102e_1[] = {
		{ 0x01,	0, 0x6e65 },
		{ 0x02,	0, 0x091f },
		{ 0x03,	0, 0xc2f9 },
		{ 0x06,	0, 0xafb5 },
		{ 0x07,	0, 0x0e00 },
		{ 0x19,	0, 0xec80 },
		{ 0x01,	0, 0x2e65 },
		{ 0x01,	0, 0x6e65 }
	};
	u8 cfg1;

	rtl_set_def_aspm_entry_latency(tp);

	RTL_W8(tp, DBG_REG, FIX_NAK_1);

	RTL_W8(tp, Config1,
	       LEDS1 | LEDS0 | Speed_down | MEMMAP | IOMAP | VPD | PMEnable);
	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Beacon_en);

	cfg1 = RTL_R8(tp, Config1);
	if ((cfg1 & LEDS0) && (cfg1 & LEDS1))
		RTL_W8(tp, Config1, cfg1 & ~LEDS0);

	rtl_ephy_init(tp, e_info_8102e_1);
}

static void rtl_hw_start_8102e_2(struct rtl8169_private *tp)
{
	rtl_set_def_aspm_entry_latency(tp);

	RTL_W8(tp, Config1, MEMMAP | IOMAP | VPD | PMEnable);
	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Beacon_en);
}

static void rtl_hw_start_8102e_3(struct rtl8169_private *tp)
{
	rtl_hw_start_8102e_2(tp);

	rtl_ephy_write(tp, 0x03, 0xc2f9);
}

static void rtl_hw_start_8401(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8401[] = {
		{ 0x01,	0xffff, 0x6fe5 },
		{ 0x03,	0xffff, 0x0599 },
		{ 0x06,	0xffff, 0xaf25 },
		{ 0x07,	0xffff, 0x8e68 },
	};

	rtl_ephy_init(tp, e_info_8401);
	RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~Beacon_en);
}

static void rtl_hw_start_8105e_1(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8105e_1[] = {
		{ 0x07,	0, 0x4000 },
		{ 0x19,	0, 0x0200 },
		{ 0x19,	0, 0x0020 },
		{ 0x1e,	0, 0x2000 },
		{ 0x03,	0, 0x0001 },
		{ 0x19,	0, 0x0100 },
		{ 0x19,	0, 0x0004 },
		{ 0x0a,	0, 0x0020 }
	};

	/* Force LAN exit from ASPM if Rx/Tx are not idle */
	RTL_W32(tp, FuncEvent, RTL_R32(tp, FuncEvent) | 0x002800);

	/* Disable Early Tally Counter */
	RTL_W32(tp, FuncEvent, RTL_R32(tp, FuncEvent) & ~0x010000);

	RTL_W8(tp, MCU, RTL_R8(tp, MCU) | EN_NDP | EN_OOB_RESET);
	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) | PFM_EN);

	rtl_ephy_init(tp, e_info_8105e_1);

	rtl_pcie_state_l2l3_disable(tp);
}

static void rtl_hw_start_8105e_2(struct rtl8169_private *tp)
{
	rtl_hw_start_8105e_1(tp);
	rtl_ephy_write(tp, 0x1e, rtl_ephy_read(tp, 0x1e) | 0x8000);
}

static void rtl_hw_start_8402(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8402[] = {
		{ 0x19,	0xffff, 0xff64 },
		{ 0x1e,	0, 0x4000 }
	};

	rtl_set_def_aspm_entry_latency(tp);

	/* Force LAN exit from ASPM if Rx/Tx are not idle */
	RTL_W32(tp, FuncEvent, RTL_R32(tp, FuncEvent) | 0x002800);

	RTL_W8(tp, MCU, RTL_R8(tp, MCU) & ~NOW_IS_OOB);

	rtl_ephy_init(tp, e_info_8402);

	rtl_set_fifo_size(tp, 0x00, 0x00, 0x02, 0x06);
	rtl_reset_packet_filter(tp);
	rtl_eri_write(tp, 0xc0, ERIAR_MASK_0011, 0x0000);
	rtl_eri_write(tp, 0xb8, ERIAR_MASK_0011, 0x0000);
	rtl_w0w1_eri(tp, 0x0d4, 0x0e00, 0xff00);

	/* disable EEE */
	rtl_eri_write(tp, 0x1b0, ERIAR_MASK_0011, 0x0000);

	rtl_pcie_state_l2l3_disable(tp);
}

static void rtl_hw_start_8106(struct rtl8169_private *tp)
{
	rtl_hw_aspm_clkreq_enable(tp, false);

	/* Force LAN exit from ASPM if Rx/Tx are not idle */
	RTL_W32(tp, FuncEvent, RTL_R32(tp, FuncEvent) | 0x002800);

	RTL_W32(tp, MISC, (RTL_R32(tp, MISC) | DISABLE_LAN_EN) & ~EARLY_TALLY_EN);
	RTL_W8(tp, MCU, RTL_R8(tp, MCU) | EN_NDP | EN_OOB_RESET);
	RTL_W8(tp, DLLPR, RTL_R8(tp, DLLPR) & ~PFM_EN);

	/* L0 7us, L1 32us - needed to avoid issues with link-up detection */
	rtl_set_aspm_entry_latency(tp, 0x2f);

	rtl_eri_write(tp, 0x1d0, ERIAR_MASK_0011, 0x0000);

	/* disable EEE */
	rtl_eri_write(tp, 0x1b0, ERIAR_MASK_0011, 0x0000);

	rtl_pcie_state_l2l3_disable(tp);
	rtl_hw_aspm_clkreq_enable(tp, true);
}

DECLARE_RTL_COND(rtl_mac_ocp_e00e_cond)
{
	return r8168_mac_ocp_read(tp, 0xe00e) & BIT(13);
}

static void rtl_hw_start_8125_common(struct rtl8169_private *tp)
{
	rtl_pcie_state_l2l3_disable(tp);

	RTL_W16(tp, 0x382, 0x221b);
	RTL_W8(tp, 0x4500, 0);
	RTL_W16(tp, 0x4800, 0);

	/* disable UPS */
	r8168_mac_ocp_modify(tp, 0xd40a, 0x0010, 0x0000);

	RTL_W8(tp, Config1, RTL_R8(tp, Config1) & ~0x10);

	r8168_mac_ocp_write(tp, 0xc140, 0xffff);
	r8168_mac_ocp_write(tp, 0xc142, 0xffff);

	r8168_mac_ocp_modify(tp, 0xd3e2, 0x0fff, 0x03a9);
	r8168_mac_ocp_modify(tp, 0xd3e4, 0x00ff, 0x0000);
	r8168_mac_ocp_modify(tp, 0xe860, 0x0000, 0x0080);

	/* disable new tx descriptor format */
	r8168_mac_ocp_modify(tp, 0xeb58, 0x0001, 0x0000);

	if (tp->mac_version == RTL_GIGA_MAC_VER_63)
		r8168_mac_ocp_modify(tp, 0xe614, 0x0700, 0x0200);
	else
		r8168_mac_ocp_modify(tp, 0xe614, 0x0700, 0x0400);

	if (tp->mac_version == RTL_GIGA_MAC_VER_63)
		r8168_mac_ocp_modify(tp, 0xe63e, 0x0c30, 0x0000);
	else
		r8168_mac_ocp_modify(tp, 0xe63e, 0x0c30, 0x0020);

	r8168_mac_ocp_modify(tp, 0xc0b4, 0x0000, 0x000c);
	r8168_mac_ocp_modify(tp, 0xeb6a, 0x00ff, 0x0033);
	r8168_mac_ocp_modify(tp, 0xeb50, 0x03e0, 0x0040);
	r8168_mac_ocp_modify(tp, 0xe056, 0x00f0, 0x0030);
	r8168_mac_ocp_modify(tp, 0xe040, 0x1000, 0x0000);
	r8168_mac_ocp_modify(tp, 0xea1c, 0x0003, 0x0001);
	r8168_mac_ocp_modify(tp, 0xe0c0, 0x4f0f, 0x4403);
	r8168_mac_ocp_modify(tp, 0xe052, 0x0080, 0x0068);
	r8168_mac_ocp_modify(tp, 0xd430, 0x0fff, 0x047f);

	r8168_mac_ocp_modify(tp, 0xea1c, 0x0004, 0x0000);
	r8168_mac_ocp_modify(tp, 0xeb54, 0x0000, 0x0001);
	udelay(1);
	r8168_mac_ocp_modify(tp, 0xeb54, 0x0001, 0x0000);
	RTL_W16(tp, 0x1880, RTL_R16(tp, 0x1880) & ~0x0030);

	r8168_mac_ocp_write(tp, 0xe098, 0xc302);

	rtl_loop_wait_low(tp, &rtl_mac_ocp_e00e_cond, 1000, 10);

	rtl_disable_rxdvgate(tp);
}

static void rtl_hw_start_8125a_2(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8125a_2[] = {
		{ 0x04, 0xffff, 0xd000 },
		{ 0x0a, 0xffff, 0x8653 },
		{ 0x23, 0xffff, 0xab66 },
		{ 0x20, 0xffff, 0x9455 },
		{ 0x21, 0xffff, 0x99ff },
		{ 0x29, 0xffff, 0xfe04 },

		{ 0x44, 0xffff, 0xd000 },
		{ 0x4a, 0xffff, 0x8653 },
		{ 0x63, 0xffff, 0xab66 },
		{ 0x60, 0xffff, 0x9455 },
		{ 0x61, 0xffff, 0x99ff },
		{ 0x69, 0xffff, 0xfe04 },
	};

	rtl_set_def_aspm_entry_latency(tp);

	/* disable aspm and clock request before access ephy */
	rtl_hw_aspm_clkreq_enable(tp, false);
	rtl_ephy_init(tp, e_info_8125a_2);

	rtl_hw_start_8125_common(tp);
	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_start_8125b(struct rtl8169_private *tp)
{
	static const struct ephy_info e_info_8125b[] = {
		{ 0x0b, 0xffff, 0xa908 },
		{ 0x1e, 0xffff, 0x20eb },
		{ 0x4b, 0xffff, 0xa908 },
		{ 0x5e, 0xffff, 0x20eb },
		{ 0x22, 0x0030, 0x0020 },
		{ 0x62, 0x0030, 0x0020 },
	};

	rtl_set_def_aspm_entry_latency(tp);
	rtl_hw_aspm_clkreq_enable(tp, false);

	rtl_ephy_init(tp, e_info_8125b);
	rtl_hw_start_8125_common(tp);

	rtl_hw_aspm_clkreq_enable(tp, true);
}

static void rtl_hw_config(struct rtl8169_private *tp)
{
	static const rtl_generic_fct hw_configs[] = {
		[RTL_GIGA_MAC_VER_07] = rtl_hw_start_8102e_1,
		[RTL_GIGA_MAC_VER_08] = rtl_hw_start_8102e_3,
		[RTL_GIGA_MAC_VER_09] = rtl_hw_start_8102e_2,
		[RTL_GIGA_MAC_VER_10] = NULL,
		[RTL_GIGA_MAC_VER_11] = rtl_hw_start_8168b,
		[RTL_GIGA_MAC_VER_14] = rtl_hw_start_8401,
		[RTL_GIGA_MAC_VER_17] = rtl_hw_start_8168b,
		[RTL_GIGA_MAC_VER_18] = rtl_hw_start_8168cp_1,
		[RTL_GIGA_MAC_VER_19] = rtl_hw_start_8168c_1,
		[RTL_GIGA_MAC_VER_20] = rtl_hw_start_8168c_2,
		[RTL_GIGA_MAC_VER_21] = rtl_hw_start_8168c_2,
		[RTL_GIGA_MAC_VER_22] = rtl_hw_start_8168c_4,
		[RTL_GIGA_MAC_VER_23] = rtl_hw_start_8168cp_2,
		[RTL_GIGA_MAC_VER_24] = rtl_hw_start_8168cp_3,
		[RTL_GIGA_MAC_VER_25] = rtl_hw_start_8168d,
		[RTL_GIGA_MAC_VER_26] = rtl_hw_start_8168d,
		[RTL_GIGA_MAC_VER_28] = rtl_hw_start_8168d_4,
		[RTL_GIGA_MAC_VER_29] = rtl_hw_start_8105e_1,
		[RTL_GIGA_MAC_VER_30] = rtl_hw_start_8105e_2,
		[RTL_GIGA_MAC_VER_31] = rtl_hw_start_8168d,
		[RTL_GIGA_MAC_VER_32] = rtl_hw_start_8168e_1,
		[RTL_GIGA_MAC_VER_33] = rtl_hw_start_8168e_1,
		[RTL_GIGA_MAC_VER_34] = rtl_hw_start_8168e_2,
		[RTL_GIGA_MAC_VER_35] = rtl_hw_start_8168f_1,
		[RTL_GIGA_MAC_VER_36] = rtl_hw_start_8168f_1,
		[RTL_GIGA_MAC_VER_37] = rtl_hw_start_8402,
		[RTL_GIGA_MAC_VER_38] = rtl_hw_start_8411,
		[RTL_GIGA_MAC_VER_39] = rtl_hw_start_8106,
		[RTL_GIGA_MAC_VER_40] = rtl_hw_start_8168g_1,
		[RTL_GIGA_MAC_VER_42] = rtl_hw_start_8168g_2,
		[RTL_GIGA_MAC_VER_43] = rtl_hw_start_8168g_2,
		[RTL_GIGA_MAC_VER_44] = rtl_hw_start_8411_2,
		[RTL_GIGA_MAC_VER_46] = rtl_hw_start_8168h_1,
		[RTL_GIGA_MAC_VER_48] = rtl_hw_start_8168h_1,
		[RTL_GIGA_MAC_VER_51] = rtl_hw_start_8168ep_3,
		[RTL_GIGA_MAC_VER_52] = rtl_hw_start_8117,
		[RTL_GIGA_MAC_VER_53] = rtl_hw_start_8117,
		[RTL_GIGA_MAC_VER_61] = rtl_hw_start_8125a_2,
		[RTL_GIGA_MAC_VER_63] = rtl_hw_start_8125b,
	};

	if (hw_configs[tp->mac_version])
		hw_configs[tp->mac_version](tp);
}

static void rtl_hw_start_8125(struct rtl8169_private *tp)
{
	int i;

	/* disable interrupt coalescing */
	for (i = 0xa00; i < 0xb00; i += 4)
		RTL_W32(tp, i, 0);

	rtl_hw_config(tp);
}

static void rtl_hw_start_8168(struct rtl8169_private *tp)
{
	if (rtl_is_8168evl_up(tp))
		RTL_W8(tp, MaxTxPacketSize, EarlySize);
	else
		RTL_W8(tp, MaxTxPacketSize, TxPacketMax);

	rtl_hw_config(tp);

	/* disable interrupt coalescing */
	RTL_W16(tp, IntrMitigate, 0x0000);
}

static void rtl_hw_start_8169(struct rtl8169_private *tp)
{
	RTL_W8(tp, EarlyTxThres, NoEarlyTx);

	tp->cp_cmd |= PCIMulRW;

	if (tp->mac_version == RTL_GIGA_MAC_VER_02 ||
	    tp->mac_version == RTL_GIGA_MAC_VER_03)
		tp->cp_cmd |= EnAnaPLL;

	RTL_W16(tp, CPlusCmd, tp->cp_cmd);

	rtl8169_set_magic_reg(tp);

	/* disable interrupt coalescing */
	RTL_W16(tp, IntrMitigate, 0x0000);
}

static void rtl_hw_start(struct  rtl8169_private *tp)
{
	rtl_unlock_config_regs(tp);

	RTL_W16(tp, CPlusCmd, tp->cp_cmd);

	if (tp->mac_version <= RTL_GIGA_MAC_VER_06)
		rtl_hw_start_8169(tp);
	else if (rtl_is_8125(tp))
		rtl_hw_start_8125(tp);
	else
		rtl_hw_start_8168(tp);

	rtl_enable_exit_l1(tp);
	rtl_set_rx_max_size(tp);
	rtl_set_rx_tx_desc_registers(tp);
	rtl_lock_config_regs(tp);

	/* Initially a 10 us delay. Turned it into a PCI commit. - FR */
	rtl_pci_commit(tp);

	RTL_W8(tp, ChipCmd, CmdTxEnb | CmdRxEnb);
	rtl_init_rxcfg(tp);
	rtl_set_tx_config_registers(tp);
	rtl_set_rx_mode(tp);
}

static void rtl8169_cleanup(struct rtl8169_private *tp)
{
	rtl_rx_close(tp);

	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_28:
	case RTL_GIGA_MAC_VER_31:
		rtl_loop_wait_low(tp, &rtl_npq_cond, 20, 2000);
		break;
	case RTL_GIGA_MAC_VER_34 ... RTL_GIGA_MAC_VER_38:
		RTL_W8(tp, ChipCmd, RTL_R8(tp, ChipCmd) | StopReq);
		rtl_loop_wait_high(tp, &rtl_txcfg_empty_cond, 100, 666);
		break;
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_63:
		rtl_enable_rxdvgate(tp);
		udelay(2000);
		break;
	default:
		RTL_W8(tp, ChipCmd, RTL_R8(tp, ChipCmd) | StopReq);
		udelay(100);
		break;
	}

	rtl_hw_reset(tp);
}

static void rtl_read_mac_address(struct rtl8169_private *tp,
				 u8 mac_addr[ETH_ALEN])
{
	/* Get MAC address */
	if (rtl_is_8168evl_up(tp) && tp->mac_version != RTL_GIGA_MAC_VER_34) {
		u32 value;

		value = rtl_eri_read(tp, 0xe0);
		put_unaligned_le32(value, mac_addr);
		value = rtl_eri_read(tp, 0xe4);
		put_unaligned_le16(value, mac_addr + 4);
	} else if (rtl_is_8125(tp)) {
		rtl_read_mac_from_reg(tp, mac_addr, MAC0_BKP);
	}
}

DECLARE_RTL_COND(rtl_link_list_ready_cond)
{
	return RTL_R8(tp, MCU) & LINK_LIST_RDY;
}

static void r8168g_wait_ll_share_fifo_ready(struct rtl8169_private *tp)
{
	rtl_loop_wait_high(tp, &rtl_link_list_ready_cond, 100, 42);
}

static void rtl_hw_init_8168g(struct rtl8169_private *tp)
{
	rtl_enable_rxdvgate(tp);

	RTL_W8(tp, ChipCmd, RTL_R8(tp, ChipCmd) & ~(CmdTxEnb | CmdRxEnb));
	mdelay(1);
	RTL_W8(tp, MCU, RTL_R8(tp, MCU) & ~NOW_IS_OOB);

	r8168_mac_ocp_modify(tp, 0xe8de, BIT(14), 0);
	r8168g_wait_ll_share_fifo_ready(tp);

	r8168_mac_ocp_modify(tp, 0xe8de, 0, BIT(15));
	r8168g_wait_ll_share_fifo_ready(tp);
}

static void rtl_hw_init_8125(struct rtl8169_private *tp)
{
	rtl_enable_rxdvgate(tp);

	RTL_W8(tp, ChipCmd, RTL_R8(tp, ChipCmd) & ~(CmdTxEnb | CmdRxEnb));
	mdelay(1);
	RTL_W8(tp, MCU, RTL_R8(tp, MCU) & ~NOW_IS_OOB);

	r8168_mac_ocp_modify(tp, 0xe8de, BIT(14), 0);
	r8168g_wait_ll_share_fifo_ready(tp);

	r8168_mac_ocp_write(tp, 0xc0aa, 0x07d0);
	r8168_mac_ocp_write(tp, 0xc0a6, 0x0150);
	r8168_mac_ocp_write(tp, 0xc01e, 0x5555);
	r8168g_wait_ll_share_fifo_ready(tp);
}

static void rtl_hw_initialize(struct rtl8169_private *tp)
{
	switch (tp->mac_version) {
	case RTL_GIGA_MAC_VER_51 ... RTL_GIGA_MAC_VER_53:
		rtl8168ep_stop_cmac(tp);
		fallthrough;
	case RTL_GIGA_MAC_VER_40 ... RTL_GIGA_MAC_VER_48:
		rtl_hw_init_8168g(tp);
		break;
	case RTL_GIGA_MAC_VER_61 ... RTL_GIGA_MAC_VER_63:
		rtl_hw_init_8125(tp);
		break;
	default:
		break;
	}
}

static int rtl8169_init_dev(struct eth_device *edev)
{
	struct rtl8169_private *tp = edev->priv;
	int ret;

	rtl_request_firmware(tp);

	pci_set_master(tp->pci_dev);

	tp->phydev = get_phy_device(&tp->miibus, 0);
	if (IS_ERR(tp->phydev))
		return PTR_ERR(tp->phydev);

	ret = phy_register_device(tp->phydev);
	if (ret)
		return ret;

	rtl8169_init_phy(tp);

	return 0;
}

#define ETH_ZLEN        60              /* Min. octets in frame sans FCS */
#define PKT_BUF_SIZE   1536

static void rtl8169_init_ring(struct rtl8169_private *tp)
{
	struct eth_device *edev = &tp->edev;
	int i;

	tp->cur_rx = tp->cur_tx = 0;

	tp->TxDescArray = dma_alloc_coherent(NUM_TX_DESC * sizeof(struct TxDesc),
					   &tp->TxPhyAddr);
	tp->tx_buf = dma_alloc(NUM_TX_DESC * PKT_BUF_SIZE);
	tp->tx_buf_phys = dma_map_single(edev->parent, tp->tx_buf,
					   NUM_TX_DESC * PKT_BUF_SIZE, DMA_TO_DEVICE);

	tp->RxDescArray = dma_alloc_coherent(NUM_RX_DESC * sizeof(struct RxDesc),
					   &tp->RxPhyAddr);
	tp->rx_buf = dma_alloc(NUM_RX_DESC * PKT_BUF_SIZE);
	tp->rx_buf_phys = dma_map_single(edev->parent, tp->rx_buf,
					   NUM_RX_DESC * PKT_BUF_SIZE, DMA_FROM_DEVICE);

	for (i = 0; i < NUM_RX_DESC; i++) {
		if (i == (NUM_RX_DESC - 1))
			tp->RxDescArray[i].opts1 =
				cpu_to_le32(DescOwn | RingEnd | PKT_BUF_SIZE);
		else
			tp->RxDescArray[i].opts1 =
				cpu_to_le32(DescOwn | PKT_BUF_SIZE);

		tp->RxDescArray[i].addr =
				cpu_to_le64(tp->rx_buf_phys + i * PKT_BUF_SIZE);
	}
}

static void r8169_phylink_handler(struct eth_device *edev)
{
	struct rtl8169_private *tp = edev->priv;

	rtl_link_chg_patch(tp);
}

static int rtl8169_eth_open(struct eth_device *edev)
{
	struct rtl8169_private *tp = edev->priv;
	int ret;

	pci_set_master(tp->pci_dev);

	rtl8169_init_ring(tp);
	rtl_hw_start(tp);

	ret = phy_device_connect(edev, &tp->miibus, 0, r8169_phylink_handler, 0,
				 PHY_INTERFACE_MODE_NA);

	return ret;
}

static int rtl8169_phy_write(struct mii_bus *bus, int phyaddr, int phyreg, u16 val)
{
	struct rtl8169_private *tp = bus->priv;

	if (phyaddr > 0)
		return -ENODEV;

	rtl_writephy(tp, phyreg, val);

	return 0;
}

static int rtl8169_phy_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct rtl8169_private *tp = bus->priv;

	if (phyaddr > 0)
		return -ENODEV;

	return rtl_readphy(tp, phyreg);
}

static void rtl8169_doorbell(struct rtl8169_private *tp)
{
	if (rtl_is_8125(tp))
		RTL_W16(tp, TxPoll_8125, BIT(0));
	else
		RTL_W8(tp, TxPoll, NPQ);
}

static int rtl8169_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct rtl8169_private *tp = edev->priv;
	struct device *dev = &tp->pci_dev->dev;
	unsigned int entry;
	u64 start;
	int ret = 0;

	entry = tp->cur_tx % NUM_TX_DESC;

	if (packet_length < ETH_ZLEN)
		memset(tp->tx_buf + entry * PKT_BUF_SIZE, 0, ETH_ZLEN);
	memcpy(tp->tx_buf + entry * PKT_BUF_SIZE, packet, packet_length);
	dma_sync_single_for_device(dev, tp->tx_buf_phys + entry *
				   PKT_BUF_SIZE, PKT_BUF_SIZE, DMA_TO_DEVICE);

	tp->TxDescArray[entry].addr = cpu_to_le64(tp->tx_buf_phys + entry * PKT_BUF_SIZE);

	if (entry != (NUM_TX_DESC - 1)) {
		tp->TxDescArray[entry].opts1 =
			cpu_to_le32(DescOwn | FirstFrag | LastFrag |
			((packet_length > ETH_ZLEN) ? packet_length : ETH_ZLEN));
	} else {
		tp->TxDescArray[entry].opts1 =
			cpu_to_le32(DescOwn | RingEnd | FirstFrag | LastFrag |
			((packet_length > ETH_ZLEN) ? packet_length : ETH_ZLEN));
	}

	rtl8169_doorbell(tp);

	start = get_time_ns();

	while (le32_to_cpu(tp->TxDescArray[entry].opts1) & DescOwn) {
		if (is_timeout(start, 100 * MSECOND)) {
			ret = -ETIMEDOUT;
			break;
		}
	}

	dma_sync_single_for_cpu(dev, tp->tx_buf_phys + entry * PKT_BUF_SIZE,
				PKT_BUF_SIZE, DMA_TO_DEVICE);

	tp->cur_tx++;

	return ret;
}

static int rtl8169_eth_rx(struct eth_device *edev)
{
	struct rtl8169_private *tp = edev->priv;
	struct device *dev = &tp->pci_dev->dev;
	unsigned int entry, pkt_size = 0;
	u8 status;

	entry = tp->cur_rx % NUM_RX_DESC;

	if ((le32_to_cpu(tp->RxDescArray[entry].opts1) & DescOwn) == 0) {
		if (!(le32_to_cpu(tp->RxDescArray[entry].opts1) & RxRES)) {
			pkt_size = (le32_to_cpu(tp->RxDescArray[entry].opts1) & 0x1fff) - 4;

			dma_sync_single_for_cpu(dev, tp->rx_buf_phys + entry * PKT_BUF_SIZE,
						pkt_size, DMA_FROM_DEVICE);

			net_receive(edev, tp->rx_buf + entry * PKT_BUF_SIZE,
			            pkt_size);

			dma_sync_single_for_device(dev, tp->rx_buf_phys + entry * PKT_BUF_SIZE,
						   pkt_size, DMA_FROM_DEVICE);

			if (entry == NUM_RX_DESC - 1)
				tp->RxDescArray[entry].opts1 = cpu_to_le32(DescOwn |
					RingEnd | PKT_BUF_SIZE);
			else
				tp->RxDescArray[entry].opts1 =
					cpu_to_le32(DescOwn | PKT_BUF_SIZE);
			tp->RxDescArray[entry].addr = cpu_to_le64(tp->rx_buf_phys +
					    entry * PKT_BUF_SIZE);
		} else {
			dev_err(&edev->dev, "rx error\n");
		}

		tp->cur_rx++;

		return pkt_size;

	} else {
		status = RTL_R8(tp, IntrStatus);
		RTL_W8(tp, IntrStatus, status & ~(TxErr | RxErr | SYSErr));
		udelay(100);	/* wait */
	}

	return 0;
}

static int rtl8169_get_ethaddr(struct eth_device *edev, unsigned char *mac_addr)
{
	struct rtl8169_private *tp = edev->priv;

	rtl_read_mac_address(tp, mac_addr);
	if (is_valid_ether_addr(mac_addr))
		return 0;

	rtl_read_mac_address(tp, mac_addr);
	if (is_valid_ether_addr(mac_addr))
		return 0;

	rtl_read_mac_from_reg(tp, mac_addr, MAC0);
	if (is_valid_ether_addr(mac_addr))
		return 0;

	return 0;
}

static int rtl8169_set_ethaddr(struct eth_device *edev, const unsigned char *mac_addr)
{
	struct rtl8169_private *tp = edev->priv;

	rtl_rar_set(tp, mac_addr);

	return 0;
}

static void rtl8169_eth_halt(struct eth_device *edev)
{
	struct rtl8169_private *tp = edev->priv;

	/* Stop the chip's Tx and Rx DMA processes. */
	RTL_W8(tp, ChipCmd, 0x00);

	/* Disable interrupts by clearing the interrupt mask. */
	RTL_W16(tp, IntrMask, 0x0000);
	RTL_W32(tp, RxMissed, 0);

	pci_clear_master(tp->pci_dev);
	rtl_pci_commit(tp);

	rtl8169_cleanup(tp);
	rtl_disable_exit_l1(tp);

	dma_unmap_single(edev->parent, tp->tx_buf_phys, NUM_TX_DESC * PKT_BUF_SIZE,
			 DMA_TO_DEVICE);
	free(tp->tx_buf);
	dma_free_coherent((void *)tp->TxDescArray, tp->TxPhyAddr,
			  NUM_TX_DESC * sizeof(struct TxDesc));

	dma_unmap_single(edev->parent, tp->rx_buf_phys, NUM_RX_DESC * PKT_BUF_SIZE,
			 DMA_FROM_DEVICE);
	free(tp->rx_buf);
	dma_free_coherent((void *)tp->RxDescArray, tp->RxPhyAddr,
			  NUM_RX_DESC * sizeof(struct RxDesc));
}

static int rtl8169_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device *dev = &pdev->dev;
	struct eth_device *edev;
	struct rtl8169_private *tp;
	int region, ret;
	u16 xid;
	enum mac_version chipset;

	/* enable pci device */
	pci_enable_device(pdev);

	tp = xzalloc(sizeof(*tp));

	edev = &tp->edev;
	dev->type_data = edev;
	edev->priv = tp;

	tp->pci_dev = pdev;
	tp->dev = &pdev->dev;

	tp->miibus.read = rtl8169_phy_read;
	tp->miibus.write = rtl8169_phy_write;
	tp->miibus.priv = tp;
	tp->miibus.parent = dev;

	/* use first MMIO region */
	region = ffs(pci_select_bars(pdev, IORESOURCE_MEM)) - 1;
	if (region < 0) {
		dev_err(&pdev->dev, "no MMIO resource found\n");
		return -ENODEV;
	}

	tp->mmio_addr = pci_iomap(pdev, region);

	rtl_hw_reset(tp);

	xid = (RTL_R32(tp, TxConfig) >> 20) & 0xfcf;

	/* Identify chip attached to board */
	chipset = rtl8169_get_mac_version(xid, 1);
	if (chipset == RTL_GIGA_MAC_NONE) {
		dev_err(&pdev->dev, "unknown chip XID %03x\n", xid);
		return -ENODEV;
	}

	dev_info(dev, "found %s (base=0x%p)\n",
		 rtl_chip_infos[chipset].name, tp->mmio_addr);

	tp->mac_version = chipset;

	tp->dash_type = rtl_check_dash(tp);

	tp->cp_cmd = RTL_R16(tp, CPlusCmd) & CPCMD_MASK;

	rtl_init_rxcfg(tp);

	rtl_hw_initialize(tp);

	tp->fw_name = rtl_chip_infos[chipset].fw_name;

	if (tp->dash_type != RTL_DASH_NONE)
		rtl8168_driver_start(tp);

	rtl_set_d3_pll_down(tp, tp->dash_type == RTL_DASH_NONE);

	edev->init = rtl8169_init_dev;
	edev->open = rtl8169_eth_open;
	edev->send = rtl8169_eth_send;
	edev->recv = rtl8169_eth_rx;
	edev->get_ethaddr = rtl8169_get_ethaddr;
	edev->set_ethaddr = rtl8169_set_ethaddr;
	edev->halt = rtl8169_eth_halt;
	edev->parent = dev;
	tp->ocp_base = OCP_STD_PHY_BASE;

	ret = mdiobus_register(&tp->miibus);
	if (ret)
		goto mdio_err;

	ret = eth_register(edev);
	if (ret)
		goto eth_err;

	return 0;

mdio_err:
	eth_unregister(edev);

eth_err:
	free(tp);

	return ret;
}

static struct pci_driver rtl8169_eth_driver = {
	.name = "rtl8169_eth",
	.id_table = rtl8169_pci_tbl,
	.probe = rtl8169_probe,
};
device_pci_driver(rtl8169_eth_driver);
