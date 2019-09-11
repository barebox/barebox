/*
 * (C) Copyright 2010 Juergen Beisert, Pengutronix
 *
 * This code is havily inspired and in parts from the u-boot project:
 *
 * Copyright 2008, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the Linux code
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

/* #define DEBUG */

#include <init.h>
#include <common.h>
#include <mci.h>
#include <malloc.h>
#include <errno.h>
#include <asm-generic/div64.h>
#include <asm/byteorder.h>
#include <block.h>
#include <disks.h>
#include <of.h>
#include <linux/err.h>

#define MAX_BUFFER_NUMBER 0xffffffff

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

LIST_HEAD(mci_list);

/**
 * @file
 * @brief Memory Card framework
 *
 * Checked with the following cards:
 * - Canon MMC 16 MiB
 * - Integral MicroSDHC, 8 GiB (Class 4)
 * - Kingston 512 MiB
 * - SanDisk 512 MiB
 * - Transcend SD Ultra, 1 GiB (Industrial)
 * - Transcend SDHC, 4 GiB (Class 6)
 * - Transcend SDHC, 8 GiB (Class 6)
 */

static inline unsigned mci_caps(struct mci *mci)
{
	return mci->card_caps & mci->host->host_caps;
}

/**
 * Call the MMC/SD instance driver to run the command on the MMC/SD card
 * @param mci MCI instance
 * @param cmd The information about the command to run
 * @param data The data according to the command (can be NULL)
 * @return Driver's answer (0 on success)
 */
static int mci_send_cmd(struct mci *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	struct mci_host *host = mci->host;

	return host->send_cmd(mci->host, cmd, data);
}

/**
 * @param p Command definition to setup
 * @param cmd Valid SD/MMC command (refer MMC_CMD_* / SD_CMD_*)
 * @param arg Argument for the command (optional)
 * @param response Command's response type (refer MMC_RSP_*)
 *
 * Note: When calling, the 'response' must match command's requirements
 */
static void mci_setup_cmd(struct mci_cmd *p, unsigned cmd, unsigned arg, unsigned response)
{
	p->cmdidx = cmd;
	p->cmdarg = arg;
	p->resp_type = response;
}

/**
 * configure optional DSR value
 * @param mci_dev MCI instance
 * @return Transaction status (0 on success)
 */
static int mci_set_dsr(struct mci *mci)
{
	struct mci_cmd cmd;

	mci_setup_cmd(&cmd, MMC_CMD_SET_DSR,
			(mci->host->dsr_val >> 16) | 0xffff, MMC_RSP_NONE);
	return mci_send_cmd(mci, &cmd, NULL);
}

/**
 * Setup SD/MMC card's blocklength to be used for future transmitts
 * @param mci_dev MCI instance
 * @param len Blocklength in bytes
 * @return Transaction status (0 on success)
 */
static int mci_set_blocklen(struct mci *mci, unsigned len)
{
	struct mci_cmd cmd;

	mci_setup_cmd(&cmd, MMC_CMD_SET_BLOCKLEN, len, MMC_RSP_R1);
	return mci_send_cmd(mci, &cmd, NULL);
}

static void *sector_buf;

/**
 * Write one or several blocks of data to the card
 * @param mci_dev MCI instance
 * @param src Where to read from to write to the card
 * @param blocknum Block number to write
 * @param blocks Block count to write
 * @return Transaction status (0 on success)
 */
static int mci_block_write(struct mci *mci, const void *src, int blocknum,
	int blocks)
{
	struct mci_cmd cmd;
	struct mci_data data;
	const void *buf;
	unsigned mmccmd;
	int ret;

	if (blocks > 1)
		mmccmd = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	else
		mmccmd = MMC_CMD_WRITE_SINGLE_BLOCK;

	if ((unsigned long)src & 0x3) {
		memcpy(sector_buf, src, 512);
		buf = sector_buf;
	} else {
		buf = src;
	}

	mci_setup_cmd(&cmd,
		mmccmd,
		mci->high_capacity != 0 ? blocknum : blocknum * mci->write_bl_len,
		MMC_RSP_R1);

	data.src = buf;
	data.blocks = blocks;
	data.blocksize = mci->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	ret = mci_send_cmd(mci, &cmd, &data);

	if (ret || blocks > 1) {
		mci_setup_cmd(&cmd, MMC_CMD_STOP_TRANSMISSION, 0, MMC_RSP_R1b);
		mci_send_cmd(mci, &cmd, NULL);
        }

	return ret;
}

/**
 * Read one or several block(s) of data from the card
 * @param mci MCI instance
 * @param dst Where to store the data read from the card
 * @param blocknum Block number to read
 * @param blocks number of blocks to read
 */
static int mci_read_block(struct mci *mci, void *dst, int blocknum,
		int blocks)
{
	struct mci_cmd cmd;
	struct mci_data data;
	int ret;
	unsigned mmccmd;

	if (blocks > 1)
		mmccmd = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		mmccmd = MMC_CMD_READ_SINGLE_BLOCK;

	mci_setup_cmd(&cmd,
		mmccmd,
		mci->high_capacity != 0 ? blocknum : blocknum * mci->read_bl_len,
		MMC_RSP_R1);

	data.dest = dst;
	data.blocks = blocks;
	data.blocksize = mci->read_bl_len;
	data.flags = MMC_DATA_READ;

	ret = mci_send_cmd(mci, &cmd, &data);

	if (ret || blocks > 1) {
		mci_setup_cmd(&cmd, MMC_CMD_STOP_TRANSMISSION, 0, MMC_RSP_R1b);
		mci_send_cmd(mci, &cmd, NULL);
	}
	return ret;
}

/**
 * Reset the attached MMC/SD card
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int mci_go_idle(struct mci *mci)
{
	struct mci_cmd cmd;
	int err;

	udelay(1000);

	mci_setup_cmd(&cmd, MMC_CMD_GO_IDLE_STATE, 0, MMC_RSP_NONE);
	err = mci_send_cmd(mci, &cmd, NULL);

	if (err) {
		dev_dbg(&mci->dev, "Activating IDLE state failed: %d\n", err);
		return err;
	}

	udelay(2000);	/* WTF? */

	return 0;
}

/**
 * FIXME
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int sd_send_op_cond(struct mci *mci)
{
	struct mci_host *host = mci->host;
	struct mci_cmd cmd;
	int timeout = 1000;
	int err;
	unsigned voltages;
	unsigned busy;
	unsigned arg;

	/*
	 * Most cards do not answer if some reserved bits
	 * in the ocr are set. However, Some controller
	 * can set bit 7 (reserved for low voltages), but
	 * how to manage low voltages SD card is not yet
	 * specified.
	 */
	voltages = host->voltages & 0xff8000;

	do {
		mci_setup_cmd(&cmd, MMC_CMD_APP_CMD, 0, MMC_RSP_R1);
		err = mci_send_cmd(mci, &cmd, NULL);
		if (err) {
			dev_dbg(&mci->dev, "Preparing SD for operating conditions failed: %d\n", err);
			return err;
		}

		arg = mmc_host_is_spi(host) ? 0 : voltages;

		if (mci->version == SD_VERSION_2)
			arg |= OCR_HCS;

		mci_setup_cmd(&cmd, SD_CMD_APP_SEND_OP_COND, arg, MMC_RSP_R3);
		err = mci_send_cmd(mci, &cmd, NULL);
		if (err) {
			dev_dbg(&mci->dev, "SD operation condition set failed: %d\n", err);
			return err;
		}
		udelay(1000);

		if (mmc_host_is_spi(host))
			busy = cmd.response[0] & R1_SPI_IDLE;
		else
			busy = !(cmd.response[0] & OCR_BUSY);

	} while (busy && timeout--);

	if (timeout <= 0) {
		dev_dbg(&mci->dev, "SD operation condition set timed out\n");
		return -ENODEV;
	}

	if (mci->version != SD_VERSION_2)
		mci->version = SD_VERSION_1_0;

	if (mmc_host_is_spi(host)) { /* read OCR for spi */
		mci_setup_cmd(&cmd, MMC_CMD_SPI_READ_OCR, 0, MMC_RSP_R3);
		err = mci_send_cmd(mci, &cmd, NULL);
		if (err)
			return err;
	}

	mci->ocr = cmd.response[0];

	mci->high_capacity = ((mci->ocr & OCR_HCS) == OCR_HCS);
	mci->rca = 0;

	return 0;
}

/**
 * Setup the operation conditions to a MultiMediaCard
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int mmc_send_op_cond(struct mci *mci)
{
	struct mci_host *host = mci->host;
	struct mci_cmd cmd;
	int timeout = 1000;
	int err;

	/* Some cards seem to need this */
	mci_go_idle(mci);

	do {
		mci_setup_cmd(&cmd, MMC_CMD_SEND_OP_COND, OCR_HCS |
				host->voltages, MMC_RSP_R3);
		err = mci_send_cmd(mci, &cmd, NULL);

		if (err) {
			dev_dbg(&mci->dev, "Preparing MMC for operating conditions failed: %d\n", err);
			return err;
		}

		udelay(1000);
	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);

	if (timeout <= 0) {
		dev_dbg(&mci->dev, "SD operation condition set timed out\n");
		return -ENODEV;
	}

	mci->version = MMC_VERSION_UNKNOWN;
	mci->ocr = cmd.response[0];

	mci->high_capacity = ((mci->ocr & OCR_HCS) == OCR_HCS);
	mci->rca = 0;

	return 0;
}

/**
 * FIXME
 * @param mci MCI instance
 * @param ext_csd Buffer for a 512 byte sized extended CSD
 * @return Transaction status (0 on success)
 *
 * Note: Only cards newer than Version 1.1 (Physical Layer Spec) support
 * this command
 */
int mci_send_ext_csd(struct mci *mci, char *ext_csd)
{
	struct mci_cmd cmd;
	struct mci_data data;

	/* Get the Card Status Register */
	mci_setup_cmd(&cmd, MMC_CMD_SEND_EXT_CSD, 0, MMC_RSP_R1);

	data.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	return mci_send_cmd(mci, &cmd, &data);
}

/**
 * FIXME
 * @param mci MCI instance
 * @param set FIXME
 * @param index FIXME
 * @param value FIXME
 * @return Transaction status (0 on success)
 */
int mci_switch(struct mci *mci, unsigned index, unsigned value)
{
	struct mci_cmd cmd;

	mci_setup_cmd(&cmd, MMC_CMD_SWITCH,
		(MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		(index << 16) |
		(value << 8),
		 MMC_RSP_R1b);

	return mci_send_cmd(mci, &cmd, NULL);
}

static int mci_calc_blk_cnt(uint64_t cap, unsigned shift)
{
	unsigned ret = cap >> shift;

	if (ret > 0x7fffffff) {
		pr_warn("Limiting card size due to 31 bit contraints\n");
		return 0x7fffffff;
	}

	return (int)ret;
}

static void mci_part_add(struct mci *mci, uint64_t size,
                        unsigned int part_cfg, char *name, char *partname, int idx, bool ro,
                        int area_type)
{
	struct mci_part *part = &mci->part[mci->nr_parts];

	part->mci = mci;
	part->size = size;
	part->blk.cdev.name = name;
	part->blk.cdev.partname = partname;
	part->blk.blockbits = SECTOR_SHIFT;
	part->blk.num_blocks = mci_calc_blk_cnt(size, part->blk.blockbits);
	part->area_type = area_type;
	part->part_cfg = part_cfg;
	part->idx = idx;

	if (area_type == MMC_BLK_DATA_AREA_MAIN)
		part->blk.cdev.device_node = mci->host->hw_dev->device_node;

	mci->nr_parts++;
}

/**
 * Change transfer frequency for an MMC card
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int mmc_change_freq(struct mci *mci)
{
	char cardtype;
	int err;

	mci->ext_csd = xmalloc(512);
	mci->card_caps = 0;

	/* Only version 4 supports high-speed */
	if (mci->version < MMC_VERSION_4)
		return 0;

	mci->card_caps |= MMC_CAP_4_BIT_DATA;

	err = mci_send_ext_csd(mci, mci->ext_csd);
	if (err) {
		dev_dbg(&mci->dev, "Preparing for frequency setup failed: %d\n", err);
		return err;
	}

	cardtype = mci->ext_csd[EXT_CSD_DEVICE_TYPE] & EXT_CSD_CARD_TYPE_MASK;

	err = mci_switch(mci, EXT_CSD_HS_TIMING, 1);

	if (err) {
		dev_dbg(&mci->dev, "MMC frequency changing failed: %d\n", err);
		return err;
	}

	/* Now check to see that it worked */
	err = mci_send_ext_csd(mci, mci->ext_csd);

	if (err) {
		dev_dbg(&mci->dev, "Verifying frequency change failed: %d\n", err);
		return err;
	}

	/* No high-speed support */
	if (!mci->ext_csd[EXT_CSD_HS_TIMING]) {
		dev_dbg(&mci->dev, "No high-speed support\n");
		return 0;
	}

	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & EXT_CSD_CARD_TYPE_52)
		mci->card_caps |= MMC_CAP_MMC_HIGHSPEED_52MHZ | MMC_CAP_MMC_HIGHSPEED;
	else
		mci->card_caps |= MMC_CAP_MMC_HIGHSPEED;

	if (IS_ENABLED(CONFIG_MCI_MMC_BOOT_PARTITIONS) &&
			mci->ext_csd[EXT_CSD_REV] >= 3 && mci->ext_csd[EXT_CSD_BOOT_SIZE_MULT]) {
		int idx;
		unsigned int part_size;

		for (idx = 0; idx < MMC_NUM_BOOT_PARTITION; idx++) {
			char *name, *partname;
			part_size = mci->ext_csd[EXT_CSD_BOOT_SIZE_MULT] << 17;

			partname = basprintf("boot%d", idx);
			name = basprintf("%s.%s", mci->cdevname, partname);
			mci_part_add(mci, part_size,
					EXT_CSD_PART_CONFIG_ACC_BOOT0 + idx,
					name, partname, idx, true,
					MMC_BLK_DATA_AREA_BOOT);
		}

		mci->ext_csd_part_config = mci->ext_csd[EXT_CSD_PARTITION_CONFIG];
		mci->bootpart = (mci->ext_csd_part_config >> 3) & 0x7;
	}

	return 0;
}

/**
 * FIXME
 * @param mci MCI instance
 * @param mode FIXME
 * @param group FIXME
 * @param value FIXME
 * @param resp FIXME
 * @return Transaction status (0 on success)
 */
static int sd_switch(struct mci *mci, unsigned mode, unsigned group,
			unsigned value, uint8_t *resp)
{
	struct mci_cmd cmd;
	struct mci_data data;
	unsigned arg;

	arg = (mode << 31) | 0xffffff;
	arg &= ~(0xf << (group << 2));
	arg |= value << (group << 2);

	/* Switch the frequency */
	mci_setup_cmd(&cmd, SD_CMD_SWITCH_FUNC, arg, MMC_RSP_R1);

	data.dest = resp;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	return mci_send_cmd(mci, &cmd, &data);
}

/**
 * Change transfer frequency for an SD card
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int sd_change_freq(struct mci *mci)
{
	struct mci_cmd cmd;
	struct mci_data data;
	struct mci_host *host = mci->host;
	uint32_t *switch_status = sector_buf;
	uint32_t *scr = sector_buf;
	int timeout;
	int err;

	if (mmc_host_is_spi(host))
		return 0;

	dev_dbg(&mci->dev, "Changing transfer frequency\n");
	mci->card_caps = 0;

	/* Read the SCR to find out if this card supports higher speeds */
	mci_setup_cmd(&cmd, MMC_CMD_APP_CMD, mci->rca << 16, MMC_RSP_R1);
	err = mci_send_cmd(mci, &cmd, NULL);
	if (err) {
		dev_dbg(&mci->dev, "Query SD card capabilities failed: %d\n", err);
		return err;
	}

	mci_setup_cmd(&cmd, SD_CMD_APP_SEND_SCR, 0, MMC_RSP_R1);

	timeout = 3;

retry_scr:
	dev_dbg(&mci->dev, "Trying to read the SCR (try %d of %d)\n", 4 - timeout, 3);
	data.dest = (char *)scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = mci_send_cmd(mci, &cmd, &data);
	if (err) {
		dev_dbg(&mci->dev, " Catch error (%d)", err);
		if (timeout--) {
			dev_dbg(&mci->dev, "-- retrying\n");
			goto retry_scr;
		}
		dev_dbg(&mci->dev, "-- giving up\n");
		return err;
	}

	mci->scr[0] = __be32_to_cpu(scr[0]);
	mci->scr[1] = __be32_to_cpu(scr[1]);

	switch ((mci->scr[0] >> 24) & 0xf) {
	case 0:
		mci->version = SD_VERSION_1_0;
		break;
	case 1:
		mci->version = SD_VERSION_1_10;
		break;
	case 2:
		mci->version = SD_VERSION_2;
		break;
	default:
		mci->version = SD_VERSION_1_0;
		break;
	}

	if (mci->scr[0] & SD_DATA_4BIT)
		mci->card_caps |= MMC_CAP_4_BIT_DATA;

	/* Version 1.0 doesn't support switching */
	if (mci->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sd_switch(mci, SD_SWITCH_CHECK, 0, 1,
				(uint8_t*)switch_status);
		if (err) {
			dev_dbg(&mci->dev, "Checking SD transfer switch frequency feature failed: %d\n", err);
			return err;
		}

		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	err = sd_switch(mci, SD_SWITCH_SWITCH, 0, 1, (uint8_t*)switch_status);
	if (err) {
		dev_dbg(&mci->dev, "Switching SD transfer frequency failed: %d\n", err);
		return err;
	}

	if ((__be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
		mci->card_caps |= MMC_CAP_SD_HIGHSPEED;

	if (mci_caps(mci) & MMC_CAP_SD_HIGHSPEED)
		mci->tran_speed = 50000000;

	return 0;
}

/**
 * Setup host's interface bus width and transfer frequency
 * @param mci MCI instance
 */
static void mci_set_ios(struct mci *mci)
{
	struct mci_host *host = mci->host;
	struct mci_ios ios;

	ios.bus_width = host->bus_width;
	ios.clock = host->clock;

	host->set_ios(host, &ios);
}

/**
 * Setup host's interface transfer frequency
 * @param mci MCI instance
 * @param clock New clock in Hz to set
 */
static void mci_set_clock(struct mci *mci, unsigned clock)
{
	struct mci_host *host = mci->host;

	/* check against any given limits at the host's side */
	if (clock > host->f_max)
		clock = host->f_max;

	if (clock < host->f_min)
		clock = host->f_min;

	host->clock = clock;	/* the new target frequency */
	mci_set_ios(mci);
}

/**
 * Setup host's interface bus width
 * @param mci MCI instance
 * @param width New interface bit width (1, 4 or 8)
 */
static void mci_set_bus_width(struct mci *mci, unsigned width)
{
	struct mci_host *host = mci->host;

	host->bus_width = width;	/* the new target bus width */
	mci_set_ios(mci);
}

/**
 * Extract card's version from its CSD
 * @param mci MCI instance
 * @return 0 on success
 */
static void mci_detect_version_from_csd(struct mci *mci)
{
	int version;

	if (mci->version == MMC_VERSION_UNKNOWN) {
		/* the version is coded in the bits 127:126 (left aligned) */
		version = (mci->csd[0] >> 26) & 0xf;	/* FIXME why other width? */

		switch (version) {
		case 0:
			mci->version = MMC_VERSION_1_2;
			break;
		case 1:
			mci->version = MMC_VERSION_1_4;
			break;
		case 2:
			mci->version = MMC_VERSION_2_2;
			break;
		case 3:
			mci->version = MMC_VERSION_3;
			break;
		case 4:
			mci->version = MMC_VERSION_4;
			break;
		default:
			printf("unknown card version, fallback to 1.02\n");
			mci->version = MMC_VERSION_1_2;
			break;
		}
	}
}

/**
 * correct the version from ext_csd data if it's not an SD-card, detected
 * version is at least 4 and we have ext_csd data
 */
static void mci_correct_version_from_ext_csd(struct mci *mci)
{
	if (!IS_SD(mci) && (mci->version >= MMC_VERSION_4) && mci->ext_csd) {
		switch (mci->ext_csd[EXT_CSD_REV]) {
		case 1:
			mci->version = MMC_VERSION_4_1;
			break;
		case 2:
			mci->version = MMC_VERSION_4_2;
			break;
		case 3:
			mci->version = MMC_VERSION_4_3;
			break;
		case 5:
			mci->version = MMC_VERSION_4_41;
			break;
		case 6:
			mci->version = MMC_VERSION_4_5;
			break;
		case 7:
			mci->version = MMC_VERSION_5_0;
			break;
		case 8:
			mci->version = MMC_VERSION_5_1;
			break;
		}
	}
}

/**
 * meaning of the encoded 'unit' bits in the CSD's field 'TRAN_SPEED'
 * (divided by 10 to be nice to platforms without floating point)
 */
static const unsigned tran_speed_unit[] = {
	[0] = 10000,		/* 100 kbit/s */
	[1] = 100000,		/* 1 Mbit/s */
	[2] = 1000000,		/* 10 Mbit/s */
	[3] = 10000000,		/* 100 Mbit/s */
	/* [4]...[7] are reserved */
};

/**
 * meaning of the 'time' bits  in the CSD's field 'TRAN_SPEED'
 * (multiplied by 10 to be nice to platforms without floating point)
 */
static const unsigned char tran_speed_time[] = {
	0,	/* reserved */
	10,	/* 1.0 ns */
	12,	/* 1.2 ns */
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,	/* 7.0 ns */
	80,	/* 8.0 ns */
};

/**
 * Extract max. transfer speed from the CSD
 * @param mci MCI instance
 *
 * Encoded in bit 103:96 (103: reserved, 102:99: time, 98:96 unit)
 */
static void mci_extract_max_tran_speed_from_csd(struct mci *mci)
{
	unsigned unit, time;

	unit = tran_speed_unit[(mci->csd[0] & 0x7)];
	time = tran_speed_time[((mci->csd[0] >> 3) & 0xf)];
	if ((unit == 0) || (time == 0)) {
		dev_dbg(&mci->dev, "Unsupported 'TRAN_SPEED' unit/time value."
				" Can't calculate card's max. transfer speed\n");
		return;
	}

	mci->tran_speed = time * unit;
	dev_dbg(&mci->dev, "Transfer speed: %u\n", mci->tran_speed);
}

/**
 * Extract max read and write block length from the CSD
 * @param mci MCI instance
 *
 * Encoded in bit 83:80 (read) and 25:22 (write)
 */
static void mci_extract_block_lengths_from_csd(struct mci *mci)
{
	mci->read_bl_len = 1 << UNSTUFF_BITS(mci->csd, 80, 4);

	if (IS_SD(mci))
		mci->write_bl_len = mci->read_bl_len;	/* FIXME why? */
	else
		mci->write_bl_len = 1 << ((mci->csd[3] >> 22) & 0xf);

	dev_dbg(&mci->dev, "Max. block length are: Write=%u, Read=%u Bytes\n",
		mci->write_bl_len, mci->read_bl_len);
}

/**
 * Extract card's capacitiy from the CSD
 * @param mci MCI instance
 */
static void mci_extract_card_capacity_from_csd(struct mci *mci)
{
	uint64_t csize, cmult;

	if (mci->high_capacity) {
		if (IS_SD(mci)) {
			csize = UNSTUFF_BITS(mci->csd, 48, 22);
			mci->capacity = (1 + csize) << 10;
		} else {
			mci->capacity = mci->ext_csd[EXT_CSD_SEC_COUNT] << 0 |
				mci->ext_csd[EXT_CSD_SEC_COUNT + 1] << 8 |
				mci->ext_csd[EXT_CSD_SEC_COUNT + 2] << 16 |
				mci->ext_csd[EXT_CSD_SEC_COUNT + 3] << 24;
		}
	} else {
		cmult = UNSTUFF_BITS(mci->csd, 47, 3);
		csize = UNSTUFF_BITS(mci->csd, 62, 12);
		mci->capacity = (csize + 1) << (cmult + 2);
	}

	mci->capacity *= 1 << UNSTUFF_BITS(mci->csd, 80, 4);
	dev_dbg(&mci->dev, "Capacity: %u MiB\n", (unsigned)(mci->capacity >> 20));
}

/**
 * Extract card's DSR implementation state from CSD
 * @param mci MCI instance
 */
static void mci_extract_card_dsr_imp_from_csd(struct mci *mci)
{
	mci->dsr_imp = UNSTUFF_BITS(mci->csd, 76, 1);
}

static int mmc_compare_ext_csds(struct mci *mci, unsigned bus_width)
{
	u8 *bw_ext_csd;
	int err;

	if (bus_width == MMC_BUS_WIDTH_1)
		return 0;

	bw_ext_csd = xmalloc(512);
	err = mci_send_ext_csd(mci, bw_ext_csd);
	if (err) {
		dev_info(&mci->dev, "mci_send_ext_csd failed with %d\n", err);
		if (bus_width != MMC_BUS_WIDTH_1)
			err = -EINVAL;
		goto out;
	}

	if (bus_width == MMC_BUS_WIDTH_1)
		goto out;
	/* only compare read only fields */
	err = (mci->ext_csd[EXT_CSD_PARTITIONING_SUPPORT] ==
			bw_ext_csd[EXT_CSD_PARTITIONING_SUPPORT]) &&
		(mci->ext_csd[EXT_CSD_ERASED_MEM_CONT] ==
			bw_ext_csd[EXT_CSD_ERASED_MEM_CONT]) &&
		(mci->ext_csd[EXT_CSD_REV] ==
			bw_ext_csd[EXT_CSD_REV]) &&
		(mci->ext_csd[EXT_CSD_CSD_STRUCTURE] ==
			bw_ext_csd[EXT_CSD_CSD_STRUCTURE]) &&
		(mci->ext_csd[EXT_CSD_DEVICE_TYPE] ==
			bw_ext_csd[EXT_CSD_DEVICE_TYPE]) &&
		(mci->ext_csd[EXT_CSD_S_A_TIMEOUT] ==
			bw_ext_csd[EXT_CSD_S_A_TIMEOUT]) &&
		(mci->ext_csd[EXT_CSD_HC_WP_GRP_SIZE] ==
			bw_ext_csd[EXT_CSD_HC_WP_GRP_SIZE]) &&
		(mci->ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] ==
			bw_ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT]) &&
		(mci->ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] ==
			bw_ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]) &&
		(mci->ext_csd[EXT_CSD_SEC_TRIM_MULT] ==
			bw_ext_csd[EXT_CSD_SEC_TRIM_MULT]) &&
		(mci->ext_csd[EXT_CSD_SEC_ERASE_MULT] ==
			bw_ext_csd[EXT_CSD_SEC_ERASE_MULT]) &&
		(mci->ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT] ==
			bw_ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT]) &&
		(mci->ext_csd[EXT_CSD_TRIM_MULT] ==
			bw_ext_csd[EXT_CSD_TRIM_MULT]) &&
		(mci->ext_csd[EXT_CSD_SEC_COUNT + 0] ==
			bw_ext_csd[EXT_CSD_SEC_COUNT + 0]) &&
		(mci->ext_csd[EXT_CSD_SEC_COUNT + 1] ==
			bw_ext_csd[EXT_CSD_SEC_COUNT + 1]) &&
		(mci->ext_csd[EXT_CSD_SEC_COUNT + 2] ==
			bw_ext_csd[EXT_CSD_SEC_COUNT + 2]) &&
		(mci->ext_csd[EXT_CSD_SEC_COUNT + 3] ==
			bw_ext_csd[EXT_CSD_SEC_COUNT + 3]) ?
				0 : -EINVAL;

out:
	free(bw_ext_csd);
	return err;
}

static char *mci_version_string(struct mci *mci)
{
	static char version[sizeof("xx.xxx")];
	unsigned major, minor, micro;
	int n;

	major = (mci->version >> 8) & 0xf;
	minor = (mci->version >> 4) & 0xf;
	micro = mci->version & 0xf;

	n = sprintf(version, "%u.%u", major, minor);
	/* Omit zero micro versions */
	if (micro)
		sprintf(version + n, "%u", micro);

	return version;
}

static int mci_startup_sd(struct mci *mci)
{
	struct mci_cmd cmd;
	int err;

	if (mci_caps(mci) & MMC_CAP_4_BIT_DATA) {
		dev_dbg(&mci->dev, "Prepare for bus width change\n");
		mci_setup_cmd(&cmd, MMC_CMD_APP_CMD, mci->rca << 16, MMC_RSP_R1);
		err = mci_send_cmd(mci, &cmd, NULL);
		if (err) {
			dev_dbg(&mci->dev, "Preparing SD for bus width change failed: %d\n", err);
			return err;
		}

		dev_dbg(&mci->dev, "Set SD bus width to 4 bit\n");
		mci_setup_cmd(&cmd, SD_CMD_APP_SET_BUS_WIDTH, 2, MMC_RSP_R1);
		err = mci_send_cmd(mci, &cmd, NULL);
		if (err) {
			dev_warn(&mci->dev, "Changing SD bus width failed: %d\n", err);
			/* TODO continue with 1 bit? */
			return err;
		}
		mci_set_bus_width(mci, MMC_BUS_WIDTH_4);
	}

	mci_set_clock(mci, mci->tran_speed);

	return 0;
}

static int mci_startup_mmc(struct mci *mci)
{
	struct mci_host *host = mci->host;
	int err;
	int idx = 0;
	static unsigned ext_csd_bits[] = {
		EXT_CSD_BUS_WIDTH_4,
		EXT_CSD_BUS_WIDTH_8,
	};
	static unsigned bus_widths[] = {
		MMC_BUS_WIDTH_4,
		MMC_BUS_WIDTH_8,
	};

	/* if possible, speed up the transfer */
	if (mci_caps(mci) & MMC_CAP_MMC_HIGHSPEED) {
		if (mci->card_caps & MMC_CAP_MMC_HIGHSPEED_52MHZ)
			mci->tran_speed = 52000000;
		else
			mci->tran_speed = 26000000;
	}

	mci_set_clock(mci, mci->tran_speed);

	/*
	 * Unlike SD, MMC cards dont have a configuration register to notify
	 * supported bus width. So bus test command should be run to identify
	 * the supported bus width or compare the ext csd values of current
	 * bus width and ext csd values of 1 bit mode read earlier.
	 */
	if (host->host_caps & MMC_CAP_8_BIT_DATA)
		idx = 1;

	for (; idx >= 0; idx--) {

		/*
		 * Host is capable of 8bit transfer, then switch
		 * the device to work in 8bit transfer mode. If the
		 * mmc switch command returns error then switch to
		 * 4bit transfer mode. On success set the corresponding
		 * bus width on the host.
		 */
		err = mci_switch(mci, EXT_CSD_BUS_WIDTH, ext_csd_bits[idx]);
		if (err) {
			if (idx == 0)
				dev_warn(&mci->dev, "Changing MMC bus width failed: %d\n", err);
			continue;
		}

		mci_set_bus_width(mci, bus_widths[idx]);

		err = mmc_compare_ext_csds(mci, bus_widths[idx]);
		if (!err)
			break;
	}

	return err;
}

/**
 * Scan the given host interfaces and detect connected MMC/SD cards
 * @param mci MCI instance
 * @return 0 on success, negative value else
 */
static int mci_startup(struct mci *mci)
{
	struct mci_host *host = mci->host;
	struct mci_cmd cmd;
	int err;

	if (IS_ENABLED(CONFIG_MMC_SPI_CRC_ON) && mmc_host_is_spi(host)) { /* enable CRC check for spi */

		mci_setup_cmd(&cmd, MMC_CMD_SPI_CRC_ON_OFF, 1, MMC_RSP_R1);
		err = mci_send_cmd(mci, &cmd, NULL);

		if (err) {
			dev_dbg(&mci->dev, "Can't enable CRC check : %d\n", err);
			return err;
		}
	}

	dev_dbg(&mci->dev, "Put the Card in Identify Mode\n");

	/* Put the Card in Identify Mode */
	mci_setup_cmd(&cmd, mmc_host_is_spi(host) ? MMC_CMD_SEND_CID : MMC_CMD_ALL_SEND_CID, 0, MMC_RSP_R2);
	err = mci_send_cmd(mci, &cmd, NULL);
	if (err) {
		dev_dbg(&mci->dev, "Can't bring card into identify mode: %d\n", err);
		return err;
	}

	memcpy(mci->cid, cmd.response, 16);

	dev_dbg(&mci->dev, "Card's identification data is: %08X-%08X-%08X-%08X\n",
		mci->cid[0], mci->cid[1], mci->cid[2], mci->cid[3]);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	if (!mmc_host_is_spi(host)) { /* cmd not supported in spi */
		dev_dbg(&mci->dev, "Get/Set relative address\n");
		mci_setup_cmd(&cmd, SD_CMD_SEND_RELATIVE_ADDR, mci->rca << 16, MMC_RSP_R6);
		err = mci_send_cmd(mci, &cmd, NULL);
		if (err) {
			dev_dbg(&mci->dev, "Get/Set relative address failed: %d\n", err);
			return err;
		}
	}

	if (IS_SD(mci))
		mci->rca = (cmd.response[0] >> 16) & 0xffff;

	dev_dbg(&mci->dev, "Get card's specific data\n");
	/* Get the Card-Specific Data */
	mci_setup_cmd(&cmd, MMC_CMD_SEND_CSD, mci->rca << 16, MMC_RSP_R2);
	err = mci_send_cmd(mci, &cmd, NULL);
	if (err) {
		dev_dbg(&mci->dev, "Getting card's specific data failed: %d\n", err);
		return err;
	}

	/* CSD is of 128 bit */
	memcpy(mci->csd, cmd.response, 16);

	dev_dbg(&mci->dev, "Card's specific data is: %08X-%08X-%08X-%08X\n",
		mci->csd[0], mci->csd[1], mci->csd[2], mci->csd[3]);

	mci_detect_version_from_csd(mci);
	mci_extract_max_tran_speed_from_csd(mci);
	mci_extract_block_lengths_from_csd(mci);
	mci_extract_card_dsr_imp_from_csd(mci);

	/* sanitiy? */
	if (mci->read_bl_len > SECTOR_SIZE) {
		mci->read_bl_len = SECTOR_SIZE;
		dev_dbg(&mci->dev, "Limiting max. read block size down to %u\n",
				mci->read_bl_len);
	}

	if (mci->write_bl_len > SECTOR_SIZE) {
		mci->write_bl_len = SECTOR_SIZE;
		dev_dbg(&mci->dev, "Limiting max. write block size down to %u\n",
				mci->read_bl_len);
	}
	dev_dbg(&mci->dev, "Read block length: %u, Write block length: %u\n",
		mci->read_bl_len, mci->write_bl_len);

	if (mci->dsr_imp && mci->host->use_dsr)
		mci_set_dsr(mci);

	if (!mmc_host_is_spi(host)) { /* cmd not supported in spi */
		dev_dbg(&mci->dev, "Select the card, and put it into Transfer Mode\n");
		/* Select the card, and put it into Transfer Mode */
		mci_setup_cmd(&cmd, MMC_CMD_SELECT_CARD, mci->rca << 16, MMC_RSP_R1b);
		err = mci_send_cmd(mci, &cmd, NULL);
		if (err) {
			dev_dbg(&mci->dev, "Putting in transfer mode failed: %d\n", err);
			return err;
		}
	}

	if (IS_SD(mci))
		err = sd_change_freq(mci);
	else
		err = mmc_change_freq(mci);

	if (err)
		return err;

	mci_correct_version_from_ext_csd(mci);
	dev_info(&mci->dev, "detected %s card version %s\n", IS_SD(mci) ? "SD" : "MMC",
		mci_version_string(mci));
	mci_extract_card_capacity_from_csd(mci);

	if (IS_SD(mci))
		err = mci_startup_sd(mci);
	else
		err = mci_startup_mmc(mci);

	if (err)
		return err;

	/* we setup the blocklength only one times for all accesses to this media  */
	err = mci_set_blocklen(mci, mci->read_bl_len);

	mci_part_add(mci, mci->capacity, 0,
			mci->cdevname, NULL, 0, true,
			MMC_BLK_DATA_AREA_MAIN);

	return err;
}

/**
 * Detect a SD 2.0 card and enable its features
 * @param mci MCI instance
 * @return Transfer status (0 on success)
 *
 * By issuing the CMD8 command SDHC/SDXC cards realize that the host supports
 * the Physical Layer Version 2.00 or later and the card can enable
 * corresponding new functions.
 *
 * If this CMD8 command will end with a timeout it is a MultiMediaCard only.
 */
static int sd_send_if_cond(struct mci *mci)
{
	struct mci_host *host = mci->host;
	struct mci_cmd cmd;
	int err;

	mci_setup_cmd(&cmd, SD_CMD_SEND_IF_COND,
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
		((host->voltages & 0x00ff8000) != 0) << 8 | 0xaa,
		MMC_RSP_R7);
	err = mci_send_cmd(mci, &cmd, NULL);
	if (err) {
		dev_dbg(&mci->dev, "Query interface conditions failed: %d\n", err);
		return err;
	}

	if ((cmd.response[0] & 0xff) != 0xaa) {
		dev_dbg(&mci->dev, "Card cannot work with hosts supply voltages\n");
		return -EINVAL;
	} else {
		dev_dbg(&mci->dev, "SD Card Rev. 2.00 or later detected\n");
		mci->version = SD_VERSION_2;
	}

	return 0;
}

static int mci_blk_part_switch(struct mci_part *part)
{
	struct mci *mci = part->mci;
	int ret;

	if (!IS_ENABLED(CONFIG_MCI_MMC_BOOT_PARTITIONS))
		return 0;

	if (mci->part_curr == part)
		return 0;

	if (!IS_SD(mci)) {
		u8 part_config = mci->ext_csd_part_config;

		part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		part_config |= part->part_cfg;

		ret = mci_switch(mci, EXT_CSD_PARTITION_CONFIG, part_config);
		if (ret)
			return ret;

		mci->ext_csd_part_config = part_config;
	}

	mci->part_curr = part;

	return 0;
}

/* ------------------ attach to the blocklayer --------------------------- */

/**
 * Write a chunk of sectors to media
 * @param blk All info about the block device we need
 * @param buffer Buffer to write from
 * @param block Sector's number to start write to
 * @param num_blocks Sector count to write
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to read all data!
 */
static int __maybe_unused mci_sd_write(struct block_device *blk,
				const void *buffer, int block, int num_blocks)
{
	struct mci_part *part = container_of(blk, struct mci_part, blk);
	struct mci *mci = part->mci;
	struct mci_host *host = mci->host;
	int rc;
	unsigned max_req_block = num_blocks;
	int write_block;

	if (mci->host->max_req_size)
		max_req_block = mci->host->max_req_size / mci->write_bl_len;

	mci_blk_part_switch(part);

	if (host->card_write_protected && host->card_write_protected(host)) {
		dev_err(&mci->dev, "card write protected\n");
		return -EPERM;
	}

	dev_dbg(&mci->dev, "%s: Write %d block(s), starting at %d\n",
		__func__, num_blocks, block);

	if (mci->write_bl_len != SECTOR_SIZE) {
		dev_dbg(&mci->dev, "MMC/SD block size is not %d bytes (its %u bytes instead)\n",
				SECTOR_SIZE, mci->read_bl_len);
		return -EINVAL;
	}

	/* size of the block number field in the MMC/SD command is 32 bit only */
	if (block > MAX_BUFFER_NUMBER) {
		dev_dbg(&mci->dev, "Cannot handle block number %d. Too large!\n", block);
		return -EINVAL;
	}

	while (num_blocks) {
		write_block = min_t(int, num_blocks, max_req_block);
		rc = mci_block_write(mci, buffer, block, write_block);
		if (rc != 0) {
			dev_dbg(&mci->dev, "Writing block %d failed with %d\n", block, rc);
			return rc;
		}
		num_blocks -= write_block;
		block += write_block;
		buffer += write_block * mci->write_bl_len;
	}

	return 0;
}

/**
 * Read a chunk of sectors from the drive
 * @param blk All info about the block device we need
 * @param buffer Buffer to read into
 * @param block Sector's LBA number to start read from
 * @param num_blocks Sector count to read
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to store all data!
 */
static int mci_sd_read(struct block_device *blk, void *buffer, int block,
				int num_blocks)
{
	struct mci_part *part = container_of(blk, struct mci_part, blk);
	struct mci *mci = part->mci;
	unsigned max_req_block = num_blocks;
	int read_block;
	int rc;

	if (mci->host->max_req_size)
		max_req_block = mci->host->max_req_size / mci->read_bl_len;

	mci_blk_part_switch(part);

	dev_dbg(&mci->dev, "%s: Read %d block(s), starting at %d\n",
		__func__, num_blocks, block);

	if (mci->read_bl_len != 512) {
		dev_dbg(&mci->dev, "MMC/SD block size is not 512 bytes (its %u bytes instead)\n",
				mci->read_bl_len);
		return -EINVAL;
	}

	if (block > MAX_BUFFER_NUMBER) {
		dev_err(&mci->dev, "Cannot handle block number %d. Too large!\n", block);
		return -EINVAL;
	}

	while (num_blocks) {
		read_block = min_t(int, num_blocks, max_req_block);
		rc = mci_read_block(mci, buffer, block, read_block);
		if (rc != 0) {
			dev_dbg(&mci->dev, "Reading block %d failed with %d\n", block, rc);
			return rc;
		}
		num_blocks -= read_block;
		block += read_block;
		buffer += read_block * mci->read_bl_len;
	}

	return 0;
}

/* ------------------ attach to the device API --------------------------- */

/**
 * Extract the Manufacturer ID from the CID
 * @param mci Instance data
 *
 * The 'MID' is encoded in bit 127:120 in the CID
 */
static unsigned extract_mid(struct mci *mci)
{
	if (!IS_SD(mci) && mci->version <= MMC_VERSION_1_4)
		return UNSTUFF_BITS(mci->cid, 104, 24);
	else
		return UNSTUFF_BITS(mci->cid, 120, 8);
}

/**
 * Extract the OEM/Application ID from the CID
 * @param mci Instance data
 *
 * The 'OID' is encoded in bit 119:104 in the CID
 */
static unsigned extract_oid(struct mci *mci)
{
	return (mci->cid[0] >> 8) & 0xffff;
}

/**
 * Extract the product revision from the CID
 * @param mci Instance data
 *
 * The 'PRV' is encoded in bit 63:56 in the CID
 */
static unsigned extract_prv(struct mci *mci)
{
	return mci->cid[2] >> 24;
}

/**
 * Extract the product serial number from the CID
 * @param mci Instance data
 *
 * The 'PSN' is encoded in bit 55:24 in the CID
 */
static unsigned extract_psn(struct mci *mci)
{
	if (IS_SD(mci)) {
		return UNSTUFF_BITS(mci->csd, 24, 32);
	} else {
		if (mci->version > MMC_VERSION_1_4)
			return UNSTUFF_BITS(mci->cid, 16, 32);
		else
			return UNSTUFF_BITS(mci->cid, 16, 24);
	}

}

/**
 * Extract the month of the manufacturing date from the CID
 * @param mci Instance data
 *
 * The 'MTD' is encoded in bit 19:8 in the CID, month in 11:8
 */
static unsigned extract_mtd_month(struct mci *mci)
{
	if (IS_SD(mci))
		return UNSTUFF_BITS(mci->cid, 8, 4);
	else
		return UNSTUFF_BITS(mci->cid, 12, 4);
}

/**
 * Extract the year of the manufacturing date from the CID
 * @param mci Instance data
 *
 * The 'MTD' is encoded in bit 19:8 in the CID, year in 19:12
 * An encoded 0 means the year 2000
 */
static unsigned extract_mtd_year(struct mci *mci)
{
	unsigned year;
	if (IS_SD(mci))
		year = UNSTUFF_BITS(mci->cid, 12, 8) + 2000;
	else if (mci->version < MMC_VERSION_4_41)
		return UNSTUFF_BITS(mci->cid, 8, 4) + 1997;
	else {
		year = UNSTUFF_BITS(mci->cid, 8, 4) + 1997;
		if (year < 2010)
			year += 16;
	}
	return year;
}

static void mci_print_caps(unsigned caps)
{
	printf("  capabilities: %s%s%s%s%s\n",
		caps & MMC_CAP_4_BIT_DATA ? "4bit " : "",
		caps & MMC_CAP_8_BIT_DATA ? "8bit " : "",
		caps & MMC_CAP_SD_HIGHSPEED ? "sd-hs " : "",
		caps & MMC_CAP_MMC_HIGHSPEED ? "mmc-hs " : "",
		caps & MMC_CAP_MMC_HIGHSPEED_52MHZ ? "mmc-52MHz " : "");
}

/**
 * Output some valuable information when the user runs 'devinfo' on an MCI device
 * @param mci MCI device instance
 */
static void mci_info(struct device_d *dev)
{
	struct mci *mci = container_of(dev, struct mci, dev);
	struct mci_host *host = mci->host;
	int bw;

	if (mci->ready_for_use == 0) {
		printf(" No information available:\n  MCI card not probed yet\n");
		return;
	}

	printf("Host information:\n");
	printf("  current clock: %d\n", host->clock);

	if (host->bus_width == MMC_BUS_WIDTH_8)
		bw = 8;
	else if (host->bus_width == MMC_BUS_WIDTH_4)
		bw = 4;
	else
		bw = 1;

	printf("  current buswidth: %d\n", bw);
	mci_print_caps(host->host_caps);

	printf("Card information:\n");
	printf("  Attached is a %s card\n", IS_SD(mci) ? "SD" : "MMC");
	printf("  Version: %s\n", mci_version_string(mci));
	printf("  Capacity: %u MiB\n", (unsigned)(mci->capacity >> 20));

	if (mci->high_capacity)
		printf("  High capacity card\n");
	printf("   CID: %08X-%08X-%08X-%08X\n", mci->cid[0], mci->cid[1],
		mci->cid[2], mci->cid[3]);
	printf("   CSD: %08X-%08X-%08X-%08X\n", mci->csd[0], mci->csd[1],
		mci->csd[2], mci->csd[3]);
	printf("  Max. transfer speed: %u Hz\n", mci->tran_speed);
	mci_print_caps(mci->card_caps);
	printf("  Manufacturer ID: %02X\n", extract_mid(mci));
	printf("  OEM/Application ID: %04X\n", extract_oid(mci));
	printf("  Product name: '%c%c%c%c%c'\n", mci->cid[0] & 0xff,
		(mci->cid[1] >> 24), (mci->cid[1] >> 16) & 0xff,
		(mci->cid[1] >> 8) & 0xff, mci->cid[1] & 0xff);
	printf("  Product revision: %u.%u\n", extract_prv(mci) >> 4,
		extract_prv(mci) & 0xf);
	printf("  Serial no: %0u\n", extract_psn(mci));
	printf("  Manufacturing date: %u.%u\n", extract_mtd_month(mci),
		extract_mtd_year(mci));
}

/**
 * Check if the MCI card is already probed
 * @param mci MCI device instance
 * @return 0 when not probed yet, -EPERM if already probed
 *
 * @a barebox cannot really cope with hot plugging. So, probing an attached
 * MCI card is a one time only job. If its already done, there is no way to
 * return.
 */
static int mci_check_if_already_initialized(struct mci *mci)
{
	if (mci->ready_for_use != 0)
		return -EPERM;

	return 0;
}

static struct block_device_ops mci_ops = {
	.read = mci_sd_read,
#ifdef CONFIG_BLOCK_WRITE
	.write = mci_sd_write,
#endif
};

static int mci_set_boot(struct param_d *param, void *priv)
{
	struct mci *mci = priv;

	mci->ext_csd_part_config &= ~(7 << 3);
	mci->ext_csd_part_config |= mci->bootpart << 3;

	return mci_switch(mci,
			  EXT_CSD_PARTITION_CONFIG, mci->ext_csd_part_config);
}

static const char *mci_boot_names[] = {
	"disabled",
	"boot0",
	"boot1",
	NULL, /* reserved */
	NULL, /* reserved */
	NULL, /* reserved */
	NULL, /* reserved */
	"user",
};

static int mci_register_partition(struct mci_part *part)
{
	struct mci *mci = part->mci;
	struct mci_host *host = mci->host;
	const char *partnodename = NULL;
	struct device_node *np;
	int rc;

	/*
	 * An MMC/SD card acts like an ordinary disk.
	 * So, re-use the disk driver to gain access to this media
	 */
	part->blk.dev = &mci->dev;
	part->blk.ops = &mci_ops;

	rc = blockdevice_register(&part->blk);
	if (rc != 0) {
		dev_err(&mci->dev, "Failed to register MCI/SD blockdevice\n");
		return rc;
	}
	dev_info(&mci->dev, "registered %s\n", part->blk.cdev.name);

	np = host->hw_dev->device_node;

	/* create partitions on demand */
	switch (part->area_type) {
	case MMC_BLK_DATA_AREA_BOOT:
		if (part->idx == 0)
			partnodename = "boot0-partitions";
		else
			partnodename = "boot1-partitions";

		np = of_get_child_by_name(host->hw_dev->device_node,
					  partnodename);
		break;
	case MMC_BLK_DATA_AREA_MAIN:
		break;
	default:
		return 0;
	}

	rc = parse_partition_table(&part->blk);
	if (rc != 0) {
		dev_warn(&mci->dev, "No partition table found\n");
		rc = 0; /* it's not a failure */
	}

	if (np) {
		of_parse_partitions(&part->blk.cdev, np);

		/* bootN-partitions binding barebox-specific, so don't register
		 * for fixup into kernel device tree
		 */
		if (part->area_type != MMC_BLK_DATA_AREA_BOOT)
			of_partitions_register_fixup(&part->blk.cdev);
	}

	return 0;
}

/**
 * Probe an MCI card at the given host interface
 * @param mci MCI device instance
 * @return 0 on success, negative values else
 */
static int mci_card_probe(struct mci *mci)
{
	struct mci_host *host = mci->host;
	int i, rc, disknum, ret;

	if (host->card_present && !host->card_present(host) &&
	    !host->non_removable) {
		dev_err(&mci->dev, "no card inserted\n");
		return -ENODEV;
	}

	ret = regulator_enable(host->supply);
	if (ret) {
		dev_err(&mci->dev, "failed to enable regulator: %s\n",
			strerror(-ret));
		return ret;
	}

	/* start with a host interface reset */
	rc = (host->init)(host, &mci->dev);
	if (rc) {
		dev_err(&mci->dev, "Cannot reset the SD/MMC interface\n");
		goto on_error;
	}

	mci_set_bus_width(mci, MMC_BUS_WIDTH_1);
	/* according to the SD card spec the detection can happen at 400 kHz */
	mci_set_clock(mci, 400000);

	/* reset the card */
	rc = mci_go_idle(mci);
	if (rc) {
		dev_warn(&mci->dev, "Cannot reset the SD/MMC card\n");
		goto on_error;
	}

	/* Check if this card can handle the "SD Card Physical Layer Specification 2.0" */
	if (!host->no_sd) {
		rc = sd_send_if_cond(mci);
		rc = sd_send_op_cond(mci);
	}
	if (host->no_sd || rc == -ETIMEDOUT) {
		/* If SD card initialization was skipped or if it timed out,
		 * we check for an MMC card */
		dev_dbg(&mci->dev, "Card seems to be a MultiMediaCard\n");
		rc = mmc_send_op_cond(mci);
	}

	if (rc)
		goto on_error;

	if (host->devname) {
		mci->cdevname = strdup(host->devname);
	} else {
		disknum = cdev_find_free_index("disk");
		mci->cdevname = basprintf("disk%d", disknum);
	}

	rc = mci_startup(mci);
	if (rc) {
		dev_warn(&mci->dev, "Card's startup fails with %d\n", rc);
		goto on_error;
	}

	dev_dbg(&mci->dev, "Card is up and running now, registering as a disk\n");
	mci->ready_for_use = 1;	/* TODO now or later? */

	for (i = 0; i < mci->nr_parts; i++) {
		struct mci_part *part = &mci->part[i];

		rc = mci_register_partition(part);

		if (IS_ENABLED(CONFIG_MCI_MMC_BOOT_PARTITIONS) &&
				part->area_type == MMC_BLK_DATA_AREA_BOOT &&
				!mci->param_boot) {
			mci->param_boot = dev_add_param_enum(&mci->dev, "boot",
					mci_set_boot, NULL, &mci->bootpart,
					mci_boot_names, ARRAY_SIZE(mci_boot_names), mci);
		}
	}

	dev_dbg(&mci->dev, "SD Card successfully added\n");

on_error:
	if (rc != 0) {
		host->clock = 0;	/* disable the MCI clock */
		mci_set_ios(mci);
		regulator_disable(host->supply);
	}

	return rc;
}

/**
 * Trigger probing of an attached MCI card
 * @param mci_dev MCI device instance
 * @param param FIXME
 * @param val "0" does nothing, a "1" will probe for a MCI card
 * @return 0 on success
 */
static int mci_set_probe(struct param_d *param, void *priv)
{
	struct mci *mci = priv;
	int rc;

	if (!mci->probe)
		return 0;

	rc = mci_check_if_already_initialized(mci);
	if (rc != 0)
		return 0;

	rc = mci_card_probe(mci);
	if (rc != 0)
		return rc;

	return 0;
}

static int mci_init(void)
{
	sector_buf = xmemalign(32, 512);

	return 0;
}

device_initcall(mci_init);

int mci_detect_card(struct mci_host *host)
{
	int rc;

	rc = mci_check_if_already_initialized(host->mci);
	if (rc != 0)
		return 0;

	return mci_card_probe(host->mci);
}

static int mci_detect(struct device_d *dev)
{
	struct mci *mci = container_of(dev, struct mci, dev);

	return mci_detect_card(mci->host);
}

/**
 * Create a new mci device (for convenience)
 * @param host mci_host for this MCI device
 * @return 0 on success
 */
int mci_register(struct mci_host *host)
{
	struct mci *mci;
	int ret;

	mci = xzalloc(sizeof(*mci));
	mci->host = host;

	if (host->devname) {
		dev_set_name(&mci->dev, host->devname);
		mci->dev.id = DEVICE_ID_SINGLE;
	} else {
		dev_set_name(&mci->dev, "mci");
		mci->dev.id = DEVICE_ID_DYNAMIC;
	}

	mci->dev.platform_data = host;
	mci->dev.parent = host->hw_dev;
	mci->host = host;
	host->mci = mci;
	mci->dev.detect = mci_detect;

	host->supply = regulator_get(host->hw_dev, "vmmc");
	if (IS_ERR(host->supply)) {
		if (host->supply == ERR_PTR(-EPROBE_DEFER)) {
			ret = -EPROBE_DEFER;
			goto err_free;
		}
		dev_err(&mci->dev, "Failed to get 'vmmc' regulator.\n");
		host->supply = NULL;
	}

	ret = register_device(&mci->dev);
	if (ret)
		goto err_free;

	dev_info(mci->host->hw_dev, "registered as %s\n", dev_name(&mci->dev));

	mci->param_probe = dev_add_param_bool(&mci->dev, "probe",
			mci_set_probe, NULL, &mci->probe, mci);

	if (IS_ERR(mci->param_probe) && PTR_ERR(mci->param_probe) != -ENOSYS) {
		ret = PTR_ERR(mci->param_probe);
		dev_dbg(&mci->dev, "Failed to add 'probe' parameter to the MCI device\n");
		goto err_unregister;
	}

	if (IS_ENABLED(CONFIG_MCI_INFO))
		mci->dev.info = mci_info;

	/* if enabled, probe the attached card immediately */
	if (IS_ENABLED(CONFIG_MCI_STARTUP))
		mci_card_probe(mci);

	list_add_tail(&mci->list, &mci_list);

	return 0;

err_unregister:
	unregister_device(&mci->dev);
err_free:
	free(mci);

	return ret;
}

void mci_of_parse_node(struct mci_host *host,
		       struct device_node *np)
{
	u32 bus_width;
	u32 dsr_val;
	const char *alias;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return;

	if (!host->hw_dev || !np)
		return;

	alias = of_alias_get(np);
	if (alias)
		host->devname = xstrdup(alias);

	/* "bus-width" is translated to MMC_CAP_*_BIT_DATA flags */
	if (of_property_read_u32(np, "bus-width", &bus_width) < 0) {
		/* If bus-width is missing we get the driver's default, which
		 * is, unfortunately, not consistent from driver to driver.
		 * Better to specify it in the device tree.  */
		dev_dbg(host->hw_dev,
			"\"bus-width\" property missing, default is %d\n",
			(host->host_caps & MMC_CAP_8_BIT_DATA) ? 8 :
			(host->host_caps & MMC_CAP_4_BIT_DATA) ? 4 : 1);
	} else {
		/* Set data width caps to exactly those specified in the DT.
		 * bus-width isn't a list, so widths smaller than the specified
		 * value are implictly supported as well.  */
		host->host_caps &= ~MMC_CAP_BIT_DATA_MASK;
		switch (bus_width) {
		case 8:
			host->host_caps |= MMC_CAP_8_BIT_DATA;
		case 4: /* note fall through from above */
			host->host_caps |= MMC_CAP_4_BIT_DATA;
		case 1:
			break;
		default:
			dev_err(host->hw_dev,
				"Invalid \"bus-width\" value %u!\n", bus_width);
		}
	}

	/* f_max is obtained from the optional "max-frequency" property */
	of_property_read_u32(np, "max-frequency", &host->f_max);

	if (!of_property_read_u32(np, "dsr", &dsr_val)) {
		if (dsr_val < 0x10000) {
			host->use_dsr = 1;
			host->dsr_val = dsr_val;
		}
	}

	host->non_removable = of_property_read_bool(np, "non-removable");
	host->no_sd = of_property_read_bool(np, "no-sd");
}

void mci_of_parse(struct mci_host *host)
{
	return mci_of_parse_node(host, host->hw_dev->device_node);
}

struct mci *mci_get_device_by_name(const char *name)
{
	struct mci *mci;

	list_for_each_entry(mci, &mci_list, list) {
		if (!mci->cdevname)
			continue;
		if (!strcmp(mci->cdevname, name))
			return mci;
	}

	return NULL;
}
