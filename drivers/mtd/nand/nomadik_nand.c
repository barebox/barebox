/*
 *  drivers/mtd/nand/nomadik_nand.c
 *
 *  Overview:
 *  	Driver for on-board NAND flash on Nomadik Platforms
 *
 * Copyright (C) 2007 STMicroelectronics Pvt. Ltd.
 * Author: Sachin Verma <sachin.verma@st.com>
 *
 * Copyright (C) 2009 Alessandro Rubini
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>

#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>

#include <io.h>
#include <mach/nand.h>
#include <mach/fsmc.h>

#include <errno.h>

struct nomadik_nand_host {
	struct mtd_info		mtd;
	struct nand_chip	nand;
	void __iomem *cmd_va;
	void __iomem *addr_va;
	struct nand_bbt_descr *bbt_desc;
};

static inline int parity(int b) /* uses low 8 bits: returns 0 or all-1 */
{
	b = b ^ (b >> 4);
	b = b ^ (b >> 2);
	return (b ^ (b >> 1)) & 1
		? ~0 : 0;
}

/*
 * This is the ECC routine used in hardware, according to the manual.
 * HW claims to make the calculation but not the correction. However,
 * I haven't managed to get the desired data out of it; so do it in sw.
 * There is problably some errata involved, but currently miss the info.
 */
static int nomadik_ecc512_calc(struct mtd_info *mtd, const u_char *data,
			u_char *ecc)
{
	int gpar = 0;
	int i, val, par;
	int pbits = 0;		/* P8, P16, ... P2048 */
	int pprime = 0;		/* P8', P16', ... P2048' */
	int lowbits;		/* P1, P2, P4 and primes */

	for (i = 0; i < 512; i++) {
		par = parity((val = data[i]));
		gpar ^= val;
		pbits ^= (i & par);
	}
	/*
	 * Ok, now gpar is global parity (xor of all bytes)
	 * pbits are all the parity bits (non-prime ones)
	 */
	par = parity(gpar);
	pprime = pbits ^ par;
	/* Put low bits in the right position for ecc[2] (bits 7..2) */
	lowbits = 0
		| (parity(gpar & 0xf0) & 0x80)	/* P4  */
		| (parity(gpar & 0x0f) & 0x40)	/* P4' */
		| (parity(gpar & 0xcc) & 0x20)	/* P2  */
		| (parity(gpar & 0x33) & 0x10)	/* P2' */
		| (parity(gpar & 0xaa) & 0x08)	/* P1  */
		| (parity(gpar & 0x55) & 0x04);	/* P1' */

	ecc[2] = ~(lowbits | ((pbits & 0x100) >> 7) | ((pprime & 0x100) >> 8));
	/* now intermix bits for ecc[1] (P1024..P128') and ecc[0] (P64..P8') */
	ecc[1] = ~(    (pbits & 0x80) >> 0  | ((pprime & 0x80) >> 1)
		    | ((pbits & 0x40) >> 1) | ((pprime & 0x40) >> 2)
		    | ((pbits & 0x20) >> 2) | ((pprime & 0x20) >> 3)
		    | ((pbits & 0x10) >> 3) | ((pprime & 0x10) >> 4));

	ecc[0] = ~(    (pbits & 0x8) << 4  | ((pprime & 0x8) << 3)
		    | ((pbits & 0x4) << 3) | ((pprime & 0x4) << 2)
		    | ((pbits & 0x2) << 2) | ((pprime & 0x2) << 1)
		    | ((pbits & 0x1) << 1) | ((pprime & 0x1) << 0));
	return 0;
}

static int nomadik_ecc512_correct(struct mtd_info *mtd, uint8_t *dat,
				uint8_t *r_ecc, uint8_t *c_ecc)
{
	struct nand_chip *chip = mtd->priv;
	uint32_t r, c, d, diff; /*read, calculated, xor of them */

	if (!memcmp(r_ecc, c_ecc, chip->ecc.bytes))
		return 0;

	/* Reorder the bytes into ascending-order 24 bits -- see manual */
	r = r_ecc[2] << 22 | r_ecc[1] << 14 | r_ecc[0] << 6 | r_ecc[2] >> 2;
	c = c_ecc[2] << 22 | c_ecc[1] << 14 | c_ecc[0] << 6 | c_ecc[2] >> 2;
	diff = (r ^ c) & ((1 << 24) - 1); /* use 24 bits only */

	/* If 12 bits are different, one per pair, it's correctable */
	if (((diff | (diff>>1)) & 0x555555) == 0x555555) {
		int bit = ((diff & 2) >> 1)
			| ((diff & 0x8) >> 2) | ((diff & 0x20) >> 3);
		int byte;

		d = diff >> 6; /* remove bit-order info */
		byte =  ((d & 2) >> 1)
			| ((d & 0x8) >> 2) | ((d & 0x20) >> 3)
			| ((d & 0x80) >> 4) | ((d & 0x200) >> 5)
			| ((d & 0x800) >> 6) | ((d & 0x2000) >> 7)
			| ((d & 0x8000) >> 8) | ((d & 0x20000) >> 9);
		/* correct the single bit */
		dat[byte] ^= 1 << bit;
		return 0;
	}
	/* If 1 bit only differs, it's one bit error in ECC, ignore */
	if ((diff ^ (1 << (ffs(diff) - 1))) == 0)
		return 0;
	/* Otherwise, uncorrectable */
	return -1;
}

static struct nand_ecclayout nomadik_ecc_layout = {
	.eccbytes = 3 * 4,
	.eccpos = { /* each subpage has 16 bytes: pos 2,3,4 hosts ECC */
		0x02, 0x03, 0x04,
		0x12, 0x13, 0x14,
		0x22, 0x23, 0x24,
		0x32, 0x33, 0x34},
	/* let's keep bytes 5,6,7 for us, just in case we change ECC algo */
	.oobfree = { {0x08, 0x08}, {0x18, 0x08}, {0x28, 0x08}, {0x38, 0x08} },
};

static void nomadik_ecc_control(struct mtd_info *mtd, int mode)
{
	/* No need to enable hw ecc, it's on by default */
}

static void nomadik_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *nand = mtd->priv;
	struct nomadik_nand_host *host = nand->priv;

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		writeb(cmd, host->cmd_va);
	else
		writeb(cmd, host->addr_va);
}

static int nomadik_nand_probe(struct device_d *dev)
{
	struct nomadik_nand_platform_data *pdata = dev->platform_data;
	struct nomadik_nand_host *host;
	struct mtd_info *mtd;
	struct nand_chip *nand;
	int ret = 0;

	/* Allocate memory for the device structure (and zero it) */
	host = kzalloc(sizeof(struct nomadik_nand_host), GFP_KERNEL);
	if (!host) {
		dev_err(dev, "Failed to allocate device structure.\n");
		return -ENOMEM;
	}

	/* Call the client's init function, if any */
	if (pdata->init && (ret = pdata->init()) < 0) {
		dev_err(dev, "Init function failed\n");
		goto err;
	}

	host->cmd_va = dev_request_mem_region_by_name(dev, "nand_cmd");
	host->addr_va = dev_request_mem_region_by_name(dev, "nand_addr");

	/* Link all private pointers */
	mtd = &host->mtd;
	nand = &host->nand;
	mtd->priv = nand;
	nand->priv = host;
	mtd->parent = dev;

	nand->IO_ADDR_W = nand->IO_ADDR_R = dev_request_mem_region_by_name(dev, "nand_data");
	nand->cmd_ctrl = nomadik_cmd_ctrl;

	nand->ecc.mode = NAND_ECC_HW;
	nand->ecc.layout = &nomadik_ecc_layout;
	nand->ecc.calculate = nomadik_ecc512_calc;
	nand->ecc.correct = nomadik_ecc512_correct;
	nand->ecc.hwctl = nomadik_ecc_control;
	nand->ecc.size = 512;
	nand->ecc.bytes = 3;
	nand->ecc.strength = 1;

	nand->options = pdata->options;

	/*
	 * Scan to find existance of the device
	 */
	if (nand_scan(&host->mtd, 1)) {
		ret = -ENXIO;
		goto err;
	}

	pr_info("Registering %s as whole device\n", mtd->name);
	add_mtd_nand_device(mtd, "nand");

	return 0;

err:
	kfree(host);
	return ret;
}

static struct driver_d nomadik_nand_driver = {
	.probe = nomadik_nand_probe,
	.name = "nomadik_nand",
};
device_platform_driver(nomadik_nand_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ST Microelectronics (sachin.verma@st.com)");
MODULE_DESCRIPTION("NAND driver for Nomadik Platform");
