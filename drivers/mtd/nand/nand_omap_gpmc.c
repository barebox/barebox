/**
 * @file
 * @brief Provide Generic GPMC NAND implementation for OMAP platforms
 *
 * FileName: arch/arm/mach-omap/gpmc_nand.c
 *
 * GPMC has a NAND controller inbuilt. This provides a generic implementation
 * for board files to register a nand device. drivers/nand/nand_base.c takes
 * care of identifing the type of device, size etc.
 *
 * A typical device registration is as follows:
 *
 * @code
 * static struct device_d my_nand_device = {
 *	.name = "gpmc_nand",
 *	.id = some identifier you need to show.. e.g. "gpmc_nand0"
 *	.resource[0].start = GPMC base address
 *	.resource[0].size = GPMC address map size.
 *	.platform_data = platform data - required - explained below
 * };
 * platform data required:
 * static struct gpmc_nand_platform_data nand_plat = {
 *	.cs = give the chip select of the device
 *	.device_width = what is the width of the device 8 or 16?
 *	.wait_mon_pin = do you use wait monitoring? if so wait pin
 *	.oob = if you would like to replace oob with a custom OOB.
 *	.nand_setup  = if you would like a special setup function to be called
 *	.priv = any params you'd like to save(e.g. like nand_setup to use)
 *};
 * then in your code, you'd device_register(&my_nand_device);
 * @endcode
 *
 * Note:
 * @li Enable CONFIG_NAND_OMAP_GPMC_HWECC in menuconfig to get H/w ECC support
 * @li You may choose to register two "devices" for the same CS to get BOTH
 * hwecc and swecc devices.
 * @li You can choose to have your own OOB definition for compliance with ROM
 * code organization - only if you dont want to use NAND's default oob layout.
 * see GPMC_NAND_ECC_LP_x8_LAYOUT etc..
 *
 * @see gpmc_nand_platform_data
 * @warning Remember to initialize GPMC before initializing the nand dev.
 */
/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * Based on:
 * drivers/mtd/nand/omap2.c from linux kernel
 *
 * Copyright (c) 2004 Texas Instruments, Jian Zhang <jzhang@ti.com>
 * Copyright (c) 2004 Micron Technology Inc.
 * Copyright (c) 2004 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <clock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <io.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>

#define GPMC_ECC_CONFIG_ECCENABLE		(1 << 0)
#define GPMC_ECC_CONFIG_ECCCS(x)		(((x) & 0x7) << 1)
#define GPMC_ECC_CONFIG_ECCTOPSECTOR(x)		(((x) & 0x7) << 4)
#define GPMC_ECC_CONFIG_ECC16B			(1 << 7)
#define GPMC_ECC_CONFIG_ECCWRAPMODE(x)		(((x) & 0xf) << 8)
#define GPMC_ECC_CONFIG_ECCBCHTSEL(x)		(((x) & 0x3) << 12)
#define GPMC_ECC_CONFIG_ECCALGORITHM		(1 << 16)

#define GPMC_ECC_CONTROL_ECCPOINTER(x)		((x) & 0xf)
#define GPMC_ECC_CONTROL_ECCCLEAR		(1 << 8)

#define GPMC_ECC_SIZE_CONFIG_ECCSIZE0(x)	((x) << 12)
#define GPMC_ECC_SIZE_CONFIG_ECCSIZE1(x)	((x) << 22)

#define BCH8_MAX_ERROR	8	/* upto 8 bit correctable */

static const uint8_t bch8_vector[] = {0xf3, 0xdb, 0x14, 0x16, 0x8b, 0xd2,
		0xbe, 0xcc, 0xac, 0x6b, 0xff, 0x99, 0x7b};

int omap_gpmc_decode_bch(int select_4_8, unsigned char *ecc, unsigned int *err_loc);

static const char *ecc_mode_strings[] = {
	"software",
	"hamming_hw_romcode",
	"bch8_hw",
	"bch8_hw_romcode",
};

/** internal structure maintained for nand information */
struct gpmc_nand_info {
	struct nand_hw_control controller;
	struct device_d *pdev;
	struct gpmc_nand_platform_data *pdata;
	struct nand_chip nand;
	struct mtd_info minfo;
	int gpmc_cs;
	void *gpmc_command;
	void *gpmc_address;
	void *gpmc_data;
	void __iomem *gpmc_base;
	u32 wait_mon_mask;
	unsigned inuse:1;
	unsigned char ecc_parity_pairs;
	enum gpmc_ecc_mode ecc_mode;
	void *cs_base;
};

/* Typical BOOTROM oob layouts-requires hwecc **/
static struct nand_ecclayout omap_oobinfo;
/* Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks
 */
static uint8_t scan_ff_pattern[] = { 0xff };
static struct nand_bbt_descr bb_descrip_flashbased = {
	.options = NAND_BBT_SCANEMPTY | NAND_BBT_SCANALLPAGES,
	.offs = 0,
	.len = 1,
	.pattern = scan_ff_pattern,
};

/**
 * @brief calls the platform specific dev_ready functionds
 *
 * @param mtd - mtd info structure
 *
 * @return
 */
static int omap_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);

	if (readl(oinfo->gpmc_base + GPMC_STATUS) & oinfo->wait_mon_mask)
		return 1;
	else
		return 0;
}

/**
 * @brief This function will enable or disable the Write Protect feature on
 * NAND device. GPMC has a single WP bit for all CS devices..
 *
 * @param oinfo  omap nand info
 * @param mode 0-disable else enable
 *
 * @return none
 */
static void gpmc_nand_wp(struct gpmc_nand_info *oinfo, int mode)
{
	unsigned long config = readl(oinfo->gpmc_base + GPMC_CFG);

	debug("%s: mode=%x\n", __func__, mode);

	if (mode)
		config &= ~(NAND_WP_BIT);	/* WP is ON */
	else
		config |= (NAND_WP_BIT);	/* WP is OFF */

	writel(config, oinfo->gpmc_base + GPMC_CFG);
}

/**
 * @brief respond to hw event change request
 *
 * MTD layer uses NAND_CTRL_CLE etc to control selection of the latch
 * we hoodwink by changing the R and W registers according to the state
 * we are requested.
 *
 * @param mtd - mtd info structure
 * @param cmd command mtd layer is requesting
 *
 * @return none
 */
static void omap_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);

	switch (ctrl) {
	case NAND_CTRL_CHANGE | NAND_CTRL_CLE:
		nand->IO_ADDR_W = oinfo->gpmc_command;
		nand->IO_ADDR_R = oinfo->gpmc_data;
		break;

	case NAND_CTRL_CHANGE | NAND_CTRL_ALE:
		nand->IO_ADDR_W = oinfo->gpmc_address;
		nand->IO_ADDR_R = oinfo->gpmc_data;
		break;

	case NAND_CTRL_CHANGE | NAND_NCE:
		nand->IO_ADDR_W = oinfo->gpmc_data;
		nand->IO_ADDR_R = oinfo->gpmc_data;
		break;
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, nand->IO_ADDR_W);
	return;
}

/**
 * @brief This function will generate true ECC value, which can be used
 * when correcting data read from NAND flash memory core
 *
 * @param ecc_buf buffer to store ecc code
 *
 * @return re-formatted ECC value
 */
static unsigned int gen_true_ecc(u8 *ecc_buf)
{
	debug("%s: eccbuf:  0x%02x 0x%02x 0x%02x\n", __func__,
		  ecc_buf[0], ecc_buf[1], ecc_buf[2]);

	return ecc_buf[0] | (ecc_buf[1] << 16) | ((ecc_buf[2] & 0xF0) << 20) |
	    ((ecc_buf[2] & 0x0F) << 8);
}

static int __omap_calculate_ecc(struct mtd_info *mtd, const uint8_t *dat,
			      uint8_t *ecc_code, int sblock)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);
	unsigned int reg;
	unsigned int val1 = 0x0, val2 = 0x0;
	unsigned int val3 = 0x0, val4 = 0x0;
	int i;
	int ecc_size = 8;

	switch (oinfo->ecc_mode) {
	case OMAP_ECC_BCH8_CODE_HW:
	case OMAP_ECC_BCH8_CODE_HW_ROMCODE:
		for (i = 0; i < 4; i++) {
			/*
			 * Reading HW ECC_BCH_Results
			 * 0x240-0x24C, 0x250-0x25C, 0x260-0x26C, 0x270-0x27C
			 */
			reg =  GPMC_ECC_BCH_RESULT_0 + (0x10 * (i + sblock));
			val1 = readl(oinfo->gpmc_base + reg);
			val2 = readl(oinfo->gpmc_base + reg + 4);
			if (ecc_size == 8) {
				val3 = readl(oinfo->gpmc_base  +reg + 8);
				val4 = readl(oinfo->gpmc_base + reg + 12);

				*ecc_code++ = (val4 & 0xFF);
				*ecc_code++ = ((val3 >> 24) & 0xFF);
				*ecc_code++ = ((val3 >> 16) & 0xFF);
				*ecc_code++ = ((val3 >> 8) & 0xFF);
				*ecc_code++ = (val3 & 0xFF);
				*ecc_code++ = ((val2 >> 24) & 0xFF);
			}
			*ecc_code++ = ((val2 >> 16) & 0xFF);
			*ecc_code++ = ((val2 >> 8) & 0xFF);
			*ecc_code++ = (val2 & 0xFF);
			*ecc_code++ = ((val1 >> 24) & 0xFF);
			*ecc_code++ = ((val1 >> 16) & 0xFF);
			*ecc_code++ = ((val1 >> 8) & 0xFF);
			*ecc_code++ = (val1 & 0xFF);
		}
		break;
	case OMAP_ECC_HAMMING_CODE_HW_ROMCODE:
		/* read ecc result */
		val1 = readl(oinfo->gpmc_base + GPMC_ECC1_RESULT);
		*ecc_code++ = val1;          /* P128e, ..., P1e */
		*ecc_code++ = val1 >> 16;    /* P128o, ..., P1o */
		/* P2048o, P1024o, P512o, P256o, P2048e, P1024e, P512e, P256e */
		*ecc_code++ = ((val1 >> 8) & 0x0f) | ((val1 >> 20) & 0xf0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int omap_calculate_ecc(struct mtd_info *mtd, const uint8_t *dat,
			      uint8_t *ecc_code)
{
	return __omap_calculate_ecc(mtd, dat, ecc_code, 0);
}

static int omap_correct_bch(struct mtd_info *mtd, uint8_t *dat,
			     uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);
	int j, actual_eccsize;
	const uint8_t *erased_ecc_vec;
	unsigned int err_loc[8];
	int bch_max_err;
	int bitflip_count = 0;
	bool eccflag = 0;

	int eccsize = oinfo->nand.ecc.bytes;

	switch (oinfo->ecc_mode) {
	case OMAP_ECC_BCH8_CODE_HW:
		actual_eccsize = eccsize;
		erased_ecc_vec = bch8_vector;
		bch_max_err = BCH8_MAX_ERROR;
		break;
	case OMAP_ECC_BCH8_CODE_HW_ROMCODE:
		actual_eccsize = eccsize - 1;
		erased_ecc_vec = bch8_vector;
		bch_max_err = BCH8_MAX_ERROR;
		break;
	default:
		dev_err(oinfo->pdev, "invalid driver configuration\n");
		return -EINVAL;
	}

	/* check for any ecc error */
	for (j = 0; j < actual_eccsize; j++) {
		if (calc_ecc[j] != 0) {
			eccflag = 1;
			break;
		}
	}

	if (!eccflag)
		return 0;

	if (memcmp(calc_ecc, erased_ecc_vec, actual_eccsize) == 0) {
		/*
		 * calc_ecc[] matches pattern for ECC
		 * (all 0xff) so this is definitely
		 * an erased-page
		 */
		return 0;
	}

	bitflip_count = nand_check_erased_ecc_chunk(
			dat, oinfo->nand.ecc.size, read_ecc,
			eccsize, NULL, 0, bch_max_err);
	if (bitflip_count >= 0)
		return bitflip_count;

	bitflip_count = omap_gpmc_decode_bch(1,
			calc_ecc, err_loc);
	if (bitflip_count < 0)
		return bitflip_count;

	for (j = 0; j < bitflip_count; j++) {
		if (err_loc[j] < 4096)
			dat[err_loc[j] >> 3] ^= 1 << (err_loc[j] & 7);
			/* else, not interested to correct ecc */
	}

	return bitflip_count;
}

static int omap_correct_hamming(struct mtd_info *mtd, uint8_t *dat,
			     uint8_t *read_ecc, uint8_t *calc_ecc)
{
	unsigned int orig_ecc, new_ecc, res, hm;
	unsigned short parity_bits, byte;
	unsigned char bit;
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);

	if (read_ecc[0] == 0xff && read_ecc[1] == 0xff &&
			read_ecc[2] == 0xff && calc_ecc[0] == 0x0 &&
			calc_ecc[1] == 0x0 && calc_ecc[0] == 0x0)
		return 0;

	/* Regenerate the orginal ECC */
	orig_ecc = gen_true_ecc(read_ecc);
	new_ecc = gen_true_ecc(calc_ecc);
	/* Get the XOR of real ecc */
	res = orig_ecc ^ new_ecc;
	if (res) {
		/* Get the hamming width */
		hm = hweight32(res);
		/* Single bit errors can be corrected! */
		if (hm == oinfo->ecc_parity_pairs) {
			/* Correctable data! */
			parity_bits = res >> 16;
			bit = (parity_bits & 0x7);
			byte = (parity_bits >> 3) & 0x1FF;
			/* Flip the bit to correct */
			dat[byte] ^= (0x1 << bit);
		} else if (hm == 1) {
			printf("Ecc is wrong\n");
			/* ECC itself is corrupted */
			return 2;
		} else {
			printf("bad compare! failed\n");
			/* detected 2 bit error */
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Compares the ecc read from nand spare area with ECC
 * registers values and corrects one bit error if it has occured
 * Further details can be had from OMAP TRM and the following selected links:
 * http://en.wikipedia.org/wiki/Hamming_code
 * http://www.cs.utexas.edu/users/plaxton/c/337/05f/slides/ErrorCorrection-4.pdf
 *
 * @param mtd - mtd info structure
 * @param dat  page data
 * @param read_ecc ecc readback
 * @param calc_ecc calculated ecc (from reg)
 *
 * @return 0 if data is OK or corrected, else returns -1
 */
static int omap_correct_data(struct mtd_info *mtd, uint8_t *dat,
			     uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);

	debug("%s\n", __func__);

	switch (oinfo->ecc_mode) {
	case OMAP_ECC_HAMMING_CODE_HW_ROMCODE:
		return omap_correct_hamming(mtd, dat, read_ecc, calc_ecc);
	case OMAP_ECC_BCH8_CODE_HW:
	case OMAP_ECC_BCH8_CODE_HW_ROMCODE:
		/*
		 * The nand layer already called omap_calculate_ecc,
		 * but before it has read the oob data. Do it again,
		 * this time with oob data.
		 */
		__omap_calculate_ecc(mtd, dat, calc_ecc, 0);
		return omap_correct_bch(mtd, dat, read_ecc, calc_ecc);
	default:
		return -EINVAL;
	}

	return 0;
}

static void omap_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);
	unsigned int bch_mod = 0, bch_wrapmode = 0, eccsize1 = 0, eccsize0 = 0;
	unsigned int ecc_conf_val = 0, ecc_size_conf_val = 0;
	int dev_width = nand->options & NAND_BUSWIDTH_16 ? GPMC_ECC_CONFIG_ECC16B : 0;
	int ecc_size = nand->ecc.size;
	int cs = 0;

	switch (oinfo->ecc_mode) {
	case OMAP_ECC_BCH8_CODE_HW:
	case OMAP_ECC_BCH8_CODE_HW_ROMCODE:
		if (mode == NAND_ECC_READ) {
			eccsize1 = 0x1A; eccsize0 = 0x18;
			bch_mod = 1;
			bch_wrapmode = 0x04;
		} else {
			eccsize1 = 0x20; eccsize0 = 0x00;
			bch_mod = 1;
			bch_wrapmode = 0x06;
		}
		break;
	case OMAP_ECC_HAMMING_CODE_HW_ROMCODE:
		eccsize1 = ((ecc_size >> 1) - 1) << 22;
		break;
	case OMAP_ECC_SOFT:
		return;
	}

	/* clear ecc and enable bits */
	if (oinfo->ecc_mode == OMAP_ECC_HAMMING_CODE_HW_ROMCODE) {
		writel(GPMC_ECC_CONTROL_ECCCLEAR |
				GPMC_ECC_CONTROL_ECCPOINTER(1),
				oinfo->gpmc_base + GPMC_ECC_CONTROL);

		/*
		 * Size 0 = 0xFF, Size1 is 0xFF - both are 512 bytes
		 * tell all regs to generate size0 sized regs
		 * we just have a single ECC engine for all CS
		 */
		ecc_size_conf_val = GPMC_ECC_SIZE_CONFIG_ECCSIZE1(0xff) |
			GPMC_ECC_SIZE_CONFIG_ECCSIZE0(0xff);
		ecc_conf_val = dev_width | GPMC_ECC_CONFIG_ECCCS(cs) |
			GPMC_ECC_CONFIG_ECCENABLE;
	} else {
		writel(GPMC_ECC_CONTROL_ECCPOINTER(1),
				oinfo->gpmc_base + GPMC_ECC_CONTROL);

		ecc_size_conf_val = GPMC_ECC_SIZE_CONFIG_ECCSIZE1(eccsize1) |
			GPMC_ECC_SIZE_CONFIG_ECCSIZE0(eccsize0);

		ecc_conf_val = (GPMC_ECC_CONFIG_ECCALGORITHM |
				GPMC_ECC_CONFIG_ECCBCHTSEL(bch_mod) |
				GPMC_ECC_CONFIG_ECCWRAPMODE(bch_wrapmode) |
				dev_width |
				GPMC_ECC_CONFIG_ECCTOPSECTOR(3) |
				GPMC_ECC_CONFIG_ECCCS(cs) |
				GPMC_ECC_CONFIG_ECCENABLE);
	}

	writel(ecc_size_conf_val, oinfo->gpmc_base + GPMC_ECC_SIZE_CONFIG);
	writel(ecc_conf_val, oinfo->gpmc_base + GPMC_ECC_CONFIG);
	writel(GPMC_ECC_CONTROL_ECCCLEAR | GPMC_ECC_CONTROL_ECCPOINTER(1),
			oinfo->gpmc_base + GPMC_ECC_CONTROL);
}

static int omap_gpmc_read_buf_manual(struct mtd_info *mtd, struct nand_chip *chip,
		void *buf, int bytes, int result_reg)
{
	struct gpmc_nand_info *oinfo = chip->priv;

	writel(GPMC_ECC_SIZE_CONFIG_ECCSIZE1(0) |
			GPMC_ECC_SIZE_CONFIG_ECCSIZE0(bytes * 2),
			oinfo->gpmc_base + GPMC_ECC_SIZE_CONFIG);

	writel(GPMC_ECC_CONTROL_ECCPOINTER(result_reg),
			oinfo->gpmc_base + GPMC_ECC_CONTROL);

	chip->read_buf(mtd, buf, bytes);

	return bytes;
}

/**
 * omap_read_buf_pref - read data from NAND controller into buffer
 * @mtd: MTD device structure
 * @buf: buffer to store date
 * @len: number of bytes to read
 */
static void omap_read_buf_pref(struct mtd_info *mtd, u_char *buf, int len)
{
	struct gpmc_nand_info *info = container_of(mtd,
						struct gpmc_nand_info, minfo);
	u32 r_count = 0;
	u32 *p = (u32 *)buf;

	/* take care of subpage reads */
	if (len % 4) {
		if (info->nand.options & NAND_BUSWIDTH_16)
			readsw(info->cs_base, buf, (len % 4) / 2);
		else
			readsb(info->cs_base, buf, len % 4);
		p = (u32 *) (buf + len % 4);
		len -= len % 4;
	}

	/* configure and start prefetch transfer */
	gpmc_prefetch_enable(info->gpmc_cs,
			PREFETCH_FIFOTHRESHOLD_MAX, 0x0, len, 0x0);

	do {
		r_count = readl(info->gpmc_base + GPMC_PREFETCH_STATUS);
		r_count = GPMC_PREFETCH_STATUS_FIFO_CNT(r_count);
		r_count = r_count >> 2;
		readsl(info->cs_base, p, r_count);
		p += r_count;
		len -= r_count << 2;
	} while (len);

	/* disable and stop the PFPW engine */
	gpmc_prefetch_reset(info->gpmc_cs);
}

/**
 * omap_write_buf_pref - write buffer to NAND controller
 * @mtd: MTD device structure
 * @buf: data buffer
 * @len: number of bytes to write
 */
static void omap_write_buf_pref(struct mtd_info *mtd,
					const u_char *buf, int len)
{
	struct gpmc_nand_info *info = container_of(mtd,
						struct gpmc_nand_info, minfo);
	u32 w_count = 0;
	u_char *buf1 = (u_char *)buf;
	u32 *p32 = (u32 *)buf;
	uint64_t start;

	/* take care of subpage writes */
	while (len % 4 != 0) {
		writeb(*buf, info->nand.IO_ADDR_W);
		buf1++;
		p32 = (u32 *)buf1;
		len--;
	}

	/*  configure and start prefetch transfer */
	gpmc_prefetch_enable(info->gpmc_cs,
			PREFETCH_FIFOTHRESHOLD_MAX, 0x0, len, 0x1);

	while (len >= 0) {
		w_count = readl(info->gpmc_base + GPMC_PREFETCH_STATUS);
		w_count = GPMC_PREFETCH_STATUS_FIFO_CNT(w_count);
		w_count = w_count >> 2;
		writesl(info->cs_base, p32, w_count);
		p32 += w_count;
		len -= w_count << 2;
	}

	/* wait for data to flushed-out before reset the prefetch */
	start = get_time_ns();
	while (1) {
		u32 regval, status;
		regval = readl(info->gpmc_base + GPMC_PREFETCH_STATUS);
		status = GPMC_PREFETCH_STATUS_COUNT(regval);
		if (!status)
			break;
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(info->pdev, "prefetch flush timed out\n");
			break;
		}
	}

	/* disable and stop the PFPW engine */
	gpmc_prefetch_reset(info->gpmc_cs);
}

/*
 * read a page with the ecc layout used by the OMAP4 romcode. The
 * romcode expects an unusual ecc layout (f = free, e = ecc):
 *
 * 2f, 13e, 1f, 13e, 1f, 13e, 1f, 13e, 7f
 *
 * This can't be accomplished with the predefined ecc modes, so
 * we have to use the manual mode here.
 *
 * For the manual mode we can't use the ECC_RESULTx_0 register set
 * because it would disable ecc generation completeley. Also, the
 * hardware seems to ignore eccsize1 (which should bypass ecc
 * generation), so we use the otherwise unused ECC_RESULTx_5 to
 * generate dummy eccs for the unprotected oob area.
 */
static int omap_gpmc_read_page_bch_rom_mode(struct mtd_info *mtd,
		struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
	struct gpmc_nand_info *oinfo = chip->priv;
	int dev_width = chip->options & NAND_BUSWIDTH_16 ? GPMC_ECC_CONFIG_ECC16B : 0;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	int eccsize = chip->ecc.size;
	unsigned int max_bitflips = 0;
	int stat, i, j;


	writel(GPMC_ECC_SIZE_CONFIG_ECCSIZE1(0) |
			GPMC_ECC_SIZE_CONFIG_ECCSIZE0(64),
				oinfo->gpmc_base + GPMC_ECC_SIZE_CONFIG);

	writel(GPMC_ECC_CONTROL_ECCPOINTER(1),
			oinfo->gpmc_base + GPMC_ECC_CONTROL);

	writel(GPMC_ECC_CONFIG_ECCALGORITHM |
			GPMC_ECC_CONFIG_ECCBCHTSEL(1) |
			GPMC_ECC_CONFIG_ECCWRAPMODE(0) |
			dev_width | GPMC_ECC_CONFIG_ECCTOPSECTOR(3) |
			GPMC_ECC_CONFIG_ECCCS(0) |
			GPMC_ECC_CONFIG_ECCENABLE,
			oinfo->gpmc_base + GPMC_ECC_CONFIG);

	writel(GPMC_ECC_CONTROL_ECCCLEAR |
			GPMC_ECC_CONTROL_ECCPOINTER(1),
			oinfo->gpmc_base + GPMC_ECC_CONTROL);

	for (i = 0; i < 32; i++)
		p += omap_gpmc_read_buf_manual(mtd, chip, p, 64, (i >> 3) + 1);

	p = chip->oob_poi;

	p += omap_gpmc_read_buf_manual(mtd, chip, p, 2, 5);

	for (i = 0; i < 4; i++) {
		p += omap_gpmc_read_buf_manual(mtd, chip, p, 13, i + 1);
		p += omap_gpmc_read_buf_manual(mtd, chip, p, 1, 5);
	}

	p += omap_gpmc_read_buf_manual(mtd, chip, p, 6, 5);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	__omap_calculate_ecc(mtd, buf, ecc_calc, 1);

	p = buf;

	for (i = 0, j = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize, j++) {
		stat = omap_correct_bch(mtd, p, &ecc_code[i], &ecc_calc[i - j]);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}
	}

	return max_bitflips;
}

static int omap_gpmc_eccmode(struct gpmc_nand_info *oinfo,
		enum gpmc_ecc_mode mode)
{
	struct mtd_info *minfo = &oinfo->minfo;
	struct nand_chip *nand = &oinfo->nand;
	int offset;
	int i, j;

	if (nand->options & NAND_BUSWIDTH_16)
		nand->badblock_pattern = &bb_descrip_flashbased;
	else
		nand->badblock_pattern = NULL;

	if (oinfo->nand.options & NAND_BUSWIDTH_16)
		offset = 2;
	else
		offset = 1;

	if (mode != OMAP_ECC_SOFT) {
		nand->ecc.layout = &omap_oobinfo;
		nand->ecc.calculate = omap_calculate_ecc;
		nand->ecc.hwctl = omap_enable_hwecc;
		nand->ecc.correct = omap_correct_data;
		nand->ecc.read_page = NULL;
		nand->ecc.write_page = NULL;
		nand->ecc.read_oob = NULL;
		nand->ecc.write_oob = NULL;
		nand->ecc.mode = NAND_ECC_HW;
	}

	switch (mode) {
	case OMAP_ECC_HAMMING_CODE_HW_ROMCODE:
		oinfo->nand.ecc.bytes    = 3;
		oinfo->nand.ecc.size     = 512;
		oinfo->nand.ecc.strength = 1;
		oinfo->ecc_parity_pairs  = 12;
		if (oinfo->nand.options & NAND_BUSWIDTH_16) {
			offset = 2;
		} else {
			offset = 1;
			oinfo->nand.badblock_pattern = &bb_descrip_flashbased;
		}
		omap_oobinfo.eccbytes = 3 * (minfo->oobsize / 16);
		for (i = 0; i < omap_oobinfo.eccbytes; i++)
			omap_oobinfo.eccpos[i] = i + offset;
		omap_oobinfo.oobfree->offset = offset + omap_oobinfo.eccbytes;
		omap_oobinfo.oobfree->length = minfo->oobsize -
					offset - omap_oobinfo.eccbytes;
		break;
	case OMAP_ECC_BCH8_CODE_HW:
		oinfo->nand.ecc.bytes    = 13;
		oinfo->nand.ecc.size     = 512;
		oinfo->nand.ecc.strength = BCH8_MAX_ERROR;
		omap_oobinfo.oobfree->offset = offset;
		omap_oobinfo.oobfree->length = minfo->oobsize -
					offset - omap_oobinfo.eccbytes;
		offset = minfo->oobsize - oinfo->nand.ecc.bytes;
		for (i = 0; i < oinfo->nand.ecc.bytes; i++)
			omap_oobinfo.eccpos[i] = i + offset;
		break;
	case OMAP_ECC_BCH8_CODE_HW_ROMCODE:
		oinfo->nand.ecc.bytes    = 13 + 1;
		oinfo->nand.ecc.size     = 512;
		oinfo->nand.ecc.strength = BCH8_MAX_ERROR;
		nand->ecc.read_page = omap_gpmc_read_page_bch_rom_mode;
		omap_oobinfo.oobfree->length = 0;
		j = 0;
		for (i = 2; i < 58; i++)
			omap_oobinfo.eccpos[j++] = i;
		break;
	case OMAP_ECC_SOFT:
		nand->ecc.layout = NULL;
		nand->ecc.mode = NAND_ECC_SOFT;
		oinfo->nand.ecc.strength = 1;
		break;
	default:
		return -EINVAL;
	}

	oinfo->ecc_mode = mode;

	/* second phase scan */
	if (nand_scan_tail(minfo))
		return -ENXIO;

	nand->options |= NAND_SKIP_BBTSCAN;

	return 0;
}

static int omap_gpmc_eccmode_set(struct param_d *param, void *priv)
{
	struct gpmc_nand_info *oinfo = priv;

	return omap_gpmc_eccmode(oinfo, oinfo->ecc_mode);
}

static int gpmc_set_buswidth(struct nand_chip *chip, int buswidth)
{
	struct gpmc_nand_info *oinfo = chip->priv;

	if (buswidth == NAND_BUSWIDTH_16)
		oinfo->pdata->nand_cfg->cfg[0] |= 0x00001000;
	else
		oinfo->pdata->nand_cfg->cfg[0] &= ~0x00001000;

	gpmc_cs_config(oinfo->pdata->cs, oinfo->pdata->nand_cfg);

	return 0;
}

/**
 * @brief nand device probe.
 *
 * @param pdev -matching device
 *
 * @return -failure reason or give 0
 */
static int gpmc_nand_probe(struct device_d *pdev)
{
	struct resource *iores;
	struct gpmc_nand_info *oinfo;
	struct gpmc_nand_platform_data *pdata;
	struct nand_chip *nand;
	struct mtd_info *minfo;
	void __iomem *cs_base;
	int err;

	pdata = (struct gpmc_nand_platform_data *)pdev->platform_data;
	if (pdata == NULL) {
		dev_dbg(pdev, "platform data missing\n");
		return -ENODEV;
	}

	oinfo = xzalloc(sizeof(*oinfo));

	/* fill up my data structures */
	oinfo->pdev = pdev;
	oinfo->pdata = pdata;
	pdev->platform_data = (void *)oinfo;
	pdev->priv = oinfo;

	nand = &oinfo->nand;
	nand->priv = (void *)oinfo;

	minfo = &oinfo->minfo;
	minfo->priv = (void *)nand;
	minfo->parent = pdev;

	if (pdata->cs >= GPMC_NUM_CS) {
		dev_dbg(pdev, "Invalid CS!\n");
		err = -EINVAL;
		goto out_release_mem;
	}
	/* Setup register specific data */
	oinfo->gpmc_cs = pdata->cs;
	iores = dev_request_mem_resource(pdev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	oinfo->gpmc_base = IOMEM(iores->start);
	cs_base = oinfo->gpmc_base + GPMC_CONFIG1_0 +
		(pdata->cs * GPMC_CONFIG_CS_SIZE);
	oinfo->gpmc_command = (void *)(cs_base + GPMC_CS_NAND_COMMAND);
	oinfo->gpmc_address = (void *)(cs_base + GPMC_CS_NAND_ADDRESS);
	oinfo->gpmc_data = (void *)(cs_base + GPMC_CS_NAND_DATA);
	oinfo->cs_base = (void *)pdata->nand_cfg->base;
	dev_dbg(pdev, "GPMC base=0x%p cmd=0x%p address=0x%p data=0x%p cs_base=0x%p\n",
		oinfo->gpmc_base, oinfo->gpmc_command, oinfo->gpmc_address,
		oinfo->gpmc_data, cs_base);

	switch (pdata->device_width) {
	case 0:
		dev_dbg(pdev, "probing buswidth\n");
		nand->options |= NAND_BUSWIDTH_AUTO;
		break;
	case 8:
		break;
	case 16:
		nand->options |= NAND_BUSWIDTH_16;
		break;
	default:
		return -EINVAL;
	}

	/* Same data register for in and out */
	nand->IO_ADDR_W = nand->IO_ADDR_R = (void *)oinfo->gpmc_data;
	/*
	 * If RDY/BSY line is connected to OMAP then use the omap ready
	 * function and the generic nand_wait function which reads the
	 * status register after monitoring the RDY/BSY line. Otherwise
	 * use a standard chip delay which is slightly more than tR
	 * (AC Timing) of the NAND device and read the status register
	 * until you get a failure or success
	 */
	if (pdata->wait_mon_pin > 4) {
		dev_dbg(pdev, "Invalid wait monitoring pin\n");
		err = -EINVAL;
		goto out_release_mem;
	}

	if (pdata->wait_mon_pin) {
		/* Set up the wait monitoring mask
		 * This is GPMC_STATUS reg relevant */
		oinfo->wait_mon_mask = (0x1 << (pdata->wait_mon_pin - 1)) << 8;
		nand->dev_ready = omap_dev_ready;
		nand->chip_delay = 0;
	} else {
		/* use the default nand_wait function */
		nand->chip_delay = 50;
	}

	/* Use default cmdfunc */
	/* nand cmd control */
	nand->cmd_ctrl = omap_hwcontrol;

	/* Dont do a bbt scan at the start */
	nand->options |= NAND_SKIP_BBTSCAN;

	nand->options |= NAND_OWN_BUFFERS;
	nand->buffers = xzalloc(sizeof(*nand->buffers));

	/* State my controller */
	nand->controller = &oinfo->controller;

	/* All information is ready.. now lets call setup, if present */
	if (pdata->nand_setup) {
		err = pdata->nand_setup(pdata);
		if (err) {
			dev_dbg(pdev, "pdataform setup failed\n");
			goto out_release_mem;
		}
	}
	/* Remove write protection */
	gpmc_nand_wp(oinfo, 0);

	/* we do not know what state of device we have is, so
	 * Send a reset to the device
	 * 8 bit write will work on 16 and 8 bit devices
	 */
	writeb(NAND_CMD_RESET, oinfo->gpmc_command);
	mdelay(1);

	/* first scan to find the device and get the page size */
	if (nand_scan_ident(minfo, 1, NULL)) {
		err = -ENXIO;
		goto out_release_mem;
	}

	gpmc_set_buswidth(nand, nand->options & NAND_BUSWIDTH_16);

	nand->read_buf   = omap_read_buf_pref;
	if (IS_ENABLED(CONFIG_MTD_WRITE))
		nand->write_buf  = omap_write_buf_pref;

	nand->options |= NAND_SKIP_BBTSCAN;

	oinfo->ecc_mode = pdata->ecc_mode;

	dev_add_param_enum(pdev, "eccmode",
			omap_gpmc_eccmode_set, NULL, (int *)&oinfo->ecc_mode,
			ecc_mode_strings, ARRAY_SIZE(ecc_mode_strings), oinfo);

	omap_gpmc_eccmode(oinfo, oinfo->ecc_mode);

	/* We are all set to register with the system now! */
	err = add_mtd_nand_device(minfo, "nand");
	if (err) {
		dev_dbg(pdev, "device registration failed\n");
		goto out_release_mem;
	}

	return 0;

out_release_mem:
	if (oinfo)
		free(oinfo);

	dev_dbg(pdev, "Failed!!\n");
	return err;
}

/** GMPC nand driver -> device registered by platforms */
static struct driver_d gpmc_nand_driver = {
	.name = "gpmc_nand",
	.probe = gpmc_nand_probe,
};
device_platform_driver(gpmc_nand_driver);
