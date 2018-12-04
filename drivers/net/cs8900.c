// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * cs8900.c
 *
 * Cirrus Logic Crystal CS89X0 Ethernet driver for barebox
 * Copyright (C) 2009 Ivo Clarysse <ivo.clarysse@gmail.com>
 *
 * Based on cs8900.c from barebox mainline,
 *   (C) 2003 Wolfgang Denk, wd@denx.de
 *   (C) Copyright 2002 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 *   Marius Groeger <mgroeger@sysgo.de>
 *   Copyright (C) 1999 Ben Williamson <benw@pobox.com>
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <net.h>
#include <malloc.h>
#include <clock.h>
#include <io.h>
#include <errno.h>

/* I/O ports for I/O Space operation */
#define CS8900_RTDATA0	0x0000	/* Receive/Transmit Data (Port 0) */
#define CS8900_RTDATA1	0x0002	/* Receive/Transmit Data (Port 1) */
#define CS8900_TXCMD	0x0004	/* Transmit Command */
#define CS8900_TXLEN	0x0006	/* Transmit Length */
#define CS8900_ISQ	0x0008	/* Interrupt Status Queue */
#define CS8900_PP_PTR	0x000a	/* PacketPage Pointer */
#define CS8900_PP_DATA0	0x000c	/* PacketPage Data (Port 0) */
#define CS8900_PP_DATA1	0x000e	/* PacketPage Data (Port 1) */

/* PacketPage registers */

/* Bus Interface registers */
#define PP_BI_VID	0x0000	/* Vendor (EISA registration #) */
#define PP_BI_PID	0x0002	/* Product ID */
#define PP_BI_IOBASE	0x0020	/* I/O Base Address */
#define PP_BI_IRQ	0x0022	/* Interrupt Number */
#define PP_BI_DMACH	0x0024	/* DMA Channel Number */
#define PP_BI_DMASOF	0x0026	/* DMA Start of Frame */
#define PP_BI_DMAFC	0x0028	/* DMA Frame Count (12bit) */
#define PP_BI_RXDMAC	0x002A	/* RxDMA Byte Count */
#define PP_BI_MBAR	0x002C	/* Memory Base Address Register (20bit) */
#define PP_BI_BPBA	0x0030	/* Boot PROM Base Address (32bit) */
#define PP_BI_BPAM	0x0034	/* Boot PROM Address Mark (32bit) */
#define PP_BI_EE_CMD	0x0040	/* EEPROM Command */
#define PP_BI_EE_DAT	0x0042	/* EEPROM Data */
#define PP_BI_RFBC	0x0050	/* Received Frame Byte Counter */

/* Status and Control registers */
#define PP_REG_ISQ	0x0120	/* Register 0: ISQ */
#define PP_REG_01	0x0100	/* Register 1: -reserved- */
#define PP_REG_02	0x0122	/* Register 2: -reserved- */
#define PP_REG_RXCFG	0x0102	/* Register 3: RxCFG */
#define PP_REG_RXEVENT	0x0124	/* Register 4: RxEvent/RxEventalt */
#define PP_REG_RXCTL	0x0104	/* Register 5: RxCTL */
#define PP_REG_06	0x0126	/* Register 6: -reserved- */
#define PP_REG_TXCFG	0x0106	/* Register 7: TxCFG */
#define PP_REG_TXEVENT	0x0128	/* Register 8: TxEvent */
#define PP_REG_TXCMD	0x0108	/* Register 9: TxCmd (Transmit Command Status) */
#define PP_REG_0A	0x012A	/* Register A: -reserved- */
#define PP_REG_BUFCFG	0x010A	/* Register B: BufCFG */
#define PP_REG_BUFEVENT	0x012C	/* Register C: BufEvent */
#define PP_REG_0C	0x010C	/* Register D: -reserved- */
#define PP_REG_0E	0x012E	/* Register E: -reserved- */
#define PP_REG_0F	0x010E	/* Register F: -reserved- */
#define PP_REG_RXMISS	0x0130	/* Register 10: RxMISS */
#define PP_REG_11	0x0110	/* Register 11: -reserved- */
#define PP_REG_TXCOL	0x0132	/* Register 12: TxCOL */
#define PP_REG_LINECTL	0x0112	/* Register 13: LineCTL */
#define PP_REG_LINEST	0x0134	/* Register 14: LineST */
#define PP_REG_SELFCTL	0x0114	/* Register 15: SelfCTL */
#define PP_REG_SELFST	0x0136	/* Register 16: SelfST */
#define PP_REG_BUSCTL	0x0116	/* Register 17: BusCTL */
#define PP_REG_BUSST	0x0138	/* Register 18: BusST */
#define PP_REG_TESTCTL	0x0118	/* Register 19: TestCTL */
#define PP_REG_1A	0x013A	/* Register 1A: -reserved- */
#define PP_REG_1B	0x011A	/* Register 1B: -reserved- */
#define PP_REG_TDR	0x013C	/* Register 1C: TDR */
#define PP_REG_1D	0x011C	/* Register 1D: -reserved- */
#define PP_REG_1E	0x013E	/* Register 1E: -reserved- */
#define PP_REG_1F	0x011E	/* Register 1F: -reserved- */

/* Initiate Transmit registers */
#define PP_IT_TXCMD	0x0144	/* Transmit Command */
#define PP_IT_TXLEN	0x0146	/* Transmit Length */

/* Address Filter registers */
#define PP_AF_LAF	0x0150	/* Logical Address Filter */
#define PP_AF_IA	0x0158	/* Individual address (IEEE address) */

/* Frame Location */
#define PP_FL_RXSTATUS	0x0400	/* RXStatus */
#define PP_FL_RXLENGTH	0x0402	/* RxLength */
#define PP_FL_RXLOC	0x0404	/* Receive Frame Location */
#define PP_FL_TXLOC	0x0A00	/* Transmit Frame Location */

/* Register bit masks */
#define RXEVENT_RXOK		0x0100
#define RXCTL_RXOKA		0x0100
#define RXCTL_MULTICASTA	0x0200
#define RXCTL_INDIVIDUALA	0x0400
#define RXCTL_BROADCASTA	0x0800
#define TXCMD_TXSTART_5		0x0000	/* Start Tx after 5 bytes */
#define TXCMD_TXSTART_381	0x0040	/* Start Tx after 381 bytes */
#define TXCMD_TXSTART_1021	0x0080	/* Start Tx after 1021 bytes */
#define TXCMD_TXSTART_FULL	0x00C0	/* Start Tx after all bytes loaded */
#define LINECTL_SERRXON		0x0040
#define LINECTL_SERTXON		0x0080
#define LINEST_LINKOK		0x0080
#define LINEST_AUI		0x0100
#define LINEST_10BT		0x0200
#define LINEST_POLARITYOK	0x1000
#define LINEST_CRS		0x4000
#define SELFCTL_RESET		0x0040
#define SELFST_33VACTIVE	0x0040
#define SELFST_INITD		0x0080
#define SELFST_SIBUSY		0x0100
#define SELFST_EEPROMP		0x0200
#define SELFST_EEPROMOK		0x0400
#define SELFST_ELPRESENT	0x0800
#define SELFST_EESIZE		0x1000
#define BUSST_TXBIDERR		0x0080
#define BUSST_RDY4TXNOW		0x0100

#define CRYSTAL_SEMI_EISA_ID	0x630E

/* Origin: CS8900A datasheet */
#define CS8X0_PID_CS8900A_B	0x0700
#define CS8X0_PID_CS8900A_C	0x0800
#define CS8X0_PID_CS8900A_D	0x0900
#define CS8X0_PID_CS8900A_F	0x0A00
/* Origin: CS8920A datasheet */
#define CS8X0_PID_CS8920_B 	0x6100
#define CS8X0_PID_CS8920_C 	0x6200
#define CS8X0_PID_CS8920_D 	0x6300
#define CS8X0_PID_CS8920A_AB 	0x6400
#define CS8X0_PID_CS8920A_C	0x6500

typedef enum {
	CS8900,
	CS8900A,
	CS8920,
	CS8920A,
} cs89x0_chip_type_t;

struct cs89x0_chip {
	cs89x0_chip_type_t chip_type;
	const char *name;
};

struct cs89x0_product {
	u16 product_id;
	cs89x0_chip_type_t chip_type;
	const char *rev_name;
};

static struct cs89x0_chip cs89x0_chips[] = {
	{CS8900A, "CS8900A"},
	{CS8920A, "CS8920A"},	/* EOL: March 30, 2003 */
	{CS8900, "CS8900"},	/* EOL: September 31, 1999 */
	{CS8920, "CS8920"},	/* EOL: December 9, 1998 */
};

static struct cs89x0_product cs89x0_product_ids[] = {
	{CS8X0_PID_CS8900A_F, CS8900A, "F"},
	{CS8X0_PID_CS8900A_D, CS8900A, "D"},
	{CS8X0_PID_CS8900A_C, CS8900A, "C"},
	{CS8X0_PID_CS8900A_B, CS8900A, "B"},
	{CS8X0_PID_CS8920A_C, CS8920A, "C"},
	{CS8X0_PID_CS8920A_AB, CS8920A, "A/B"},
	{CS8X0_PID_CS8920_D, CS8920, "D"},
	{CS8X0_PID_CS8920_C, CS8920, "C"},
	{CS8X0_PID_CS8920_B, CS8920, "B"},
};

struct cs8900_priv {
	void *regs;
	struct cs89x0_product *product;
	struct cs89x0_chip *chip;
};

/* Read a 16-bit value from PacketPage Memory using I/O Space operation */
static u16 cs8900_ior(struct cs8900_priv *priv, int reg)
{
	u16 result;
	writew(reg, priv->regs + CS8900_PP_PTR);
	result = readw(priv->regs + CS8900_PP_DATA0);
	return result;
}

/* Write a 16-bit value to PacketPage Memory using I/O Space operation */
static void cs8900_iow(struct cs8900_priv *priv, int reg, u16 value)
{
	writew(reg, priv->regs + CS8900_PP_PTR);
	writew(value, priv->regs + CS8900_PP_DATA0);
}

static void cs8900_reginit(struct cs8900_priv *priv)
{
	/* receive only error free packets addressed to this card */
	cs8900_iow(priv, PP_REG_RXCTL,
		   RXCTL_BROADCASTA | RXCTL_INDIVIDUALA | RXCTL_RXOKA);
	/* do not generate any interrupts on receive operations */
	cs8900_iow(priv, PP_REG_RXCFG, 0);
	/* do not generate any interrupts on transmit operations */
	cs8900_iow(priv, PP_REG_TXCFG, 0);
	/* do not generate any interrupts on buffer operations */
	cs8900_iow(priv, PP_REG_BUFCFG, 0);
	/* enable transmitter/receiver mode */
	cs8900_iow(priv, PP_REG_LINECTL,
		   LINECTL_SERRXON | LINECTL_SERTXON);
}

static void cs8900_reset(struct cs8900_priv *priv)
{
	u16 v;
	uint64_t tmo;

	v = cs8900_ior(priv, PP_REG_SELFCTL);
	v |= SELFCTL_RESET;
	cs8900_iow(priv, PP_REG_SELFCTL, v);

	/* wait 200ms */
	udelay(200000);

	tmo = get_time_ns();
	while ((cs8900_ior(priv, PP_REG_SELFST) & SELFST_INITD) == 0) {
		if (is_timeout(tmo, 1 * SECOND)) {
			printf("%s: timeout\n", __func__);
			break;
		}
	}
}


static int cs8900_dev_init(struct eth_device *dev)
{
	return 0;
}

static int cs8900_open(struct eth_device *dev)
{
	struct cs8900_priv *priv = (struct cs8900_priv *)dev->priv;
	cs8900_reset(priv);
	cs8900_reginit(priv);
	return 0;
}

static int cs8900_send(struct eth_device *dev, void *eth_data,
		       int data_length)
{
	struct cs8900_priv *priv = (struct cs8900_priv *)dev->priv;
	u16 *addr;
	u16 v;

	writew(TXCMD_TXSTART_FULL, priv->regs + CS8900_TXCMD);
	writew(data_length, priv->regs + CS8900_TXLEN);

	v = cs8900_ior(priv, PP_REG_BUSST);

	if (v & BUSST_TXBIDERR) {
		printf("%s: frame error\n", __func__);
		return -1;
	}
	if ((v & BUSST_RDY4TXNOW) == 0) {
		printf("%s: busy\n", __func__);
		return -1;
	}

	for (addr = eth_data; data_length > 0; data_length -= 2) {
		writew(*addr++, priv->regs + CS8900_RTDATA0);
	}

	return 0;
}

static int cs8900_recv(struct eth_device *dev)
{
	struct cs8900_priv *priv = (struct cs8900_priv *)dev->priv;
	int len = 0;
	u16 status;
	u16 *addr;
	int i;

	status = cs8900_ior(priv, PP_REG_RXEVENT);
	if ((status & RXEVENT_RXOK) == 0) {
		/* No packet received. */
		return 0;
	}

	status = readw(priv->regs + CS8900_RTDATA0);
	len = readw(priv->regs + CS8900_RTDATA0);

	for (addr = (u16 *) NetRxPackets[0], i = len >> 1; i > 0; i--) {
		*addr++ = readw(priv->regs + CS8900_RTDATA0);
	}
	if (len & 1) {
		*addr++ = readw(priv->regs + CS8900_RTDATA0);
	}
	net_receive(dev, NetRxPackets[0], len);

	return len;
}

static void cs8900_halt(struct eth_device *dev)
{
	struct cs8900_priv *priv = (struct cs8900_priv *)dev->priv;
	cs8900_iow(priv, PP_REG_BUSCTL, 0);
	cs8900_iow(priv, PP_REG_TESTCTL, 0);
	cs8900_iow(priv, PP_REG_SELFCTL, 0);
	cs8900_iow(priv, PP_REG_LINECTL, 0);
	cs8900_iow(priv, PP_REG_BUFCFG, 0);
	cs8900_iow(priv, PP_REG_TXCFG, 0);
	cs8900_iow(priv, PP_REG_RXCTL, 0);
	cs8900_iow(priv, PP_REG_RXCFG, 0);
}

static int cs8900_get_ethaddr(struct eth_device *dev, unsigned char *mac)
{
	struct cs8900_priv *priv = (struct cs8900_priv *)dev->priv;
	int i;
	for (i = 0; i < 6 / 2; i++) {
		u16 v;
		v = cs8900_ior(priv, PP_AF_IA + i * 2);
		mac[i * 2] = v & 0xFF;
		mac[i * 2 + 1] = v >> 8;
	}

	return 0;
}

static int cs8900_set_ethaddr(struct eth_device *dev, const unsigned char *mac)
{
	struct cs8900_priv *priv = (struct cs8900_priv *)dev->priv;
	int i;
	for (i = 0; i < 6 / 2; i++) {
		u16 v;
		v = (mac[i * 2 + 1] << 8) | mac[i * 2];
		cs8900_iow(priv, PP_AF_IA + i * 2, v);
	}
	return 0;
}

static const char *yesno_str(int v)
{
	return v ? "yes" : "no";
}

static void cs8900_info(struct device_d *dev)
{
	struct eth_device *edev = dev_to_edev(dev);
	struct cs8900_priv *priv = (struct cs8900_priv *)edev->priv;
	u16 v;

	printf("%s Rev. %s (PID: 0x%04x) at 0x%p\n", priv->chip->name,
	       priv->product->rev_name, priv->product->product_id,
	       priv->regs);

	v = cs8900_ior(priv, PP_REG_LINEST);
	printf("Register 14: LineST     0x%04x\n", v);
	printf("  Link OK: %s\n", yesno_str(v & LINEST_LINKOK));
	printf("  Using AUI: %s\n", yesno_str(v & LINEST_AUI));
	printf("  Using 10Base-T: %s\n", yesno_str(v & LINEST_10BT));
	printf("  Polarity OK: %s\n", yesno_str(v & LINEST_POLARITYOK));
	printf("  Incoming frame: %s\n", yesno_str(v & LINEST_CRS));

	v = cs8900_ior(priv, PP_REG_SELFST);
	printf("Register 16: SelfST     0x%04x\n", v);
	printf("  Voltage: %sV\n", (v & SELFST_33VACTIVE) ? "3.3" : "5");
	printf("  Initialization complete: %s\n",
	       yesno_str(v & SELFST_INITD));
	printf("  EEPROM busy: %s\n", yesno_str(v & SELFST_SIBUSY));
	printf("  EEPROM present: %s\n", yesno_str(v & SELFST_EEPROMP));
	printf("  EEPROM checksum OK: %s\n",
	       yesno_str(v & SELFST_EEPROMOK));
	printf("  EL present: %s\n", yesno_str(v & SELFST_ELPRESENT));
	printf("  EEPROM size: %s words\n",
	       (v & SELFST_EESIZE) ? "64" : "128 or 256");
}

static int cs8900_check_id(struct cs8900_priv *priv)
{
	int result = -1;
	struct cs89x0_product *product = NULL;
	struct cs89x0_chip *chip = NULL;
	int i;
	u16 v;
	v = cs8900_ior(priv, PP_BI_VID);
	if (v != CRYSTAL_SEMI_EISA_ID) {
		printf("No CS89x0 variant found at 0x%p vid: 0x%04x\n",
		       priv->regs, v);
		return -1;
	}
	v = cs8900_ior(priv, PP_BI_PID);
	for (i = 0; i < ARRAY_SIZE(cs89x0_product_ids); i++) {
		if (cs89x0_product_ids[i].product_id == v) {
			product = &(cs89x0_product_ids[i]);
			break;
		}
	}
	if (product == NULL) {
		printf("Unable to identify CS89x0 variant 0x%04x\n", v);
		goto out;
	}
	for (i = 0; i < ARRAY_SIZE(cs89x0_chips); i++) {
		if (cs89x0_chips[i].chip_type == product->chip_type) {
			chip = &(cs89x0_chips[i]);
			break;
		}
	}
	if (chip == NULL) {
		printf("Invalid chip type %d\n", product->chip_type);
		goto out;
	}
	printf("%s Rev. %s (PID: 0x%04x) found at 0x%p\n", chip->name,
	       product->rev_name, v, priv->regs);
	priv->chip = chip;
	priv->product = product;
	result = 0;
      out:
	return result;
}

static int cs8900_probe(struct device_d *dev)
{
	struct resource *iores;
	struct eth_device *edev;
	struct cs8900_priv *priv;

	debug("cs8900_init()\n");

	priv = (struct cs8900_priv *)xmalloc(sizeof(*priv));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->regs = IOMEM(iores->start);
	if (cs8900_check_id(priv)) {
		free(priv);
		return -1;
	}

	edev = (struct eth_device *)xmalloc(sizeof(struct eth_device));
	edev->priv = priv;

	edev->init = cs8900_dev_init;
	edev->open = cs8900_open;
	edev->send = cs8900_send;
	edev->recv = cs8900_recv;
	edev->halt = cs8900_halt;
	edev->get_ethaddr = cs8900_get_ethaddr;
	edev->set_ethaddr = cs8900_set_ethaddr;
	edev->parent = dev;

	dev->info = cs8900_info;

	eth_register(edev);
	return 0;
}

static struct driver_d cs8900_driver = {
	.name = "cs8900",
	.probe = cs8900_probe,
};
device_platform_driver(cs8900_driver);
