/*
 *	dm9000.c
 *
 *	A Davicom DM9000 ISA NIC fast Ethernet driver for Linux.
 *	Copyright (C) 1997  Sten Wang
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 * (C)Copyright 1997-1998 DAVICOM Semiconductor,Inc. All Rights Reserved.
 *
 * V0.11	06/20/2001	REG_0A bit3=1, default enable BP with DA match
 *		06/22/2001 	Support DM9801 progrmming
 *	 	 		E3: R25 = ((R24 + NF) & 0x00ff) | 0xf000
 *		 		E4: R25 = ((R24 + NF) & 0x00ff) | 0xc200
 *				R17 = (R17 & 0xfff0) | NF + 3
 *		 		E5: R25 = ((R24 + NF - 3) & 0x00ff) | 0xc200
 *     				R17 = (R17 & 0xfff0) | NF
 *
 * v1.00			modify by simon 2001.9.5
 *				change for kernel 2.4.x
 *
 *  v1.1	11/09/2001      fix force mode bug
 *
 *  v1.2	03/18/2003	Weilun Huang <weilun_huang@davicom.com.tw>:
 *				Fixed phy reset.
 *				Added tx/rx 32 bit mode.
 *				Cleaned up for kernel merge.
 *
 *
 *
 *		12/15/2003      Initial port to barebox by Sascha Hauer
 *				<saschahauer@web.de>
 *
 * ... see commit logs
 */

#include <common.h>
#include <command.h>
#include <driver.h>
#include <clock.h>
#include <miidev.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <asm/io.h>
#include <xfuncs.h>
#include <dm9000.h>
#include <errno.h>

#define DM9000_ID		0x90000A46
#define DM9000_PKT_MAX		1536	/* Received packet max size */
#define DM9000_PKT_RDY		0x01	/* Packet ready to receive */

#define DM9000_NCR             0x00
#define DM9000_NSR             0x01
#define DM9000_TCR             0x02
#define DM9000_TSR1            0x03
#define DM9000_TSR2            0x04
#define DM9000_RCR             0x05
#define DM9000_RSR             0x06
#define DM9000_ROCR            0x07
#define DM9000_BPTR            0x08
#define DM9000_FCTR            0x09
#define DM9000_FCR             0x0A
#define DM9000_EPCR            0x0B
#define DM9000_EPAR            0x0C
#define DM9000_EPDRL           0x0D
#define DM9000_EPDRH           0x0E
#define DM9000_WCR             0x0F

#define DM9000_PAR             0x10
#define DM9000_MAR             0x16

#define DM9000_GPCR            0x1e
#define DM9000_GPR             0x1f
#define DM9000_TRPAL           0x22
#define DM9000_TRPAH           0x23
#define DM9000_RWPAL           0x24
#define DM9000_RWPAH           0x25

#define DM9000_VIDL            0x28
#define DM9000_VIDH            0x29
#define DM9000_PIDL            0x2A
#define DM9000_PIDH            0x2B

#define DM9000_CHIPR           0x2C
#define DM9000_SMCR            0x2F

#define DM9000_PHY		0x40	/* PHY address 0x01 */

#define DM9000_MRCMDX          0xF0
#define DM9000_MRCMD           0xF2
#define DM9000_MRRL            0xF4
#define DM9000_MRRH            0xF5
#define DM9000_MWCMDX			0xF6
#define DM9000_MWCMD           0xF8
#define DM9000_MWRL            0xFA
#define DM9000_MWRH            0xFB
#define DM9000_TXPLL           0xFC
#define DM9000_TXPLH           0xFD
#define DM9000_ISR             0xFE
#define DM9000_IMR             0xFF

#define NCR_EXT_PHY		(1<<7)
#define NCR_WAKEEN		(1<<6)
#define NCR_FCOL		(1<<4)
#define NCR_FDX			(1<<3)
#define NCR_LBK			(3<<1)
#define NCR_RST			(1<<0)

#define NSR_SPEED		(1<<7)
#define NSR_LINKST		(1<<6)
#define NSR_WAKEST		(1<<5)
#define NSR_TX2END		(1<<3)
#define NSR_TX1END		(1<<2)
#define NSR_RXOV		(1<<1)

#define TCR_TJDIS		(1<<6)
#define TCR_EXCECM		(1<<5)
#define TCR_PAD_DIS2		(1<<4)
#define TCR_CRC_DIS2		(1<<3)
#define TCR_PAD_DIS1		(1<<2)
#define TCR_CRC_DIS1		(1<<1)
#define TCR_TXREQ		(1<<0)

#define TSR_TJTO		(1<<7)
#define TSR_LC			(1<<6)
#define TSR_NC			(1<<5)
#define TSR_LCOL		(1<<4)
#define TSR_COL			(1<<3)
#define TSR_EC			(1<<2)

#define RCR_WTDIS		(1<<6)
#define RCR_DIS_LONG		(1<<5)
#define RCR_DIS_CRC		(1<<4)
#define RCR_ALL			(1<<3)
#define RCR_RUNT		(1<<2)
#define RCR_PRMSC		(1<<1)
#define RCR_RXEN		(1<<0)

#define RSR_RF			(1<<7)
#define RSR_MF			(1<<6)
#define RSR_LCS			(1<<5)
#define RSR_RWTO		(1<<4)
#define RSR_PLE			(1<<3)
#define RSR_AE			(1<<2)
#define RSR_CE			(1<<1)
#define RSR_FOE			(1<<0)

#define FCTR_HWOT(ot)	(( ot & 0xf ) << 4 )
#define FCTR_LWOT(ot)	( ot & 0xf )

#define IMR_PAR			(1<<7)
#define IMR_ROOM		(1<<3)
#define IMR_ROM			(1<<2)
#define IMR_PTM			(1<<1)
#define IMR_PRM			(1<<0)

struct dm9000_priv {
	void __iomem *iobase;
	void __iomem *iodata;
	struct mii_device miidev;
	int buswidth;
	int srom;
};

#ifdef CONFIG_DM9000_DEBUG
static void
dump_regs(void)
{
	debug("\n");
	debug("NCR   (0x00): %02x\n", DM9000_ior(0));
	debug("NSR   (0x01): %02x\n", DM9000_ior(1));
	debug("TCR   (0x02): %02x\n", DM9000_ior(2));
	debug("TSRI  (0x03): %02x\n", DM9000_ior(3));
	debug("TSRII (0x04): %02x\n", DM9000_ior(4));
	debug("RCR   (0x05): %02x\n", DM9000_ior(5));
	debug("RSR   (0x06): %02x\n", DM9000_ior(6));
	debug("ISR   (0xFE): %02x\n", DM9000_ior(ISR));
	debug("\n");
}
#endif

static u8 DM9000_ior(struct dm9000_priv *priv, int reg)
{
	writeb(reg, priv->iobase);
	return readb(priv->iodata);
}

static void DM9000_iow(struct dm9000_priv *priv, int reg, u8 value)
{
	writeb(reg, priv->iobase);
	writeb(value, priv->iodata);
}

static int dm9000_phy_read(struct mii_device *mdev, int addr, int reg)
{
	int val;
	struct eth_device *edev = mdev->edev;
	struct dm9000_priv *priv = edev->priv;

	/* Fill the phyxcer register into REG_0C */
	DM9000_iow(priv, DM9000_EPAR, DM9000_PHY | reg);
	DM9000_iow(priv, DM9000_EPCR, 0xc);	/* Issue phyxcer read command */
	udelay(100);			/* Wait read complete */
	DM9000_iow(priv, DM9000_EPCR, 0x0);	/* Clear phyxcer read command */
	val = (DM9000_ior(priv, DM9000_EPDRH) << 8) | DM9000_ior(priv, DM9000_EPDRL);

	/* The read data keeps on REG_0D & REG_0E */
	debug("phy_read(%d): %d\n", reg, val);
	return val;
}

static int dm9000_phy_write(struct mii_device *mdev, int addr, int reg, int val)
{
	struct eth_device *edev = mdev->edev;
	struct dm9000_priv *priv = edev->priv;

	/* Fill the phyxcer register into REG_0C */
	DM9000_iow(priv, DM9000_EPAR, DM9000_PHY | reg);

	/* Fill the written data into REG_0D & REG_0E */
	DM9000_iow(priv, DM9000_EPDRL, (val & 0xff));
	DM9000_iow(priv, DM9000_EPDRH, ((val >> 8) & 0xff));
	DM9000_iow(priv, DM9000_EPCR, 0xa);	/* Issue phyxcer write command */
	udelay(500);			/* Wait write complete */
	DM9000_iow(priv, DM9000_EPCR, 0x0);	/* Clear phyxcer write command */

	debug("phy_write(reg:%d, value:%d)\n", reg, value);

	return 0;
}

static int dm9000_check_id(struct dm9000_priv *priv)
{
	u32 id_val;
	id_val = DM9000_ior(priv, DM9000_VIDL);
	id_val |= DM9000_ior(priv, DM9000_VIDH) << 8;
	id_val |= DM9000_ior(priv, DM9000_PIDL) << 16;
	id_val |= DM9000_ior(priv, DM9000_PIDH) << 24;
	if (id_val == DM9000_ID) {
		printf("dm9000 i/o: 0x%p, id: 0x%x \n", priv->iobase,
		       id_val);
		return 0;
	} else {
		printf("dm9000 not found at 0x%p id: 0x%08x\n",
		       priv->iobase, id_val);
		return -1;
	}
}

static void dm9000_reset(struct dm9000_priv *priv)
{
	debug("resetting\n");
	DM9000_iow(priv, DM9000_NCR, NCR_RST);
	udelay(1000);		/* delay 1ms */
}

static int dm9000_eth_open(struct eth_device *edev)
{
	struct dm9000_priv *priv = (struct dm9000_priv *)edev->priv;

	miidev_wait_aneg(&priv->miidev);
	miidev_print_status(&priv->miidev);
	return 0;
}

static int dm9000_eth_send (struct eth_device *edev,
		void *packet, int length)
{
	struct dm9000_priv *priv = (struct dm9000_priv *)edev->priv;
	char *data_ptr;
	u32 tmplen, i;
	uint64_t tmo;

	debug("eth_send: length: %d\n", length);

	for (i = 0; i < length; i++) {
		if (i % 8 == 0)
			debug("\nSend: 02x: ", i);
		debug("%02x ", ((unsigned char *) packet)[i]);
	} debug("\n");

	/* Move data to DM9000 TX RAM */
	data_ptr = (char *) packet;
	writeb(DM9000_MWCMD, priv->iobase);

	switch (priv->buswidth) {
	case DM9000_WIDTH_8:
		for (i = 0; i < length; i++)
			writeb(data_ptr[i] & 0xff, priv->iodata);
		break;
	case DM9000_WIDTH_16:
		tmplen = (length + 1) / 2;
		for (i = 0; i < tmplen; i++)
			writew(((u16 *)data_ptr)[i], priv->iodata);
		break;
	case DM9000_WIDTH_32:
		tmplen = (length + 3) / 4;
		for (i = 0; i < tmplen; i++)
			writel(((u32 *) data_ptr)[i], priv->iodata);
		break;
	default:
		/* dm9000_probe makes sure this cannot happen */
		return -EINVAL;
	}

	/* Set TX length to DM9000 */
	DM9000_iow(priv, DM9000_TXPLL, length & 0xff);
	DM9000_iow(priv, DM9000_TXPLH, (length >> 8) & 0xff);

	/* Issue TX polling command */
	DM9000_iow(priv, DM9000_TCR, TCR_TXREQ);	/* Cleared after TX complete */

	/* wait for end of transmission */
	tmo = get_time_ns();
	while (DM9000_ior(priv, DM9000_TCR) & TCR_TXREQ) {
		if (is_timeout(tmo, 5 * SECOND)) {
			printf("transmission timeout\n");
			break;
		}
	}
	debug("transmit done\n\n");
	return 0;
}

static void dm9000_eth_halt (struct eth_device *edev)
{
	debug("eth_halt\n");
#if 0
	phy_write(0, 0x8000);	/* PHY RESET */
	DM9000_iow(DM9000_GPR, 0x01);	/* Power-Down PHY */
	DM9000_iow(DM9000_IMR, 0x80);	/* Disable all interrupt */
	DM9000_iow(DM9000_RCR, 0x00);	/* Disable RX */
#endif
}

static int dm9000_eth_rx (struct eth_device *edev)
{
	struct dm9000_priv *priv = (struct dm9000_priv *)edev->priv;
	u8 rxbyte, *rdptr = (u8 *) NetRxPackets[0];
	u16 RxStatus, RxLen = 0;
	u32 tmplen, i;
	u32 tmpdata;

	/* Check packet ready or not */
	DM9000_ior(priv, DM9000_MRCMDX);	/* Dummy read */
	rxbyte = readb(priv->iodata);	/* Got most updated data */
	if (rxbyte == 0)
		return 0;

	/* Status check: this byte must be 0 or 1 */
	if (rxbyte > 1) {
		DM9000_iow(priv, DM9000_RCR, 0x00);	/* Stop Device */
		DM9000_iow(priv, DM9000_ISR, 0x80);	/* Stop INT request */
		debug("rx status check: %d\n", rxbyte);
	}
	debug("receiving packet\n");

	/* A packet ready now  & Get status/length */
	writeb(DM9000_MRCMD, priv->iobase);

	/* Move data from DM9000 */
	/* Read received packet from RX SRAM */
	switch (priv->buswidth) {
	case DM9000_WIDTH_8:
		RxStatus = readb(priv->iodata) + (readb(priv->iodata) << 8);
		RxLen = readb(priv->iodata) + (readb(priv->iodata) << 8);
		for (i = 0; i < RxLen; i++)
			rdptr[i] = readb(priv->iodata);
		break;
	case DM9000_WIDTH_16:
		RxStatus = readw(priv->iodata);
		RxLen = readw(priv->iodata);
		tmplen = (RxLen + 1) / 2;
		for (i = 0; i < tmplen; i++)
			((u16 *) rdptr)[i] = readw(priv->iodata);
		break;
	case DM9000_WIDTH_32:
		tmpdata = readl(priv->iodata);
		RxStatus = tmpdata;
		RxLen = tmpdata >> 16;
		tmplen = (RxLen + 3) / 4;
		for (i = 0; i < tmplen; i++)
			((u32 *) rdptr)[i] = readl(priv->iodata);
		break;
	default:
		/* dm9000_probe makes sure this cannot happen */
		return -EINVAL;
	}

	if ((RxStatus & 0xbf00) || (RxLen < 0x40)
	    || (RxLen > DM9000_PKT_MAX)) {
		if (RxStatus & 0x100) {
			printf("rx fifo error\n");
		}
		if (RxStatus & 0x200) {
			printf("rx crc error\n");
		}
		if (RxStatus & 0x8000) {
			printf("rx length error\n");
		}
		if (RxLen > DM9000_PKT_MAX) {
			printf("rx length too big\n");
			dm9000_reset(priv);
		}
	} else {

		/* Pass to upper layer */
		debug("passing packet to upper layer\n");
		net_receive(NetRxPackets[0], RxLen);
		return RxLen;
	}
	return 0;
}

static u16 read_srom_word(struct dm9000_priv *priv, int offset)
{
	DM9000_iow(priv, DM9000_EPAR, offset);
	DM9000_iow(priv, DM9000_EPCR, 0x4);
	udelay(200);
	DM9000_iow(priv, DM9000_EPCR, 0x0);
	return (DM9000_ior(priv, DM9000_EPDRL) + (DM9000_ior(priv, DM9000_EPDRH) << 8));
}

static int dm9000_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct dm9000_priv *priv = (struct dm9000_priv *)edev->priv;
	int i, oft;

	if (priv->srom) {
		for (i = 0; i < 3; i++)
			((u16 *) adr)[i] = read_srom_word(priv, i);
	} else {
		for (i = 0, oft = 0x10; i < 6; i++, oft++)
			adr[i] = DM9000_ior(priv, oft);
	}

	return 0;
}

static int dm9000_set_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct dm9000_priv *priv = (struct dm9000_priv *)edev->priv;
	int i, oft;

	debug("%s\n", __FUNCTION__);

	for (i = 0, oft = 0x10; i < 6; i++, oft++)
		DM9000_iow(priv, oft, adr[i]);
	for (i = 0, oft = 0x16; i < 8; i++, oft++)
		DM9000_iow(priv, oft, 0xff);

#if 0
	for (i = 0; i < 5; i++)
		printf ("%02x:", adr[i]);
	printf ("%02x\n", adr[5]);
#endif
	return 0;
}

static int dm9000_init_dev(struct eth_device *edev)
{
	struct dm9000_priv *priv = (struct dm9000_priv *)edev->priv;

	miidev_restart_aneg(&priv->miidev);
	return 0;
}

static int dm9000_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct dm9000_priv *priv;
	struct dm9000_platform_data *pdata;

	debug("dm9000_eth_init()\n");

	edev = xzalloc(sizeof(struct eth_device) + sizeof(struct dm9000_priv));
	dev->type_data = edev;
	edev->priv = (struct dm9000_priv *)(edev + 1);

	if (!dev->platform_data) {
		printf("dm9000: no platform_data\n");
		return -ENODEV;
	}
	pdata = dev->platform_data;

	priv = edev->priv;
	priv->buswidth = pdata->buswidth;
	priv->iodata = (void *)pdata->iodata;
	priv->iobase = (void *)pdata->iobase;
	priv->srom = pdata->srom;

	edev->init = dm9000_init_dev;
	edev->open = dm9000_eth_open;
	edev->send = dm9000_eth_send;
	edev->recv = dm9000_eth_rx;
	edev->halt = dm9000_eth_halt;
	edev->set_ethaddr = dm9000_set_ethaddr;
	edev->get_ethaddr = dm9000_get_ethaddr;

	/* RESET device */
	dm9000_reset(priv);
	if(dm9000_check_id(priv))
		return -1;

	/* Program operating register */
	DM9000_iow(priv, DM9000_NCR, 0x0);	/* only intern phy supported by now */
	DM9000_iow(priv, DM9000_TCR, 0);	/* TX Polling clear */
	DM9000_iow(priv, DM9000_BPTR, 0x3f);	/* Less 3Kb, 200us */
	DM9000_iow(priv, DM9000_FCTR, FCTR_HWOT(3) | FCTR_LWOT(8));	/* Flow Control : High/Low Water */
	DM9000_iow(priv, DM9000_FCR, 0x0);	/* SH FIXME: This looks strange! Flow Control */
	DM9000_iow(priv, DM9000_SMCR, 0);	/* Special Mode */
	DM9000_iow(priv, DM9000_NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END);	/* clear TX status */
	DM9000_iow(priv, DM9000_ISR, 0x0f);	/* Clear interrupt status */

	/* Activate DM9000 */
	DM9000_iow(priv, DM9000_GPCR, 0x01);	/* Let GPIO0 output */
	DM9000_iow(priv, DM9000_GPR, 0x00);	/* Enable PHY */
	DM9000_iow(priv, DM9000_RCR, RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN);	/* RX enable */
	DM9000_iow(priv, DM9000_IMR, IMR_PAR);	/* Enable TX/RX interrupt mask */

	priv->miidev.read = dm9000_phy_read;
	priv->miidev.write = dm9000_phy_write;
	priv->miidev.address = 0;
	priv->miidev.flags = 0;
	priv->miidev.edev = edev;

	mii_register(&priv->miidev);
	eth_register(edev);

        return 0;
}

static struct driver_d dm9000_driver = {
        .name  = "dm9000",
        .probe = dm9000_probe,
};

static int dm9000_init(void)
{
        register_driver(&dm9000_driver);
        return 0;
}

device_initcall(dm9000_init);

