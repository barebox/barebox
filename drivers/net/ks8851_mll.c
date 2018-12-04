// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2012 Jan Weitzel <j.weitzel@phytec.de>
 * based on kernel driver drivers/net/ks8851_mll.c
 * Copyright (c) 2009 Micrel Inc.
 */

/**
 * Supports:
 * KS8851 16bit MLL chip from Micrel Inc.
 */
#include <common.h>
#include <driver.h>

#include <command.h>
#include <net.h>
#include <malloc.h>
#include <init.h>
#include <xfuncs.h>
#include <errno.h>
#include <clock.h>
#include <io.h>
#include <linux/phy.h>
#include <linux/err.h>

#define MAX_RECV_FRAMES			32
#define MAX_BUF_SIZE			2048
#define TX_BUF_SIZE			2000
#define RX_BUF_SIZE			2000

#define KS_CCR				0x08
#define CCR_EEPROM			(1 << 9)
#define CCR_SPI				(1 << 8)
#define CCR_8BIT			(1 << 7)
#define CCR_16BIT			(1 << 6)
#define CCR_32BIT			(1 << 5)
#define CCR_SHARED			(1 << 4)
#define CCR_32PIN			(1 << 0)

/* MAC address registers */
#define KS_MARL				0x10
#define KS_MARM				0x12
#define KS_MARH				0x14

#define KS_OBCR				0x20
#define OBCR_ODS_16MA			(1 << 6)

#define KS_EEPCR			0x22
#define EEPCR_EESA			(1 << 4)
#define EEPCR_EESB			(1 << 3)
#define EEPCR_EEDO			(1 << 2)
#define EEPCR_EESCK			(1 << 1)
#define EEPCR_EECS			(1 << 0)

#define KS_MBIR				0x24
#define MBIR_TXMBF			(1 << 12)
#define MBIR_TXMBFA			(1 << 11)
#define MBIR_RXMBF			(1 << 4)
#define MBIR_RXMBFA			(1 << 3)

#define KS_GRR				0x26
#define GRR_QMU				(1 << 1)
#define GRR_GSR				(1 << 0)

#define KS_WFCR				0x2A
#define WFCR_MPRXE			(1 << 7)
#define WFCR_WF3E			(1 << 3)
#define WFCR_WF2E			(1 << 2)
#define WFCR_WF1E			(1 << 1)
#define WFCR_WF0E			(1 << 0)

#define KS_WF0CRC0			0x30
#define KS_WF0CRC1			0x32
#define KS_WF0BM0			0x34
#define KS_WF0BM1			0x36
#define KS_WF0BM2			0x38
#define KS_WF0BM3			0x3A

#define KS_WF1CRC0			0x40
#define KS_WF1CRC1			0x42
#define KS_WF1BM0			0x44
#define KS_WF1BM1			0x46
#define KS_WF1BM2			0x48
#define KS_WF1BM3			0x4A

#define KS_WF2CRC0			0x50
#define KS_WF2CRC1			0x52
#define KS_WF2BM0			0x54
#define KS_WF2BM1			0x56
#define KS_WF2BM2			0x58
#define KS_WF2BM3			0x5A

#define KS_WF3CRC0			0x60
#define KS_WF3CRC1			0x62
#define KS_WF3BM0			0x64
#define KS_WF3BM1			0x66
#define KS_WF3BM2			0x68
#define KS_WF3BM3			0x6A

#define KS_TXCR				0x70
#define TXCR_TCGICMP			(1 << 8)
#define TXCR_TCGUDP			(1 << 7)
#define TXCR_TCGTCP			(1 << 6)
#define TXCR_TCGIP			(1 << 5)
#define TXCR_FTXQ			(1 << 4)
#define TXCR_TXFCE			(1 << 3)
#define TXCR_TXPE			(1 << 2)
#define TXCR_TXCRC			(1 << 1)
#define TXCR_TXE			(1 << 0)

#define KS_TXSR				0x72
#define TXSR_TXLC			(1 << 13)
#define TXSR_TXMC			(1 << 12)
#define TXSR_TXFID_MASK			(0x3f << 0)
#define TXSR_TXFID_SHIFT		(0)
#define TXSR_TXFID_GET(_v)		(((_v) >> 0) & 0x3f)


#define KS_RXCR1			0x74
#define RXCR1_FRXQ			(1 << 15)
#define RXCR1_RXUDPFCC			(1 << 14)
#define RXCR1_RXTCPFCC			(1 << 13)
#define RXCR1_RXIPFCC			(1 << 12)
#define RXCR1_RXPAFMA			(1 << 11)
#define RXCR1_RXFCE			(1 << 10)
#define RXCR1_RXEFE			(1 << 9)
#define RXCR1_RXMAFMA			(1 << 8)
#define RXCR1_RXBE			(1 << 7)
#define RXCR1_RXME			(1 << 6)
#define RXCR1_RXUE			(1 << 5)
#define RXCR1_RXAE			(1 << 4)
#define RXCR1_RXINVF			(1 << 1)
#define RXCR1_RXE			(1 << 0)
#define RXCR1_FILTER_MASK		(RXCR1_RXINVF | RXCR1_RXAE | \
					 RXCR1_RXMAFMA | RXCR1_RXPAFMA)

#define KS_RXCR2			0x76
#define RXCR2_SRDBL_MASK		(0x7 << 5)
#define RXCR2_SRDBL_SHIFT		(5)
#define RXCR2_SRDBL_4B			(0x0 << 5)
#define RXCR2_SRDBL_8B			(0x1 << 5)
#define RXCR2_SRDBL_16B			(0x2 << 5)
#define RXCR2_SRDBL_32B			(0x3 << 5)
/* #define RXCR2_SRDBL_FRAME		(0x4 << 5) */
#define RXCR2_IUFFP			(1 << 4)
#define RXCR2_RXIUFCEZ			(1 << 3)
#define RXCR2_UDPLFE			(1 << 2)
#define RXCR2_RXICMPFCC			(1 << 1)
#define RXCR2_RXSAF			(1 << 0)

#define KS_TXMIR			0x78

#define KS_RXFHSR			0x7C
#define RXFSHR_RXFV			(1 << 15)
#define RXFSHR_RXICMPFCS		(1 << 13)
#define RXFSHR_RXIPFCS			(1 << 12)
#define RXFSHR_RXTCPFCS			(1 << 11)
#define RXFSHR_RXUDPFCS			(1 << 10)
#define RXFSHR_RXBF			(1 << 7)
#define RXFSHR_RXMF			(1 << 6)
#define RXFSHR_RXUF			(1 << 5)
#define RXFSHR_RXMR			(1 << 4)
#define RXFSHR_RXFT			(1 << 3)
#define RXFSHR_RXFTL			(1 << 2)
#define RXFSHR_RXRF			(1 << 1)
#define RXFSHR_RXCE			(1 << 0)
#define	RXFSHR_ERR			(RXFSHR_RXCE | RXFSHR_RXRF |\
					RXFSHR_RXFTL | RXFSHR_RXMR |\
					RXFSHR_RXICMPFCS | RXFSHR_RXIPFCS |\
					RXFSHR_RXTCPFCS)
#define KS_RXFHBCR			0x7E
#define RXFHBCR_CNT_MASK		0x0FFF

#define KS_TXQCR			0x80
#define TXQCR_AETFE			(1 << 2)
#define TXQCR_TXQMAM			(1 << 1)
#define TXQCR_METFE			(1 << 0)

#define KS_RXQCR			0x82
#define RXQCR_RXDTTS			(1 << 12)
#define RXQCR_RXDBCTS			(1 << 11)
#define RXQCR_RXFCTS			(1 << 10)
#define RXQCR_RXIPHTOE			(1 << 9)
#define RXQCR_RXDTTE			(1 << 7)
#define RXQCR_RXDBCTE			(1 << 6)
#define RXQCR_RXFCTE			(1 << 5)
#define RXQCR_ADRFE			(1 << 4)
#define RXQCR_SDA			(1 << 3)
#define RXQCR_RRXEF			(1 << 0)
#define RXQCR_CMD_CNTL			(RXQCR_RXFCTE|RXQCR_ADRFE)

#define KS_TXFDPR			0x84
#define TXFDPR_TXFPAI			(1 << 14)
#define TXFDPR_TXFP_MASK		(0x7ff << 0)
#define TXFDPR_TXFP_SHIFT		(0)

#define KS_RXFDPR			0x86
#define RXFDPR_RXFPAI			(1 << 14)

#define KS_RXDTTR			0x8C
#define KS_RXDBCTR			0x8E

#define KS_IER				0x90
#define KS_ISR				0x92
#define IRQ_LCI				(1 << 15)
#define IRQ_TXI				(1 << 14)
#define IRQ_RXI				(1 << 13)
#define IRQ_RXOI			(1 << 11)
#define IRQ_TXPSI			(1 << 9)
#define IRQ_RXPSI			(1 << 8)
#define IRQ_TXSAI			(1 << 6)
#define IRQ_RXWFDI			(1 << 5)
#define IRQ_RXMPDI			(1 << 4)
#define IRQ_LDI				(1 << 3)
#define IRQ_EDI				(1 << 2)
#define IRQ_SPIBEI			(1 << 1)
#define IRQ_DEDI			(1 << 0)

#define KS_RXFCTR			0x9C
#define RXFCTR_THRESHOLD_MASK		0x00FF

#define KS_RXFC				0x9D
#define RXFCTR_RXFC_MASK		(0xff << 8)
#define RXFCTR_RXFC_SHIFT		(8)
#define RXFCTR_RXFC_GET(_v)		(((_v) >> 8) & 0xff)
#define RXFCTR_RXFCT_MASK		(0xff << 0)
#define RXFCTR_RXFCT_SHIFT		(0)

#define KS_TXNTFSR			0x9E

#define KS_MAHTR0			0xA0
#define KS_MAHTR1			0xA2
#define KS_MAHTR2			0xA4
#define KS_MAHTR3			0xA6

#define KS_FCLWR			0xB0
#define KS_FCHWR			0xB2
#define KS_FCOWR			0xB4

#define KS_CIDER			0xC0
#define CIDER_ID			0x8870
#define CIDER_REV_MASK			(0x7 << 1)
#define CIDER_REV_SHIFT			(1)
#define CIDER_REV_GET(_v)		(((_v) >> 1) & 0x7)

#define KS_CGCR				0xC6
#define KS_IACR				0xC8
#define IACR_RDEN			(1 << 12)
#define IACR_TSEL_MASK			(0x3 << 10)
#define IACR_TSEL_SHIFT			(10)
#define IACR_TSEL_MIB			(0x3 << 10)
#define IACR_ADDR_MASK			(0x1f << 0)
#define IACR_ADDR_SHIFT			(0)

#define KS_IADLR			0xD0
#define KS_IADHR			0xD2

#define KS_PMECR			0xD4
#define PMECR_PME_DELAY			(1 << 14)
#define PMECR_PME_POL			(1 << 12)
#define PMECR_WOL_WAKEUP		(1 << 11)
#define PMECR_WOL_MAGICPKT		(1 << 10)
#define PMECR_WOL_LINKUP		(1 << 9)
#define PMECR_WOL_ENERGY		(1 << 8)
#define PMECR_AUTO_WAKE_EN		(1 << 7)
#define PMECR_WAKEUP_NORMAL		(1 << 6)
#define PMECR_WKEVT_MASK		(0xf << 2)
#define PMECR_WKEVT_SHIFT		(2)
#define PMECR_WKEVT_GET(_v)		(((_v) >> 2) & 0xf)
#define PMECR_WKEVT_ENERGY		(0x1 << 2)
#define PMECR_WKEVT_LINK		(0x2 << 2)
#define PMECR_WKEVT_MAGICPKT		(0x4 << 2)
#define PMECR_WKEVT_FRAME		(0x8 << 2)
#define PMECR_PM_MASK			(0x3 << 0)
#define PMECR_PM_SHIFT			(0)
#define PMECR_PM_NORMAL			(0x0 << 0)
#define PMECR_PM_ENERGY			(0x1 << 0)
#define PMECR_PM_SOFTDOWN		(0x2 << 0)
#define PMECR_PM_POWERSAVE		(0x3 << 0)

/* Standard MII PHY data */
#define KS_P1MBCR			0xE4
#define P1MBCR_FORCE_FDX		(1 << 8)

#define KS_P1MBSR			0xE6
#define P1MBSR_AN_COMPLETE		(1 << 5)
#define P1MBSR_AN_CAPABLE		(1 << 3)
#define P1MBSR_LINK_UP			(1 << 2)

#define KS_PHY1ILR			0xE8
#define KS_PHY1IHR			0xEA
#define KS_P1ANAR			0xEC
#define KS_P1ANLPR			0xEE

#define KS_P1SCLMD			0xF4
#define P1SCLMD_LEDOFF			(1 << 15)
#define P1SCLMD_TXIDS			(1 << 14)
#define P1SCLMD_RESTARTAN		(1 << 13)
#define P1SCLMD_DISAUTOMDIX		(1 << 10)
#define P1SCLMD_FORCEMDIX		(1 << 9)
#define P1SCLMD_AUTONEGEN		(1 << 7)
#define P1SCLMD_FORCE100		(1 << 6)
#define P1SCLMD_FORCEFDX		(1 << 5)
#define P1SCLMD_ADV_FLOW		(1 << 4)
#define P1SCLMD_ADV_100BT_FDX		(1 << 3)
#define P1SCLMD_ADV_100BT_HDX		(1 << 2)
#define P1SCLMD_ADV_10BT_FDX		(1 << 1)
#define P1SCLMD_ADV_10BT_HDX		(1 << 0)

#define KS_P1CR				0xF6
#define P1CR_HP_MDIX			(1 << 15)
#define P1CR_REV_POL			(1 << 13)
#define P1CR_OP_100M			(1 << 10)
#define P1CR_OP_FDX			(1 << 9)
#define P1CR_OP_MDI			(1 << 7)
#define P1CR_AN_DONE			(1 << 6)
#define P1CR_LINK_GOOD			(1 << 5)
#define P1CR_PNTR_FLOW			(1 << 4)
#define P1CR_PNTR_100BT_FDX		(1 << 3)
#define P1CR_PNTR_100BT_HDX		(1 << 2)
#define P1CR_PNTR_10BT_FDX		(1 << 1)
#define P1CR_PNTR_10BT_HDX		(1 << 0)

/* TX Frame control */

#define TXFR_TXIC			(1 << 15)
#define TXFR_TXFID_MASK			(0x3f << 0)
#define TXFR_TXFID_SHIFT		(0)

#define KS_P1SR				0xF8
#define P1SR_HP_MDIX			(1 << 15)
#define P1SR_REV_POL			(1 << 13)
#define P1SR_OP_100M			(1 << 10)
#define P1SR_OP_FDX			(1 << 9)
#define P1SR_OP_MDI			(1 << 7)
#define P1SR_AN_DONE			(1 << 6)
#define P1SR_LINK_GOOD			(1 << 5)
#define P1SR_PNTR_FLOW			(1 << 4)
#define P1SR_PNTR_100BT_FDX		(1 << 3)
#define P1SR_PNTR_100BT_HDX		(1 << 2)
#define P1SR_PNTR_10BT_FDX		(1 << 1)
#define P1SR_PNTR_10BT_HDX		(1 << 0)

#define	ENUM_BUS_NONE			0
#define	ENUM_BUS_8BIT			1
#define	ENUM_BUS_16BIT			2
#define	ENUM_BUS_32BIT			3

#define MAX_MCAST_LST			32
#define HW_MCAST_SIZE			8

/**
 * struct ks_net - KS8851 driver private data
 * @hw_addr	: start address of data register.
 * @hw_addr_cmd	: start address of command register.
 * @pdev	: Pointer to platform device.
 * @bus_width	: i/o bus width.
 * @extra_byte	: number of extra byte prepended rx pkt.
 *
 */

struct ks_net {
	struct eth_device	edev;
	struct mii_bus	miibus;
	void __iomem		*hw_addr;
	void __iomem		*hw_addr_cmd;
	struct platform_device	*pdev;
	int			bus_width;
};

#define BE3             0x8000      /* Byte Enable 3 */
#define BE2             0x4000      /* Byte Enable 2 */
#define BE1             0x2000      /* Byte Enable 1 */
#define BE0             0x1000      /* Byte Enable 0 */

/**
 * ks_rdreg16 - read 16 bit register from device
 * @ks	  : The chip information
 * @offset: The register address
 *
 * Read a 16bit register from the chip, returning the result
 */

static u16 ks_rdreg16(struct ks_net *ks, int offset)
{
	u16 value;
	u16 cmd_reg_cache = (u16)offset | ((BE1 | BE0) << (offset & 0x02));

	writew(cmd_reg_cache, ks->hw_addr_cmd);
	value = readw(ks->hw_addr);

	return value;
}

/**
 * ks_wrreg16 - write 16bit register value to chip
 * @ks: The chip information
 * @offset: The register address
 * @value: The value to write
 *
 */

static void ks_wrreg16(struct ks_net *ks, int offset, u16 value)
{
	u16 cmd_reg_cache = (u16)offset | ((BE1 | BE0) << (offset & 0x02));
	writew(cmd_reg_cache, ks->hw_addr_cmd);
	writew(value, ks->hw_addr);
}

/**
 * ks_inblk - read a block of data from QMU.
 * @ks: The chip state
 * @wptr: buffer address to save data
 * @len: length in byte to read
 *
 */
static inline void ks_inblk(struct ks_net *ks, u16 *wptr, u32 len)
{
	len >>= 1;
	while (len--)
		*wptr++ = (u16)readw(ks->hw_addr);
}

/**
 * ks_outblk - write data to QMU.
 * @ks: The chip information
 * @wptr: buffer address
 * @len: length in byte to write
 *
 */
static inline void ks_outblk(struct ks_net *ks, u16 *wptr, u32 len)
{
	len >>= 1;
	while (len--)
		writew(*wptr++, ks->hw_addr);
}

void ks_enable_qmu(struct ks_net *ks)
{
	u16 w;

	w = ks_rdreg16(ks, KS_TXCR);
	/* Enables QMU Transmit (TXCR). */
	ks_wrreg16(ks, KS_TXCR, w | TXCR_TXE);

	/*
	 * RX Frame Count Threshold Enable and Auto-Dequeue RXQ Frame
	 * Enable
	 */

	w = ks_rdreg16(ks, KS_RXQCR);
	ks_wrreg16(ks, KS_RXQCR, w | RXQCR_RXFCTE);

	/* Enables QMU Receive (RXCR1). */
	w = ks_rdreg16(ks, KS_RXCR1);
	ks_wrreg16(ks, KS_RXCR1, w | RXCR1_RXE);
}  /* ks_enable_qmu */

static void ks_disable_qmu(struct ks_net *ks)
{
	u16	w;

	w = ks_rdreg16(ks, KS_TXCR);

	/* Disables QMU Transmit (TXCR). */
	w  &= ~TXCR_TXE;
	ks_wrreg16(ks, KS_TXCR, w);

	/* Disables QMU Receive (RXCR1). */
	w = ks_rdreg16(ks, KS_RXCR1);
	w &= ~RXCR1_RXE ;
	ks_wrreg16(ks, KS_RXCR1, w);

}  /* ks_disable_qmu */

/* MII interface controls */

/**
 * ks_phy_reg - convert MII register into a KS8851 register
 * @reg: MII register number.
 *
 * Return the KS8851 register number for the corresponding MII PHY register
 * if possible. Return zero if the MII register has no direct mapping to the
 * KS8851 register set.
 */
static int ks_phy_reg(int reg)
{
	int retval;

	switch (reg) {
	case MII_BMCR:
		retval = KS_P1MBCR;
		break;
	case MII_BMSR:
		retval = KS_P1MBSR;
		break;
	case MII_PHYSID1:
		retval = KS_PHY1ILR;
		break;
	case MII_PHYSID2:
		retval = KS_PHY1IHR;
		break;
	case MII_ADVERTISE:
		retval = KS_P1ANAR;
		break;
	case MII_LPA:
		retval = KS_P1ANLPR;
		break;
	default:
		retval = 0x0;
	}

	return retval;
}

/**
 * ks_phy_read - MII interface PHY register read.
 *
 * This call reads data from the PHY register specified in @reg. Since the
 * device does not support all the MII registers, the non-existent values
 * are always returned as zero.
 *
 * We return zero for unsupported registers as the MII code does not check
 * the value returned for any error status, and simply returns it to the
 * caller. The mii-tool that the driver was tested with takes any -ve error
 * as real PHY capabilities, thus displaying incorrect data to the user.
 */
static int ks_phy_read(struct mii_bus *bus, int addr, int reg)
{
	struct ks_net *priv = (struct ks_net *)bus->priv;
	int ksreg;
	int result;

	ksreg = ks_phy_reg(reg);
	if (!ksreg)
		return 0x0;	/* no error return allowed, so use zero */

	result = ks_rdreg16(priv, ksreg);

	return result;
}

static int ks_phy_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	struct ks_net *priv = (struct ks_net *)bus->priv;
	int ksreg;

	ksreg = ks_phy_reg(reg);
	if (ksreg)
		ks_wrreg16(priv, ksreg, val);

	return 0;
}

static int ks8851_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct ks_net *priv = (struct ks_net *)edev->priv;

	((u16 *) adr)[0] = be16_to_cpu(ks_rdreg16(priv, KS_MARH));
	((u16 *) adr)[1] = be16_to_cpu(ks_rdreg16(priv, KS_MARM));
	((u16 *) adr)[2] = be16_to_cpu(ks_rdreg16(priv, KS_MARL));

	return 0;
}

static int ks8851_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct ks_net *priv = (struct ks_net *)edev->priv;

	ks_wrreg16(priv, KS_MARH, cpu_to_be16(((u16 *) adr)[0]));
	ks_wrreg16(priv, KS_MARM, cpu_to_be16(((u16 *) adr)[1]));
	ks_wrreg16(priv, KS_MARL, cpu_to_be16(((u16 *) adr)[2]));

	return 0;
}

static void ks_soft_reset(struct ks_net *ks, unsigned op)
{
	/* Disable interrupt first */
	ks_wrreg16(ks, KS_IER, 0x0000);
	ks_wrreg16(ks, KS_GRR, op);
	mdelay(10);	/* wait a short time to effect reset */
	ks_wrreg16(ks, KS_GRR, 0);
	mdelay(1);	/* wait for condition to clear */
}

/**
 * ks_read_selftest - read the selftest memory info.
 * @ks: The device state
 *
 * Read and check the TX/RX memory selftest information.
 */
static int ks_read_selftest(struct ks_net *ks)
{
	struct device_d *dev = &ks->edev.dev;
	unsigned both_done = MBIR_TXMBF | MBIR_RXMBF;
	int ret = 0;
	unsigned rd;

	rd = ks_rdreg16(ks, KS_MBIR);

	if ((rd & both_done) != both_done) {
		dev_err(dev, "Memory selftest not finished\n");
		return 0;
	}

	if (rd & MBIR_TXMBFA) {
		dev_err(dev, "TX memory selftest fails\n");
		ret |= 1;
	}

	if (rd & MBIR_RXMBFA) {
		dev_err(dev, "RX memory selftest fails\n");
		ret |= 2;
	}

	dev_dbg(dev, "the selftest passes\n");
	return ret;
}

static void ks_setup(struct ks_net *ks)
{
	u16	w;

	/**
	 * Configure QMU Transmit
	 */

	/* Setup Transmit Frame Data Pointer Auto-Increment (TXFDPR) */
	ks_wrreg16(ks, KS_TXFDPR, TXFDPR_TXFPAI);

	/* Setup Receive Frame Data Pointer Auto-Increment */
	ks_wrreg16(ks, KS_RXFDPR, RXFDPR_RXFPAI);

	/* Setup Receive Frame Threshold - 1 frame (RXFCTFC) */
	ks_wrreg16(ks, KS_RXFCTR, 1 & RXFCTR_THRESHOLD_MASK);

	/* Setup RxQ Command Control (RXQCR) */
	ks_wrreg16(ks, KS_RXQCR, RXQCR_CMD_CNTL);

	/**
	 * set the force mode to half duplex, default is full duplex
	 *  because if the auto-negotiation fails, most switch uses
	 *  half-duplex.
	 */

	w = ks_rdreg16(ks, KS_P1MBCR);
	w &= ~P1MBCR_FORCE_FDX;
	ks_wrreg16(ks, KS_P1MBCR, w);

	w = TXCR_TXFCE | TXCR_TXPE | TXCR_TXCRC | TXCR_TCGIP;
	ks_wrreg16(ks, KS_TXCR, w);

	w = RXCR1_RXFCE | RXCR1_RXBE | RXCR1_RXUE | RXCR1_RXME | RXCR1_RXIPFCC |
		RXCR1_RXPAFMA;
	ks_wrreg16(ks, KS_RXCR1, w);
}  /*ks_setup */

static int ks8851_rx_frame(struct ks_net *ks)
{
	struct device_d *dev = &ks->edev.dev;
	u16 *rdptr = (u16 *) NetRxPackets[0];
	u16 RxStatus, RxLen = 0;
	u16 tmp_rxqcr;

	dev_dbg(dev, "receiving packet\n");

	RxStatus = ks_rdreg16(ks, KS_RXFHSR);
	RxLen = ks_rdreg16(ks, KS_RXFHBCR) & RXFHBCR_CNT_MASK;
	dev_dbg(dev, "%s RxLen %d (%d) RxStatus 0x%04x\n",
		__func__, RxLen, ALIGN(RxLen, 4), RxStatus);

	if (RxLen > PKTSIZE)
		dev_err(dev, "rx length too big\n");

	/* reset Frame pointer */
	ks_wrreg16(ks, KS_RXFDPR, RXFDPR_RXFPAI);

	tmp_rxqcr = ks_rdreg16(ks, KS_RXQCR);
	ks_wrreg16(ks, KS_RXQCR, tmp_rxqcr | RXQCR_SDA);
	/* read 2 bytes for dummy, 2 for status, 2 for len*/
	ks_inblk(ks, rdptr, 2 + 2 + 2);
	ks_inblk(ks, rdptr, ALIGN(RxLen, 4));
	ks_wrreg16(ks, KS_RXQCR, tmp_rxqcr);

	if (RxStatus & RXFSHR_RXFV) {
		/* Pass to upper layer */
		dev_dbg(dev, "passing packet to upper layer\n\n");
		net_receive(&ks->edev, NetRxPackets[0], RxLen);
		return RxLen;
	} else if (RxStatus & RXFSHR_ERR) {
		dev_err(dev, "RxStatus error 0x%04x\n", RxStatus & RXFSHR_ERR);
		if (RxStatus & RXFSHR_RXICMPFCS)
			dev_dbg(dev, "ICMP frame checksum field is incorrect\n");
		if (RxStatus & RXFSHR_RXIPFCS)
			dev_dbg(dev, "IP frame checksum field is incorrect\n");
		if (RxStatus & RXFSHR_RXTCPFCS)
			dev_dbg(dev, "TCP frame checksum field is incorrect\n");
		if (RxStatus & RXFSHR_RXCE)
			dev_dbg(dev, "CRC Error\n");
		if (RxStatus & RXFSHR_RXRF)
			dev_dbg(dev, "frame collision\n");
		if (RxStatus & RXFSHR_RXFTL)
			dev_dbg(dev, "frame too long\n");
		if (RxStatus & RXFSHR_RXMR)
			dev_dbg(dev, "MII symbol error\n");
	} else
		dev_err(dev, "other RxStatus error 0x%04x\n", RxStatus);
	return 0;
}

static int ks8851_eth_rx(struct eth_device *edev)
{
	struct ks_net *ks = (struct ks_net *)edev->priv;
	struct device_d *dev = &edev->dev;
	u16 frame_cnt;

	if (!(ks_rdreg16(ks, KS_ISR) & IRQ_RXI))
		return 0;
	ks_wrreg16(ks, KS_ISR, IRQ_RXI);

	frame_cnt = RXFCTR_RXFC_GET(ks_rdreg16(ks, KS_RXFCTR));

	while (frame_cnt--) {
		dev_dbg(dev, "%s frame %d\n", __func__, frame_cnt);
		ks8851_rx_frame(ks);
	}

	return 0;
}

static int ks8851_eth_send(struct eth_device *edev,
		void *packet, int length)
{
	struct ks_net *ks = (struct ks_net *)edev->priv;
	struct device_d *dev = &edev->dev;
	uint64_t tmo;
	u16 tmp_rxqcr;

	dev_dbg(dev, "%s: length: %d (%d)\n", __func__, length, ALIGN(length, 4));

	/* Enable TXQ write access */
	tmp_rxqcr = ks_rdreg16(ks, KS_RXQCR);
	ks_wrreg16(ks, KS_RXQCR, tmp_rxqcr | RXQCR_SDA);

	/* write status/lenth info */
	writew(0, ks->hw_addr);
	writew(cpu_to_le16(length), ks->hw_addr);

	/* write pkt data */
	ks_outblk(ks, (u16 *) packet, ALIGN(length, 4));
	ks_wrreg16(ks, KS_RXQCR, tmp_rxqcr);

	/* (move the pkt from TX buffer into TXQ) */
	ks_wrreg16(ks, KS_TXQCR, TXQCR_METFE);
	/* wait for transmit done */
	tmo = get_time_ns();
	while (ks_rdreg16(ks, KS_TXQCR) & TXQCR_METFE) {
		if (is_timeout(tmo, 5 * SECOND)) {
			dev_err(dev, "transmission timeout\n");
			break;
		}
	}

	dev_dbg(dev, "transmit done\n\n");
	return 0;
}

static int ks8851_eth_open(struct eth_device *edev)
{
	struct ks_net *priv = (struct ks_net *)edev->priv;
	struct device_d *dev = &edev->dev;
	int ret;

	ks_enable_qmu(priv);

	ret = phy_device_connect(edev, &priv->miibus, 1, NULL,
				 0, PHY_INTERFACE_MODE_NA);
	if (ret)
		return ret;

	dev_dbg(dev, "eth_open\n");

	return 0;
}

static int ks8851_init_dev(struct eth_device *edev)
{
	return 0;
}

static void ks8851_eth_halt(struct eth_device *edev)
{
	struct ks_net *priv = (struct ks_net *)edev->priv;
	struct device_d *dev = &edev->dev;

	ks_disable_qmu(priv);

	dev_dbg(dev, "eth_halt\n");
}

static int ks8851_probe(struct device_d *dev)
{
	struct resource *iores;
	struct eth_device *edev;
	struct ks_net *ks;
	u16 id;

	ks = xzalloc(sizeof(struct ks_net));
	edev = &ks->edev;
	dev->type_data = edev;
	edev->priv = ks;

	if (dev->num_resources < 2) {
		dev_err(dev, "ks8851: need 2 resources addr and addr_cmd");
		return -ENODEV;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	ks->hw_addr = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	ks->hw_addr_cmd = IOMEM(iores->start);

	ks->bus_width = dev->resource[0].flags & IORESOURCE_MEM_TYPE_MASK;

	edev->init = ks8851_init_dev;
	edev->open = ks8851_eth_open;
	edev->send = ks8851_eth_send;
	edev->recv = ks8851_eth_rx;
	edev->halt = ks8851_eth_halt;
	edev->set_ethaddr = ks8851_set_ethaddr;
	edev->get_ethaddr = ks8851_get_ethaddr;
	edev->parent = dev;

	/* setup mii state */
	ks->miibus.read = ks_phy_read;
	ks->miibus.write = ks_phy_write;
	ks->miibus.priv = ks;
	ks->miibus.parent = dev;

	/* simple check for a valid chip being connected to the bus */

	id = ks_rdreg16(ks, KS_CIDER);

	if ((id & ~CIDER_REV_MASK) != CIDER_ID) {
		dev_err(dev, "failed to read device ID\n");
		return -ENODEV;
	}
	dev_dbg(dev, "Found chip, family: 0x%x, id: 0x%x, rev: 0x%x\n",
	     (id >> 8) & 0xff, (id >> 4) & 0xf, (id >> 1) & 0x7);

	if (ks_read_selftest(ks)) {
		dev_err(dev, "failed to read device ID\n");
		return -ENODEV;
	}

	ks_soft_reset(ks, GRR_GSR);
	ks_setup(ks);

	mdiobus_register(&ks->miibus);
	eth_register(edev);
	dev_dbg(dev, "%s MARL 0x%04x MARM 0x%04x MARH 0x%04x\n", __func__,
		ks_rdreg16(ks, KS_MARL), ks_rdreg16(ks, KS_MARM),
		ks_rdreg16(ks, KS_MARH));

	return 0;
}

static struct driver_d ks8851_driver = {
	.name  = "ks8851_mll",
	.probe = ks8851_probe,
};
device_platform_driver(ks8851_driver);
