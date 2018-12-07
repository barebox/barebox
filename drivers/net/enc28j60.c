// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Microchip ENC28J60 ethernet driver (MAC + PHY)
 *
 * This code was ported from linux-3.15 kernel by Antony Pavlov.
 *
 * Copyright (C) 2007 Eurek srl
 * Author: Claudio Lanconelli <lanconelli.claudio@eptar.com>
 * based on enc28j60.c written by David Anders for 2.4 kernel version
 */

#include <common.h>
#include <net.h>
#include <malloc.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <xfuncs.h>
#include <errno.h>

#include "enc28j60_hw.h"

#define DRV_NAME	"enc28j60"

#define SPI_OPLEN	1

#define ENC28J60_MSG_DEFAULT	\
	(NETIF_MSG_PROBE | NETIF_MSG_IFUP | NETIF_MSG_IFDOWN | NETIF_MSG_LINK)

/* Buffer size required for the largest SPI transfer (i.e., reading a
 * frame). */
#define SPI_TRANSFER_BUF_LEN	(4 + MAX_FRAMELEN)

/* Driver local data */
struct enc28j60_net {
	struct eth_device edev;
	struct spi_device *spi;
	u8 bank;		/* current register bank selected */
	u16 next_pk_ptr;	/* next packet pointer within FIFO */
	u16 max_pk_counter;	/* statistics: max packet counter */
	u16 tx_retry_count;
	bool hw_enable;
	bool full_duplex;
	int rxfilter;
	u8 spi_transfer_buf[SPI_TRANSFER_BUF_LEN];
	/* store MAC address here while hardware is in the reset state */
	u8 hwaddr[ETH_ALEN];
	struct mii_bus miibus;
};

/*
 * SPI read buffer
 * wait for the SPI transfer and copy received data to destination
 */
static int
spi_read_buf(struct enc28j60_net *priv, int len, u8 *data)
{
	u8 *rx_buf = priv->spi_transfer_buf + 4;
	u8 *tx_buf = priv->spi_transfer_buf;
	struct spi_transfer t = {
		.tx_buf = tx_buf,
		.rx_buf = rx_buf,
		.len = SPI_OPLEN + len,
	};
	struct spi_message msg;
	int ret;

	tx_buf[0] = ENC28J60_READ_BUF_MEM;
	tx_buf[1] = tx_buf[2] = tx_buf[3] = 0;	/* don't care */

	spi_message_init(&msg);
	spi_message_add_tail(&t, &msg);
	ret = spi_sync(priv->spi, &msg);
	if (ret == 0) {
		memcpy(data, &rx_buf[SPI_OPLEN], len);
		ret = msg.status;
	}
	if (ret)
		dev_err(&priv->edev.dev,
			"%s() failed: ret = %d\n", __func__, ret);

	return ret;
}

/*
 * SPI write buffer
 */
static int spi_write_buf(struct enc28j60_net *priv, int len,
			 const u8 *data)
{
	int ret;

	if (len > SPI_TRANSFER_BUF_LEN - 1 || len <= 0)
		ret = -EINVAL;
	else {
		priv->spi_transfer_buf[0] = ENC28J60_WRITE_BUF_MEM;
		memcpy(&priv->spi_transfer_buf[1], data, len);
		ret = spi_write(priv->spi, priv->spi_transfer_buf, len + 1);
		if (ret)
			dev_err(&priv->edev.dev,
				"%s() failed: ret = %d\n", __func__, ret);
	}
	return ret;
}

/*
 * basic SPI read operation
 */
static u8 spi_read_op(struct enc28j60_net *priv, u8 op,
			   u8 addr)
{
	u8 tx_buf[2];
	u8 rx_buf[4];
	u8 val = 0;
	int ret;
	int slen = SPI_OPLEN;

	/* do dummy read if needed */
	if (addr & SPRD_MASK)
		slen++;

	tx_buf[0] = op | (addr & ADDR_MASK);
	ret = spi_write_then_read(priv->spi, tx_buf, 1, rx_buf, slen);
	if (ret)
		dev_err(&priv->edev.dev,
			"%s() failed: ret = %d\n", __func__, ret);
	else
		val = rx_buf[slen - 1];

	return val;
}

/*
 * basic SPI write operation
 */
static int spi_write_op(struct enc28j60_net *priv, u8 op,
			u8 addr, u8 val)
{
	int ret;

	priv->spi_transfer_buf[0] = op | (addr & ADDR_MASK);
	priv->spi_transfer_buf[1] = val;
	ret = spi_write(priv->spi, priv->spi_transfer_buf, 2);
	if (ret)
		dev_err(&priv->edev.dev,
			"%s() failed: ret = %d\n", __func__, ret);

	return ret;
}

static void enc28j60_soft_reset(struct enc28j60_net *priv)
{
	dev_dbg(&priv->edev.dev, "%s() enter\n", __func__);

	spi_write_op(priv, ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	/* Errata workaround #1, CLKRDY check is unreliable,
	 * delay at least 1 mS instead */
	udelay(2000);
}

/*
 * select the current register bank if necessary
 */
static void enc28j60_set_bank(struct enc28j60_net *priv, u8 addr)
{
	u8 b = (addr & BANK_MASK) >> 5;

	/* These registers (EIE, EIR, ESTAT, ECON2, ECON1)
	 * are present in all banks, no need to switch bank
	 */
	if (addr >= EIE && addr <= ECON1)
		return;

	/* Clear or set each bank selection bit as needed */
	if ((b & ECON1_BSEL0) != (priv->bank & ECON1_BSEL0)) {
		if (b & ECON1_BSEL0)
			spi_write_op(priv, ENC28J60_BIT_FIELD_SET, ECON1,
					ECON1_BSEL0);
		else
			spi_write_op(priv, ENC28J60_BIT_FIELD_CLR, ECON1,
					ECON1_BSEL0);
	}
	if ((b & ECON1_BSEL1) != (priv->bank & ECON1_BSEL1)) {
		if (b & ECON1_BSEL1)
			spi_write_op(priv, ENC28J60_BIT_FIELD_SET, ECON1,
					ECON1_BSEL1);
		else
			spi_write_op(priv, ENC28J60_BIT_FIELD_CLR, ECON1,
					ECON1_BSEL1);
	}
	priv->bank = b;
}

/*
 * Register access routines through the SPI bus.
 * Some registers can be accessed through the bit field clear and
 * bit field set to avoid a read modify write cycle.
 */

/*
 * Register bit field Set
 */
static void enc28j60_reg_bfset(struct enc28j60_net *priv,
				      u8 addr, u8 mask)
{
	enc28j60_set_bank(priv, addr);
	spi_write_op(priv, ENC28J60_BIT_FIELD_SET, addr, mask);
}

/*
 * Register bit field Clear
 */
static void enc28j60_reg_bfclr(struct enc28j60_net *priv,
				      u8 addr, u8 mask)
{
	enc28j60_set_bank(priv, addr);
	spi_write_op(priv, ENC28J60_BIT_FIELD_CLR, addr, mask);
}

/*
 * Register byte read
 */
static int enc28j60_regb_read(struct enc28j60_net *priv,
				     u8 address)
{
	enc28j60_set_bank(priv, address);
	return spi_read_op(priv, ENC28J60_READ_CTRL_REG, address);
}

/*
 * Register word read
 */
static int enc28j60_regw_read(struct enc28j60_net *priv,
				     u8 address)
{
	int rl, rh;

	enc28j60_set_bank(priv, address);
	rl = spi_read_op(priv, ENC28J60_READ_CTRL_REG, address);
	rh = spi_read_op(priv, ENC28J60_READ_CTRL_REG, address + 1);

	return (rh << 8) | rl;
}

/*
 * Register byte write
 */
static void enc28j60_regb_write(struct enc28j60_net *priv,
				       u8 address, u8 data)
{
	enc28j60_set_bank(priv, address);
	spi_write_op(priv, ENC28J60_WRITE_CTRL_REG, address, data);
}

/*
 * Register word write
 */
static void enc28j60_regw_write(struct enc28j60_net *priv,
				       u8 address, u16 data)
{
	enc28j60_set_bank(priv, address);
	spi_write_op(priv, ENC28J60_WRITE_CTRL_REG, address, (u8) data);
	spi_write_op(priv, ENC28J60_WRITE_CTRL_REG, address + 1,
		     (u8) (data >> 8));
}

/*
 * Buffer memory read
 * Select the starting address and execute a SPI buffer read
 */
static void enc28j60_mem_read(struct enc28j60_net *priv,
				     u16 addr, int len, u8 *data)
{
	enc28j60_regw_write(priv, ERDPTL, addr);

	if (IS_ENABLED(CONFIG_ENC28J60_WRITEVERIFY)) {
		u16 reg;
		reg = enc28j60_regw_read(priv, ERDPTL);
		if (reg != addr)
			dev_err(&priv->edev.dev, "%s() error writing ERDPT "
				"(0x%04x - 0x%04x)\n", __func__, reg, addr);
	}

	spi_read_buf(priv, len, data);
}

/*
 * Write packet to enc28j60 TX buffer memory
 */
static void
enc28j60_packet_write(struct enc28j60_net *priv, int len, const u8 *data)
{
	/* Set the write pointer to start of transmit buffer area */
	enc28j60_regw_write(priv, EWRPTL, TXSTART_INIT);

	if (IS_ENABLED(CONFIG_ENC28J60_WRITEVERIFY)) {
		u16 reg;
		reg = enc28j60_regw_read(priv, EWRPTL);
		if (reg != TXSTART_INIT)
			dev_err(&priv->edev.dev,
				"%s() ERWPT:0x%04x != 0x%04x\n",
				__func__, reg, TXSTART_INIT);
	}
	/* Set the TXND pointer to correspond to the packet size given */
	enc28j60_regw_write(priv, ETXNDL, TXSTART_INIT + len);
	/* write per-packet control byte */
	spi_write_op(priv, ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	dev_dbg(&priv->edev.dev,
		"%s() after control byte ERWPT:0x%04x\n",
		__func__, enc28j60_regw_read(priv, EWRPTL));
	/* copy the packet into the transmit buffer */
	spi_write_buf(priv, len, data);
	dev_dbg(&priv->edev.dev,
		"%s() after write packet ERWPT:0x%04x, len=%d\n",
		 __func__, enc28j60_regw_read(priv, EWRPTL), len);
}

static int poll_ready(struct enc28j60_net *priv, u8 reg, u8 mask, u8 val)
{
	if ((enc28j60_regb_read(priv, reg) & mask) == val) {
		return 0;
	}

	/* 20 msec timeout read */
	mdelay(20);

	if ((enc28j60_regb_read(priv, reg) & mask) != val) {
		dev_err(&priv->spi->dev, "reg %02x ready timeout!\n", reg);
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Wait until the PHY operation is complete.
 */
static int wait_phy_ready(struct enc28j60_net *priv)
{
	return poll_ready(priv, MISTAT, MISTAT_BUSY, 0) ? 0 : 1;
}

/*
 * PHY register read
 * PHY registers are not accessed directly, but through the MII
 */
static u16 enc28j60_phy_read(struct enc28j60_net *priv, u8 address)
{
	u16 ret;

	/* set the PHY register address */
	enc28j60_regb_write(priv, MIREGADR, address);
	/* start the register read operation */
	enc28j60_regb_write(priv, MICMD, MICMD_MIIRD);
	/* wait until the PHY read completes */
	wait_phy_ready(priv);
	/* quit reading */
	enc28j60_regb_write(priv, MICMD, 0x00);
	/* return the data */
	ret = enc28j60_regw_read(priv, MIRDL);

	return ret;
}

static int enc28j60_phy_write(struct enc28j60_net *priv, u8 address, u16 data)
{
	int ret;

	/* set the PHY register address */
	enc28j60_regb_write(priv, MIREGADR, address);
	/* write the PHY data */
	enc28j60_regw_write(priv, MIWRL, data);
	/* wait until the PHY write completes and return */
	ret = wait_phy_ready(priv);

	return ret;
}

/*
 * MII-interface related functions
 */
static int enc28j60_miibus_read(struct mii_bus *bus, int phy_addr, int reg_addr)
{
	struct enc28j60_net *priv = (struct enc28j60_net *)bus->priv;

	if (phy_addr == 0) {
		return enc28j60_phy_read(priv, reg_addr);
	}

	return 0xffff;
}

static int enc28j60_miibus_write(struct mii_bus *bus, int phy_addr,
	int reg_addr, u16 data)
{
	struct enc28j60_net *priv = (struct enc28j60_net *)bus->priv;

	if (phy_addr == 0) {
		enc28j60_phy_write(priv, reg_addr, data);
	}

	return 0;
}

static int enc28j60_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	struct enc28j60_net *priv = edev->priv;

	memcpy(m, priv->hwaddr, ETH_ALEN);

	dev_dbg(&edev->dev, "getting MAC address\n");

	return 0;
}

/*
 * Program the hardware MAC address from dev->dev_addr.
 */
static int enc28j60_set_ethaddr(struct eth_device *edev,
				const unsigned char *mac_addr)
{
	int ret;
	struct enc28j60_net *priv = edev->priv;

	if (!priv->hw_enable) {
		dev_dbg(&edev->dev, "%s(): Setting MAC address\n", __func__);
		/* NOTE: MAC address in ENC28J60 is byte-backward */
		enc28j60_regb_write(priv, MAADR5, mac_addr[0]);
		enc28j60_regb_write(priv, MAADR4, mac_addr[1]);
		enc28j60_regb_write(priv, MAADR3, mac_addr[2]);
		enc28j60_regb_write(priv, MAADR2, mac_addr[3]);
		enc28j60_regb_write(priv, MAADR1, mac_addr[4]);
		enc28j60_regb_write(priv, MAADR0, mac_addr[5]);
		ret = 0;
		memcpy(priv->hwaddr, mac_addr, ETH_ALEN);
	} else {
		dev_err(&edev->dev, "%s() Hardware must be disabled to set "
			"Mac address\n", __func__);
		ret = -EBUSY;
	}

	return ret;
}

/*
 * Debug routine to dump useful register contents
 */
static inline void enc28j60_dump_regs(struct enc28j60_net *priv, const char *msg)
{
	dev_dbg(&priv->edev.dev, " %s\n"
		"HwRevID: 0x%02x\n"
		"Cntrl: ECON1 ECON2 ESTAT  EIR  EIE\n"
		"       0x%02x  0x%02x  0x%02x  0x%02x  0x%02x\n"
		"MAC  : MACON1 MACON3 MACON4\n"
		"       0x%02x   0x%02x   0x%02x\n"
		"Rx   : ERXST  ERXND  ERXWRPT ERXRDPT ERXFCON EPKTCNT MAMXFL\n"
		"       0x%04x 0x%04x 0x%04x  0x%04x  "
		"0x%02x    0x%02x    0x%04x\n"
		"Tx   : ETXST  ETXND  MACLCON1 MACLCON2 MAPHSUP\n"
		"       0x%04x 0x%04x 0x%02x     0x%02x     0x%02x\n",
		msg, enc28j60_regb_read(priv, EREVID),
		enc28j60_regb_read(priv, ECON1), enc28j60_regb_read(priv, ECON2),
		enc28j60_regb_read(priv, ESTAT), enc28j60_regb_read(priv, EIR),
		enc28j60_regb_read(priv, EIE), enc28j60_regb_read(priv, MACON1),
		enc28j60_regb_read(priv, MACON3), enc28j60_regb_read(priv, MACON4),
		enc28j60_regw_read(priv, ERXSTL), enc28j60_regw_read(priv, ERXNDL),
		enc28j60_regw_read(priv, ERXWRPTL),
		enc28j60_regw_read(priv, ERXRDPTL),
		enc28j60_regb_read(priv, ERXFCON),
		enc28j60_regb_read(priv, EPKTCNT),
		enc28j60_regw_read(priv, MAMXFLL), enc28j60_regw_read(priv, ETXSTL),
		enc28j60_regw_read(priv, ETXNDL),
		enc28j60_regb_read(priv, MACLCON1),
		enc28j60_regb_read(priv, MACLCON2),
		enc28j60_regb_read(priv, MAPHSUP));
}

/*
 * ERXRDPT need to be set always at odd addresses, refer to errata datasheet
 */
static u16 erxrdpt_workaround(u16 next_packet_ptr, u16 start, u16 end)
{
	u16 erxrdpt;

	if ((next_packet_ptr - 1 < start) || (next_packet_ptr - 1 > end))
		erxrdpt = end;
	else
		erxrdpt = next_packet_ptr - 1;

	return erxrdpt;
}

/*
 * Calculate wrap around when reading beyond the end of the RX buffer
 */
static u16 rx_packet_start(u16 ptr)
{
	if (ptr + RSV_SIZE > RXEND_INIT)
		return (ptr + RSV_SIZE) - (RXEND_INIT - RXSTART_INIT + 1);
	else
		return ptr + RSV_SIZE;
}

static void enc28j60_rxfifo_init(struct enc28j60_net *priv, u16 start, u16 end)
{
	u16 erxrdpt;

	if (start > 0x1FFF || end > 0x1FFF || start > end) {
		dev_err(&priv->edev.dev,
			"%s(%d, %d) RXFIFO bad parameters!\n",
			__func__, start, end);
		return;
	}

	/* set receive buffer start + end */
	priv->next_pk_ptr = start;
	enc28j60_regw_write(priv, ERXSTL, start);
	erxrdpt = erxrdpt_workaround(priv->next_pk_ptr, start, end);
	enc28j60_regw_write(priv, ERXRDPTL, erxrdpt);
	enc28j60_regw_write(priv, ERXNDL, end);
}

static void enc28j60_txfifo_init(struct enc28j60_net *priv, u16 start, u16 end)
{
	if (start > 0x1FFF || end > 0x1FFF || start > end) {
		dev_err(&priv->edev.dev,
			"%s(%d, %d) TXFIFO bad parameters!\n",
			__func__, start, end);
		return;
	}
	/* set transmit buffer start + end */
	enc28j60_regw_write(priv, ETXSTL, start);
	enc28j60_regw_write(priv, ETXNDL, end);
}

/*
 * Low power mode shrinks power consumption about 100x, so we'd like
 * the chip to be in that mode whenever it's inactive.  (However, we
 * can't stay in lowpower mode during suspend with WOL active.)
 */
static void enc28j60_lowpower(struct enc28j60_net *priv, bool is_low)
{
	dev_dbg(&priv->spi->dev, "%s power...\n", is_low ? "low" : "high");

	if (is_low) {
		enc28j60_reg_bfclr(priv, ECON1, ECON1_RXEN);
		poll_ready(priv, ESTAT, ESTAT_RXBUSY, 0);
		poll_ready(priv, ECON1, ECON1_TXRTS, 0);
		/* ECON2_VRPS was set during initialization */
		enc28j60_reg_bfset(priv, ECON2, ECON2_PWRSV);
	} else {
		enc28j60_reg_bfclr(priv, ECON2, ECON2_PWRSV);
		poll_ready(priv, ESTAT, ESTAT_CLKRDY, ESTAT_CLKRDY);
		/* caller sets ECON1_RXEN */
	}
}

static int enc28j60_hw_init(struct enc28j60_net *priv)
{
	u8 reg;

	dev_dbg(&priv->edev.dev, "%s() - %s\n", __func__,
		priv->full_duplex ? "FullDuplex" : "HalfDuplex");

	/* first reset the chip */
	enc28j60_soft_reset(priv);
	/* Clear ECON1 */
	spi_write_op(priv, ENC28J60_WRITE_CTRL_REG, ECON1, 0x00);
	priv->bank = 0;
	priv->hw_enable = false;
	priv->tx_retry_count = 0;
	priv->max_pk_counter = 0;

	/* enable address auto increment and voltage regulator powersave */
	enc28j60_regb_write(priv, ECON2, ECON2_AUTOINC | ECON2_VRPS);

	enc28j60_rxfifo_init(priv, RXSTART_INIT, RXEND_INIT);
	enc28j60_txfifo_init(priv, TXSTART_INIT, TXEND_INIT);

	/*
	 * Check the RevID.
	 * If it's 0x00 or 0xFF probably the enc28j60 is not mounted or
	 * damaged
	 */
	reg = enc28j60_regb_read(priv, EREVID);

	dev_dbg(&priv->edev.dev, "chip RevID: 0x%02x\n", reg);

	if (reg == 0x00 || reg == 0xff) {
		dev_err(&priv->edev.dev,
			"%s() Invalid RevId %d\n", __func__, reg);
		return 0;
	}

	/* default filter mode: (unicast OR broadcast) AND crc valid */
	enc28j60_regb_write(priv, ERXFCON,
			    ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);

	/* enable MAC receive */
	enc28j60_regb_write(priv, MACON1,
			    MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);

	/* enable automatic padding and CRC operations */
	if (priv->full_duplex) {
		enc28j60_regb_write(priv, MACON3,
				    MACON3_PADCFG0 | MACON3_TXCRCEN |
				    MACON3_FRMLNEN | MACON3_FULDPX);
		/* set inter-frame gap (non-back-to-back) */
		enc28j60_regb_write(priv, MAIPGL, 0x12);
		/* set inter-frame gap (back-to-back) */
		enc28j60_regb_write(priv, MABBIPG, 0x15);
	} else {
		enc28j60_regb_write(priv, MACON3,
				    MACON3_PADCFG0 | MACON3_TXCRCEN |
				    MACON3_FRMLNEN);
		enc28j60_regb_write(priv, MACON4, 1 << 6);	/* DEFER bit */
		/* set inter-frame gap (non-back-to-back) */
		enc28j60_regw_write(priv, MAIPGL, 0x0C12);
		/* set inter-frame gap (back-to-back) */
		enc28j60_regb_write(priv, MABBIPG, 0x12);
	}
	/*
	 * MACLCON1 (default)
	 * MACLCON2 (default)
	 * Set the maximum packet size which the controller will accept
	 */
	enc28j60_regw_write(priv, MAMXFLL, MAX_FRAMELEN);

	/* Configure LEDs */
	if (!enc28j60_phy_write(priv, PHLCON, ENC28J60_LAMPS_MODE))
		return 0;

	if (priv->full_duplex) {
		if (!enc28j60_phy_write(priv, PHCON1, PHCON1_PDPXMD))
			return 0;
		if (!enc28j60_phy_write(priv, PHCON2, 0x00))
			return 0;
	} else {
		if (!enc28j60_phy_write(priv, PHCON1, 0x00))
			return 0;
		if (!enc28j60_phy_write(priv, PHCON2, PHCON2_HDLDIS))
			return 0;
	}

	enc28j60_dump_regs(priv, "Hw initialized.");

	return 1;
}

static void enc28j60_hw_enable(struct enc28j60_net *priv)
{
	dev_dbg(&priv->edev.dev, "%s()\n", __func__);

	/* enable receive logic */
	enc28j60_reg_bfset(priv, ECON1, ECON1_RXEN);
	priv->hw_enable = true;
}

static void enc28j60_hw_disable(struct enc28j60_net *priv)
{
	/* disable interrutps and packet reception */
	enc28j60_regb_write(priv, EIE, 0x00);
	enc28j60_reg_bfclr(priv, ECON1, ECON1_RXEN);
	priv->hw_enable = false;
}

/*
 * Receive Status vector
 */
static inline void enc28j60_dump_rsv(struct enc28j60_net *priv, const char *msg,
			      u16 pk_ptr, int len, u16 sts)
{
	struct device_d *dev = &priv->edev.dev;

	dev_dbg(dev, "%s - NextPk: 0x%04x - RSV:\n",
		msg, pk_ptr);
	dev_dbg(dev, "ByteCount: %d, DribbleNibble: %d\n", len,
		 RSV_GETBIT(sts, RSV_DRIBBLENIBBLE));
	dev_dbg(dev, "RxOK: %d, CRCErr:%d, LenChkErr: %d,"
		 " LenOutOfRange: %d\n", RSV_GETBIT(sts, RSV_RXOK),
		 RSV_GETBIT(sts, RSV_CRCERROR),
		 RSV_GETBIT(sts, RSV_LENCHECKERR),
		 RSV_GETBIT(sts, RSV_LENOUTOFRANGE));
	dev_dbg(dev, "Multicast: %d, Broadcast: %d, "
		 "LongDropEvent: %d, CarrierEvent: %d\n",
		 RSV_GETBIT(sts, RSV_RXMULTICAST),
		 RSV_GETBIT(sts, RSV_RXBROADCAST),
		 RSV_GETBIT(sts, RSV_RXLONGEVDROPEV),
		 RSV_GETBIT(sts, RSV_CARRIEREV));
	dev_dbg(dev, "ControlFrame: %d, PauseFrame: %d,"
		 " UnknownOp: %d, VLanTagFrame: %d\n",
		 RSV_GETBIT(sts, RSV_RXCONTROLFRAME),
		 RSV_GETBIT(sts, RSV_RXPAUSEFRAME),
		 RSV_GETBIT(sts, RSV_RXUNKNOWNOPCODE),
		 RSV_GETBIT(sts, RSV_RXTYPEVLAN));
}

/*
 * Hardware transmit function.
 * Fill the buffer memory and send the contents of the transmit buffer
 * onto the network
 */
static int enc28j60_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct enc28j60_net *priv = edev->priv;

	dev_dbg(&edev->dev, "Tx Packet Len:%d\n", packet_length);

	enc28j60_packet_write(priv, packet_length, packet);

	/* readback and verify written data */
	if (IS_ENABLED(CONFIG_ENC28J60_WRITEVERIFY)) {
		int test_len, k;
		u8 test_buf[64]; /* limit the test to the first 64 bytes */
		int okflag;

		test_len = packet_length;
		if (test_len > sizeof(test_buf))
			test_len = sizeof(test_buf);

		/* + 1 to skip control byte */
		enc28j60_mem_read(priv, TXSTART_INIT + 1, test_len, test_buf);
		okflag = 1;
		for (k = 0; k < test_len; k++) {
			if (((u8 *)packet)[k] != test_buf[k]) {
				dev_dbg(&edev->dev,
					 "Error, %d location differ: "
					 "0x%02x-0x%02x\n", k,
					 ((u8 *)packet)[k], test_buf[k]);
				okflag = 0;
			}
		}
		if (!okflag)
			dev_dbg(&edev->dev, "Tx write buffer verify ERROR!\n");
	}

	/* set TX request flag */
	enc28j60_reg_bfset(priv, ECON1, ECON1_TXRTS);

	return 0;
}

/*
 * Hardware receive function.
 * Read the buffer memory, update the FIFO pointer to free the buffer,
 * check the status vector and decrement the packet counter.
 */
static void enc28j60_hw_rx(struct eth_device *edev)
{
	struct enc28j60_net *priv = edev->priv;
	u16 erxrdpt, next_packet, rxstat;
	u8 rsv[RSV_SIZE];
	int len;

	dev_dbg(&edev->dev, "RX pk_addr:0x%04x\n", priv->next_pk_ptr);

	if (unlikely(priv->next_pk_ptr > RXEND_INIT)) {
		dev_err(&edev->dev, "%s() Invalid packet address!! 0x%04x\n",
			__func__, priv->next_pk_ptr);

		/* packet address corrupted: reset RX logic */
		enc28j60_reg_bfclr(priv, ECON1, ECON1_RXEN);
		enc28j60_reg_bfset(priv, ECON1, ECON1_RXRST);
		enc28j60_reg_bfclr(priv, ECON1, ECON1_RXRST);
		enc28j60_rxfifo_init(priv, RXSTART_INIT, RXEND_INIT);
		enc28j60_reg_bfclr(priv, EIR, EIR_RXERIF);
		enc28j60_reg_bfset(priv, ECON1, ECON1_RXEN);

		return;
	}

	/* Read next packet pointer and rx status vector */
	enc28j60_mem_read(priv, priv->next_pk_ptr, sizeof(rsv), rsv);

	next_packet = rsv[1];
	next_packet <<= 8;
	next_packet |= rsv[0];

	len = rsv[3];
	len <<= 8;
	len |= rsv[2];

	rxstat = rsv[5];
	rxstat <<= 8;
	rxstat |= rsv[4];

	enc28j60_dump_rsv(priv, __func__, next_packet, len, rxstat);

	if (!RSV_GETBIT(rxstat, RSV_RXOK) || len > MAX_FRAMELEN) {
		dev_err(&edev->dev, "Rx Error (%04x)\n", rxstat);
	} else {
		/* copy the packet from the receive buffer */
		enc28j60_mem_read(priv,
			rx_packet_start(priv->next_pk_ptr),
			len, NetRxPackets[0]);

		net_receive(edev, NetRxPackets[0], len);
	}

	/*
	 * Move the RX read pointer to the start of the next
	 * received packet.
	 * This frees the memory we just read out
	 */
	erxrdpt = erxrdpt_workaround(next_packet, RXSTART_INIT, RXEND_INIT);
	dev_dbg(&edev->dev, ": %s() ERXRDPT:0x%04x\n", __func__, erxrdpt);

	enc28j60_regw_write(priv, ERXRDPTL, erxrdpt);

	if (IS_ENABLED(CONFIG_ENC28J60_WRITEVERIFY)) {
		u16 reg;
		reg = enc28j60_regw_read(priv, ERXRDPTL);
		if (reg != erxrdpt)
			dev_dbg(&edev->dev, "%s() ERXRDPT verify "
				"error (0x%04x - 0x%04x)\n", __func__,
				reg, erxrdpt);
	}

	priv->next_pk_ptr = next_packet;

	/* we are done with this packet, decrement the packet counter */
	enc28j60_reg_bfset(priv, ECON2, ECON2_PKTDEC);
}

/*
 * Access the PHY to determine link status
 */
static void enc28j60_check_link_status(struct eth_device *edev)
{
	struct enc28j60_net *priv = edev->priv;
	u16 reg;
	int duplex;

	reg = enc28j60_phy_read(priv, PHSTAT2);
	dev_dbg(&edev->dev, "%s() PHSTAT1: %04x, PHSTAT2: %04x\n", __func__,
			enc28j60_phy_read(priv, PHSTAT1), reg);
	duplex = reg & PHSTAT2_DPXSTAT;

	if (reg & PHSTAT2_LSTAT) {
		dev_dbg(&edev->dev, "link up - %s\n",
			duplex ? "Full duplex" : "Half duplex");
	} else {
		dev_dbg(&edev->dev, "link down\n");
	}
}

static int enc28j60_eth_rx(struct eth_device *edev)
{
	struct enc28j60_net *priv = edev->priv;
	int pk_counter;

	pk_counter = enc28j60_regb_read(priv, EPKTCNT);
	if (pk_counter)
		dev_dbg(&edev->dev, "intRX, pk_cnt: %d\n", pk_counter);

	if (pk_counter > priv->max_pk_counter) {
		priv->max_pk_counter = pk_counter;
		if (priv->max_pk_counter > 1)
			dev_dbg(&edev->dev,
				"RX max_pk_cnt: %d\n", priv->max_pk_counter);
	}

	while (pk_counter-- > 0)
		enc28j60_hw_rx(edev);

	return 0;
}

static int enc28j60_init_dev(struct eth_device *edev)
{
	/* empty */

	return 0;
}

/*
 * Open/initialize the board. This is called (in the current kernel)
 * sometime after booting when the 'ifconfig' program is run.
 *
 * This routine should set everything up anew at each open, even
 * registers that "should" only need to be set once at boot, so that
 * there is non-reboot way to recover if something goes wrong.
 */
static int enc28j60_eth_open(struct eth_device *edev)
{
	struct enc28j60_net *priv = edev->priv;
	int ret;

	dev_dbg(&edev->dev, "%s() enter\n", __func__);

	/* Reset the hardware here (and take it out of low power mode) */
	enc28j60_lowpower(priv, false);
	enc28j60_hw_disable(priv);
	if (!enc28j60_hw_init(priv)) {
		dev_err(&edev->dev, "hw_init() failed\n");
		return -EINVAL;
	}

	/* Update the MAC address after reset */
	enc28j60_set_ethaddr(edev, priv->hwaddr);

	enc28j60_hw_enable(priv);

	/* Wait for link up after hardware reset */
	mdelay(2000);

	ret = phy_device_connect(edev, &priv->miibus, 0,
				&enc28j60_check_link_status, 0,
				PHY_INTERFACE_MODE_MII);

	return ret;
}

static void enc28j60_eth_halt(struct eth_device *edev)
{
	struct enc28j60_net *priv = edev->priv;

	dev_dbg(&edev->dev, "%s() enter\n", __func__);

	enc28j60_hw_disable(priv);
	enc28j60_lowpower(priv, true);
}

static int enc28j60_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct enc28j60_net *priv;
	int ret = 0;

	priv = xzalloc(sizeof(*priv));

	priv->spi = (struct spi_device *)dev->type_data;

	edev = &priv->edev;
	edev->priv = priv;
	edev->init = enc28j60_init_dev;
	edev->open = enc28j60_eth_open;
	edev->send = enc28j60_eth_send;
	edev->recv = enc28j60_eth_rx;
	edev->get_ethaddr = enc28j60_get_ethaddr;
	edev->set_ethaddr = enc28j60_set_ethaddr;
	edev->halt = enc28j60_eth_halt;
	edev->parent = dev;

	/*
	 * Here is a quote from ENC28J60 Data Sheet:
	 *
	 * The ENC28J60 does not support automatic duplex
	 * negotiation. If it is connected to an automatic duplex
	 * negotiation enabled network switch or Ethernet controller,
	 * the ENC28J60 will be detected as a half-duplex device.
	 *
	 * So most people prefer to set up half-duplex mode.
	 */
	priv->full_duplex = 0;

	ret = eth_register(edev);
	if (ret)
		goto eth_error_register;

	if (!enc28j60_hw_init(priv)) {
		dev_err(dev, DRV_NAME " chip not found\n");
		ret = -EIO;
		goto hw_error_register;
	}

	enc28j60_lowpower(priv, true);

	priv->miibus.read = enc28j60_miibus_read;
	priv->miibus.write = enc28j60_miibus_write;

	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	ret = mdiobus_register(&priv->miibus);
	if (ret)
		goto mdio_error_register;

	dev_info(dev, DRV_NAME " driver registered\n");

	return 0;

mdio_error_register:
hw_error_register:
	eth_unregister(edev);

eth_error_register:
	free(priv);

	return ret;
}

static __maybe_unused struct of_device_id enc28j60_dt_ids[] = {
	{
		.compatible = "microchip,enc28j60",
	}, {
		/* sentinel */
	}
};

static struct driver_d enc28j60_driver = {
	.name = DRV_NAME,
	.probe = enc28j60_probe,
	.of_compatible = DRV_OF_COMPAT(enc28j60_dt_ids),
};
device_spi_driver(enc28j60_driver);
