/*
 * Copyright (C) 2014 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */

#define pr_fmt(fmt) "imx-bbu-nand-fcb: " fmt

#include <filetype.h>
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <ioctl.h>
#include <linux/sizes.h>
#include <bbu.h>
#include <fs.h>
#include <linux/mtd/mtd-abi.h>
#include <linux/mtd/nand_mxs.h>
#include <linux/mtd/mtd.h>
#include <linux/stat.h>
#include <io.h>

struct dbbt_block {
	uint32_t Checksum;
	uint32_t FingerPrint;
	uint32_t Version;
	uint32_t numberBB; /* reserved on i.MX6 */
	uint32_t DBBTNumOfPages;
};

struct fcb_block {
	uint32_t Checksum;		/* First fingerprint in first byte */
	uint32_t FingerPrint;		/* 2nd fingerprint at byte 4 */
	uint32_t Version;		/* 3rd fingerprint at byte 8 */
	uint8_t DataSetup;
	uint8_t DataHold;
	uint8_t AddressSetup;
	uint8_t DSAMPLE_TIME;
	/* These are for application use only and not for ROM. */
	uint8_t NandTimingState;
	uint8_t REA;
	uint8_t RLOH;
	uint8_t RHOH;
	uint32_t PageDataSize;		/* 2048 for 2K pages, 4096 for 4K pages */
	uint32_t TotalPageSize;		/* 2112 for 2K pages, 4314 for 4K pages */
	uint32_t SectorsPerBlock;	/* Number of 2K sections per block */
	uint32_t NumberOfNANDs;		/* Total Number of NANDs - not used by ROM */
	uint32_t TotalInternalDie;	/* Number of separate chips in this NAND */
	uint32_t CellType;		/* MLC or SLC */
	uint32_t EccBlockNEccType;	/* Type of ECC, can be one of BCH-0-20 */
	uint32_t EccBlock0Size;		/* Number of bytes for Block0 - BCH */
	uint32_t EccBlockNSize;		/* Block size in bytes for all blocks other than Block0 - BCH */
	uint32_t EccBlock0EccType;	/* Ecc level for Block 0 - BCH */
	uint32_t MetadataBytes;		/* Metadata size - BCH */
	uint32_t NumEccBlocksPerPage;	/* Number of blocks per page for ROM use - BCH */
	uint32_t EccBlockNEccLevelSDK;	/* Type of ECC, can be one of BCH-0-20 */
	uint32_t EccBlock0SizeSDK;	/* Number of bytes for Block0 - BCH */
	uint32_t EccBlockNSizeSDK;	/* Block size in bytes for all blocks other than Block0 - BCH */
	uint32_t EccBlock0EccLevelSDK;	/* Ecc level for Block 0 - BCH */
	uint32_t NumEccBlocksPerPageSDK;/* Number of blocks per page for SDK use - BCH */
	uint32_t MetadataBytesSDK;	/* Metadata size - BCH */
	uint32_t EraseThreshold;	/* To set into BCH_MODE register */
	uint32_t BootPatch;		/* 0 for normal boot and 1 to load patch starting next to FCB */
	uint32_t PatchSectors;		/* Size of patch in sectors */
	uint32_t Firmware1_startingPage;/* Firmware image starts on this sector */
	uint32_t Firmware2_startingPage;/* Secondary FW Image starting Sector */
	uint32_t PagesInFirmware1;	/* Number of sectors in firmware image */
	uint32_t PagesInFirmware2;	/* Number of sector in secondary FW image */
	uint32_t DBBTSearchAreaStartAddress; /* Page address where dbbt search area begins */
	uint32_t BadBlockMarkerByte;	/* Byte in page data that have manufacturer marked bad block marker, */
					/* this will be swapped with metadata[0] to complete page data. */
	uint32_t BadBlockMarkerStartBit;/* For BCH ECC sizes other than 8 and 16 the bad block marker does not */
					/* start at 0th bit of BadBlockMarkerByte. This field is used to get to */
					/* the start bit of bad block marker byte with in BadBlockMarkerByte */
	uint32_t BBMarkerPhysicalOffset;/* FCB value that gives byte offset for bad block marker on physical NAND page */
	uint32_t BCHType;

	uint32_t TMTiming2_ReadLatency;
	uint32_t TMTiming2_PreambleDelay;
	uint32_t TMTiming2_CEDelay;
	uint32_t TMTiming2_PostambleDelay;
	uint32_t TMTiming2_CmdAddPause;
	uint32_t TMTiming2_DataPause;
	uint32_t TMSpeed;
	uint32_t TMTiming1_BusyTimeout;

	uint32_t DISBBM;	/* the flag to enable (1)/disable(0) bi swap */
	uint32_t BBMarkerPhysicalOffsetInSpareData; /* The swap position of main area in spare area */
};

struct imx_nand_fcb_bbu_handler {
	struct bbu_handler handler;

	void (*fcb_create)(struct imx_nand_fcb_bbu_handler *imx_handler,
		struct fcb_block *fcb, struct mtd_info *mtd);
	void (*dbbt_create)(struct imx_nand_fcb_bbu_handler *imx_handler,
		struct dbbt_block *dbbt, int num_bad_blocks);
	enum filetype filetype;
};

#define BF_VAL(v, bf)		(((v) & bf##_MASK) >> bf##_OFFSET)
#define GETBIT(v,n)	(((v) >> (n)) & 0x1)

static uint8_t calculate_parity_13_8(uint8_t d)
{
	uint8_t p = 0;

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
	uint8_t *src = _src;
	uint8_t *ecc = _ecc;

	for (i = 0; i < size; i++)
		ecc[i] = calculate_parity_13_8(src[i]);
}

static uint32_t calc_chksum(void *buf, size_t size)
{
	u32 chksum = 0;
	u8 *bp = buf;
	size_t i;

	for (i = 0; i < size; i++)
		chksum += bp[i];

	return ~chksum;
}

static __maybe_unused void dump_fcb(void *buf)
{
	struct fcb_block *fcb = buf;

	pr_debug("Checksum:                   0x%08x\n", fcb->Checksum);
	pr_debug("FingerPrint:                0x%08x\n", fcb->FingerPrint);
	pr_debug("Version:                    0x%08x\n", fcb->Version);
	pr_debug("DataSetup:                  0x%02x\n", fcb->DataSetup);
	pr_debug("DataHold:                   0x%02x\n", fcb->DataHold);
	pr_debug("AddressSetup:               0x%02x\n", fcb->AddressSetup);
	pr_debug("DSAMPLE_TIME:               0x%02x\n", fcb->DSAMPLE_TIME);
	pr_debug("NandTimingState:            0x%02x\n", fcb->NandTimingState);
	pr_debug("REA:                        0x%02x\n", fcb->REA);
	pr_debug("RLOH:                       0x%02x\n", fcb->RLOH);
	pr_debug("RHOH:                       0x%02x\n", fcb->RHOH);
	pr_debug("PageDataSize:               0x%08x\n", fcb->PageDataSize);
	pr_debug("TotalPageSize:              0x%08x\n", fcb->TotalPageSize);
	pr_debug("SectorsPerBlock:            0x%08x\n", fcb->SectorsPerBlock);
	pr_debug("NumberOfNANDs:              0x%08x\n", fcb->NumberOfNANDs);
	pr_debug("TotalInternalDie:           0x%08x\n", fcb->TotalInternalDie);
	pr_debug("CellType:                   0x%08x\n", fcb->CellType);
	pr_debug("EccBlockNEccType:           0x%08x\n", fcb->EccBlockNEccType);
	pr_debug("EccBlock0Size:              0x%08x\n", fcb->EccBlock0Size);
	pr_debug("EccBlockNSize:              0x%08x\n", fcb->EccBlockNSize);
	pr_debug("EccBlock0EccType:           0x%08x\n", fcb->EccBlock0EccType);
	pr_debug("MetadataBytes:              0x%08x\n", fcb->MetadataBytes);
	pr_debug("NumEccBlocksPerPage:        0x%08x\n", fcb->NumEccBlocksPerPage);
	pr_debug("EccBlockNEccLevelSDK:       0x%08x\n", fcb->EccBlockNEccLevelSDK);
	pr_debug("EccBlock0SizeSDK:           0x%08x\n", fcb->EccBlock0SizeSDK);
	pr_debug("EccBlockNSizeSDK:           0x%08x\n", fcb->EccBlockNSizeSDK);
	pr_debug("EccBlock0EccLevelSDK:       0x%08x\n", fcb->EccBlock0EccLevelSDK);
	pr_debug("NumEccBlocksPerPageSDK:     0x%08x\n", fcb->NumEccBlocksPerPageSDK);
	pr_debug("MetadataBytesSDK:           0x%08x\n", fcb->MetadataBytesSDK);
	pr_debug("EraseThreshold:             0x%08x\n", fcb->EraseThreshold);
	pr_debug("BootPatch:                  0x%08x\n", fcb->BootPatch);
	pr_debug("PatchSectors:               0x%08x\n", fcb->PatchSectors);
	pr_debug("Firmware1_startingPage:     0x%08x\n", fcb->Firmware1_startingPage);
	pr_debug("Firmware2_startingPage:     0x%08x\n", fcb->Firmware2_startingPage);
	pr_debug("PagesInFirmware1:           0x%08x\n", fcb->PagesInFirmware1);
	pr_debug("PagesInFirmware2:           0x%08x\n", fcb->PagesInFirmware2);
	pr_debug("DBBTSearchAreaStartAddress: 0x%08x\n", fcb->DBBTSearchAreaStartAddress);
	pr_debug("BadBlockMarkerByte:         0x%08x\n", fcb->BadBlockMarkerByte);
	pr_debug("BadBlockMarkerStartBit:     0x%08x\n", fcb->BadBlockMarkerStartBit);
	pr_debug("BBMarkerPhysicalOffset:     0x%08x\n", fcb->BBMarkerPhysicalOffset);
	pr_debug("BCHType:                    0x%08x\n", fcb->BCHType);
	pr_debug("TMTiming2_ReadLatency:      0x%08x\n", fcb->TMTiming2_ReadLatency);
	pr_debug("TMTiming2_PreambleDelay:    0x%08x\n", fcb->TMTiming2_PreambleDelay);
	pr_debug("TMTiming2_CEDelay:          0x%08x\n", fcb->TMTiming2_CEDelay);
	pr_debug("TMTiming2_PostambleDelay:   0x%08x\n", fcb->TMTiming2_PostambleDelay);
	pr_debug("TMTiming2_CmdAddPause:      0x%08x\n", fcb->TMTiming2_CmdAddPause);
	pr_debug("TMTiming2_DataPause:        0x%08x\n", fcb->TMTiming2_DataPause);
	pr_debug("TMSpeed:                    0x%08x\n", fcb->TMSpeed);
	pr_debug("TMTiming1_BusyTimeout:      0x%08x\n", fcb->TMTiming1_BusyTimeout);
	pr_debug("DISBBM:                     0x%08x\n", fcb->DISBBM);
	pr_debug("BBMarkerPhysOfsInSpareData: 0x%08x\n", fcb->BBMarkerPhysicalOffsetInSpareData);
}

static __maybe_unused ssize_t raw_read_page(struct mtd_info *mtd, void *dst, loff_t offset)
{
	struct mtd_oob_ops ops;
	ssize_t ret;

	ops.mode = MTD_OPS_RAW;
	ops.ooboffs = 0;
	ops.datbuf = dst;
	ops.len = mtd->writesize;
	ops.oobbuf = dst + mtd->writesize;
	ops.ooblen = mtd->oobsize;
	ret = mtd_read_oob(mtd, offset, &ops);

        return ret;
}

static ssize_t raw_write_page(struct mtd_info *mtd, void *buf, loff_t offset)
{
	struct mtd_oob_ops ops;
	ssize_t ret;

	ops.mode = MTD_OPS_RAW;
	ops.ooboffs = 0;
	ops.datbuf = buf;
	ops.len = mtd->writesize;
	ops.oobbuf = buf + mtd->writesize;
	ops.ooblen = mtd->oobsize;
	ret = mtd_write_oob(mtd, offset, &ops);

        return ret;
}

static int fcb_create(struct imx_nand_fcb_bbu_handler *imx_handler,
		struct fcb_block *fcb, struct mtd_info *mtd)
{
	fcb->FingerPrint = 0x20424346;
	fcb->Version = 0x01000000;
	fcb->PageDataSize = mtd->writesize;
	fcb->TotalPageSize = mtd->writesize + mtd->oobsize;
	fcb->SectorsPerBlock = mtd->erasesize / mtd->writesize;

	/* Divide ECC strength by two and save the value into FCB structure. */
	fcb->EccBlock0EccType =
		mxs_nand_get_ecc_strength(mtd->writesize, mtd->oobsize) >> 1;
	fcb->EccBlockNEccType = fcb->EccBlock0EccType;

	fcb->EccBlock0Size = 0x00000200;
	fcb->EccBlockNSize = 0x00000200;

	fcb->NumEccBlocksPerPage = mtd->writesize / fcb->EccBlock0Size - 1;

	/* DBBT search area starts at second page on first block */
	fcb->DBBTSearchAreaStartAddress = 1;

	fcb->BadBlockMarkerByte = mxs_nand_mark_byte_offset(mtd);
	fcb->BadBlockMarkerStartBit = mxs_nand_mark_bit_offset(mtd);

	fcb->BBMarkerPhysicalOffset = mtd->writesize;

	imx_handler->fcb_create(imx_handler, fcb, mtd);

	fcb->Checksum = calc_chksum((void *)fcb + 4, sizeof(*fcb) - 4);

	return 0;
}

static int imx_bbu_erase(struct mtd_info *mtd)
{
	uint64_t offset = 0;
	struct erase_info erase;
	int ret;

	while (offset < mtd->size) {
		pr_debug("erasing at 0x%08llx\n", offset);
		if (mtd_block_isbad(mtd, offset)) {
			pr_debug("erase skip block @ 0x%08llx\n", offset);
			offset += mtd->erasesize;
			continue;
		}

		memset(&erase, 0, sizeof(erase));
		erase.addr = offset;
		erase.len = mtd->erasesize;

		ret = mtd_erase(mtd, &erase);
		if (ret)
			return ret;

		offset += mtd->erasesize;
	}

	return 0;
}

static int imx_bbu_write_firmware(struct mtd_info *mtd, unsigned block,
		unsigned num_blocks, void *buf, size_t len)
{
	uint64_t offset = block * mtd->erasesize;
	int ret;
	size_t written;

	while (len > 0) {
		int now = min(len, mtd->erasesize);

		if (!num_blocks)
			return -ENOSPC;

		pr_debug("writing %p at 0x%08llx, left 0x%08x\n",
				buf, offset, len);

		if (mtd_block_isbad(mtd, offset)) {
			pr_debug("write skip block @ 0x%08llx\n", offset);
			offset += mtd->erasesize;
			block++;
			continue;
		}

		ret = mtd_write(mtd, offset, now, &written, buf);
		if (ret)
			return ret;

		offset += now;
		len -= now;
		buf += now;
		block++;
		num_blocks--;
	}

	return block;
}

static int dbbt_data_create(struct mtd_info *mtd, void *buf, int block_last)
{
	int n;
	int n_bad_blocks = 0;
	uint32_t *bb = buf + 0x8;
	uint32_t *n_bad_blocksp = buf + 0x4;

	for (n = 0; n <= block_last; n++) {
		loff_t offset = n * mtd->erasesize;
		if (mtd_block_isbad(mtd, offset)) {
			n_bad_blocks++;
			*bb = n;
			bb++;
		}
	}

	*n_bad_blocksp = n_bad_blocks;

	return n_bad_blocks;
}

static int imx_bbu_nand_update(struct bbu_handler *handler, struct bbu_data *data)
{
	struct imx_nand_fcb_bbu_handler *imx_handler =
		container_of(handler, struct imx_nand_fcb_bbu_handler, handler);
	struct cdev *bcb_cdev;
	struct mtd_info *mtd;
	int ret, block_fw1, block_fw2;
	struct fcb_block *fcb;
	struct dbbt_block *dbbt;
	void *fcb_raw_page, *dbbt_page, *dbbt_data_page;
	void *ecc;
	int written;
	void *fw;
	unsigned fw_size, partition_size;
	int i;
	enum filetype filetype;
	unsigned num_blocks_fcb_dbbt, num_blocks, num_blocks_fw;

	filetype = file_detect_type(data->image, data->len);

	if (filetype != imx_handler->filetype &&
			!bbu_force(data, "Image is not of type %s but of type %s",
				file_type_to_string(imx_handler->filetype),
				file_type_to_string(filetype)))
		return -EINVAL;

	bcb_cdev = cdev_by_name(handler->devicefile);
	if (!bcb_cdev) {
		pr_err("%s: No FCB device!\n", __func__);
		return -ENODEV;
	}

	mtd = bcb_cdev->mtd;
	partition_size = mtd->size;

	fcb_raw_page = xzalloc(mtd->writesize + mtd->oobsize);

	fcb = fcb_raw_page + 12;
	ecc = fcb_raw_page + 512 + 12;

	dbbt_page = xzalloc(mtd->writesize);
	dbbt_data_page = xzalloc(mtd->writesize);
	dbbt = dbbt_page;

	/*
	 * We have to write one additional page to make the ROM happy.
	 * Maybe the PagesInFirmwarex fields are really the number of pages - 1.
	 * kobs-ng has the same.
	 */
	fw_size = ALIGN(data->len + mtd->writesize, mtd->writesize);
	fw = xzalloc(fw_size);
	memcpy(fw, data->image, data->len);

	num_blocks_fcb_dbbt = 4;
	num_blocks = partition_size / mtd->erasesize;
	num_blocks_fw = (num_blocks - num_blocks_fcb_dbbt) / 2;

	block_fw1 = num_blocks_fcb_dbbt;
	block_fw2 = num_blocks_fcb_dbbt + num_blocks_fw;

	pr_info("writing first firmware to block %d (ofs 0x%08x)\n",
			block_fw1, block_fw1 * mtd->erasesize);
	pr_info("writing second firmware to block %d (ofs 0x%08x)\n",
			block_fw2, block_fw2 * mtd->erasesize);
	pr_info("maximum size per firmware: 0x%08x bytes\n",
			num_blocks_fw * mtd->erasesize);

	if (num_blocks_fw * mtd->erasesize < fw_size)
		return -ENOSPC;

	ret = bbu_confirm(data);
	if (ret)
		goto out;

	ret = imx_bbu_erase(mtd);
	if (ret)
		goto out;

	ret = imx_bbu_write_firmware(mtd, block_fw1, num_blocks_fw, fw, fw_size);
	if (ret < 0)
		goto out;

	ret = imx_bbu_write_firmware(mtd, block_fw2, num_blocks_fw, fw, fw_size);
	if (ret < 0)
		goto out;

	fcb->Firmware1_startingPage = block_fw1 * mtd->erasesize / mtd->writesize;
	fcb->Firmware2_startingPage = block_fw2 * mtd->erasesize / mtd->writesize;
	fcb->PagesInFirmware1 = ALIGN(data->len, mtd->writesize) / mtd->writesize;
	fcb->PagesInFirmware2 = fcb->PagesInFirmware1;

	fcb_create(imx_handler, fcb, mtd);
	encode_hamming_13_8(fcb, ecc, 512);

	/*
	 * Set the first and second byte of OOB data to 0xFF, not 0x00. These
	 * bytes are used as the Manufacturers Bad Block Marker (MBBM). Since
	 * the FCB is mostly written to the first page in a block, a scan for
	 * factory bad blocks will detect these blocks as bad, e.g. when
	 * function nand_scan_bbt() is executed to build a new bad block table.
	 */
	memset(fcb_raw_page + mtd->writesize, 0xFF, 2);

	ret = raw_write_page(mtd, fcb_raw_page, mtd->erasesize);
	if (ret)
		goto out;

	dbbt->Checksum = 0;
	dbbt->FingerPrint = 0x54424244;
	dbbt->Version = 0x01000000;

	ret = dbbt_data_create(mtd, dbbt_data_page, block_fw2 + num_blocks_fw);
	if (ret < 0)
		goto out;

	if (ret > 0) {
		dbbt->DBBTNumOfPages = 1;
		if (imx_handler->dbbt_create)
			imx_handler->dbbt_create(imx_handler, dbbt, ret);
	}

	for (i = 0; i < 4; i++) {
		ret = raw_write_page(mtd, fcb_raw_page, mtd->erasesize * i);
		if (ret)
			goto out;

		ret = mtd_write(mtd, mtd->erasesize * i + mtd->writesize,
				mtd->writesize, &written, dbbt_page);
		if (ret)
			goto out;

		if (dbbt->DBBTNumOfPages > 0) {
			ret = mtd_write(mtd, mtd->erasesize * i + mtd->writesize * 5,
					mtd->writesize, &written, dbbt_data_page);
			if (ret)
				goto out;
		}
	}

out:
	free(dbbt_page);
	free(dbbt_data_page);
	free(fcb_raw_page);
	free(fw);

	return ret;
}

static void imx6_fcb_create(struct imx_nand_fcb_bbu_handler *imx_handler,
		struct fcb_block *fcb, struct mtd_info *mtd)
{
	/* Also hardcoded in kobs-ng */
	fcb->DataSetup = 80;
	fcb->DataHold = 60;
	fcb->AddressSetup = 25;
	fcb->DSAMPLE_TIME = 6;
	fcb->MetadataBytes = 10;
}

int imx6_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	struct imx_nand_fcb_bbu_handler *imx_handler;
	struct bbu_handler *handler;
	int ret;

	imx_handler = xzalloc(sizeof(*imx_handler));
	imx_handler->fcb_create = imx6_fcb_create;
	imx_handler->filetype = filetype_arm_barebox;

	handler = &imx_handler->handler;
	handler->devicefile = "nand0.barebox";
	handler->name = name;
	handler->flags = flags;
	handler->handler = imx_bbu_nand_update;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}

#ifdef CONFIG_ARCH_IMX28
#include <mach/imx28-regs.h>

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

static void imx28_fcb_create(struct imx_nand_fcb_bbu_handler *imx_handler,
		struct fcb_block *fcb, struct mtd_info *mtd)
{
	u32 fl0, fl1, t0;
	void __iomem *bch_regs = (void *)MXS_BCH_BASE;
	void __iomem *gpmi_regs = (void *)MXS_GPMI_BASE;

	fl0 = readl(bch_regs + BCH_FLASH0LAYOUT0);
	fl1 = readl(bch_regs + BCH_FLASH0LAYOUT1);
	t0 = readl(gpmi_regs + GPMI_TIMING0);

	fcb->MetadataBytes = BF_VAL(fl0, BCH_FLASHLAYOUT0_META_SIZE);
	fcb->DataSetup = BF_VAL(t0, GPMI_TIMING0_DATA_SETUP);
	fcb->DataHold = BF_VAL(t0, GPMI_TIMING0_DATA_HOLD);
	fcb->AddressSetup = BF_VAL(t0, GPMI_TIMING0_ADDRESS_SETUP);
	fcb->MetadataBytes = BF_VAL(fl0, BCH_FLASHLAYOUT0_META_SIZE);
	fcb->NumEccBlocksPerPage = BF_VAL(fl0, BCH_FLASHLAYOUT0_NBLOCKS);
	fcb->EraseThreshold = readl(bch_regs + BCH_MODE);
}

static void imx28_dbbt_create(struct imx_nand_fcb_bbu_handler *imx_handler,
		struct dbbt_block *dbbt, int num_bad_blocks)
{
	uint32_t a = 0;
	uint8_t *p = (void *)dbbt;
	int i;

	dbbt->numberBB = num_bad_blocks;

	for (i = 4; i < 512; i++)
		a += p[i];

	a ^= 0xffffffff;

	dbbt->Checksum = a;
}

int imx28_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	struct imx_nand_fcb_bbu_handler *imx_handler;
	struct bbu_handler *handler;
	int ret;

	imx_handler = xzalloc(sizeof(*imx_handler));
	imx_handler->fcb_create = imx28_fcb_create;
	imx_handler->dbbt_create = imx28_dbbt_create;

	imx_handler->filetype = filetype_mxs_bootstream;

	handler = &imx_handler->handler;
	handler->devicefile = "nand0.barebox";
	handler->name = name;
	handler->flags = flags;
	handler->handler = imx_bbu_nand_update;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}
#endif
