// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) STMicroelectronics 2018
 * Author: Christophe Kerello <christophe.kerello@st.com>
 */

#include <common.h>
#include <init.h>
#include <of_address.h>
#include <linux/regmap.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/iopoll.h>
#include <linux/reset.h>
#include <mfd/syscon.h>

#include "internals.h"

/* Bad block marker length */
#define FMC2_BBM_LEN			2

/* ECC step size */
#define FMC2_ECC_STEP_SIZE		512

/* Max requests done for a 8k nand page size */
#define FMC2_MAX_SG			16

/* Max chip enable */
#define FMC2_MAX_CE			2

#define FMC2_TIMEOUT_MS			5000

/* Timings */
#define FMC2_THIZ			1
#define FMC2_TIO			8000
#define FMC2_TSYNC			3000
#define FMC2_PCR_TIMING_MASK		0xf
#define FMC2_PMEM_PATT_TIMING_MASK	0xff

/* FMC2 Controller Registers */
#define FMC2_BCR1			0x0
#define FMC2_PCR			0x80
#define FMC2_SR				0x84
#define FMC2_PMEM			0x88
#define FMC2_PATT			0x8c
#define FMC2_HECCR			0x94
#define FMC2_ISR			0x184
#define FMC2_ICR			0x188
#define FMC2_CSQCR			0x200
#define FMC2_CSQCFGR1			0x204
#define FMC2_CSQCFGR2			0x208
#define FMC2_CSQCFGR3			0x20c
#define FMC2_CSQAR1			0x210
#define FMC2_CSQAR2			0x214
#define FMC2_CSQIER			0x220
#define FMC2_CSQISR			0x224
#define FMC2_CSQICR			0x228
#define FMC2_CSQEMSR			0x230
#define FMC2_BCHIER			0x250
#define FMC2_BCHISR			0x254
#define FMC2_BCHICR			0x258
#define FMC2_BCHPBR1			0x260
#define FMC2_BCHPBR2			0x264
#define FMC2_BCHPBR3			0x268
#define FMC2_BCHPBR4			0x26c
#define FMC2_BCHDSR0			0x27c
#define FMC2_BCHDSR1			0x280
#define FMC2_BCHDSR2			0x284
#define FMC2_BCHDSR3			0x288
#define FMC2_BCHDSR4			0x28c

/* Register: FMC2_BCR1 */
#define FMC2_BCR1_FMC2EN		BIT(31)

/* Register: FMC2_PCR */
#define FMC2_PCR_PWAITEN		BIT(1)
#define FMC2_PCR_PBKEN			BIT(2)
#define FMC2_PCR_PWID			GENMASK(5, 4)
#define FMC2_PCR_PWID_BUSWIDTH_8	0
#define FMC2_PCR_PWID_BUSWIDTH_16	1
#define FMC2_PCR_ECCEN			BIT(6)
#define FMC2_PCR_ECCALG			BIT(8)
#define FMC2_PCR_TCLR			GENMASK(12, 9)
#define FMC2_PCR_TCLR_DEFAULT		0xf
#define FMC2_PCR_TAR			GENMASK(16, 13)
#define FMC2_PCR_TAR_DEFAULT		0xf
#define FMC2_PCR_ECCSS			GENMASK(19, 17)
#define FMC2_PCR_ECCSS_512		1
#define FMC2_PCR_ECCSS_2048		3
#define FMC2_PCR_BCHECC			BIT(24)
#define FMC2_PCR_WEN			BIT(25)

/* Register: FMC2_SR */
#define FMC2_SR_NWRF			BIT(6)

/* Register: FMC2_PMEM */
#define FMC2_PMEM_MEMSET		GENMASK(7, 0)
#define FMC2_PMEM_MEMWAIT		GENMASK(15, 8)
#define FMC2_PMEM_MEMHOLD		GENMASK(23, 16)
#define FMC2_PMEM_MEMHIZ		GENMASK(31, 24)
#define FMC2_PMEM_DEFAULT		0x0a0a0a0a

/* Register: FMC2_PATT */
#define FMC2_PATT_ATTSET		GENMASK(7, 0)
#define FMC2_PATT_ATTWAIT		GENMASK(15, 8)
#define FMC2_PATT_ATTHOLD		GENMASK(23, 16)
#define FMC2_PATT_ATTHIZ		GENMASK(31, 24)
#define FMC2_PATT_DEFAULT		0x0a0a0a0a

/* Register: FMC2_ISR */
#define FMC2_ISR_IHLF			BIT(1)

/* Register: FMC2_BCHISR */
#define FMC2_BCHISR_DERF		BIT(1)
#define FMC2_BCHISR_EPBRF		BIT(4)

/* Register: FMC2_ICR */
#define FMC2_ICR_CIHLF			BIT(1)

/* Register: FMC2_CSQCR */
#define FMC2_CSQCR_CSQSTART		BIT(0)

/* Register: FMC2_CSQCFGR1 */
#define FMC2_CSQCFGR1_CMD2EN		BIT(1)
#define FMC2_CSQCFGR1_DMADEN		BIT(2)
#define FMC2_CSQCFGR1_ACYNBR		GENMASK(6, 4)
#define FMC2_CSQCFGR1_CMD1		GENMASK(15, 8)
#define FMC2_CSQCFGR1_CMD2		GENMASK(23, 16)
#define FMC2_CSQCFGR1_CMD1T		BIT(24)
#define FMC2_CSQCFGR1_CMD2T		BIT(25)

/* Register: FMC2_CSQCFGR2 */
#define FMC2_CSQCFGR2_SQSDTEN		BIT(0)
#define FMC2_CSQCFGR2_RCMD2EN		BIT(1)
#define FMC2_CSQCFGR2_DMASEN		BIT(2)
#define FMC2_CSQCFGR2_RCMD1		GENMASK(15, 8)
#define FMC2_CSQCFGR2_RCMD2		GENMASK(23, 16)
#define FMC2_CSQCFGR2_RCMD1T		BIT(24)
#define FMC2_CSQCFGR2_RCMD2T		BIT(25)

/* Register: FMC2_CSQCFGR3 */
#define FMC2_CSQCFGR3_SNBR		GENMASK(13, 8)
#define FMC2_CSQCFGR3_AC1T		BIT(16)
#define FMC2_CSQCFGR3_AC2T		BIT(17)
#define FMC2_CSQCFGR3_AC3T		BIT(18)
#define FMC2_CSQCFGR3_AC4T		BIT(19)
#define FMC2_CSQCFGR3_AC5T		BIT(20)
#define FMC2_CSQCFGR3_SDT		BIT(21)
#define FMC2_CSQCFGR3_RAC1T		BIT(22)
#define FMC2_CSQCFGR3_RAC2T		BIT(23)

/* Register: FMC2_CSQCAR1 */
#define FMC2_CSQCAR1_ADDC1		GENMASK(7, 0)
#define FMC2_CSQCAR1_ADDC2		GENMASK(15, 8)
#define FMC2_CSQCAR1_ADDC3		GENMASK(23, 16)
#define FMC2_CSQCAR1_ADDC4		GENMASK(31, 24)

/* Register: FMC2_CSQCAR2 */
#define FMC2_CSQCAR2_ADDC5		GENMASK(7, 0)
#define FMC2_CSQCAR2_NANDCEN		GENMASK(11, 10)
#define FMC2_CSQCAR2_SAO		GENMASK(31, 16)

/* Register: FMC2_CSQIER */
#define FMC2_CSQIER_TCIE		BIT(0)

/* Register: FMC2_CSQICR */
#define FMC2_CSQICR_CLEAR_IRQ		GENMASK(4, 0)

/* Register: FMC2_CSQEMSR */
#define FMC2_CSQEMSR_SEM		GENMASK(15, 0)

/* Register: FMC2_BCHIER */
#define FMC2_BCHIER_DERIE		BIT(1)
#define FMC2_BCHIER_EPBRIE		BIT(4)

/* Register: FMC2_BCHICR */
#define FMC2_BCHICR_CLEAR_IRQ		GENMASK(4, 0)

/* Register: FMC2_BCHDSR0 */
#define FMC2_BCHDSR0_DUE		BIT(0)
#define FMC2_BCHDSR0_DEF		BIT(1)
#define FMC2_BCHDSR0_DEN		GENMASK(7, 4)

/* Register: FMC2_BCHDSR1 */
#define FMC2_BCHDSR1_EBP1		GENMASK(12, 0)
#define FMC2_BCHDSR1_EBP2		GENMASK(28, 16)

/* Register: FMC2_BCHDSR2 */
#define FMC2_BCHDSR2_EBP3		GENMASK(12, 0)
#define FMC2_BCHDSR2_EBP4		GENMASK(28, 16)

/* Register: FMC2_BCHDSR3 */
#define FMC2_BCHDSR3_EBP5		GENMASK(12, 0)
#define FMC2_BCHDSR3_EBP6		GENMASK(28, 16)

/* Register: FMC2_BCHDSR4 */
#define FMC2_BCHDSR4_EBP7		GENMASK(12, 0)
#define FMC2_BCHDSR4_EBP8		GENMASK(28, 16)

enum stm32_fmc2_ecc {
	FMC2_ECC_HAM = 1,
	FMC2_ECC_BCH4 = 4,
	FMC2_ECC_BCH8 = 8
};

struct stm32_fmc2_timings {
	u8 tclr;
	u8 tar;
	u8 thiz;
	u8 twait;
	u8 thold_mem;
	u8 tset_mem;
	u8 thold_att;
	u8 tset_att;
};

struct stm32_fmc2_nand {
	struct nand_chip chip;
	struct gpio_desc *wp_gpio;
	struct stm32_fmc2_timings timings;
	int ncs;
	int cs_used[FMC2_MAX_CE];
};

static inline struct stm32_fmc2_nand *to_fmc2_nand(struct nand_chip *chip)
{
	return container_of(chip, struct stm32_fmc2_nand, chip);
}

struct stm32_fmc2_nfc {
	struct nand_controller base;
	struct stm32_fmc2_nand nand;
	struct device *dev;
	struct device *cdev;
	struct regmap *regmap;
	void __iomem *data_base[FMC2_MAX_CE];
	void __iomem *cmd_base[FMC2_MAX_CE];
	void __iomem *addr_base[FMC2_MAX_CE];
	struct clk *clk;

	u8 cs_assigned;
	int cs_sel;
};

static inline struct stm32_fmc2_nfc *to_stm32_nfc(struct nand_controller *base)
{
	return container_of(base, struct stm32_fmc2_nfc, base);
}

static void stm32_fmc2_nfc_timings_init(struct nand_chip *chip)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	struct stm32_fmc2_nand *nand = to_fmc2_nand(chip);
	struct stm32_fmc2_timings *timings = &nand->timings;
	u32 pmem, patt;

	/* Set tclr/tar timings */
	regmap_update_bits(nfc->regmap, FMC2_PCR,
			   FMC2_PCR_TCLR | FMC2_PCR_TAR,
			   FIELD_PREP(FMC2_PCR_TCLR, timings->tclr) |
			   FIELD_PREP(FMC2_PCR_TAR, timings->tar));

	/* Set tset/twait/thold/thiz timings in common bank */
	pmem = FIELD_PREP(FMC2_PMEM_MEMSET, timings->tset_mem);
	pmem |= FIELD_PREP(FMC2_PMEM_MEMWAIT, timings->twait);
	pmem |= FIELD_PREP(FMC2_PMEM_MEMHOLD, timings->thold_mem);
	pmem |= FIELD_PREP(FMC2_PMEM_MEMHIZ, timings->thiz);
	regmap_write(nfc->regmap, FMC2_PMEM, pmem);

	/* Set tset/twait/thold/thiz timings in attribut bank */
	patt = FIELD_PREP(FMC2_PATT_ATTSET, timings->tset_att);
	patt |= FIELD_PREP(FMC2_PATT_ATTWAIT, timings->twait);
	patt |= FIELD_PREP(FMC2_PATT_ATTHOLD, timings->thold_att);
	patt |= FIELD_PREP(FMC2_PATT_ATTHIZ, timings->thiz);
	regmap_write(nfc->regmap, FMC2_PATT, patt);
}

static void stm32_fmc2_nfc_setup(struct nand_chip *chip)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	u32 pcr = 0, pcr_mask;

	/* Configure ECC algorithm (default configuration is Hamming) */
	pcr_mask = FMC2_PCR_ECCALG;
	pcr_mask |= FMC2_PCR_BCHECC;
	if (chip->ecc.strength == FMC2_ECC_BCH8) {
		pcr |= FMC2_PCR_ECCALG;
		pcr |= FMC2_PCR_BCHECC;
	} else if (chip->ecc.strength == FMC2_ECC_BCH4) {
		pcr |= FMC2_PCR_ECCALG;
	}

	/* Set buswidth */
	pcr_mask |= FMC2_PCR_PWID;
	if (chip->options & NAND_BUSWIDTH_16)
		pcr |= FIELD_PREP(FMC2_PCR_PWID, FMC2_PCR_PWID_BUSWIDTH_16);

	/* Set ECC sector size */
	pcr_mask |= FMC2_PCR_ECCSS;
	pcr |= FIELD_PREP(FMC2_PCR_ECCSS, FMC2_PCR_ECCSS_512);

	regmap_update_bits(nfc->regmap, FMC2_PCR, pcr_mask, pcr);
}

static void stm32_fmc2_nfc_select_chip(struct nand_chip *chip, int chipnr)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	struct stm32_fmc2_nand *nand = to_fmc2_nand(chip);

	if (nand->cs_used[chipnr] == nfc->cs_sel)
		return;

	nfc->cs_sel = nand->cs_used[chipnr];
	stm32_fmc2_nfc_setup(chip);
	stm32_fmc2_nfc_timings_init(chip);
}

static void stm32_fmc2_nfc_set_buswidth_16(struct stm32_fmc2_nfc *nfc, bool set)
{
	u32 pcr;

	pcr = set ? FIELD_PREP(FMC2_PCR_PWID, FMC2_PCR_PWID_BUSWIDTH_16) :
		    FIELD_PREP(FMC2_PCR_PWID, FMC2_PCR_PWID_BUSWIDTH_8);

	regmap_update_bits(nfc->regmap, FMC2_PCR, FMC2_PCR_PWID, pcr);
}

static void stm32_fmc2_nfc_set_ecc(struct stm32_fmc2_nfc *nfc, bool enable)
{
	regmap_update_bits(nfc->regmap, FMC2_PCR, FMC2_PCR_ECCEN,
			   enable ? FMC2_PCR_ECCEN : 0);
}

static void stm32_fmc2_nfc_clear_bch_irq(struct stm32_fmc2_nfc *nfc)
{
	regmap_write(nfc->regmap, FMC2_BCHICR, FMC2_BCHICR_CLEAR_IRQ);
}

/*
 * Enable ECC logic and reset syndrome/parity bits previously calculated
 * Syndrome/parity bits is cleared by setting the ECCEN bit to 0
 */
static void stm32_fmc2_nfc_hwctl(struct nand_chip *chip, int mode)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);

	stm32_fmc2_nfc_set_ecc(nfc, false);

	if (chip->ecc.strength != FMC2_ECC_HAM) {
		regmap_update_bits(nfc->regmap, FMC2_PCR, FMC2_PCR_WEN,
				   mode == NAND_ECC_WRITE ? FMC2_PCR_WEN : 0);

		stm32_fmc2_nfc_clear_bch_irq(nfc);
	}

	stm32_fmc2_nfc_set_ecc(nfc, true);
}

/*
 * ECC Hamming calculation
 * ECC is 3 bytes for 512 bytes of data (supports error correction up to
 * max of 1-bit)
 */
static void stm32_fmc2_nfc_ham_set_ecc(const u32 ecc_sta, u8 *ecc)
{
	ecc[0] = ecc_sta;
	ecc[1] = ecc_sta >> 8;
	ecc[2] = ecc_sta >> 16;
}

static int stm32_fmc2_nfc_ham_calculate(struct nand_chip *chip, const u8 *data,
					u8 *ecc)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	u32 sr, heccr;
	int ret;

	ret = regmap_read_poll_timeout(nfc->regmap, FMC2_SR, sr,
				       sr & FMC2_SR_NWRF,
				       1000 * FMC2_TIMEOUT_MS);
	if (ret) {
		dev_err(nfc->dev, "ham timeout\n");
		return ret;
	}

	regmap_read(nfc->regmap, FMC2_HECCR, &heccr);
	stm32_fmc2_nfc_ham_set_ecc(heccr, ecc);
	stm32_fmc2_nfc_set_ecc(nfc, false);

	return 0;
}

static int stm32_fmc2_nfc_ham_correct(struct nand_chip *chip, u8 *dat,
				      u8 *read_ecc, u8 *calc_ecc)
{
	u8 bit_position = 0, b0, b1, b2;
	u32 byte_addr = 0, b;
	u32 i, shifting = 1;

	/* Indicate which bit and byte is faulty (if any) */
	b0 = read_ecc[0] ^ calc_ecc[0];
	b1 = read_ecc[1] ^ calc_ecc[1];
	b2 = read_ecc[2] ^ calc_ecc[2];
	b = b0 | (b1 << 8) | (b2 << 16);

	/* No errors */
	if (likely(!b))
		return 0;

	/* Calculate bit position */
	for (i = 0; i < 3; i++) {
		switch (b % 4) {
		case 2:
			bit_position += shifting;
			break;
		case 1:
			break;
		default:
			return -EBADMSG;
		}
		shifting <<= 1;
		b >>= 2;
	}

	/* Calculate byte position */
	shifting = 1;
	for (i = 0; i < 9; i++) {
		switch (b % 4) {
		case 2:
			byte_addr += shifting;
			break;
		case 1:
			break;
		default:
			return -EBADMSG;
		}
		shifting <<= 1;
		b >>= 2;
	}

	/* Flip the bit */
	dat[byte_addr] ^= (1 << bit_position);

	return 1;
}

/*
 * ECC BCH calculation and correction
 * ECC is 7/13 bytes for 512 bytes of data (supports error correction up to
 * max of 4-bit/8-bit)
 */
static int stm32_fmc2_nfc_bch_calculate(struct nand_chip *chip, const u8 *data,
					u8 *ecc)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	u32 bchisr, bchpbr;
	int ret;

	/* Wait until the BCH code is ready */
	ret = regmap_read_poll_timeout(nfc->regmap, FMC2_BCHISR, bchisr,
				       bchisr & FMC2_BCHISR_EPBRF,
				       1000 * FMC2_TIMEOUT_MS);
	if (ret) {
		dev_err(nfc->dev, "bch timeout\n");
		return -ETIMEDOUT;
	}

	/* Read parity bits */
	regmap_read(nfc->regmap, FMC2_BCHPBR1, &bchpbr);
	ecc[0] = bchpbr;
	ecc[1] = bchpbr >> 8;
	ecc[2] = bchpbr >> 16;
	ecc[3] = bchpbr >> 24;

	regmap_read(nfc->regmap, FMC2_BCHPBR2, &bchpbr);
	ecc[4] = bchpbr;
	ecc[5] = bchpbr >> 8;
	ecc[6] = bchpbr >> 16;

	if (chip->ecc.strength == FMC2_ECC_BCH8) {
		ecc[7] = bchpbr >> 24;

		regmap_read(nfc->regmap, FMC2_BCHPBR3, &bchpbr);
		ecc[8] = bchpbr;
		ecc[9] = bchpbr >> 8;
		ecc[10] = bchpbr >> 16;
		ecc[11] = bchpbr >> 24;

		regmap_read(nfc->regmap, FMC2_BCHPBR4, &bchpbr);
		ecc[12] = bchpbr;
	}

	stm32_fmc2_nfc_set_ecc(nfc, false);

	return 0;
}

static int stm32_fmc2_nfc_bch_decode(int eccsize, u8 *dat, u32 *ecc_sta)
{
	u32 bchdsr0 = ecc_sta[0];
	u32 bchdsr1 = ecc_sta[1];
	u32 bchdsr2 = ecc_sta[2];
	u32 bchdsr3 = ecc_sta[3];
	u32 bchdsr4 = ecc_sta[4];
	u16 pos[8];
	int i, den;
	unsigned int nb_errs = 0;

	/* No errors found */
	if (likely(!(bchdsr0 & FMC2_BCHDSR0_DEF)))
		return 0;

	/* Too many errors detected */
	if (unlikely(bchdsr0 & FMC2_BCHDSR0_DUE))
		return -EBADMSG;

	pos[0] = FIELD_GET(FMC2_BCHDSR1_EBP1, bchdsr1);
	pos[1] = FIELD_GET(FMC2_BCHDSR1_EBP2, bchdsr1);
	pos[2] = FIELD_GET(FMC2_BCHDSR2_EBP3, bchdsr2);
	pos[3] = FIELD_GET(FMC2_BCHDSR2_EBP4, bchdsr2);
	pos[4] = FIELD_GET(FMC2_BCHDSR3_EBP5, bchdsr3);
	pos[5] = FIELD_GET(FMC2_BCHDSR3_EBP6, bchdsr3);
	pos[6] = FIELD_GET(FMC2_BCHDSR4_EBP7, bchdsr4);
	pos[7] = FIELD_GET(FMC2_BCHDSR4_EBP8, bchdsr4);

	den = FIELD_GET(FMC2_BCHDSR0_DEN, bchdsr0);
	for (i = 0; i < den; i++) {
		if (pos[i] < eccsize * 8) {
			change_bit(pos[i], (unsigned long *)dat);
			nb_errs++;
		}
	}

	return nb_errs;
}

static int stm32_fmc2_nfc_bch_correct(struct nand_chip *chip, u8 *dat,
				      u8 *read_ecc, u8 *calc_ecc)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	u32 bchisr, ecc_sta[5];
	int ret;

	/* Wait until the decoding error is ready */
	ret = regmap_read_poll_timeout(nfc->regmap, FMC2_BCHISR, bchisr,
				       bchisr & FMC2_BCHISR_DERF,
				       1000 * FMC2_TIMEOUT_MS);
	if (ret) {
		dev_err(nfc->dev, "bch timeout\n");
		return -ETIMEDOUT;
	}

	regmap_bulk_read(nfc->regmap, FMC2_BCHDSR0, ecc_sta, ARRAY_SIZE(ecc_sta));

	stm32_fmc2_nfc_set_ecc(nfc, false);

	return stm32_fmc2_nfc_bch_decode(chip->ecc.size, dat, ecc_sta);
}

static int stm32_fmc2_nfc_read_page(struct nand_chip *chip, u8 *buf,
				    int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	int ret, i, s, stat, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	int eccstrength = chip->ecc.strength;
	u8 *p = buf;
	u8 *ecc_calc = chip->ecc.calc_buf;
	u8 *ecc_code = chip->ecc.code_buf;
	unsigned int max_bitflips = 0;

	ret = nand_read_page_op(chip, page, 0, NULL, 0);
	if (ret)
		return ret;

	for (i = mtd->writesize + FMC2_BBM_LEN, s = 0; s < eccsteps;
	     s++, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(chip, NAND_ECC_READ);

		/* Read the nand page sector (512 bytes) */
		ret = nand_change_read_column_op(chip, s * eccsize, p,
						 eccsize, false);
		if (ret)
			return ret;

		/* Read the corresponding ECC bytes */
		ret = nand_change_read_column_op(chip, i, ecc_code,
						 eccbytes, false);
		if (ret)
			return ret;

		/* Correct the data */
		stat = chip->ecc.correct(chip, p, ecc_code, ecc_calc);
		if (stat == -EBADMSG)
			/* Check for empty pages with bitflips */
			stat = nand_check_erased_ecc_chunk(p, eccsize,
							   ecc_code, eccbytes,
							   NULL, 0,
							   eccstrength);

		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}
	}

	/* Read oob */
	if (oob_required) {
		ret = nand_change_read_column_op(chip, mtd->writesize,
						 chip->oob_poi, mtd->oobsize,
						 false);
		if (ret)
			return ret;
	}

	return max_bitflips;
}

static void stm32_fmc2_nfc_read_data(struct nand_chip *chip, void *buf,
				     unsigned int len, bool force_8bit)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	void __iomem *io_addr_r = nfc->data_base[nfc->cs_sel];

	if (force_8bit && chip->options & NAND_BUSWIDTH_16)
		/* Reconfigure bus width to 8-bit */
		stm32_fmc2_nfc_set_buswidth_16(nfc, false);

	if (!IS_ALIGNED((uintptr_t)buf, sizeof(u32))) {
		if (!IS_ALIGNED((uintptr_t)buf, sizeof(u16)) && len) {
			*(u8 *)buf = readb_relaxed(io_addr_r);
			buf += sizeof(u8);
			len -= sizeof(u8);
		}

		if (!IS_ALIGNED((uintptr_t)buf, sizeof(u32)) &&
		    len >= sizeof(u16)) {
			*(u16 *)buf = readw_relaxed(io_addr_r);
			buf += sizeof(u16);
			len -= sizeof(u16);
		}
	}

	/* Buf is aligned */
	while (len >= sizeof(u32)) {
		*(u32 *)buf = readl_relaxed(io_addr_r);
		buf += sizeof(u32);
		len -= sizeof(u32);
	}

	/* Read remaining bytes */
	if (len >= sizeof(u16)) {
		*(u16 *)buf = readw_relaxed(io_addr_r);
		buf += sizeof(u16);
		len -= sizeof(u16);
	}

	if (len)
		*(u8 *)buf = readb_relaxed(io_addr_r);

	if (force_8bit && chip->options & NAND_BUSWIDTH_16)
		/* Reconfigure bus width to 16-bit */
		stm32_fmc2_nfc_set_buswidth_16(nfc, true);
}

static void stm32_fmc2_nfc_write_data(struct nand_chip *chip, const void *buf,
				      unsigned int len, bool force_8bit)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	void __iomem *io_addr_w = nfc->data_base[nfc->cs_sel];

	if (force_8bit && chip->options & NAND_BUSWIDTH_16)
		/* Reconfigure bus width to 8-bit */
		stm32_fmc2_nfc_set_buswidth_16(nfc, false);

	if (!IS_ALIGNED((uintptr_t)buf, sizeof(u32))) {
		if (!IS_ALIGNED((uintptr_t)buf, sizeof(u16)) && len) {
			writeb_relaxed(*(u8 *)buf, io_addr_w);
			buf += sizeof(u8);
			len -= sizeof(u8);
		}

		if (!IS_ALIGNED((uintptr_t)buf, sizeof(u32)) &&
		    len >= sizeof(u16)) {
			writew_relaxed(*(u16 *)buf, io_addr_w);
			buf += sizeof(u16);
			len -= sizeof(u16);
		}
	}

	/* Buf is aligned */
	while (len >= sizeof(u32)) {
		writel_relaxed(*(u32 *)buf, io_addr_w);
		buf += sizeof(u32);
		len -= sizeof(u32);
	}

	/* Write remaining bytes */
	if (len >= sizeof(u16)) {
		writew_relaxed(*(u16 *)buf, io_addr_w);
		buf += sizeof(u16);
		len -= sizeof(u16);
	}

	if (len)
		writeb_relaxed(*(u8 *)buf, io_addr_w);

	if (force_8bit && chip->options & NAND_BUSWIDTH_16)
		/* Reconfigure bus width to 16-bit */
		stm32_fmc2_nfc_set_buswidth_16(nfc, true);
}

static int stm32_fmc2_nfc_waitrdy(struct nand_chip *chip,
				  unsigned long timeout_ms)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	const struct nand_sdr_timings *timings;
	u32 isr, sr;

	/* Check if there is no pending requests to the NAND flash */
	if (regmap_read_poll_timeout(nfc->regmap, FMC2_SR, sr,
				     sr & FMC2_SR_NWRF,
				     1000 * FMC2_TIMEOUT_MS))
		dev_warn(nfc->dev, "Waitrdy timeout\n");

	/* Wait tWB before R/B# signal is low */
	timings = nand_get_sdr_timings(nand_get_interface_config(chip));
	ndelay(PSEC_TO_NSEC(timings->tWB_max));

	/* R/B# signal is low, clear high level flag */
	regmap_write(nfc->regmap, FMC2_ICR, FMC2_ICR_CIHLF);

	/* Wait R/B# signal is high */
	return regmap_read_poll_timeout(nfc->regmap, FMC2_ISR, isr,
					isr & FMC2_ISR_IHLF,
					1000 * FMC2_TIMEOUT_MS);
}

static int stm32_fmc2_nfc_exec_op(struct nand_chip *chip,
				  const struct nand_operation *op,
				  bool check_only)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	const struct nand_op_instr *instr = NULL;
	unsigned int op_id, i, timeout;
	int ret = 0;

	if (check_only)
		return 0;

	stm32_fmc2_nfc_select_chip(chip, op->cs);

	for (op_id = 0; op_id < op->ninstrs; op_id++) {
		instr = &op->instrs[op_id];

		switch (instr->type) {
		case NAND_OP_CMD_INSTR:
			writeb_relaxed(instr->ctx.cmd.opcode,
				       nfc->cmd_base[nfc->cs_sel]);
			break;

		case NAND_OP_ADDR_INSTR:
			for (i = 0; i < instr->ctx.addr.naddrs; i++)
				writeb_relaxed(instr->ctx.addr.addrs[i],
					       nfc->addr_base[nfc->cs_sel]);
			break;

		case NAND_OP_DATA_IN_INSTR:
			stm32_fmc2_nfc_read_data(chip, instr->ctx.data.buf.in,
						 instr->ctx.data.len,
						 instr->ctx.data.force_8bit);
			break;

		case NAND_OP_DATA_OUT_INSTR:
			stm32_fmc2_nfc_write_data(chip, instr->ctx.data.buf.out,
						  instr->ctx.data.len,
						  instr->ctx.data.force_8bit);
			break;

		case NAND_OP_WAITRDY_INSTR:
			timeout = instr->ctx.waitrdy.timeout_ms;
			ret = stm32_fmc2_nfc_waitrdy(chip, timeout);
			break;
		}
	}

	return ret;
}

static void stm32_fmc2_nfc_init(struct stm32_fmc2_nfc *nfc)
{
	u32 pcr;

	regmap_read(nfc->regmap, FMC2_PCR, &pcr);

	/* Set CS used to undefined */
	nfc->cs_sel = -1;

	/* Enable wait feature and nand flash memory bank */
	pcr |= FMC2_PCR_PWAITEN;
	pcr |= FMC2_PCR_PBKEN;

	/* Set buswidth to 8 bits mode for identification */
	pcr &= ~FMC2_PCR_PWID;

	/* ECC logic is disabled */
	pcr &= ~FMC2_PCR_ECCEN;

	/* Default mode */
	pcr &= ~FMC2_PCR_ECCALG;
	pcr &= ~FMC2_PCR_BCHECC;
	pcr &= ~FMC2_PCR_WEN;

	/* Set default ECC sector size */
	pcr &= ~FMC2_PCR_ECCSS;
	pcr |= FIELD_PREP(FMC2_PCR_ECCSS, FMC2_PCR_ECCSS_2048);

	/* Set default tclr/tar timings */
	pcr &= ~FMC2_PCR_TCLR;
	pcr |= FIELD_PREP(FMC2_PCR_TCLR, FMC2_PCR_TCLR_DEFAULT);
	pcr &= ~FMC2_PCR_TAR;
	pcr |= FIELD_PREP(FMC2_PCR_TAR, FMC2_PCR_TAR_DEFAULT);

	/* Enable FMC2 controller */
	if (nfc->dev == nfc->cdev)
		regmap_update_bits(nfc->regmap, FMC2_BCR1,
				   FMC2_BCR1_FMC2EN, FMC2_BCR1_FMC2EN);

	regmap_write(nfc->regmap, FMC2_PCR, pcr);
	regmap_write(nfc->regmap, FMC2_PMEM, FMC2_PMEM_DEFAULT);
	regmap_write(nfc->regmap, FMC2_PATT, FMC2_PATT_DEFAULT);
}

static void stm32_fmc2_nfc_calc_timings(struct nand_chip *chip,
					const struct nand_sdr_timings *sdrt)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	struct stm32_fmc2_nand *nand = to_fmc2_nand(chip);
	struct stm32_fmc2_timings *tims = &nand->timings;
	unsigned long hclk = clk_get_rate(nfc->clk);
	unsigned long hclkp = NSEC_PER_SEC / (hclk / 1000);
	unsigned long timing, tar, tclr, thiz, twait;
	unsigned long tset_mem, tset_att, thold_mem, thold_att;

	tar = max_t(unsigned long, hclkp, sdrt->tAR_min);
	timing = DIV_ROUND_UP(tar, hclkp) - 1;
	tims->tar = min_t(unsigned long, timing, FMC2_PCR_TIMING_MASK);

	tclr = max_t(unsigned long, hclkp, sdrt->tCLR_min);
	timing = DIV_ROUND_UP(tclr, hclkp) - 1;
	tims->tclr = min_t(unsigned long, timing, FMC2_PCR_TIMING_MASK);

	tims->thiz = FMC2_THIZ;
	thiz = (tims->thiz + 1) * hclkp;

	/*
	 * tWAIT > tRP
	 * tWAIT > tWP
	 * tWAIT > tREA + tIO
	 */
	twait = max_t(unsigned long, hclkp, sdrt->tRP_min);
	twait = max_t(unsigned long, twait, sdrt->tWP_min);
	twait = max_t(unsigned long, twait, sdrt->tREA_max + FMC2_TIO);
	timing = DIV_ROUND_UP(twait, hclkp);
	tims->twait = clamp_val(timing, 1, FMC2_PMEM_PATT_TIMING_MASK);

	/*
	 * tSETUP_MEM > tCS - tWAIT
	 * tSETUP_MEM > tALS - tWAIT
	 * tSETUP_MEM > tDS - (tWAIT - tHIZ)
	 */
	tset_mem = hclkp;
	if (sdrt->tCS_min > twait && (tset_mem < sdrt->tCS_min - twait))
		tset_mem = sdrt->tCS_min - twait;
	if (sdrt->tALS_min > twait && (tset_mem < sdrt->tALS_min - twait))
		tset_mem = sdrt->tALS_min - twait;
	if (twait > thiz && (sdrt->tDS_min > twait - thiz) &&
	    (tset_mem < sdrt->tDS_min - (twait - thiz)))
		tset_mem = sdrt->tDS_min - (twait - thiz);
	timing = DIV_ROUND_UP(tset_mem, hclkp);
	tims->tset_mem = clamp_val(timing, 1, FMC2_PMEM_PATT_TIMING_MASK);

	/*
	 * tHOLD_MEM > tCH
	 * tHOLD_MEM > tREH - tSETUP_MEM
	 * tHOLD_MEM > max(tRC, tWC) - (tSETUP_MEM + tWAIT)
	 */
	thold_mem = max_t(unsigned long, hclkp, sdrt->tCH_min);
	if (sdrt->tREH_min > tset_mem &&
	    (thold_mem < sdrt->tREH_min - tset_mem))
		thold_mem = sdrt->tREH_min - tset_mem;
	if ((sdrt->tRC_min > tset_mem + twait) &&
	    (thold_mem < sdrt->tRC_min - (tset_mem + twait)))
		thold_mem = sdrt->tRC_min - (tset_mem + twait);
	if ((sdrt->tWC_min > tset_mem + twait) &&
	    (thold_mem < sdrt->tWC_min - (tset_mem + twait)))
		thold_mem = sdrt->tWC_min - (tset_mem + twait);
	timing = DIV_ROUND_UP(thold_mem, hclkp);
	tims->thold_mem = clamp_val(timing, 1, FMC2_PMEM_PATT_TIMING_MASK);

	/*
	 * tSETUP_ATT > tCS - tWAIT
	 * tSETUP_ATT > tCLS - tWAIT
	 * tSETUP_ATT > tALS - tWAIT
	 * tSETUP_ATT > tRHW - tHOLD_MEM
	 * tSETUP_ATT > tDS - (tWAIT - tHIZ)
	 */
	tset_att = hclkp;
	if (sdrt->tCS_min > twait && (tset_att < sdrt->tCS_min - twait))
		tset_att = sdrt->tCS_min - twait;
	if (sdrt->tCLS_min > twait && (tset_att < sdrt->tCLS_min - twait))
		tset_att = sdrt->tCLS_min - twait;
	if (sdrt->tALS_min > twait && (tset_att < sdrt->tALS_min - twait))
		tset_att = sdrt->tALS_min - twait;
	if (sdrt->tRHW_min > thold_mem &&
	    (tset_att < sdrt->tRHW_min - thold_mem))
		tset_att = sdrt->tRHW_min - thold_mem;
	if (twait > thiz && (sdrt->tDS_min > twait - thiz) &&
	    (tset_att < sdrt->tDS_min - (twait - thiz)))
		tset_att = sdrt->tDS_min - (twait - thiz);
	timing = DIV_ROUND_UP(tset_att, hclkp);
	tims->tset_att = clamp_val(timing, 1, FMC2_PMEM_PATT_TIMING_MASK);

	/*
	 * tHOLD_ATT > tALH
	 * tHOLD_ATT > tCH
	 * tHOLD_ATT > tCLH
	 * tHOLD_ATT > tCOH
	 * tHOLD_ATT > tDH
	 * tHOLD_ATT > tWB + tIO + tSYNC - tSETUP_MEM
	 * tHOLD_ATT > tADL - tSETUP_MEM
	 * tHOLD_ATT > tWH - tSETUP_MEM
	 * tHOLD_ATT > tWHR - tSETUP_MEM
	 * tHOLD_ATT > tRC - (tSETUP_ATT + tWAIT)
	 * tHOLD_ATT > tWC - (tSETUP_ATT + tWAIT)
	 */
	thold_att = max_t(unsigned long, hclkp, sdrt->tALH_min);
	thold_att = max_t(unsigned long, thold_att, sdrt->tCH_min);
	thold_att = max_t(unsigned long, thold_att, sdrt->tCLH_min);
	thold_att = max_t(unsigned long, thold_att, sdrt->tCOH_min);
	thold_att = max_t(unsigned long, thold_att, sdrt->tDH_min);
	if ((sdrt->tWB_max + FMC2_TIO + FMC2_TSYNC > tset_mem) &&
	    (thold_att < sdrt->tWB_max + FMC2_TIO + FMC2_TSYNC - tset_mem))
		thold_att = sdrt->tWB_max + FMC2_TIO + FMC2_TSYNC - tset_mem;
	if (sdrt->tADL_min > tset_mem &&
	    (thold_att < sdrt->tADL_min - tset_mem))
		thold_att = sdrt->tADL_min - tset_mem;
	if (sdrt->tWH_min > tset_mem &&
	    (thold_att < sdrt->tWH_min - tset_mem))
		thold_att = sdrt->tWH_min - tset_mem;
	if (sdrt->tWHR_min > tset_mem &&
	    (thold_att < sdrt->tWHR_min - tset_mem))
		thold_att = sdrt->tWHR_min - tset_mem;
	if ((sdrt->tRC_min > tset_att + twait) &&
	    (thold_att < sdrt->tRC_min - (tset_att + twait)))
		thold_att = sdrt->tRC_min - (tset_att + twait);
	if ((sdrt->tWC_min > tset_att + twait) &&
	    (thold_att < sdrt->tWC_min - (tset_att + twait)))
		thold_att = sdrt->tWC_min - (tset_att + twait);
	timing = DIV_ROUND_UP(thold_att, hclkp);
	tims->thold_att = clamp_val(timing, 1, FMC2_PMEM_PATT_TIMING_MASK);
}

static int stm32_fmc2_nfc_setup_interface(struct nand_chip *chip, int chipnr,
					  const struct nand_interface_config *conf)
{
	const struct nand_sdr_timings *sdrt;

	sdrt = nand_get_sdr_timings(conf);
	if (IS_ERR(sdrt))
		return PTR_ERR(sdrt);

	if (conf->timings.mode > 3)
		return -EOPNOTSUPP;

	if (chipnr == NAND_DATA_IFACE_CHECK_ONLY)
		return 0;

	stm32_fmc2_nfc_calc_timings(chip, sdrt);
	stm32_fmc2_nfc_timings_init(chip);

	return 0;
}

static void stm32_fmc2_nfc_nand_callbacks_setup(struct nand_chip *chip)
{
	/*
	 * Specific callbacks to read/write a page depending on
	 * the mode (polling/sequencer) and the algo used (Hamming, BCH).
	 */
	chip->ecc.hwctl = stm32_fmc2_nfc_hwctl;
	if (chip->ecc.strength == FMC2_ECC_HAM) {
		/* Hamming is used */
		chip->ecc.calculate = stm32_fmc2_nfc_ham_calculate;
		chip->ecc.correct = stm32_fmc2_nfc_ham_correct;
		chip->ecc.options |= NAND_ECC_GENERIC_ERASED_CHECK;
	} else {
		/* BCH is used */
		chip->ecc.calculate = stm32_fmc2_nfc_bch_calculate;
		chip->ecc.correct = stm32_fmc2_nfc_bch_correct;
		chip->ecc.read_page = stm32_fmc2_nfc_read_page;
	}

	/* Specific configurations depending on the algo used */
	if (chip->ecc.strength == FMC2_ECC_HAM)
		chip->ecc.bytes = chip->options & NAND_BUSWIDTH_16 ? 4 : 3;
	else if (chip->ecc.strength == FMC2_ECC_BCH8)
		chip->ecc.bytes = chip->options & NAND_BUSWIDTH_16 ? 14 : 13;
	else
		chip->ecc.bytes = chip->options & NAND_BUSWIDTH_16 ? 8 : 7;
}

static int stm32_fmc2_nfc_ooblayout_ecc(struct mtd_info *mtd, int section,
					struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	if (section)
		return -ERANGE;

	oobregion->length = ecc->total;
	oobregion->offset = FMC2_BBM_LEN;

	return 0;
}

static int stm32_fmc2_nfc_ooblayout_free(struct mtd_info *mtd, int section,
					 struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	if (section)
		return -ERANGE;

	oobregion->length = mtd->oobsize - ecc->total - FMC2_BBM_LEN;
	oobregion->offset = ecc->total + FMC2_BBM_LEN;

	return 0;
}

static const struct mtd_ooblayout_ops stm32_fmc2_nfc_ooblayout_ops = {
	.ecc = stm32_fmc2_nfc_ooblayout_ecc,
	.free = stm32_fmc2_nfc_ooblayout_free,
};

static int stm32_fmc2_nfc_calc_ecc_bytes(int step_size, int strength)
{
	/* Hamming */
	if (strength == FMC2_ECC_HAM)
		return 4;

	/* BCH8 */
	if (strength == FMC2_ECC_BCH8)
		return 14;

	/* BCH4 */
	return 8;
}

NAND_ECC_CAPS_SINGLE(stm32_fmc2_nfc_ecc_caps, stm32_fmc2_nfc_calc_ecc_bytes,
		     FMC2_ECC_STEP_SIZE,
		     FMC2_ECC_HAM, FMC2_ECC_BCH4, FMC2_ECC_BCH8);

static int stm32_fmc2_nfc_attach_chip(struct nand_chip *chip)
{
	struct stm32_fmc2_nfc *nfc = to_stm32_nfc(chip->controller);
	struct mtd_info *mtd = nand_to_mtd(chip);
	int ret;

	/* Default ECC settings in case they are not set in the device tree */
	if (!chip->ecc.size)
		chip->ecc.size = FMC2_ECC_STEP_SIZE;

	if (!chip->ecc.strength)
		chip->ecc.strength = FMC2_ECC_BCH8;

	ret = nand_ecc_choose_conf(chip, &stm32_fmc2_nfc_ecc_caps,
				   mtd->oobsize - FMC2_BBM_LEN);
	if (ret) {
		dev_err(nfc->dev, "no valid ECC settings set\n");
		return ret;
	}

	if (mtd->writesize / chip->ecc.size > FMC2_MAX_SG) {
		dev_err(nfc->dev, "nand page size is not supported\n");
		return -EINVAL;
	}

	if (chip->bbt_options & NAND_BBT_USE_FLASH)
		chip->bbt_options |= NAND_BBT_NO_OOB;

	stm32_fmc2_nfc_nand_callbacks_setup(chip);

	mtd_set_ooblayout(mtd, &stm32_fmc2_nfc_ooblayout_ops);

	stm32_fmc2_nfc_setup(chip);

	return 0;
}

static const struct nand_controller_ops stm32_fmc2_nfc_controller_ops = {
	.attach_chip = stm32_fmc2_nfc_attach_chip,
	.exec_op = stm32_fmc2_nfc_exec_op,
	.setup_interface = stm32_fmc2_nfc_setup_interface,
};

static void stm32_fmc2_nfc_wp_enable(struct stm32_fmc2_nand *nand)
{
	if (nand->wp_gpio)
		gpiod_set_value(nand->wp_gpio, 1);
}

static void stm32_fmc2_nfc_wp_disable(struct stm32_fmc2_nand *nand)
{
	if (nand->wp_gpio)
		gpiod_set_value(nand->wp_gpio, 0);
}

static int stm32_fmc2_nfc_parse_child(struct stm32_fmc2_nfc *nfc,
				      struct device_node *dn)
{
	struct stm32_fmc2_nand *nand = &nfc->nand;
	u32 cs;
	int ret, i;

	if (!of_get_property(dn, "reg", &nand->ncs))
		return -EINVAL;

	nand->ncs /= sizeof(u32);
	if (!nand->ncs) {
		dev_err(nfc->dev, "invalid reg property size\n");
		return -EINVAL;
	}

	for (i = 0; i < nand->ncs; i++) {
		ret = of_property_read_u32_index(dn, "reg", i, &cs);
		if (ret) {
			dev_err(nfc->dev, "could not retrieve reg property: %d\n",
				ret);
			return ret;
		}

		if (cs >= FMC2_MAX_CE) {
			dev_err(nfc->dev, "invalid reg value: %d\n", cs);
			return -EINVAL;
		}

		if (nfc->cs_assigned & BIT(cs)) {
			dev_err(nfc->dev, "cs already assigned: %d\n", cs);
			return -EINVAL;
		}

		nfc->cs_assigned |= BIT(cs);
		nand->cs_used[i] = cs;
	}

	nand->wp_gpio = dev_gpiod_get(nfc->dev, dn, "wp", GPIOD_OUT_HIGH, "wp");
	if (IS_ERR(nand->wp_gpio)) {
		ret = PTR_ERR(nand->wp_gpio);
		if (ret != -ENOENT)
			return dev_err_probe(nfc->dev, ret,
					     "failed to request WP GPIO\n");

		nand->wp_gpio = NULL;
	}

	nand_set_flash_node(&nand->chip, dn);

	return 0;
}

static int stm32_fmc2_nfc_parse_dt(struct stm32_fmc2_nfc *nfc)
{
	struct device_node *dn = nfc->dev->of_node;
	struct device_node *child;
	int nchips = of_get_child_count(dn);
	int ret = 0;

	if (!nchips) {
		dev_err(nfc->dev, "NAND chip not defined\n");
		return -EINVAL;
	}

	if (nchips > 1) {
		dev_err(nfc->dev, "too many NAND chips defined\n");
		return -EINVAL;
	}

	for_each_child_of_node(dn, child) {
		ret = stm32_fmc2_nfc_parse_child(nfc, child);
		if (ret < 0) {
			of_node_put(child);
			return ret;
		}
	}

	return ret;
}

static int stm32_fmc2_nfc_set_cdev(struct stm32_fmc2_nfc *nfc)
{
	struct device *dev = nfc->dev;
	bool ebi_found = false;

	if (dev->parent && of_device_is_compatible(dev->parent->of_node,
						   "st,stm32mp1-fmc2-ebi"))
		ebi_found = true;

	if (of_device_is_compatible(dev->of_node, "st,stm32mp1-fmc2-nfc")) {
		if (ebi_found) {
			nfc->cdev = dev->parent;

			return 0;
		}

		return -EINVAL;
	}

	if (ebi_found)
		return -EINVAL;

	nfc->cdev = dev;

	return 0;
}

static int __init stm32_fmc2_nfc_probe(struct device *dev)
{
	struct stm32_fmc2_nfc *nfc;
	struct stm32_fmc2_nand *nand;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct resource cres;
	int chip_cs, mem_region, ret;
	int start_region = 0;

	nfc = kzalloc(sizeof(*nfc), GFP_KERNEL);
	if (!nfc)
		return -ENOMEM;

	nfc->dev = dev;
	nand_controller_init(&nfc->base);
	nfc->base.ops = &stm32_fmc2_nfc_controller_ops;

	ret = stm32_fmc2_nfc_set_cdev(nfc);
	if (ret)
		return ret;

	ret = stm32_fmc2_nfc_parse_dt(nfc);
	if (ret)
		return ret;

	ret = of_address_to_resource(nfc->cdev->of_node, 0, &cres);
	if (ret)
		return ret;

	nfc->regmap = device_node_to_regmap(nfc->cdev->of_node);
	if (IS_ERR(nfc->regmap))
		return PTR_ERR(nfc->regmap);

	if (nfc->dev == nfc->cdev)
		start_region = 1;

	for (chip_cs = 0, mem_region = start_region; chip_cs < FMC2_MAX_CE;
	     chip_cs++, mem_region += 3) {
		if (!(nfc->cs_assigned & BIT(chip_cs)))
			continue;

		nfc->data_base[chip_cs] = of_iomap(dev->of_node, mem_region);
		if (IS_ERR(nfc->data_base[chip_cs]))
			return PTR_ERR(nfc->data_base[chip_cs]);

		nfc->cmd_base[chip_cs] = of_iomap(dev->of_node, mem_region + 1);
		if (IS_ERR(nfc->cmd_base[chip_cs]))
			return PTR_ERR(nfc->cmd_base[chip_cs]);

		nfc->addr_base[chip_cs] = of_iomap(dev->of_node, mem_region + 2);
		if (IS_ERR(nfc->addr_base[chip_cs]))
			return PTR_ERR(nfc->addr_base[chip_cs]);
	}

	nfc->clk = clk_get(nfc->cdev, NULL);
	if (IS_ERR(nfc->clk))
		return PTR_ERR(nfc->clk);

	ret = clk_prepare_enable(nfc->clk);
	if (ret) {
		dev_err(dev, "can not enable the clock\n");
		return ret;
	}

	ret = device_reset_us(dev, 2);
	if (ret)
		goto err_clk_disable;

	stm32_fmc2_nfc_init(nfc);

	nand = &nfc->nand;
	chip = &nand->chip;
	mtd = nand_to_mtd(chip);
	mtd->dev.parent = dev;

	chip->controller = &nfc->base;
	chip->options |= NAND_BUSWIDTH_AUTO | NAND_NO_SUBPAGE_WRITE;

	/* Default ECC settings */
	chip->ecc.engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
	chip->ecc.size = FMC2_ECC_STEP_SIZE;
	chip->ecc.strength = FMC2_ECC_BCH8;

	stm32_fmc2_nfc_wp_disable(nand);

	/* Scan to find existence of the device */
	ret = nand_scan(chip, nand->ncs);
	if (ret)
		goto err_wp_enable;

	ret = add_mtd_nand_device(mtd, "nand");
	if (ret)
		goto err_nand_cleanup;

	return 0;

err_nand_cleanup:
	nand_cleanup(chip);

err_wp_enable:
	stm32_fmc2_nfc_wp_enable(nand);

err_clk_disable:
	clk_disable_unprepare(nfc->clk);

	return ret;
}

static __maybe_unused struct of_device_id stm32_fmc2_nfc_match[] = {
	{ .compatible = "st,stm32mp15-fmc2", },
	{ .compatible = "st,stm32mp1-fmc2-nfc", },
	{ }
};
MODULE_DEVICE_TABLE(of, stm32_fmc2_nfc_match);

static struct driver stm32_fmc2_nfc_driver = {
	.name = "stm32_fmc2_nfc",
	.probe	= stm32_fmc2_nfc_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_fmc2_nfc_match),
};
coredevice_platform_driver(stm32_fmc2_nfc_driver);
