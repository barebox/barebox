/*
 * (C) Copyright 2011 Wolfram Sang, Pengutronix e.K.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Based on a similar function in Karo Electronics TX28-U-Boot (flash.c).
 * Probably written by Lothar Waßmann (like tx28.c).
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include <nand.h>
#include <sizes.h>
#include <errno.h>
#include <io.h>

#include <mach/imx-regs.h>

#include <linux/err.h>
#include <linux/mtd/nand.h>

#define FCB_START_BLOCK		0
#define NUM_FCB_BLOCKS		1
#define MAX_FCB_BLOCKS		32768

#define GPMI_TIMING0				0x00000070
#define	GPMI_TIMING0_ADDRESS_SETUP_MASK			(0xff << 16)
#define	GPMI_TIMING0_ADDRESS_SETUP_OFFSET		16
#define	GPMI_TIMING0_DATA_HOLD_MASK			(0xff << 8)
#define	GPMI_TIMING0_DATA_HOLD_OFFSET			8
#define	GPMI_TIMING0_DATA_SETUP_MASK			0xff
#define	GPMI_TIMING0_DATA_SETUP_OFFSET			0

#define GPMI_TIMING1				0x00000080

#define BCH_MODE				0x00000020

#define BCH_FLASH0LAYOUT0			0x00000080
#define	BCH_FLASHLAYOUT0_NBLOCKS_MASK			(0xff << 24)
#define	BCH_FLASHLAYOUT0_NBLOCKS_OFFSET			24
#define	BCH_FLASHLAYOUT0_META_SIZE_MASK			(0xff << 16)
#define	BCH_FLASHLAYOUT0_META_SIZE_OFFSET		16
#define	BCH_FLASHLAYOUT0_ECC0_MASK			(0xf << 12)
#define	BCH_FLASHLAYOUT0_ECC0_OFFSET			12
#define	BCH_FLASHLAYOUT0_DATA0_SIZE_MASK		0xfff
#define	BCH_FLASHLAYOUT0_DATA0_SIZE_OFFSET		0

#define BCH_FLASH0LAYOUT1			0x00000090
#define	BCH_FLASHLAYOUT1_PAGE_SIZE_MASK			(0xffff << 16)
#define	BCH_FLASHLAYOUT1_PAGE_SIZE_OFFSET		16
#define	BCH_FLASHLAYOUT1_ECCN_MASK			(0xf << 12)
#define	BCH_FLASHLAYOUT1_ECCN_OFFSET			12
#define	BCH_FLASHLAYOUT1_DATAN_SIZE_MASK		0xfff
#define	BCH_FLASHLAYOUT1_DATAN_SIZE_OFFSET		0

struct mx28_nand_timing {
	u8 data_setup;
	u8 data_hold;
	u8 address_setup;
	u8 dsample_time;
	u8 nand_timing_state;
	u8 tREA;
	u8 tRLOH;
	u8 tRHOH;
};

struct mx28_fcb {
	u32 checksum;
	u32 fingerprint;
	u32 version;
	struct mx28_nand_timing timing;
	u32 page_data_size;
	u32 total_page_size;
	u32 sectors_per_block;
	u32 number_of_nands;	/* not used by ROM code */
	u32 total_internal_die;	/* not used by ROM code */
	u32 cell_type;		/* not used by ROM code */
	u32 ecc_blockn_type;
	u32 ecc_block0_size;
	u32 ecc_blockn_size;
	u32 ecc_block0_type;
	u32 metadata_size;
	u32 ecc_blocks_per_page;
	u32 rsrvd[6];		 /* not used by ROM code */
	u32 bch_mode;
	u32 boot_patch;
	u32 patch_sectors;
	u32 fw1_start_page;
	u32 fw2_start_page;
	u32 fw1_sectors;
	u32 fw2_sectors;
	u32 dbbt_search_area;
	u32 bb_mark_byte;
	u32 bb_mark_startbit;
	u32 bb_mark_phys_offset;
};

struct mx28_dbbt_header {
	u32 checksum;
	u32 fingerprint;
	u32 version;
	u32 number_bb;
	u32 number_pages;
	u8 spare[492];
};

struct mx28_dbbt {
	u32 nand_number;
	u32 number_bb;
	u32 bb_num[2040 / 4];
};

#define BF_VAL(v, bf)		(((v) & bf##_MASK) >> bf##_OFFSET)
#define GETBIT(v,n)	(((v) >> (n)) & 0x1)

static u8 calculate_parity_13_8(u8 d)
{
	u8 p = 0;

	p |= (GETBIT(d, 6) ^ GETBIT(d, 5) ^ GETBIT(d, 3) ^ GETBIT(d, 2))		 << 0;
	p |= (GETBIT(d, 7) ^ GETBIT(d, 5) ^ GETBIT(d, 4) ^ GETBIT(d, 2) ^ GETBIT(d, 1)) << 1;
	p |= (GETBIT(d, 7) ^ GETBIT(d, 6) ^ GETBIT(d, 5) ^ GETBIT(d, 1) ^ GETBIT(d, 0)) << 2;
	p |= (GETBIT(d, 7) ^ GETBIT(d, 4) ^ GETBIT(d, 3) ^ GETBIT(d, 0))		 << 3;
	p |= (GETBIT(d, 6) ^ GETBIT(d, 4) ^ GETBIT(d, 3) ^ GETBIT(d, 2) ^ GETBIT(d, 1) ^ GETBIT(d, 0)) << 4;
	return p;
}

static void encode_hamming_13_8(void *_src, void *_ecc, size_t size)
{
	int i;
	u8 *src = _src;
	u8 *ecc = _ecc;

	for (i = 0; i < size; i++)
		ecc[i] = calculate_parity_13_8(src[i]);
}

static u32 calc_chksum(void *buf, size_t size)
{
	u32 chksum = 0;
	u8 *bp = buf;
	size_t i;

	for (i = 0; i < size; i++)
		chksum += bp[i];

	return ~chksum;
}

/*
  Physical organisation of data in NAND flash:
  metadata
  payload chunk 0 (may be empty)
  ecc for metadata + payload chunk 0
  payload chunk 1
  ecc for payload chunk 1
...
  payload chunk n
  ecc for payload chunk n
 */

static int calc_bb_offset(struct mtd_info *mtd, struct mx28_fcb *fcb)
{
	int bb_mark_offset;
	int chunk_data_size = fcb->ecc_blockn_size * 8;
	int chunk_ecc_size = (fcb->ecc_blockn_type << 1) * 13;
	int chunk_total_size = chunk_data_size + chunk_ecc_size;
	int bb_mark_chunk, bb_mark_chunk_offs;

	bb_mark_offset = (mtd->writesize - fcb->metadata_size) * 8;
	if (fcb->ecc_block0_size == 0)
		bb_mark_offset -= (fcb->ecc_block0_type << 1) * 13;

	bb_mark_chunk = bb_mark_offset / chunk_total_size;
	bb_mark_chunk_offs = bb_mark_offset - (bb_mark_chunk * chunk_total_size);
	if (bb_mark_chunk_offs > chunk_data_size) {
		printf("Unsupported ECC layout; BB mark resides in ECC data: %u\n",
			bb_mark_chunk_offs);
		return -EINVAL;
	}
	bb_mark_offset -= bb_mark_chunk * chunk_ecc_size;
	return bb_mark_offset;
}

static struct mx28_fcb *create_fcb(struct mtd_info *mtd, void *buf, unsigned fw1_start_block,
				size_t fw_size, unsigned fw2_start_block)
{
	u32 fl0, fl1, t0;
	int metadata_size;
	int bb_mark_bit_offs;
	struct mx28_fcb *fcb;
	int fcb_offs;
	void __iomem *bch_regs = (void *)MXS_BCH_BASE;
	void __iomem *gpmi_regs = (void *)MXS_GPMI_BASE;

	fl0 = readl(bch_regs + BCH_FLASH0LAYOUT0);
	fl1 = readl(bch_regs + BCH_FLASH0LAYOUT1);
	t0 = readl(gpmi_regs + GPMI_TIMING0);
	metadata_size = BF_VAL(fl0, BCH_FLASHLAYOUT0_META_SIZE);

	fcb = buf + ALIGN(metadata_size, 4);
	fcb_offs = (void *)fcb - buf;

	memset(buf, 0x00, fcb_offs);
	memset(fcb, 0x00, sizeof(*fcb));
	memset(fcb + 1, 0xff, mtd->erasesize - fcb_offs - sizeof(*fcb));

	strncpy((char *)&fcb->fingerprint, "FCB ", 4);
	fcb->version = cpu_to_be32(1);

	fcb->timing.data_setup = BF_VAL(t0, GPMI_TIMING0_DATA_SETUP);
	fcb->timing.data_hold = BF_VAL(t0, GPMI_TIMING0_DATA_HOLD);
	fcb->timing.address_setup = BF_VAL(t0, GPMI_TIMING0_ADDRESS_SETUP);

	fcb->page_data_size = mtd->writesize;
	fcb->total_page_size = mtd->writesize + mtd->oobsize;
	fcb->sectors_per_block = mtd->erasesize / mtd->writesize;

	fcb->ecc_block0_type = BF_VAL(fl0, BCH_FLASHLAYOUT0_ECC0);
	fcb->ecc_block0_size = BF_VAL(fl0, BCH_FLASHLAYOUT0_DATA0_SIZE);
	fcb->ecc_blockn_type = BF_VAL(fl1, BCH_FLASHLAYOUT1_ECCN);
	fcb->ecc_blockn_size = BF_VAL(fl1, BCH_FLASHLAYOUT1_DATAN_SIZE);

	fcb->metadata_size = BF_VAL(fl0, BCH_FLASHLAYOUT0_META_SIZE);
	fcb->ecc_blocks_per_page = BF_VAL(fl0, BCH_FLASHLAYOUT0_NBLOCKS);
	fcb->bch_mode = readl(bch_regs + BCH_MODE);
/*
	fcb->boot_patch = 0;
	fcb->patch_sectors = 0;
*/
	fcb->fw1_start_page = fw1_start_block / mtd->writesize;
	fcb->fw1_sectors = DIV_ROUND_UP(fw_size, mtd->writesize);

	if (fw2_start_block) {
		fcb->fw2_start_page = fw2_start_block / mtd->writesize;
		fcb->fw2_sectors = fcb->fw1_sectors;
	}

	fcb->dbbt_search_area = 1;

	bb_mark_bit_offs = calc_bb_offset(mtd, fcb);
	if (bb_mark_bit_offs < 0)
		return ERR_PTR(bb_mark_bit_offs);
	fcb->bb_mark_byte = bb_mark_bit_offs / 8;
	fcb->bb_mark_startbit = bb_mark_bit_offs % 8;
	fcb->bb_mark_phys_offset = mtd->writesize;

	fcb->checksum = calc_chksum(&fcb->fingerprint, 512 - 4);
	return fcb;
}

static int find_fcb(struct mtd_info *mtd, void *ref, int page)
{
	int ret = 0;
	struct nand_chip *chip = mtd->priv;
	void *buf = malloc(mtd->erasesize);

	if (buf == NULL)
		return -ENOMEM;

	chip->select_chip(mtd, 0);
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
	ret = chip->ecc.read_page_raw(mtd, chip, buf, 1, page);
	if (ret) {
		printf("Failed to read FCB from page %u: %d\n", page, ret);
		return ret;
	}
	chip->select_chip(mtd, -1);
	if (memcmp(buf, ref, mtd->writesize) == 0) {
		printf("%s: Found FCB in page %u (%08x)\n", __func__,
			page, page * mtd->writesize);
		ret = 1;
	}
	free(buf);
	return ret;
}

static int write_fcb(struct mtd_info *mtd, void *buf, int block)
{
	int ret;
	struct nand_chip *chip = mtd->priv;
	int page = block / mtd->writesize;
	struct erase_info erase_opts = {
		.mtd = mtd,
		.addr = block,
		.len = mtd->erasesize,
		.callback = NULL,
	};

	ret = find_fcb(mtd, buf, page);
	if (ret > 0) {
		printf("FCB at block %08x is up to date\n", block);
		return 0;
	}

	ret = mtd->erase(mtd, &erase_opts);
	if (ret) {
		printf("Failed to erase FCB block %08x\n", block);
		return ret;
	}

	printf("Writing FCB to block %08x\n", block);
	chip->select_chip(mtd, 0);
	ret = chip->write_page(mtd, chip, 0, mtd->erasesize, buf, 1, page, 0, 1);
	if (ret) {
		printf("Failed to write FCB to block %08x: %d\n", block, ret);
	}
	chip->select_chip(mtd, -1);
	return ret;
}

int update_bcb(int argc, char *argv[])
{
	int ret;
	int block;
	void *buf;
	struct mx28_fcb *fcb;
	struct cdev *tmp_cdev, *bcb_cdev, *firmware_cdev;
	unsigned long fw2_offset = 0;
	struct mtd_info *mtd;
	unsigned fcb_written = 0;

	if (argc == 1)
		return COMMAND_ERROR_USAGE;

	tmp_cdev = cdev_by_name("nand0");
	if (!tmp_cdev || !tmp_cdev->mtd) {
		pr_err("%s: No NAND device!\n", __func__);
		return -ENODEV;
	}

	mtd = tmp_cdev->mtd;

	bcb_cdev = cdev_by_name("nand0.bcb");
	if (!bcb_cdev) {
		pr_err("%s: No FCB device!\n", __func__);
		return -ENODEV;
	}

	firmware_cdev = cdev_by_name(argv[1]);
	if (!firmware_cdev) {
		pr_err("%s: Bootstream-Image not found!\n", __func__);
		return -ENODEV;
	}

	if (argc > 2) {
		tmp_cdev = cdev_by_name(argv[2]);
		if (!tmp_cdev) {
			pr_err("%s: Redundant Bootstream-Image not found!\n", __func__);
			return -ENODEV;
		}
		fw2_offset = tmp_cdev->offset;
	}

	buf = malloc(mtd->erasesize);
	if (!buf)
		return -ENOMEM;

	fcb = create_fcb(mtd, buf, firmware_cdev->offset, firmware_cdev->size, fw2_offset);
	if (IS_ERR(fcb)) {
		printf("Failed to initialize FCB: %ld\n", PTR_ERR(fcb));
		return PTR_ERR(fcb);
	}
	encode_hamming_13_8(fcb, (void *)fcb + 512, 512);

	for (block = bcb_cdev->offset; block < bcb_cdev->offset + bcb_cdev->size / 2;
		block += mtd->erasesize) {

		if (nand_isbad_bbt(mtd, block, false))
			continue;

		ret = write_fcb(mtd, buf, block);
		if (ret) {
			printf("Failed to write FCB to block %u\n", block);
			return ret;
		}

		fcb_written++;
	}

	return fcb_written ? 0 : -ENOSPC;
}

BAREBOX_CMD_HELP_START(bcb)
BAREBOX_CMD_HELP_USAGE("bcb <first_bootstream> [second_bootstream]\n")
BAREBOX_CMD_HELP_SHORT("Write a BCB to NAND flash which an MX23/28 needs to boot.\n")
BAREBOX_CMD_HELP_TEXT ("Example: bcb nand0.bootstream\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bcb)
	.cmd = update_bcb,
	.usage = "Writes a MX23/28 BCB data structure to flash",
	BAREBOX_CMD_HELP(cmd_bcb_help)
BAREBOX_CMD_END
