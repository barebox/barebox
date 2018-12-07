/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2007 Pengutronix, Juergen Beisert <j.beisert@pengutronix.de>
 *
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This file is based on mpc4200fec.h
 * (C) Copyright Motorola, Inc., 2000
 */

#ifndef __IMX27_FEC_H
#define __IMX27_FEC_H

#define FEC_IEVENT			0x004
#define FEC_IMASK			0x008
#define FEC_R_DES_ACTIVE		0x010
#define FEC_X_DES_ACTIVE		0x014
#define FEC_ECNTRL			0x024
#define FEC_MII_DATA			0x040
#define FEC_MII_SPEED			0x044
#define FEC_R_CNTRL			0x084
#define FEC_X_CNTRL			0x0c4
#define FEC_PADDR1			0x0e4
#define FEC_OP_PAUSE			0x0ec
#define FEC_PADDR2			0x0e8
#define FEC_IADDR1			0x118
#define FEC_IADDR2			0x11c
#define FEC_GADDR1			0x120
#define FEC_GADDR2			0x124
#define FEC_X_WMRK			0x144
#define FEC_ERDSR			0x180
#define FEC_ETDSR			0x184
#define	FEC_EMRBR			0x188
#define FEC_MIIGSK_CFGR			0x300
#define FEC_MIIGSK_ENR			0x308
/*
 * Some i.MXs allows RMII mode to be configured via a gasket
 */
#define FEC_MIIGSK_CFGR_FRCONT		(1 << 6)
#define FEC_MIIGSK_CFGR_LBMODE		(1 << 4)
#define FEC_MIIGSK_CFGR_EMODE		(1 << 3)
#define FEC_MIIGSK_CFGR_IF_MODE_MASK	(3 << 0)
#define FEC_MIIGSK_CFGR_IF_MODE_MII	(0 << 0)
#define FEC_MIIGSK_CFGR_IF_MODE_RMII	(1 << 0)

#define FEC_MIIGSK_ENR_READY		(1 << 2)
#define FEC_MIIGSK_ENR_EN		(1 << 1)

#define FEC_R_CNTRL_GRS			(1 << 31)
#define FEC_R_CNTRL_NO_LGTH_CHECK	(1 << 30)
#ifdef CONFIG_ARCH_IMX28
# define FEC_R_CNTRL_MAX_FL(x)		(((x) & 0x3fff) << 16)
#else
# define FEC_R_CNTRL_MAX_FL(x)		(((x) & 0x7ff) << 16)
#endif
#define FEC_R_CNTRL_RMII_10T		(1 << 9) /* i.MX28 specific */
#define FEC_R_CNTRL_RMII_MODE		(1 << 8) /* i.MX28 specific */
#define FEC_R_CNTRL_FCE			(1 << 5)
#define FEC_R_CNTRL_MII_MODE		(1 << 2)

#define FEC_IEVENT_HBERR                0x80000000 /* Note: Not on i.MX28 */
#define FEC_IEVENT_BABR                 0x40000000
#define FEC_IEVENT_BABT                 0x20000000
#define FEC_IEVENT_GRA                  0x10000000
#define FEC_IEVENT_TFINT                0x08000000
#define FEC_IEVENT_MII                  0x00800000
#define FEC_IEVENT_LATE_COL             0x00200000
#define FEC_IEVENT_COL_RETRY_LIM        0x00100000
#define FEC_IEVENT_XFIFO_UN             0x00080000

#define FEC_IMASK_HBERR                 0x80000000 /* Note: Not on i.MX28 */
#define FEC_IMASK_BABR                  0x40000000
#define FEC_IMASK_BABT                  0x20000000
#define FEC_IMASK_GRA                   0x10000000
#define FEC_IMASK_MII                   0x00800000
#define FEC_IMASK_LATE_COL              0x00200000
#define FEC_IMASK_COL_RETRY_LIM         0x00100000
#define FEC_IMASK_XFIFO_UN              0x00080000

#define FEC_ECNTRL_RESET                0x00000001	/**< reset the FEC */
#define FEC_ECNTRL_ETHER_EN             0x00000002	/**< enable the FEC */
#define FEC_ECNTRL_SPEED                0x00000020	/**< i.MX6: enable 1000Mbps mode */

/**
 * @brief Descriptor buffer alignment
 *
 * i.MX27 requires a 16 byte alignment (but for the first element only)
 */
#define DB_ALIGNMENT 16

/**
 * @brief Data buffer alignment
 *
 * i.MX27 requires a 16 byte alignment
 *
 * Note: Valid for member data_pointer in struct buffer_descriptor
 */
#define DB_DATA_ALIGNMENT 16

/**
 * @brief Receive & Transmit Buffer Descriptor definitions
 *
 * Note: The first BD must be aligned (see DB_ALIGNMENT)
 *
 * BTW: Don't trust the i.MX27 and i.MX28 data sheet
 */
struct buffer_descriptor {
	uint16_t data_length;		/**< payload's length in bytes */
	uint16_t status;		/**< BD's staus (see datasheet) */
	uint32_t data_pointer;		/**< payload's buffer address */
};

enum fec_type {
	FEC_TYPE_IMX27,
	FEC_TYPE_IMX28,
	FEC_TYPE_IMX6,
};

enum fec_clock {
	FEC_CLK_IPG,
	FEC_CLK_AHB,
	FEC_CLK_PTP,

	FEC_CLK_NUM
};

enum fec_opt_clock {
	FEC_OPT_CLK_REF,
	FEC_OPT_CLK_OUT,

	FEC_OPT_CLK_NUM
};

/**
 * @brief i.MX27-FEC private structure
 */
struct fec_priv {
	struct eth_device edev;
	struct device_d *dev;
	void __iomem *regs;
	struct buffer_descriptor __iomem *rbd_base;	/* RBD ring                  */
	int rbd_index;				/* next receive BD to read   */
	struct buffer_descriptor __iomem *tbd_base;	/* TBD ring                  */
	int tbd_index;				/* next transmit BD to write */
	int phy_addr;
	phy_interface_t interface;
	u32 phy_flags;
	struct mii_bus miibus;
	void (*phy_init)(struct phy_device *dev);
	struct clk *clk[FEC_CLK_NUM];
	struct clk *opt_clk[FEC_OPT_CLK_NUM];
	enum fec_type type;
};

static inline int fec_is_imx27(struct fec_priv *priv)
{
	return priv->type == FEC_TYPE_IMX27;
}

static inline int fec_is_imx28(struct fec_priv *priv)
{
	return priv->type == FEC_TYPE_IMX28;
}

static inline int fec_is_imx6(struct fec_priv *priv)
{
	return priv->type == FEC_TYPE_IMX6;
}

/**
 * @brief Numbers of buffer descriptors for receiving
 *
 * The number defines the stocked memory buffers for the receiving task.
 * Larger values makes no sense in this limited environment.
 */
#define FEC_RBD_NUM		64

/**
 * @brief Define the ethernet packet size limit in memory
 *
 * Note: Do not shrink this number. This will force the FEC to spread larger
 * frames in more than one BD. This is nothing to worry about, but the current
 * driver can't handle it.
 */
#define FEC_MAX_PKT_SIZE	1536

/* Receive BD status bits */
#define FEC_RBD_EMPTY		0x8000	/**< Receive BD status: Buffer is empty */
#define FEC_RBD_WRAP		0x2000	/**< Receive BD status: Last BD in ring */
#define FEC_RBD_INT		0x1000	/**< Receive BD status: Interrupt */
#define FEC_RBD_LAST		0x0800	/**< Receive BD status: Buffer is last in frame (useless here!) */
#define FEC_RBD_MISS		0x0100	/**< Receive BD status: Miss bit for prom mode */
#define FEC_RBD_BC		0x0080	/**< Receive BD status: The received frame is broadcast frame */
#define FEC_RBD_MC		0x0040	/**< Receive BD status: The received frame is multicast frame */
#define FEC_RBD_LG		0x0020	/**< Receive BD status: Frame length violation */
#define FEC_RBD_NO		0x0010	/**< Receive BD status: Nonoctet align frame */
#define FEC_RBD_SH		0x0008	/**< Receive BD status: Short frame */
#define FEC_RBD_CR		0x0004	/**< Receive BD status: CRC error */
#define FEC_RBD_OV		0x0002	/**< Receive BD status: Receive FIFO overrun */
#define FEC_RBD_TR		0x0001	/**< Receive BD status: Frame is truncated */
#define FEC_RBD_ERR		(FEC_RBD_LG | FEC_RBD_NO | FEC_RBD_CR | \
				FEC_RBD_OV | FEC_RBD_TR)

/* Transmit BD status bits */
#define FEC_TBD_READY		0x8000	/**< Tansmit BD status: Buffer is ready */
#define FEC_TBD_WRAP		0x2000	/**< Tansmit BD status: Mark as last BD in ring */
#define FEC_TBD_INT		0x1000	/**< Tansmit BD status: Interrupt */
#define FEC_TBD_LAST		0x0800	/**< Tansmit BD status: Buffer is last in frame */
#define FEC_TBD_TC		0x0400	/**< Tansmit BD status: Transmit the CRC */
#define FEC_TBD_ABC		0x0200	/**< Tansmit BD status: Append bad CRC */

/* MII-related definitios */
#define FEC_MII_DATA_ST		0x40000000	/**< Start of frame delimiter */
#define FEC_MII_DATA_OP_RD	0x20000000	/**< Perform a read operation */
#define FEC_MII_DATA_OP_WR	0x10000000	/**< Perform a write operation */
#define FEC_MII_DATA_PA_MSK	0x0f800000	/**< PHY Address field mask */
#define FEC_MII_DATA_RA_MSK	0x007c0000	/**< PHY Register field mask */
#define FEC_MII_DATA_TA		0x00020000	/**< Turnaround */
#define FEC_MII_DATA_DATAMSK	0x0000ffff	/**< PHY data field */

#define FEC_MII_DATA_RA_SHIFT	18	/* MII Register address bits */
#define FEC_MII_DATA_PA_SHIFT	23	/* MII PHY address bits */

#endif	/* __IMX27_FEC_H */

/**
 * @file
 * @brief Definitions for the FEC driver (i.MX27)
 */
