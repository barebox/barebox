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
#include <linux/mtd/nand.h>
#include <linux/stat.h>
#include <linux/bch.h>
#include <linux/bitops.h>
#include <io.h>
#include <crc.h>
#include <mach/generic.h>
#include <mtd/mtd-peb.h>

#ifdef CONFIG_ARCH_IMX6
#include <mach/imx6.h>
static inline int fcb_is_bch_encoded(void)
{
       return cpu_is_mx6ul() || cpu_is_mx6ull();
}
#else
static inline int fcb_is_bch_encoded(void)
{
       return 0;
}
#endif

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

static uint8_t reverse_bit(uint8_t b)
{
	b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
	b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
	b = (b & 0xaa) >> 1 | (b & 0x55) << 1;

	return b;
}

static void encode_bch_ecc(void *buf, struct fcb_block *fcb, int eccbits)
{
	int i, j, m = 13;
	int blocksize = 128;
	int numblocks = 8;
	int ecc_buf_size = (m * eccbits + 7) / 8;
	struct bch_control *bch = init_bch(m, eccbits, 0);
	uint8_t *ecc_buf = xmalloc(ecc_buf_size);
	uint8_t *tmp_buf = xzalloc(blocksize * numblocks);
	uint8_t *psrc, *pdst;

	/*
	 * The blocks here are bit aligned. If eccbits is a multiple of 8,
	 * we just can copy bytes. Otherwiese we must move the blocks to
	 * the next free bit position.
	 */
	BUG_ON(eccbits % 8);

	memcpy(tmp_buf, fcb, sizeof(*fcb));

	for (i = 0; i < numblocks; i++) {
		memset(ecc_buf, 0, ecc_buf_size);
		psrc = tmp_buf + i * blocksize;
		pdst = buf + i * (blocksize + ecc_buf_size);

		/* copy data byte aligned to destination buf */
		memcpy(pdst, psrc, blocksize);

		/*
		 * imx-kobs use a modified encode_bch which reverse the
		 * bit order of the data before calculating bch.
		 * Do this in the buffer and use the bch lib here.
		 */
		for (j = 0; j < blocksize; j++)
			psrc[j] = reverse_bit(psrc[j]);

		encode_bch(bch, psrc, blocksize, ecc_buf);

		/* reverse ecc bit */
		for (j = 0; j < ecc_buf_size; j++)
			ecc_buf[j] = reverse_bit(ecc_buf[j]);

		/* Here eccbuf is byte aligned and we can just copy it */
		memcpy(pdst + blocksize, ecc_buf, ecc_buf_size);
	}

	free(ecc_buf);
	free(tmp_buf);
	free_bch(bch);
}

static struct fcb_block *read_fcb_bch(void *rawpage, int eccbits)
{
	int i, j, ret, errbit, m = 13;
	int blocksize = 128;
	int numblocks = 8;
	int ecc_buf_size = (m * eccbits + 7) / 8;
	struct bch_control *bch = init_bch(m, eccbits, 0);
	uint8_t *fcb = xmalloc(numblocks * blocksize);
	uint8_t *ecc_buf = xmalloc(ecc_buf_size);
	uint8_t *data_buf = xmalloc(blocksize);
	unsigned int *errloc = xmalloc(eccbits * sizeof(*errloc));
	uint8_t *psrc, *pdst;

	/* see encode_bch_ecc */
	BUG_ON(eccbits % 8);

	for (i = 0; i < numblocks; i++) {
		psrc = rawpage + 32 + i * (blocksize + ecc_buf_size);
		pdst = fcb + i * blocksize;

		/* reverse data bit */
		for (j = 0; j < blocksize; j++)
			data_buf[j] = reverse_bit(psrc[j]);

		/* reverse ecc bit */
		for (j = 0; j < ecc_buf_size; j++)
			ecc_buf[j] = reverse_bit(psrc[j + blocksize]);

		ret = decode_bch(bch, data_buf, blocksize, ecc_buf,
				 NULL, NULL, errloc);

		if (ret < 0) {
			pr_err("Uncorrectable error at block %d\n", i);
			free(fcb);
			fcb = ERR_PTR(ret);
			goto out;
		}
		if (ret > 0)
			pr_info("Found %d correctable errors in block %d\n",
				ret, i);

		for (j = 0; j < ret; j++) {
			/*
			 * calculate the reverse position
			 * pos - (pos % 8) -> byte offset
			 * 7 - (pos % 8) -> reverse bit position
			 */
			errbit = errloc[j] - 2 * (errloc[j] % 8) + 7;
			pr_debug("Found error: bit %d in block %d\n",
				 errbit, i);
			if (errbit < blocksize * 8)
				change_bit(errbit, psrc);
			/* else error in ecc, ignore it */
		}
		memcpy(pdst, psrc, blocksize);
	}

out:
	free(data_buf);
	free(ecc_buf);
	free(errloc);
	free_bch(bch);

	return (struct fcb_block *)fcb;
}

static void encode_hamming_13_8(void *_src, void *_ecc, size_t size)
{
	int i;
	uint8_t *src = _src;
	uint8_t *ecc = _ecc;

	for (i = 0; i < size; i++)
		ecc[i] = calculate_parity_13_8(src[i]);
}

static int lookup_single_error_13_8(unsigned char syndrome)
{
	int i;
	unsigned char syndrome_table[] = {
		0x1c, 0x16, 0x13, 0x19,
		0x1a, 0x07, 0x15, 0x0e,
		0x01, 0x02, 0x04, 0x08,
		0x10,
	};

	for (i = 0; i < 13; i ++)
		if (syndrome_table[i] == syndrome)
			return i;
	return -1;
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

static struct fcb_block *read_fcb_hamming_13_8(void *rawpage)
{
	int i;
	int bitflips = 0, bit_to_flip;
	u8 parity, np, syndrome;
	u8 *fcb, *ecc;

	fcb = rawpage + 12;
	ecc = rawpage + 512 + 12;

	/*
	 * The ROM does the check for the correct fingerprint and version before
	 * correcting bitflips. This means we cannot allow bitflips in the
	 * fingerprint and version. We bail out with an error if it's not correct.
	 * This is currently done in the i.MX6qdl path. It needs to be checked if
	 * the same happens in the BCH encoded variants (i.MX6ul(l)) aswell.
	 */
	if (((struct fcb_block *)fcb)->FingerPrint != 0x20424346 ||
	    ((struct fcb_block *)fcb)->Version != 0x01000000)
		return ERR_PTR(-EINVAL);

	for (i = 0; i < 512; i++) {
		parity = ecc[i];
		np = calculate_parity_13_8(fcb[i]);

		syndrome = np ^ parity;
		if (syndrome == 0)
			continue;

		if (!(hweight8(syndrome) & 1)) {
			pr_err("Uncorrectable error at offset %d\n", i);
			return ERR_PTR(-EIO);
		}

		bit_to_flip = lookup_single_error_13_8(syndrome);
		if (bit_to_flip < 0) {
			pr_err("Uncorrectable error at offset %d\n", i);
			return ERR_PTR(-EIO);
		}

		bitflips++;

		if (bit_to_flip > 7)
			ecc[i] ^= 1 << (bit_to_flip - 8);
		else
			fcb[i] ^= 1 << bit_to_flip;
	}

	return xmemdup(rawpage + 12, 512);
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

static int read_fcb(struct mtd_info *mtd, int num, struct fcb_block **retfcb)
{
	int ret;
	struct fcb_block *fcb;
	void *rawpage;

	*retfcb = NULL;

	rawpage = xmalloc(mtd->writesize + mtd->oobsize);

	ret = raw_read_page(mtd, rawpage, mtd->erasesize * num);
	if (ret) {
		pr_err("Cannot read block %d\n", num);
		goto err;
	}

	if (fcb_is_bch_encoded())
		fcb = read_fcb_bch(rawpage, 40);
	else
		fcb = read_fcb_hamming_13_8(rawpage);

	if (IS_ERR(fcb)) {
		pr_err("Cannot read fcb on block %d\n", num);
		ret = PTR_ERR(fcb);
		goto err;
	}

	*retfcb = fcb;
	ret = 0;
err:
	free(rawpage);

	return ret;
}

static int fcb_create(struct imx_nand_fcb_bbu_handler *imx_handler,
		struct fcb_block *fcb, struct mtd_info *mtd)
{
	int ecc_strength;
	int bb_mark_bit_offset;
	int ret;

	fcb->FingerPrint = 0x20424346;
	fcb->Version = 0x01000000;
	fcb->PageDataSize = mtd->writesize;
	fcb->TotalPageSize = mtd->writesize + mtd->oobsize;
	fcb->SectorsPerBlock = mtd->erasesize / mtd->writesize;

	ret = mxs_nand_get_geo(&ecc_strength, &bb_mark_bit_offset);
	if (ret)
		return ret;

	/* Divide ECC strength by two and save the value into FCB structure. */
	fcb->EccBlock0EccType = ecc_strength >> 1;
	fcb->EccBlockNEccType = fcb->EccBlock0EccType;

	fcb->EccBlock0Size = 0x00000200;
	fcb->EccBlockNSize = 0x00000200;

	fcb->NumEccBlocksPerPage = mtd->writesize / fcb->EccBlock0Size - 1;

	/* DBBT search area starts at second page on first block */
	fcb->DBBTSearchAreaStartAddress = 1;

	fcb->BadBlockMarkerByte = bb_mark_bit_offset >> 3;
	fcb->BadBlockMarkerStartBit = bb_mark_bit_offset & 0x7;

	fcb->BBMarkerPhysicalOffset = mtd->writesize;

	imx_handler->fcb_create(imx_handler, fcb, mtd);

	fcb->Checksum = calc_chksum((void *)fcb + 4, sizeof(*fcb) - 4);

	return 0;
}

static int mtd_peb_write_block(struct mtd_info *mtd, void *buf, int block, int len)
{
	int ret;
	int retries = 0;

	if (mtd_peb_is_bad(mtd, block))
		return -EINVAL;
again:
	ret = mtd_peb_write(mtd, buf, block, 0, len);
	if (!ret)
		return 0;

	if (ret == -EBADMSG) {
		ret = mtd_peb_torture(mtd, block);
		if (ret == -EIO)
			mtd_peb_mark_bad(mtd, block);

		if (!ret && retries++ < 3)
			goto again;
	}

	return ret;
}

/**
 * imx_bbu_firmware_max_blocks - get max number of blocks for firmware
 * @mtd: The mtd device
 *
 * We use 4 blocks for FCB/DBBT, the rest of the partition is
 * divided into two equally sized firmware slots. This function
 * returns the number of blocks available for one firmware slot.
 * The actually usable size may be smaller due to bad blocks.
 */
static int imx_bbu_firmware_max_blocks(struct mtd_info *mtd)
{
	return (mtd_div_by_eb(mtd->size, mtd) - 4) / 2;
}

/**
 * imx_bbu_firmware_start_block - get start block for a firmware slot
 * @mtd: The mtd device
 * @num: The slot number (0 or 1)
 *
 * We use 4 blocks for FCB/DBBT, the rest of the partition is
 * divided into two equally sized firmware slots. This function
 * returns the start block for the given firmware slot.
 */
static int imx_bbu_firmware_start_block(struct mtd_info *mtd, int num)
{
	return 4 + num * imx_bbu_firmware_max_blocks(mtd);
}

/**
 * imx_bbu_firmware_fcb_start_page - get start page for a firmware slot
 * @mtd: The mtd device
 * @num: The slot number (0 or 1)
 *
 * This returns the start page for a firmware slot, to be written into the
 * Firmwaren_startingPage field in the FCB.
 */
static int imx_bbu_firmware_fcb_start_page(struct mtd_info *mtd, int num)
{
	int block, blocksleft;
	int pages_per_block = mtd->erasesize / mtd->writesize;

	block = imx_bbu_firmware_start_block(mtd, num);

	blocksleft = imx_bbu_firmware_max_blocks(mtd);

	/*
	 * The ROM only checks for a bad block when advancing the read position,
	 * but not if the initial block is good, hence we cannot directly point
	 * to the first firmware block, but must instead point to the first *good*
	 * firmware block.
	 */
	while (mtd_peb_is_bad(mtd, block)) {
		block++;
		blocksleft--;
		if (!blocksleft)
			break;
	}

	return block * pages_per_block;
}


static int imx_bbu_write_firmware(struct mtd_info *mtd, unsigned num, void *buf,
				  size_t len)
{
	int ret, i, newbadblock = 0;
	int num_blocks = imx_bbu_firmware_max_blocks(mtd);
	int block = imx_bbu_firmware_start_block(mtd, num);
	int page = block * mtd->erasesize / mtd->writesize;

	pr_info("writing firmware to slot %d on pages %d-%d\n",
			num, page, page + len / mtd->writesize);

	for (i = 0; i < num_blocks; i++) {
		if (mtd_peb_is_bad(mtd, block + i))
			continue;

		ret = mtd_peb_erase(mtd, block + i);
		if (ret == -EIO) {
			newbadblock = 1;

			ret = mtd_peb_mark_bad(mtd, block + i);
			if (ret)
				return ret;
		} else if (ret) {
			return ret;
		}
	}

	while (len > 0) {
		int now = min(len, mtd->erasesize);

		if (!num_blocks) {
			pr_err("Out of good eraseblocks, cannot write firmware\n");
			return -ENOSPC;
		}

		pr_debug("writing %p peb %d, left 0x%08x\n",
				buf, block, len);

		if (mtd_peb_is_bad(mtd, block)) {
			pr_debug("skipping block %d\n", block);
			num_blocks--;
			block++;
			continue;
		}

		ret = mtd_peb_write_block(mtd, buf, block, now);

		if (ret == -EIO) {
			block++;
			num_blocks--;
			newbadblock = 1;
			continue;
		}

		if (ret) {
			pr_err("Writing block %d failed with: %s\n", block, strerror(-ret));
			return ret;
		}

		len -= now;
		buf += now;
		block++;
		num_blocks--;
	}

	return newbadblock;
}

static void *dbbt_data_create(struct mtd_info *mtd)
{
	int n;
	int n_bad_blocks = 0;
	void *dbbt = xzalloc(mtd->writesize);
	uint32_t *bb = dbbt + 0x8;
	uint32_t *n_bad_blocksp = dbbt + 0x4;
	int num_blocks = mtd_div_by_eb(mtd->size, mtd);

	for (n = 0; n < num_blocks; n++) {
		loff_t offset = n * mtd->erasesize;
		if (mtd_block_isbad(mtd, offset)) {
			n_bad_blocks++;
			*bb = n;
			bb++;
		}
	}

	if (!n_bad_blocks) {
		free(dbbt);
		return NULL;
	}

	*n_bad_blocksp = n_bad_blocks;

	return dbbt;
}

static void imx28_dbbt_create(struct dbbt_block *dbbt, int num_bad_blocks)
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

/**
 * imx_bbu_write_fcb - Write FCB and DBBT raw data to the device
 * @mtd: The mtd Nand device
 * @block: The block to write to
 * @fcb_raw_page: The raw FCB data
 * @dbbt_data_page: The DBBT data
 *
 * This function writes the FCB/DBBT data to the block given in @block
 * to the Nand device. The FCB data has to be given in the raw flash
 * layout, already with ecc data supplied.
 *
 * return: 0 on success or a negative error code otherwise.
 */
static int imx_bbu_write_fcb(struct mtd_info *mtd, int block, void *fcb_raw_page,
			     void *dbbt_data_page)
{
	struct dbbt_block *dbbt;
	int ret;
	int retries = 0;
	uint32_t *n_bad_blocksp = dbbt_data_page + 0x4;
again:
	dbbt = xzalloc(mtd->writesize);

	dbbt->Checksum = 0;
	dbbt->FingerPrint = 0x54424244;
	dbbt->Version = 0x01000000;
	if (dbbt_data_page)
		dbbt->DBBTNumOfPages = 1;
	if (cpu_is_mx28())
		imx28_dbbt_create(dbbt, *n_bad_blocksp);

	ret = mtd_peb_erase(mtd, block);
	if (ret)
		return ret;

	ret = raw_write_page(mtd, fcb_raw_page, block * mtd->erasesize);
	if (ret) {
		pr_err("Writing FCB on block %d failed with %s\n",
		       block, strerror(-ret));
		goto out;
	}

	ret = mtd_peb_write(mtd, (void *)dbbt, block, mtd->writesize,
			    mtd->writesize);
	if (ret < 0) {
		pr_err("Writing DBBT header on block %d failed with %s\n",
		       block, strerror(-ret));
		goto out;
	}

	if (dbbt_data_page) {
		ret = mtd_peb_write(mtd, dbbt_data_page, block, mtd->writesize * 5,
				    mtd->writesize);
		if (ret < 0) {
			pr_err("Writing DBBT on block %d failed with %s\n",
			       block, strerror(-ret));
			goto out;
		}
	}

	ret = 0;
out:
	free(dbbt);

	if (ret == -EBADMSG) {
		ret = mtd_peb_torture(mtd, block);
		if (ret == -EIO)
			mtd_peb_mark_bad(mtd, block);

		if (!ret && retries++ < 3)
			goto again;
	}

	return ret;
}

/**
 * dbbt_block_is_bad - Check if according to the given DBBT a block is bad
 * @dbbt: The DBBT data page
 * @block: The block to test
 *
 * This function checks if a block is marked as bad in the given DBBT.
 *
 * return: true if the block is bad, false otherwise.
 */
static int dbbt_block_is_bad(void *_dbbt, int block)
{
	int i;
	u32 *dbbt = _dbbt;
	int num_bad_blocks;

	if (!_dbbt)
		return false;

	dbbt++; /* reserved */

	num_bad_blocks = *dbbt++;

	for (i = 0; i < num_bad_blocks; i++) {
		if (*dbbt == block)
			return true;
		dbbt++;
	}

	return false;
}

/**
 * dbbt_check - Check if DBBT is readable and consistent to the mtd BBT
 * @mtd: The mtd Nand device
 * @dbbt: The page where the DBBT is found
 *
 * This function checks if the DBBT is readable and consistent to the mtd
 * layers idea of bad blocks.
 *
 * return: 0 if the DBBT is readable and consistent to the mtd BBT, a
 * negative error code otherwise.
 */
static int dbbt_check(struct mtd_info *mtd, int page)
{
	int ret, needs_cleanup = 0;
	size_t r;
	void *dbbt_header;
	void *dbbt_entries = NULL;
	struct dbbt_block *dbbt;
	int num_blocks = mtd_div_by_eb(mtd->size, mtd);
	int n;

	dbbt_header = xmalloc(mtd->writesize);

	ret = mtd_read(mtd, page * mtd->writesize, mtd->writesize, &r, dbbt_header);
	if (ret == -EUCLEAN) {
		pr_warn("page %d needs cleaning\n", page);
		needs_cleanup = 1;
	} else if (ret < 0) {
		pr_err("Cannot read page %d: %s\n", page, strerror(-ret));
		goto out;
	}

	dbbt = dbbt_header;

	if (dbbt->FingerPrint != 0x54424244) {
		pr_err("dbbt at page %d is readable but does not contain a valid DBBT\n",
		       page);
		ret = -EINVAL;
		goto out;
	}

	if (dbbt->DBBTNumOfPages) {
		dbbt_entries = xmalloc(mtd->writesize);

		ret = mtd_read(mtd, (page + 4) * mtd->writesize, mtd->writesize, &r, dbbt_entries);
		if (ret == -EUCLEAN) {
			pr_warn("page %d needs cleaning\n", page);
			needs_cleanup = 1;
		} else if (ret < 0) {
			pr_err("Cannot read page %d: %s\n", page, strerror(-ret));
			goto out;
		}
	} else {
		dbbt_entries = NULL;
	}

	for (n = 0; n < num_blocks; n++) {
		if (mtd_peb_is_bad(mtd, n) != dbbt_block_is_bad(dbbt_entries, n)) {
			ret = -EINVAL;
			goto out;
		}
	}

	ret = 0;
out:
	free(dbbt_header);
	free(dbbt_entries);

	if (ret < 0)
		return ret;
	if (needs_cleanup)
		return -EUCLEAN;
	return 0;
}

/**
 * fcb_dbbt_check - Check if a FCB/DBBT is valid
 * @mtd: The mtd Nand device
 * @num: The number of the FCB, corresponds to the eraseblock number
 * @fcb: The FCB to check against
 *
 * This function checks if FCB/DBBT found on a device are valid. This
 * means:
 * - the FCB is readable on the device
 * - the FCB is the same as the reference passed in @fcb
 * - the DBBT is consistent to the mtd BBT
 *
 * return: 0 if the FCB/DBBT are valid, a negative error code otherwise
 */
static int fcb_dbbt_check(struct mtd_info *mtd, int num, struct fcb_block *fcb)
{
	int ret;
	struct fcb_block *f;
	int pages_per_block = mtd->erasesize / mtd->writesize;

	ret = read_fcb(mtd, num, &f);
	if (ret)
		return ret;

	if (memcmp(fcb, f, sizeof(*fcb))) {
		ret = -EINVAL;
		goto out;
	}

	ret = dbbt_check(mtd, num * pages_per_block + 1);
	if (ret)
		goto out;

	ret = 0;

out:
	free(f);

	return ret;
}

/**
 * imx_bbu_write_fcbs_dbbts - Write FCBs/DBBTs to first four blocks
 * @mtd: The mtd device to write the FCBs/DBBTs to
 * @fcb: The FCB block to write
 *
 * This creates the FCBs/DBBTs and writes them to the first four blocks
 * of the Nand device. The raw FCB data is created from the input FCB
 * block, the DBBTs are created from the barebox mtd Nand Bad Block
 * Table. The DBBTs are written in the second page same of each FCB block.
 * Data will actually only be written if it differs from the data found
 * on the device or if a return value of -EUCLEAN while reading
 * indicates that a refresh is necessary.
 *
 * return: 0 for success or a negative error code otherwise.
 */
static int imx_bbu_write_fcbs_dbbts(struct mtd_info *mtd, struct fcb_block *fcb)
{
	void *dbbt = NULL;
	int i, ret, valid = 0;
	void *fcb_raw_page;

	/*
	 * The DBBT search start page is configurable in the FCB block.
	 * This function writes the DBBTs in the pages directly behind
	 * the FCBs, so everything else is invalid here.
	 */
	if (fcb->DBBTSearchAreaStartAddress != 1)
		return -EINVAL;

	fcb_raw_page = xzalloc(mtd->writesize + mtd->oobsize);

	if (fcb_is_bch_encoded()) {
		/* 40 bit BCH, for i.MX6UL(L) */
		encode_bch_ecc(fcb_raw_page + 32, fcb, 40);
	} else {
		memcpy(fcb_raw_page + 12, fcb, sizeof(struct fcb_block));
		encode_hamming_13_8(fcb_raw_page + 12,
				    fcb_raw_page + 12 + 512, 512);
	}

	dbbt = dbbt_data_create(mtd);

	/*
	 * Set the first and second byte of OOB data to 0xFF, not 0x00. These
	 * bytes are used as the Manufacturers Bad Block Marker (MBBM). Since
	 * the FCB is mostly written to the first page in a block, a scan for
	 * factory bad blocks will detect these blocks as bad, e.g. when
	 * function nand_scan_bbt() is executed to build a new bad block table.
	 */
	memset(fcb_raw_page + mtd->writesize, 0xFF, 2);

	pr_info("Writing FCBs/DBBTs with primary/secondary Firmwares at pages %d/%d\n",
		fcb->Firmware1_startingPage, fcb->Firmware2_startingPage);

	for (i = 0; i < 4; i++) {
		if (mtd_peb_is_bad(mtd, i))
			continue;

		if (!fcb_dbbt_check(mtd, i, fcb)) {
			valid++;
			pr_info("FCB/DBBT on block %d still valid\n", i);
			continue;
		}

		pr_info("Writing FCB/DBBT on block %d\n", i);

		ret = imx_bbu_write_fcb(mtd, i, fcb_raw_page, dbbt);
		if (ret)
			pr_err("Writing FCB/DBBT %d failed with: %s\n", i, strerror(-ret));
		else
			valid++;
	}

	free(fcb_raw_page);
	free(dbbt);

	if (!valid)
		pr_err("No FCBs/DBBTs could be written. System won't boot from Nand\n");

	return valid > 0 ? 0 : -EIO;
}

static int block_is_empty(struct mtd_info *mtd, int block)
{
	int rawsize = mtd->writesize + mtd->oobsize;
	u8 *rawpage = xmalloc(rawsize);
	int ret;
	loff_t offset = (loff_t)block * mtd->erasesize;

	ret = raw_read_page(mtd, rawpage, offset);
	if (ret)
		goto err;

	ret = nand_check_erased_buf(rawpage, rawsize, 4 * 13);

	if (ret == -EBADMSG)
		ret = 0;
	else if (ret >= 0)
		ret = 1;

err:
	free(rawpage);
	return ret;
}

static int read_firmware(struct mtd_info *mtd, int first_page, int num_pages,
			 void **firmware)
{
	void *buf, *pos;
	int pages_per_block = mtd->erasesize / mtd->writesize;
	int now, size, block, ret, need_cleaning = 0;

	pr_debug("%s: reading %d pages from page %d\n", __func__, num_pages, first_page);

	buf = pos = malloc(num_pages * mtd->writesize);
	if (!buf)
		return -ENOMEM;

	if (first_page % pages_per_block) {
		pr_err("Firmware does not begin on eraseblock boundary\n");
		ret = -EINVAL;
		goto err;
	}

	block = first_page / pages_per_block;
	size = num_pages * mtd->writesize;

	while (size) {
		if (block >= mtd_num_pebs(mtd)) {
			ret = -EIO;
			goto err;
		}

		if (mtd_peb_is_bad(mtd, block)) {
			block++;
			continue;
		}

		now = min_t(unsigned int , size, mtd->erasesize);

		ret = mtd_peb_read(mtd, pos, block, 0, now);
		if (ret == -EUCLEAN) {
			pr_info("Block %d needs cleaning\n", block);
			need_cleaning = 1;
		} else if (ret < 0) {
			pr_err("Reading PEB %d failed with %d\n", block, ret);
			goto err;
		}

		if (mtd_buf_all_ff(pos, now)) {
			/*
			 * At this point we do not know if this is a
			 * block that contains only 0xff or if it is
			 * really empty. We test this by reading a raw
			 * page and check if it's empty
			 */
			ret = block_is_empty(mtd, block);
			if (ret < 0)
				goto err;
			if (ret) {
				ret = -EINVAL;
				goto err;
			}
		}

		pos += now;
		size -= now;
		block++;
	}

	ret = 0;

	*firmware = buf;

	pr_info("Firmware @ page %d, size %d pages has crc32: 0x%08x\n",
	       first_page, num_pages, crc32(0, buf, num_pages * mtd->writesize));

err:
	if (ret < 0) {
		free(buf);
		pr_warn("Firmware at page %d is not readable\n", first_page);
		return ret;
	}

	if (need_cleaning) {
		pr_warn("Firmware at page %d needs cleanup\n", first_page);
		return 1;
	}

	return 0;
}

static void read_firmware_all(struct mtd_info *mtd, struct fcb_block *fcb, void **data, int *len,
			     int *used_refresh, int *unused_refresh, int *used)
{
	void *primary = NULL, *secondary = NULL;
	int fw0 = imx_bbu_firmware_fcb_start_page(mtd, 0);
	int fw1 = imx_bbu_firmware_fcb_start_page(mtd, 1);
	int first, ret, primary_refresh = 0, secondary_refresh = 0;

	*used_refresh = 0;
	*unused_refresh = 0;

	if (fcb->Firmware1_startingPage == fw0 &&
	    fcb->Firmware2_startingPage == fw1) {
		first = 0;
	} else if (fcb->Firmware1_startingPage == fw1 &&
	    fcb->Firmware2_startingPage == fw0) {
		first = 1;
	} else {
		pr_warn("FCB is not what we expect. Update will not be robust\n");
		*used = 0;
		return;
	}

	if (fcb->PagesInFirmware1 != fcb->PagesInFirmware2) {
		pr_warn("FCB is not what we expect. Update will not be robust\n");
		return;
	}

	*len = fcb->PagesInFirmware1 * mtd->writesize;

	ret = read_firmware(mtd, fcb->Firmware1_startingPage, fcb->PagesInFirmware1, &primary);
	if (ret > 0)
		primary_refresh = 1;

	ret = read_firmware(mtd, fcb->Firmware2_startingPage, fcb->PagesInFirmware2, &secondary);
	if (ret > 0)
		secondary_refresh = 1;

	if (!primary && !secondary) {
		*unused_refresh = 1;
		*used_refresh = 1;
		*used = 0;
		*data = NULL;
	} else if (primary && !secondary) {
		*used_refresh = primary_refresh;
		*unused_refresh = 1;
		*used = first;
		*data = primary;
	} else if (secondary && !primary) {
		*used_refresh = secondary_refresh;
		*unused_refresh = 1;
		*used = !first;
		*data = secondary;
	} else {
		*unused_refresh = secondary_refresh;

		if (memcmp(primary, secondary, fcb->PagesInFirmware1 * mtd->writesize))
			*unused_refresh = 1;

		*used_refresh = primary_refresh;
		*used = first;
		*data = primary;
		free(secondary);
	}

	pr_info("Primary firmware is in slot %d on pages %d-%d, %svalid, %s\n",
		first,
		fcb->Firmware1_startingPage,
		fcb->Firmware1_startingPage + fcb->PagesInFirmware1, primary ? "" : "in",
		primary_refresh ? "needs cleanup" : "clean");

	pr_info("Secondary firmware is in slot %d on pages %d-%d, %svalid, %s\n",
		!first,
		fcb->Firmware2_startingPage,
		fcb->Firmware2_startingPage + fcb->PagesInFirmware2, secondary ? "" : "in",
		secondary_refresh ? "needs cleanup" : "clean");

	pr_info("ROM uses slot %d (%s firmware)\n",
		*used, primary ? "primary" : secondary ? "secondary" : "no");
}

static int imx_bbu_nand_update(struct bbu_handler *handler, struct bbu_data *data)
{
	struct imx_nand_fcb_bbu_handler *imx_handler =
		container_of(handler, struct imx_nand_fcb_bbu_handler, handler);
	struct cdev *bcb_cdev;
	struct mtd_info *mtd;
	int ret, i;
	struct fcb_block *fcb = NULL;
	void *fw = NULL, *fw_orig = NULL;
	enum filetype filetype;
	unsigned num_blocks_fw, fw_size;
	int used = 0;
	int fw_orig_len;
	int used_refresh = 0, unused_refresh = 0;

	if (data->image) {
		filetype = file_detect_type(data->image, data->len);

		if (filetype != imx_handler->filetype &&
			!bbu_force(data, "Image is not of type %s but of type %s",
				file_type_to_string(imx_handler->filetype),
				file_type_to_string(filetype)))
			return -EINVAL;
	}

	bcb_cdev = cdev_by_name(handler->devicefile);
	if (!bcb_cdev) {
		pr_err("%s: No FCB device!\n", __func__);
		return -ENODEV;
	}

	mtd = bcb_cdev->mtd;

	num_blocks_fw = imx_bbu_firmware_max_blocks(mtd);
	if (num_blocks_fw < 1) {
		pr_err("Not enough space for firmware\n");
		return -ENOSPC;
	}

	for (i = 0; i < 4; i++) {
		read_fcb(mtd, i, &fcb);
		if (fcb)
			break;
	}

	/*
	 * This code uses the following layout in the Nand flash:
	 *
	 * fwmaxsize = (n_blocks - 4) / 2
	 *
	 * block
	 *
	 * 0              ----------------------
	 *                | FCB/DBBT 0         |
	 * 1              ----------------------
	 *                | FCB/DBBT 1         |
	 * 2              ----------------------
	 *                | FCB/DBBT 2         |
	 * 3              ----------------------
	 *                | FCB/DBBT 3         |
	 * 4              ----------------------
	 *                | Firmware slot 0    |
	 * 4 + fwmaxsize  ----------------------
	 *                | Firmware slot 1    |
	 *                ----------------------
	 *
	 * We want a robust update in which a power failure may occur
	 * everytime without bricking the board, so here's the strategy:
	 *
	 * The FCBs contain pointers to the firmware slots in the
	 * Firmware1_startingPage and Firmware2_startingPage fields. Note that
	 * Firmware1_startingPage doesn't necessarily point to slot 0. We
	 * exchange the pointers during update to atomically switch between the
	 * old and the new firmware.
	 *
	 * - We read the first valid FCB and the firmware slots.
	 * - We check which firmware slot is currently used by the ROM:
	 *    - if no FCB is found or its layout differs from the above layout,
	 *      continue without robust update
	 *   - if only one firmware slot is readable, the ROM uses it
	 *   - if both slots are readable, the ROM will use slot 0
	 * - Step 1: erase/update the slot currently unused by the ROM
	 * - Step 2: Update FCBs/DBBTs, thereby letting Firmware1_startingPage
	 *           point to the slot we just updated. From this moment
	 *           on the new firmware will be used and running a
	 *           refresh/repair after a power failure after this
	 *           step will complete the update.
	 * - Step 3: erase/update the other firmwre slot
	 * - Step 4: Eventually write FCBs/DBBTs again. This may become
	 *           necessary when step 3 revealed new bad blocks.
	 *
	 * This robust update only works when the original FCBs on the device
	 * uses the same layout as this code does. In other cases update will
	 * also work, but it won't be robust against power failures.
	 *
	 * Refreshing the firmware which is needed when blocks become unreadable
	 * due to read disturbance works the same way, only that the new firmware
	 * is the same as the old firmware and that it will only be written when
	 * reading from the device returns -EUCLEAN indicating that a block needs
	 * to be rewritten.
	 */
	if (fcb)
		read_firmware_all(mtd, fcb, &fw_orig, &fw_orig_len,
				  &used_refresh, &unused_refresh, &used);

	if (data->image) {
		/*
		 * We have to write one additional page to make the ROM happy.
		 * Maybe the PagesInFirmwarex fields are really the number of pages - 1.
		 * kobs-ng has the same.
		 */
		fw_size = ALIGN(data->len + mtd->writesize, mtd->writesize);
		fw = xzalloc(fw_size);
		memcpy(fw, data->image, data->len);
		free(fw_orig);
		used_refresh = 1;
		unused_refresh = 1;

		free(fcb);
		fcb = xzalloc(sizeof(*fcb));
		fcb->Firmware1_startingPage = imx_bbu_firmware_fcb_start_page(mtd, !used);
		fcb->Firmware2_startingPage = imx_bbu_firmware_fcb_start_page(mtd, used);
		fcb->PagesInFirmware1 = fw_size / mtd->writesize;
		fcb->PagesInFirmware2 = fcb->PagesInFirmware1;

		fcb_create(imx_handler, fcb, mtd);
	} else {
		if (!fcb) {
			pr_err("No FCB found on device, cannot refresh\n");
			ret = -EINVAL;
			goto out;
		}

		if (!fw_orig) {
			pr_err("No firmware found on device, cannot refresh\n");
			ret = -EINVAL;
			goto out;
		}

		fw = fw_orig;
		fw_size = fw_orig_len;
		pr_info("Refreshing existing firmware\n");

		if (used_refresh) {
			fcb->Firmware1_startingPage = imx_bbu_firmware_fcb_start_page(mtd, !used);
			fcb->Firmware2_startingPage = imx_bbu_firmware_fcb_start_page(mtd, used);
			fcb_create(imx_handler, fcb, mtd);
		}
	}

	if (num_blocks_fw * mtd->erasesize < fw_size) {
		pr_err("Not enough space for update\n");
		return -ENOSPC;
	}

	ret = bbu_confirm(data);
	if (ret)
		goto out;

	/* Step 1: write firmware which is currently unused by the ROM */
	if (unused_refresh) {
		pr_info("%sing slot %d\n", data->image ? "updat" : "refresh", !used);
		ret = imx_bbu_write_firmware(mtd, !used, fw, fw_size);
		if (ret < 0)
			goto out;
	} else {
		pr_info("firmware slot %d still ok, nothing to do\n", !used);
	}

	/*
	 * Step 2: Write FCBs/DBBTs. This will use the firmware we have
	 * just written as primary firmware. From now on the new
	 * firmware will be booted.
	 */
	ret = imx_bbu_write_fcbs_dbbts(mtd, fcb);
	if (ret < 0)
		goto out;

	/* Step 3: Write the secondary firmware */
	if (used_refresh) {
		pr_info("%sing slot %d\n", data->image ? "updat" : "refresh", used);
		ret = imx_bbu_write_firmware(mtd, used, fw, fw_size);
		if (ret < 0)
			goto out;
	} else {
		pr_info("firmware slot %d still ok, nothing to do\n", used);
	}

	/*
	 * Step 4: If writing the secondary firmware discovered new bad
	 * blocks, write the FCBs/DBBTs again with updated bad block
	 * information.
	 */
	if (ret > 0) {
		pr_info("New bad blocks detected, writing FCBs/DBBTs again\n");
		ret = imx_bbu_write_fcbs_dbbts(mtd, fcb);
		if (ret < 0)
			goto out;
	}

out:
	free(fw);
	free(fcb);

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
	handler->flags = flags | BBU_HANDLER_CAN_REFRESH;
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

int imx28_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	struct imx_nand_fcb_bbu_handler *imx_handler;
	struct bbu_handler *handler;
	int ret;

	imx_handler = xzalloc(sizeof(*imx_handler));
	imx_handler->fcb_create = imx28_fcb_create;

	imx_handler->filetype = filetype_mxs_bootstream;

	handler = &imx_handler->handler;
	handler->devicefile = "nand0.barebox";
	handler->name = name;
	handler->flags = flags | BBU_HANDLER_CAN_REFRESH;
	handler->handler = imx_bbu_nand_update;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}
#endif
