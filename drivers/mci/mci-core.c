// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2010 Juergen Beisert, Pengutronix
// SPDX-FileCopyrightText: 2008 Freescale Semiconductor, Inc

/*
 * This code is heavily inspired and in parts from the u-boot project which is
 * based vaguely on the Linux code
 */

/* #define DEBUG */

#include <init.h>
#include <common.h>
#include <mci.h>
#include <malloc.h>
#include <errno.h>
#include <linux/math64.h>
#include <asm/byteorder.h>
#include <block.h>
#include <disks.h>
#include <of.h>
#include <linux/err.h>
#include <linux/log2.h>
#include <linux/sizes.h>
#include <dma.h>

#define MAX_BUFFER_NUMBER 0xffffffff

static inline u32 unstuff_bits(const u32 *resp, int start, int size)
{
	const int __size = size;
	const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;
	const int __off = 3 - (start / 32);
	const int __shft = start & 31;
	u32 __res = resp[__off] >> __shft;

	if (__size + __shft > 32)
		__res |= resp[__off - 1] << ((32 - __shft) % 32);

	return __res & __mask;
}

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

	return host->ops.send_cmd(mci->host, cmd, data);
}

/**
 * mci_send_cmd_retry() - send a command to the mmc device, retrying on error
 *
 * @dev:	device to receive the command
 * @cmd:	command to send
 * @data:	additional data to send/receive
 * @retries:	how many times to retry; mci_send_cmd is always called at least
 *              once
 * Return: 0 if ok, -ve on error
 */
static int mci_send_cmd_retry(struct mci *mci, struct mci_cmd *cmd,
			      struct mci_data *data, unsigned retries)
{
	int ret;

	do
		ret = mci_send_cmd(mci, cmd, data);
	while (ret && retries--);

	return ret;
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
	struct mci_cmd cmd = {};

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
	struct mci_cmd cmd = {};

	if (mci->host->timing == MMC_TIMING_MMC_DDR52)
		return 0;

	mci_setup_cmd(&cmd, MMC_CMD_SET_BLOCKLEN, len, MMC_RSP_R1);
	return mci_send_cmd(mci, &cmd, NULL);
}

static void *sector_buf;

static int mci_send_status(struct mci *mci, unsigned int *status)
{
	struct mci_host *host = mci->host;
	struct mci_cmd cmd = {};
	int ret;

	/*
	 * While CMD13 is defined for SPI mode, the reported status bits have
	 * different layout that SD/MMC. We skip supporting this for now.
	 */
	if (mmc_host_is_spi(host))
		return -ENOSYS;

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mci->rca << 16;

	ret = mci_send_cmd_retry(mci, &cmd, NULL, 4);
	if (!ret)
		*status = cmd.response[0];

	return ret;
}

static int mci_app_sd_status(struct mci *mci, __be32 *ssr)
{
	int err;
	struct mci_cmd cmd = {};
	struct mci_data data;

	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mci->rca << 16;

	err = mci_send_cmd_retry(mci, &cmd, NULL, 4);
	if (err)
		return err;

	cmd.cmdidx = SD_CMD_APP_SD_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	data.dest = (u8 *)ssr;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	return mci_send_cmd_retry(mci, &cmd, &data, 3);
}

static int mmc_switch_status_error(struct mci_host *host, u32 status)
{
	if (mmc_host_is_spi(host)) {
		if (status & R1_SPI_ILLEGAL_COMMAND)
			return -EBADMSG;
	} else {
		if (R1_STATUS(status))
			pr_warn("unexpected status %#x after switch\n", status);
		if (status & R1_SWITCH_ERROR)
			return -EBADMSG;
	}
	return 0;
}

/* Caller must hold re-tuning */
int mci_switch_status(struct mci *mci, bool crc_err_fatal)
{
	u32 status;
	int err;

	err = mci_send_status(mci, &status);
	if (!crc_err_fatal && err == -EILSEQ)
		return 0;
	if (err)
		return err;

	return mmc_switch_status_error(mci->host, status);
}

static int mci_poll_until_ready(struct mci *mci, int timeout_ms)
{
	unsigned int status;
	int err, retries = 0;

	while (1) {
		err = mci_send_status(mci, &status);
		if (err)
			return err;

		/*
		 * Some cards mishandle the status bits, so make sure to
		 * check both the busy indication and the card state.
		 */
		if ((status & R1_READY_FOR_DATA) &&
		    R1_CURRENT_STATE(status) != R1_STATE_PRG)
			break;

		if (status & R1_STATUS_MASK) {
			dev_err(&mci->dev, "Status Error: 0x%08x\n", status);
			return -EIO;
		}

		if (retries++ == timeout_ms) {
			dev_err(&mci->dev, "Timeout awaiting card ready\n");
			return -ETIMEDOUT;
		}

		udelay(1000);
	}

	dev_dbg(&mci->dev, "Ready polling took %ums\n", retries);

	return 0;
}

/**
 * Erase one or several blocks of data to the card
 * @param mci_dev MCI instance
 * @param from Block number to start erasing from
 * @param number of blocks to erase
 * @return Transaction status (0 on success)
 */
static int mci_block_erase(struct mci *card, unsigned int from,
			   unsigned int blkcnt, unsigned int arg)
{
	unsigned int to = from + blkcnt - 1;
	struct mci_cmd cmd = {};
	int err;

	if (!card->high_capacity) {
		from *= card->write_bl_len;
		to *= card->write_bl_len;
	}

	cmd.cmdidx = IS_SD(card) ? SD_ERASE_WR_BLK_START : MMC_ERASE_GROUP_START;
	cmd.cmdarg = from;
	cmd.resp_type = MMC_RSP_R1;
	err = mci_send_cmd(card, &cmd, NULL);
	if (err)
		goto err_out;

	memset(&cmd, 0, sizeof(struct mci_cmd));
	cmd.cmdidx = IS_SD(card) ? SD_ERASE_WR_BLK_END : MMC_ERASE_GROUP_END;
	cmd.cmdarg = to;
	cmd.resp_type = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mci_send_cmd(card, &cmd, NULL);
	if (err)
		goto err_out;

	memset(&cmd, 0, sizeof(struct mci_cmd));
	cmd.cmdidx = MMC_ERASE;
	cmd.cmdarg = arg;
	cmd.resp_type = MMC_RSP_R1b;

	err = mci_send_cmd(card, &cmd, NULL);
	if (err)
		goto err_out;

	return 0;

err_out:
	dev_err(&card->dev, "erase cmd %d error %d, status %#x\n",
		cmd.cmdidx, err, cmd.response[0]);
	return -EIO;
}

static int mci_do_block_op(struct mci *mci, const void *src, void *dst, int blocknum,
		int blocks)
{
	struct mci_cmd cmd = {};
	struct mci_data data;
	int ret;
	unsigned mmccmd_multi_block, mmccmd_single_block, mmccmd;
	unsigned int flags;

	if (dst) {
		mmccmd_multi_block = MMC_CMD_READ_MULTIPLE_BLOCK;
		mmccmd_single_block = MMC_CMD_READ_SINGLE_BLOCK;
		flags = MMC_DATA_READ;
	} else {
		/*
		 * Quoting eMMC Spec v5.1 (JEDEC Standard No. 84-B51):
		 * Due to legacy reasons, a Device may still treat CMD24/25 during
		 * prg-state (while busy is active) as a legal or illegal command.
		 * A host should not send CMD24/25 while the Device is in the prg
		 * state and busy is active.
		 */
		ret = mci_poll_until_ready(mci, 1000 /* ms */);
		if (ret && ret != -ENOSYS)
			return ret;

		mmccmd_multi_block = MMC_CMD_WRITE_MULTIPLE_BLOCK;
		mmccmd_single_block = MMC_CMD_WRITE_SINGLE_BLOCK;
		flags = MMC_DATA_WRITE;
	}

	if (blocks > 1)
		mmccmd = mmccmd_multi_block;
	else
		mmccmd = mmccmd_single_block;

	mci_setup_cmd(&cmd,
		mmccmd,
		mci->high_capacity != 0 ? blocknum : blocknum * mci->read_bl_len,
		MMC_RSP_R1);

	data.src = src;
	data.dest = dst;
	data.blocks = blocks;
	data.blocksize = mci->read_bl_len;
	data.flags = flags;

	ret = mci_send_cmd(mci, &cmd, &data);

	if (ret || blocks > 1) {
		mci_setup_cmd(&cmd, MMC_CMD_STOP_TRANSMISSION, 0,
			      IS_SD(mci) ? MMC_RSP_R1b : MMC_RSP_R1);
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
static int mci_block_read(struct mci *mci, void *dst, int blocknum,
		int blocks)
{
	return mci_do_block_op(mci, NULL, dst, blocknum, blocks);
}

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
	return mci_do_block_op(mci, src, NULL, blocknum, blocks);
}

/**
 * Reset the attached MMC/SD card
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int mci_go_idle(struct mci *mci)
{
	struct mci_cmd cmd = {};
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

static int sdio_send_op_cond(struct mci *mci)
{
	struct mci_cmd cmd = {};

	mci_setup_cmd(&cmd, SD_IO_SEND_OP_COND, 0, MMC_RSP_SPI_R4 | MMC_RSP_R4 | MMC_CMD_BCR);

	return mci_send_cmd(mci, &cmd, NULL);
}

/**
 * FIXME
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int sd_send_op_cond(struct mci *mci)
{
	struct mci_host *host = mci->host;
	struct mci_cmd cmd = {};
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
	struct mci_cmd cmd = {};
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
	mci->rca = 2;

	return 0;
}

/**
 * Read-in the card's whole extended CSD configuration area
 * @param[in] mci MCI instance
 * @param[out] ext_csd Buffer for an #EXT_CSD_BLOCKSIZE byte sized extended CSD
 * @return Transaction status (0 on success)
 *
 * Note: Only cards newer than Version 1.1 (Physical Layer Spec) support
 * this command
 */
int mci_send_ext_csd(struct mci *mci, char *ext_csd)
{
	struct mci_cmd cmd = {};
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
 * Write a byte into the card's extended CSD configuration area
 * @param[in] mci MCI instance
 * @param[in] index Byte index in the extended CSD configuration area
 * @param[in] value Byte to write at index into the extended CSD configuration area
 * @return Transaction status (0 on success)
 *
 * This sends a CMD6 (aka SWITCH) to the card and writes @b value at extended CSD @b index.
 *
 * @note It always writes a full byte, the alternatives 'bit set' and
 *      'bit clear' aren't supported.
 */
int mci_switch(struct mci *mci, unsigned index, unsigned value)
{
	unsigned int status;
	struct mci_cmd cmd = {};
	int ret;

	mci_setup_cmd(&cmd, MMC_CMD_SWITCH,
		(MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		(index << 16) |
		(value << 8),
		 MMC_RSP_R1b);

	ret = mci_send_cmd(mci, &cmd, NULL);
	if (ret)
		return ret;

	ret = mci_send_status(mci, &status);
	if (ret)
		return ret;

	if (status & R1_SWITCH_ERROR)
		return -EIO;

	return 0;
}

u8 *mci_get_ext_csd(struct mci *mci)
{
	u8 *ext_csd;
	int ret;

	ext_csd = dma_alloc(512);

	ret = mci_send_ext_csd(mci, ext_csd);
	if (ret) {
		printf("Failure to read EXT_CSD register\n");
		dma_free(ext_csd);
		return ERR_PTR(-EIO);
	}

	return ext_csd;
}

static blkcnt_t mci_calc_blk_cnt(blkcnt_t cap, unsigned shift)
{
	blkcnt_t ret = cap >> shift;

	if (ret > 0x7fffffff) {
		pr_warn("Limiting card size due to 31 bit contraints\n");
		return 0x7fffffff;
	}

	return ret;
}

static void mci_part_add(struct mci *mci, uint64_t size,
                        unsigned int part_cfg, char *name, char *partname, int idx, bool ro,
                        int area_type)
{
	struct mci_part *part;

	if (mci->nr_parts == MMC_NUM_PHY_PARTITION) {
		dev_err(&mci->dev, "Out of physical partitions, cannot create partition %s\n",
			name);
		return;
	}

	part = &mci->part[mci->nr_parts];

	part->mci = mci;
	part->size = size;
	part->blk.cdev.name = name;
	part->blk.cdev.partname = partname;
	part->blk.blockbits = SECTOR_SHIFT;
	part->blk.num_blocks = mci_calc_blk_cnt(size, part->blk.blockbits);
	part->area_type = area_type;
	part->part_cfg = part_cfg;
	part->idx = idx;

	if (area_type == MMC_BLK_DATA_AREA_MAIN) {
		cdev_set_of_node(&part->blk.cdev, mci->host->hw_dev->of_node);
		part->blk.cdev.flags |= DEVFS_IS_MCI_MAIN_PART_DEV;
	}

	mci->nr_parts++;
}

/**
 * Read a value spread to three consecutive bytes in the ECSD information
 * @param[in] ecsd_info Information from the eMMC
 * @param[in] idx The index where to start to read
 * @return The GPP size in units of 'write protect group' size
 *
 * The value in the ECSD information block is meant in little endian
 */
static __maybe_unused unsigned mmc_extract_gpp_units(const char *ecsd_info, unsigned idx)
{
	unsigned val;

	val = ecsd_info[idx];
	val |= ecsd_info[idx + 1] << 8;
	val |= ecsd_info[idx + 2] << 16;

	return val;
}

/**
 * Create and enable access to 'general purpose hardware partitions' on demand
 * @param mci[in,out] MCI instance
 *
 * General Purpose hardware Partitions (aka GPPs) aren't enabled by default. Its
 * up to the application to (one-time) setup the eMMC to provide GPPs. Since
 * they aren't wildly used, enable access to them on demand only.
 */
static __maybe_unused void mmc_extract_gpp_partitions(struct mci *mci)
{
	uint64_t wpgs, part_size;
	size_t idx;
	char *name, *partname;
	static const unsigned gpp_offsets[MMC_NUM_GP_PARTITION] = {
		EXT_CSD_GP_SIZE_MULT0, EXT_CSD_GP_SIZE_MULT1,
		EXT_CSD_GP_SIZE_MULT2, EXT_CSD_GP_SIZE_MULT3, };

	if (!(mci->ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & 0x01))
		return; /* no partitioning support */
	/*
	 * The size of GPPs is defined in units of 'write protect group' size.
	 * The 'write protect group' size is defined to:
	 *  CSD_HC_ERASE_GRP_SIZE * CSD_HC_WP_GRP_SIZE * 512 kiB
	 */
	wpgs = mci->ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
	wpgs *= mci->ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
	wpgs *= SZ_512K;

	/* up to four GPPs can be enabled. */
	for (idx = 0; idx < ARRAY_SIZE(gpp_offsets); idx++) {
		part_size = mmc_extract_gpp_units(mci->ext_csd, gpp_offsets[idx]);
		if (part_size == 0)
			continue;
		/* Convert to bytes */
		part_size *= wpgs;

		partname = xasprintf("gpp%zu", idx);
		name = xasprintf("%s.%s", mci->cdevname, partname);
		/* TODO read-only flag */
		mci_part_add(mci, part_size, EXT_CSD_PART_CONFIG_ACC_GPP0 + idx,
			     name, partname, idx, false, MMC_BLK_DATA_AREA_GP);
	}
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

	mci->ext_csd = dma_alloc(512);
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

	err = mci_switch(mci, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);

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

	mci->card_caps |= MMC_CAP_MMC_HIGHSPEED;

	/* High Speed is set, there are three types: 26MHz, 52MHz, 52MHz DDR */
	if (cardtype & EXT_CSD_CARD_TYPE_52) {
		mci->card_caps |= MMC_CAP_MMC_HIGHSPEED_52MHZ;

		if (cardtype & EXT_CSD_CARD_TYPE_DDR_1_8V)
			mci->card_caps |= MMC_CAP_MMC_3_3V_DDR | MMC_CAP_MMC_1_8V_DDR;
	}

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
		mci->boot_ack_enable = (mci->ext_csd_part_config >> 6) & 0x1;
	}

	if (IS_ENABLED(CONFIG_MCI_MMC_GPP_PARTITIONS))
		mmc_extract_gpp_partitions(mci);

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
	struct mci_cmd cmd = {};
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

static int sd_read_ssr(struct mci *mci)
{
	static const unsigned int sd_au_size[] = {
		0,		SZ_16K / 512,		SZ_32K / 512,
		SZ_64K / 512,	SZ_128K / 512,		SZ_256K / 512,
		SZ_512K / 512,	SZ_1M / 512,		SZ_2M / 512,
		SZ_4M / 512,	SZ_8M / 512,		(SZ_8M + SZ_4M) / 512,
		SZ_16M / 512,	(SZ_16M + SZ_8M) / 512,	SZ_32M / 512,
		SZ_64M / 512,
	};
	__be32 *ssr;
	int err;
	unsigned int au, eo, et, es;

	if (!IS_ENABLED(CONFIG_MCI_ERASE))
		return -ENOSYS;

	ssr = dma_alloc(64);

	err = mci_app_sd_status(mci, ssr);
	if (err)
		goto out;

	au = (be32_to_cpu(ssr[2]) >> 12) & 0xF;
	if ((au <= 9) || (mci->version == SD_VERSION_3)) {
		mci->ssr.au = sd_au_size[au];
		es = (be32_to_cpu(ssr[3]) >> 24) & 0xFF;
		es |= (be32_to_cpu(ssr[2]) & 0xFF) << 8;
		et = (be32_to_cpu(ssr[3]) >> 18) & 0x3F;
		if (es && et) {
			eo = (be32_to_cpu(ssr[3]) >> 16) & 0x3;
			mci->ssr.erase_timeout = (et * 1000) / es;
			mci->ssr.erase_offset = eo * 1000;
		}
	} else {
		dev_dbg(&mci->dev, "invalid allocation unit size.\n");
	}

out:
	dma_free(ssr);
	return err;
}

/**
 * Change transfer frequency for an SD card
 * @param mci MCI instance
 * @return Transaction status (0 on success)
 */
static int sd_change_freq(struct mci *mci)
{
	struct mci_cmd cmd = {};
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

	if (mci->scr[0] & SD_DATA_STAT_AFTER_ERASE)
		mci->erased_byte = 0xFF;
	else
		mci->erased_byte = 0x0;

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
	struct mci_ios ios = {
		.bus_width = host->bus_width,
		.clock = host->clock,
		.timing = host->timing,
	};

	host->ops.set_ios(host, &ios);

	host->actual_clock = host->clock;
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
static void mci_set_bus_width(struct mci *mci, enum mci_bus_width width)
{
	struct mci_host *host = mci->host;

	host->bus_width = width;	/* the new target bus width */
	mci_set_ios(mci);
}

/**
 * Extract card's version from its CSD
 * @param mci MCI instance
 */
static void mci_detect_version_from_csd(struct mci *mci)
{
	int version;

	if (mci->version == MMC_VERSION_UNKNOWN) {
		/* this should only apply to MMC card, JESD84-B51 defines
		 * bits 125:122 as SPEC_VER (reserved bits in CSD) */
		version = (mci->csd[0] >> 26) & 0xf;

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
	mci->read_bl_len = 1 << unstuff_bits(mci->csd, 80, 4);

	/* Quoting Physical Layer Simplified Specification Version 9.10:
	 * Note that in an SD Memory Card the WRITE_BL_LEN is always
	 * equal to READ_BL_LEN.
	 */
	if (IS_SD(mci))
		mci->write_bl_len = mci->read_bl_len;
	else
		mci->write_bl_len = 1 << ((mci->csd[3] >> 22) & 0xf);

	dev_dbg(&mci->dev, "Max. block length are: Write=%u, Read=%u Bytes\n",
		mci->write_bl_len, mci->read_bl_len);
}

/**
 * Configure erase group size
 * @param mci MCI instance
 */
static void mci_configure_erase_group_size(struct mci *mci)
{
	/* Enable ERASE_GRP_DEF. This bit is lost after a reset or power off.
	 * This needs to happen even if we don't have erase support compiled-in.
	 */
	if (!IS_SD(mci) && mci->version >= MMC_VERSION_4_3) {
		int err;

		err = mci_switch(mci, EXT_CSD_ERASE_GROUP_DEF, 1);
		if (err)
			dev_warn(&mci->dev, "failed erase group switch\n");
		else
			mci->ext_csd[EXT_CSD_ERASE_GROUP_DEF] = 1;
	}

	if (!IS_ENABLED(CONFIG_MCI_ERASE) ||
	    !(unstuff_bits(mci->csd, 84, 12) & CCC_ERASE))
		return;


	if (IS_SD(mci)) {
		if (unstuff_bits(mci->csd, 126, 2) == 0) {
			unsigned int write_blkbits = unstuff_bits(mci->csd, 22, 4);

			if (unstuff_bits(mci->csd, 46, 1)) {
				mci->erase_grp_size = 1;
			} else if (write_blkbits >= 9) {
				mci->erase_grp_size = unstuff_bits(mci->csd, 39, 7) + 1;
				mci->erase_grp_size <<= write_blkbits - 9;
			}
		} else {
			/* For SD with csd_struct v1, erase group is always one sector */
			mci->erase_grp_size = 1;
		}
	} else {
		if (mci->ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 0x01) {
			/* Read out group size from ext_csd */
			mci->erase_grp_size = mci->ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 1024;
		} else {
			/* Calculate the group size from the csd value. */
			int erase_gsz, erase_gmul;

			erase_gsz = (mci->csd[2] & 0x00007c00) >> 10;
			erase_gmul = (mci->csd[2] & 0x000003e0) >> 5;
			mci->erase_grp_size = (erase_gsz + 1)
				* (erase_gmul + 1);
		}

		if (mci->ext_csd[EXT_CSD_ERASED_MEM_CONT])
			mci->erased_byte = 0xFF;
		else
			mci->erased_byte = 0x0;

		if (mci->ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT] & EXT_CSD_SEC_FEATURE_TRIM_EN)
			mci->can_trim = true;
	}

	dev_dbg(&mci->dev, "Erase group is %u sector(s). Trim %ssupported\n",
		mci->erase_grp_size, mci->can_trim ? "" : "not ");
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
			csize = unstuff_bits(mci->csd, 48, 22);
			mci->capacity = (1 + csize) << 10;
		} else {
			mci->capacity = mci->ext_csd[EXT_CSD_SEC_COUNT] << 0 |
				mci->ext_csd[EXT_CSD_SEC_COUNT + 1] << 8 |
				mci->ext_csd[EXT_CSD_SEC_COUNT + 2] << 16 |
				mci->ext_csd[EXT_CSD_SEC_COUNT + 3] << 24;
		}
	} else {
		cmult = unstuff_bits(mci->csd, 47, 3);
		csize = unstuff_bits(mci->csd, 62, 12);
		mci->capacity = (csize + 1) << (cmult + 2);
	}

	mci->capacity *= 1 << unstuff_bits(mci->csd, 80, 4);
	dev_dbg(&mci->dev, "Capacity: %u MiB\n", (unsigned)(mci->capacity >> 20));
}

/**
 * Extract card's DSR implementation state from CSD
 * @param mci MCI instance
 */
static void mci_extract_card_dsr_imp_from_csd(struct mci *mci)
{
	mci->dsr_imp = unstuff_bits(mci->csd, 76, 1);
}

static int mmc_compare_ext_csds(struct mci *mci, enum mci_bus_width bus_width)
{
	u8 *bw_ext_csd;
	int err;

	if (bus_width == MMC_BUS_WIDTH_1)
		return 0;

	bw_ext_csd = dma_alloc(512);
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
	dma_free(bw_ext_csd);
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
	struct mci_cmd cmd = {};
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

	if (mci->tran_speed > 25000000)
		mci->host->timing = MMC_TIMING_SD_HS;

	mci_set_clock(mci, mci->tran_speed);

	err = sd_read_ssr(mci);
	if (err)
		dev_dbg(&mci->dev, "unable to read ssr: %pe\n", ERR_PTR(err));

	return 0;
}

static u32 mci_bus_width_ext_csd_bits(enum mci_bus_width bus_width)
{
	switch (bus_width) {
	case MMC_BUS_WIDTH_8:
		return EXT_CSD_BUS_WIDTH_8;
	case MMC_BUS_WIDTH_4:
		return EXT_CSD_BUS_WIDTH_4;
	case MMC_BUS_WIDTH_1:
	default:
		return EXT_CSD_BUS_WIDTH_1;
	}
}

static int mci_mmc_try_bus_width(struct mci *mci, enum mci_bus_width bus_width,
				 enum mci_timing timing)
{
	u32 ext_csd_bits;
	int err;

	ext_csd_bits = mci_bus_width_ext_csd_bits(bus_width);

	if (mci_timing_is_ddr(timing))
		ext_csd_bits |= EXT_CSD_DDR_FLAG;

	err = mci_switch(mci, EXT_CSD_BUS_WIDTH, ext_csd_bits);
	if (err < 0)
		goto out;

	mci->host->timing = timing;
	mci_set_bus_width(mci, bus_width);

	switch (bus_width) {
		case MMC_BUS_WIDTH_8:
			mci->card_caps |= MMC_CAP_8_BIT_DATA;
			break;
		case MMC_BUS_WIDTH_4:
			mci->card_caps |= MMC_CAP_4_BIT_DATA;
			break;
		default:
			break;
	}

	err = mmc_compare_ext_csds(mci, bus_width);
	if (err < 0)
		goto out;

out:
	dev_dbg(&mci->dev, "Tried buswidth %u%s: %s\n", 1 << bus_width,
		mci_timing_is_ddr(timing) ? " (DDR)" : "", err ? "failed" : "OK");

	return err ?: bus_width;
}

static int mci_mmc_select_bus_width(struct mci *mci)
{
	struct mci_host *host = mci->host;
	int ret;
	int idx = 0;
	static enum mci_bus_width bus_widths[] = {
		MMC_BUS_WIDTH_4,
		MMC_BUS_WIDTH_8,
	};

	if (!(host->host_caps & (MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA)))
		return MMC_BUS_WIDTH_1;

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
		ret = mci_mmc_try_bus_width(mci, bus_widths[idx], host->timing);
		if (ret > 0)
			break;
	}

	return ret;
}

static int mci_mmc_select_hs_ddr(struct mci *mci)
{
	struct mci_host *host = mci->host;
	int ret;

	/*
	 * barebox MCI core does not change voltage, so we don't know here
	 * if we should check for the 1.8v or 3.3v mode. Until we support
	 * higher speed modes that require voltage switching like HS200/HS400,
	 * let's just check for either bit.
	 */
	if (!(mci_caps(mci) & (MMC_CAP_MMC_1_8V_DDR | MMC_CAP_MMC_3_3V_DDR)))
		return 0;

	ret = mci_mmc_try_bus_width(mci, host->bus_width, MMC_TIMING_MMC_DDR52);
	if (ret < 0)
		return mci_mmc_try_bus_width(mci, host->bus_width, MMC_TIMING_MMC_HS);

	/* Block length is fixed to 512 bytes while in DDR mode */
	mci->read_bl_len = SECTOR_SIZE;
	mci->write_bl_len = SECTOR_SIZE;

	return 0;
}

int mci_execute_tuning(struct mci *mci)
{
	struct mci_host *host = mci->host;
	u32 opcode;

	if (!host->ops.execute_tuning) {
		/*
		 * For us, implementing ->execute_tuning is mandatory to
		 * support higher speed modes
		 */
		dev_warn(&mci->dev, "tuning failed: no host diver support\n");
		return -EOPNOTSUPP;
	}

	/* Tuning is only supported for MMC / HS200 */
	if (mmc_card_hs200(mci))
		opcode = MMC_SEND_TUNING_BLOCK_HS200;
	else
		return 0;

	return host->ops.execute_tuning(host, opcode);
}

int mci_send_abort_tuning(struct mci *mci, u32 opcode)
{
	struct mci_cmd cmd = {};

	/*
	 * eMMC specification specifies that CMD12 can be used to stop a tuning
	 * command, but SD specification does not, so do nothing unless it is
	 * eMMC.
	 */
	if (opcode != MMC_SEND_TUNING_BLOCK_HS200)
		return 0;

	cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	cmd.resp_type = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	return mci_send_cmd(mci, &cmd, NULL);
}
EXPORT_SYMBOL_GPL(mci_send_abort_tuning);

static void mmc_select_max_dtr(struct mci *mci)
{
	u8 card_type = mci->ext_csd[EXT_CSD_DEVICE_TYPE];
	u32 caps2 = mci->host->caps2;
	u32 caps = mci->card_caps;
	unsigned int hs_max_dtr = 0;
	unsigned int hs200_max_dtr = 0;

	if ((caps & MMC_CAP_MMC_HIGHSPEED) &&
	    (card_type & EXT_CSD_CARD_TYPE_26)) {
		hs_max_dtr = MMC_HIGH_26_MAX_DTR;
	}

	if ((caps & MMC_CAP_MMC_HIGHSPEED) &&
	    (card_type & EXT_CSD_CARD_TYPE_52)) {
		hs_max_dtr = MMC_HIGH_52_MAX_DTR;
	}

	if ((caps2 & MMC_CAP2_HS200_1_8V_SDR) &&
	    (card_type & EXT_CSD_CARD_TYPE_HS200_1_8V)) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
	}

	if ((caps2 & MMC_CAP2_HS200_1_2V_SDR) &&
	    (card_type & EXT_CSD_CARD_TYPE_HS200_1_2V)) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
	}

	mci->host->hs200_max_dtr = hs200_max_dtr;
	mci->host->hs_max_dtr = hs_max_dtr;
}
/*
 * For device supporting HS200 mode, the following sequence
 * should be done before executing the tuning process.
 * 1. set the desired bus width(4-bit or 8-bit, 1-bit is not supported)
 * 2. switch to HS200 mode
 * 3. set the clock to > 52Mhz and <=200MHz
 */
static int mmc_select_hs200(struct mci *mci)
{
	unsigned int old_timing, old_clock;
	int err = -EINVAL;
	u8 val;

	/*
	 * Set the bus width(4 or 8) with host's support and
	 * switch to HS200 mode if bus width is set successfully.
	 */
	/* find out maximum bus width and then try DDR if supported */
	err = mci_mmc_select_bus_width(mci);
	if (err > 0) {
		/* TODO  actually set drive strength instead of 0. Currently unsupported. */
		val = EXT_CSD_TIMING_HS200 | 0 << EXT_CSD_DRV_STR_SHIFT;
		err = mci_switch(mci, EXT_CSD_HS_TIMING, val);
		if (err == -EIO)
			return -EBADMSG;
		if (err)
			goto err;

		/*
		 * Bump to HS timing and frequency. Some cards don't handle
		 * SEND_STATUS reliably at the initial frequency.
		 * NB: We can't move to full (HS200) speeds until after we've
		 * successfully switched over.
		 */
		old_timing = mci->host->timing;
		old_clock = mci->host->clock;

		mci->host->timing = MMC_TIMING_MMC_HS200;
		mci_set_ios(mci);
		mci_set_clock(mci, mci->host->hs_max_dtr);

		err = mci_switch_status(mci, true);

		/*
		 * mmc_select_timing() assumes timing has not changed if
		 * it is a switch error.
		 */
		if (err == -EBADMSG) {
			mci->host->clock = old_clock;
			mci->host->timing = old_timing;
			mci_set_ios(mci);
		}
	}
err:
	if (err) {
		dev_err(&mci->dev, "%s failed, error %d\n", __func__, err);
	}
	return err;
}

/*
 * Set the bus speed for the selected speed mode.
 */
static void mmc_set_bus_speed(struct mci *mci)
{
	unsigned int max_dtr = (unsigned int)-1;

	if (mmc_card_hs200(mci) &&
		max_dtr > mci->host->hs200_max_dtr)
		max_dtr = mci->host->hs200_max_dtr;
	else if (mmc_card_hs(mci) && max_dtr > mci->host->hs_max_dtr)
		max_dtr = mci->host->hs_max_dtr;
	else if (max_dtr > mci->tran_speed)
		max_dtr = mci->tran_speed;

	mci_set_clock(mci, max_dtr);
}

/*
 * Activate HS200 or HS400ES mode if supported.
 */
int mmc_select_timing(struct mci *mci)
{
	unsigned int mmc_avail_type;
	int err = 0;

	mmc_select_max_dtr(mci);

	mmc_avail_type = mci->ext_csd[EXT_CSD_DEVICE_TYPE] & EXT_CSD_CARD_TYPE_MASK;
	if (mmc_avail_type & EXT_CSD_CARD_TYPE_HS200) {
		err = mmc_select_hs200(mci);
		if (err == -EBADMSG)
			mmc_avail_type &= ~EXT_CSD_CARD_TYPE_HS200;
		else
			goto out;
	}

out:
	if (err && err != -EBADMSG)
		return err;

	/*
	 * Set the bus speed to the selected bus timing.
	 * If timing is not selected, backward compatible is the default.
	 */
	mmc_set_bus_speed(mci);

	return 0;
}

int mmc_hs200_tuning(struct mci *mci)
{
	return mci_execute_tuning(mci);
}

static int mci_startup_mmc(struct mci *mci)
{
	struct mci_host *host = mci->host;
	int ret = 0;

	/* if possible, speed up the transfer */
	if (mci_caps(mci) & MMC_CAP_MMC_HIGHSPEED) {
		if (mci->card_caps & MMC_CAP_MMC_HIGHSPEED_52MHZ)
			mci->tran_speed = 52000000;
		else
			mci->tran_speed = 26000000;

		host->timing = MMC_TIMING_MMC_HS;
	}

	if (IS_ENABLED(CONFIG_MCI_TUNING)) {
		/*
		 * Select timing interface
		 */
		ret = mmc_select_timing(mci);
		if (ret)
			return ret;

		if (mmc_card_hs200(mci))
			ret = mmc_hs200_tuning(mci);

		if (ret) {
			host->timing = MMC_TIMING_MMC_HS;
			mci_switch(mci, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);
		}
	}

	if (ret || !IS_ENABLED(CONFIG_MCI_TUNING)) {
		mci_set_clock(mci, mci->tran_speed);

		/* find out maximum bus width and then try DDR if supported */
		ret = mci_mmc_select_bus_width(mci);
		if (ret > MMC_BUS_WIDTH_1 && mci->tran_speed == 52000000)
			ret = mci_mmc_select_hs_ddr(mci);

		if (ret < 0) {
			dev_warn(&mci->dev, "Changing MMC bus width failed: %d\n", ret);
		}
	}

	return ret >= MMC_BUS_WIDTH_1 ? 0 : ret;
}

static void mci_init_erase(struct mci *card)
{
	if (!IS_ENABLED(CONFIG_MCI_ERASE))
		return;

	/* TODO: While it's possible to clear many erase groups at once
	 * and it greatly improves throughput, drivers need adjustment:
	 *
	 * Many drivers hardcode a maximal wait time before aborting
	 * the wait for R1b and returning -ETIMEDOUT. With long
	 * erases/trims, we are bound to run into this timeout, so for now
	 * we just split into sufficiently small erases that are unlikely
	 * to trigger the timeout.
	 *
	 * What Linux does and what we should be doing in barebox is:
	 *
	 *  - add a struct mci_cmd::busy_timeout member that drivers should
	 *    use instead of hardcoding their own timeout delay. The busy
	 *    timeout length can be calculated by the MCI core after
	 *    consulting the appropriate CSD/EXT_CSD/SSR registers.
	 *
	 *  - add a struct mci_host::max_busy_timeout member, where drivers
	 *    can indicate the maximum timeout they are able to support.
	 *    The MCI core will never set a busy_timeout that exceeds this
	 *    value.
	 *
	 *  Example Samsung eMMC 8GTF4:
	 *
	 *    time erase /dev/mmc2.part_of_512m # 1024 trims
	 *    time: 2849ms
	 *
	 *    time erase /dev/mmc2.part_of_512m # single trim
	 *    time: 56ms
	 */
	if (IS_SD(card) && card->ssr.au) {
		card->pref_erase = card->ssr.au;
	} else if (card->erase_grp_size) {
		unsigned int sz;

		sz = card->capacity / SZ_1M;
		if (sz < 128)
			card->pref_erase = 512 * 1024 / 512;
		else if (sz < 512)
			card->pref_erase = 1024 * 1024 / 512;
		else if (sz < 1024)
			card->pref_erase = 2 * 1024 * 1024 / 512;
		else
			card->pref_erase = 4 * 1024 * 1024 / 512;
		if (card->pref_erase < card->erase_grp_size)
			card->pref_erase = card->erase_grp_size;
		else {
			sz = card->pref_erase % card->erase_grp_size;
			if (sz)
				card->pref_erase += card->erase_grp_size - sz;
		}
	} else {
		card->pref_erase = 0;
		return;
	}

	dev_add_param_uint32_fixed(&card->dev, "preferred_erase_size",
				   card->pref_erase * 512, "%u");
}

/**
 * Scan the given host interfaces and detect connected MMC/SD cards
 * @param mci MCI instance
 * @return 0 on success, negative value else
 */
static int mci_startup(struct mci *mci)
{
	struct mci_host *host = mci->host;
	struct mci_cmd cmd = {};
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

	memcpy(mci->raw_cid, cmd.response, 16);

	dev_dbg(&mci->dev, "Card's identification data is: %08X-%08X-%08X-%08X\n",
		mci->raw_cid[0], mci->raw_cid[1], mci->raw_cid[2], mci->raw_cid[3]);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relative Address.
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

	/* sanity? */
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
	mci_configure_erase_group_size(mci);

	if (IS_SD(mci))
		err = mci_startup_sd(mci);
	else
		err = mci_startup_mmc(mci);

	if (err)
		return err;

	/* we setup the blocklength only one times for all accesses to this media  */
	err = mci_set_blocklen(mci, mci->read_bl_len);

	mci_init_erase(mci);

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
	struct mci_cmd cmd = {};
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

/**
 * Switch between hardware MMC partitions on demand
 */
static int mci_blk_part_switch(struct mci_part *part)
{
	struct mci *mci = part->mci;
	int ret;

	if (!IS_ENABLED(CONFIG_MCI_MMC_BOOT_PARTITIONS) && !IS_ENABLED(CONFIG_MCI_MMC_GPP_PARTITIONS))
		return 0; /* no need */

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

static int mci_sd_check_write(struct mci *mci, const char *op,
			      sector_t block, blkcnt_t num_blocks)
{
	struct mci_host *host = mci->host;

	if (!host->disable_wp &&
	    host->ops.card_write_protected && host->ops.card_write_protected(host)) {
		dev_err(&mci->dev, "card write protected\n");
		return -EPERM;
	}

	dev_dbg(&mci->dev, "%s: %s %llu block(s), starting at %llu\n",
		__func__, op, num_blocks, block);

	if (mci->write_bl_len != SECTOR_SIZE) {
		dev_dbg(&mci->dev, "MMC/SD block size is not %d bytes (its %u bytes instead)\n",
				SECTOR_SIZE, mci->read_bl_len);
		return -EINVAL;
	}

	/* size of the block number field in the MMC/SD command is 32 bit only */
	if (block > MAX_BUFFER_NUMBER) {
		dev_dbg(&mci->dev, "Cannot handle block number %llu. Too large!\n", block);
		return -EINVAL;
	}

	return 0;
}

static unsigned int mmc_align_erase_size(struct mci *card,
					 sector_t *from,
					 sector_t *to,
					 blkcnt_t nr)
{
	unsigned int from_new = *from, to_new, nr_new = nr, rem;

	/*
	 * When the 'card->erase_size' is power of 2, we can use round_up/down()
	 * to align the erase size efficiently.
	 */
	if (is_power_of_2(card->erase_grp_size)) {
		unsigned int temp = from_new;

		from_new = round_up(temp, card->erase_grp_size);
		rem = from_new - temp;

		if (nr_new > rem)
			nr_new -= rem;
		else
			return 0;

		nr_new = round_down(nr_new, card->erase_grp_size);
	} else {
		rem = from_new % card->erase_grp_size;
		if (rem) {
			rem = card->erase_grp_size - rem;
			from_new += rem;
			if (nr_new > rem)
				nr_new -= rem;
			else
				return 0;
		}

		rem = nr_new % card->erase_grp_size;
		if (rem)
			nr_new -= rem;
	}

	if (nr_new == 0)
		return 0;

	to_new = from_new + nr_new;

	if (*to != to_new || *from != from_new)
		dev_warn(&card->dev, "Erase range changed to [0x%x-0x%x] because of %u sector erase group\n",
			 from_new, to_new, card->erase_grp_size);

	*to = to_new;
	*from = from_new;

	return nr_new;
}

/**
 * Erase a memory region
 * @param blk All info about the block device we need
 * @param block first block to erase
 * @param num_blocks Number of blocks to erase
 * @return 0 on success, anything else on failure
 *
 */
static int mci_sd_erase(struct block_device *blk, sector_t from,
			blkcnt_t blkcnt)
{
	struct mci_part *part = container_of(blk, struct mci_part, blk);
	struct mci *mci = part->mci;
	sector_t i = 0;
	unsigned arg;
	sector_t to = from + blkcnt;
	int rc;

	mci_blk_part_switch(part);

	rc = mci_sd_check_write(mci, "Erase", from, blkcnt);
	if (rc)
		return rc;

	if (!mci->erase_grp_size)
		return -EOPNOTSUPP;

	if (mci->can_trim) {
		arg = MMC_TRIM_ARG;
	} else {
		/* We don't use discard, as it doesn't guarantee a fixed value */
		arg = MMC_ERASE_ARG;
		blkcnt = mmc_align_erase_size(mci, &from, &to, blkcnt);
	}

	if (blkcnt == 0)
		return 0;

	/* 'from' and 'to' are inclusive */
	to -= 1;

	while (i < blkcnt) {
		sector_t blk_r;

		blk_r = min_t(blkcnt_t, blkcnt - i, mci->pref_erase);

		rc =  mci_block_erase(mci, from + i, blk_r, arg);
		if (rc)
			break;

		/* Waiting for the ready status */
		rc = mci_poll_until_ready(mci, 1000 /* ms */);
		if (rc)
			break;

		i += blk_r;
	}

	return i == blkcnt ? 0 : rc;
}

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
static int mci_sd_write(struct block_device *blk,
			const void *buffer, sector_t block, blkcnt_t num_blocks)
{
	struct mci_part *part = container_of(blk, struct mci_part, blk);
	struct mci *mci = part->mci;
	int rc;
	blkcnt_t max_req_block = num_blocks;
	blkcnt_t write_block;

	if (mci->host->max_req_size)
		max_req_block = mci->host->max_req_size / mci->write_bl_len;

	mci_blk_part_switch(part);

	rc = mci_sd_check_write(mci, "Write", block, num_blocks);
	if (rc)
		return rc;

	while (num_blocks) {
		write_block = min(num_blocks, max_req_block);
		rc = mci_block_write(mci, buffer, block, write_block);
		if (rc != 0) {
			dev_dbg(&mci->dev, "Writing block %llu failed with %d\n", block, rc);
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
static int mci_sd_read(struct block_device *blk, void *buffer, sector_t block,
				blkcnt_t num_blocks)
{
	struct mci_part *part = container_of(blk, struct mci_part, blk);
	struct mci *mci = part->mci;
	blkcnt_t max_req_block = num_blocks;
	blkcnt_t read_block;
	int rc;

	if (mci->host->max_req_size)
		max_req_block = mci->host->max_req_size / mci->read_bl_len;

	mci_blk_part_switch(part);

	dev_dbg(&mci->dev, "%s: Read %llu block(s), starting at %llu\n",
		__func__, num_blocks, block);

	if (mci->read_bl_len != SECTOR_SIZE) {
		dev_dbg(&mci->dev, "MMC/SD block size is not %d bytes (its %u bytes instead)\n",
				SECTOR_SIZE, mci->read_bl_len);
		return -EINVAL;
	}

	if (block > MAX_BUFFER_NUMBER) {
		dev_err(&mci->dev, "Cannot handle block number %llu. Too large!\n", block);
		return -EINVAL;
	}

	while (num_blocks) {
		read_block = min(num_blocks, max_req_block);
		rc = mci_block_read(mci, buffer, block, read_block);
		if (rc != 0) {
			dev_dbg(&mci->dev, "Reading block %llu failed with %d\n", block, rc);
			return rc;
		}
		num_blocks -= read_block;
		block += read_block;
		buffer += read_block * mci->read_bl_len;
	}

	return 0;
}

/* ------------------ attach to the device API --------------------------- */

static const char *mci_timing_tostr(unsigned timing)
{
	switch (timing) {
	case MMC_TIMING_LEGACY:
		return "legacy";
	case MMC_TIMING_MMC_HS:
		return "MMC HS";
	case MMC_TIMING_SD_HS:
		return "SD HS";
	case MMC_TIMING_MMC_DDR52:
		return "MMC DDR52";
	case MMC_TIMING_MMC_HS200:
		return "HS200";
	default:
		return "unknown"; /* shouldn't happen */
	}
}

static void mci_print_caps(unsigned caps)
{
	printf("  capabilities: %s%s%s%s%s%s%s%s\n",
		caps & MMC_CAP_4_BIT_DATA ? "4bit " : "",
		caps & MMC_CAP_8_BIT_DATA ? "8bit " : "",
		caps & MMC_CAP_SD_HIGHSPEED ? "sd-hs " : "",
		caps & MMC_CAP_MMC_HIGHSPEED ? "mmc-hs " : "",
		caps & MMC_CAP_MMC_HIGHSPEED_52MHZ ? "mmc-52MHz " : "",
		caps & MMC_CAP_MMC_3_3V_DDR ? "ddr-3.3v " : "",
		caps & MMC_CAP_MMC_1_8V_DDR ? "ddr-1.8v " : "",
		caps & MMC_CAP_MMC_1_2V_DDR ? "ddr-1.2v " : "");
}

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static int mci_mmc_decode_cid(struct mci *card)
{
	u32 *resp = card->raw_cid;
	u32 mmca_vsn = unstuff_bits(card->csd, 122, 4);

	/*
	 * The selection of the format here is based upon published
	 * specs from sandisk and from what people have reported.
	 */
	switch (mmca_vsn) {
	case 0: /* MMC v1.0 - v1.2 */
	case 1: /* MMC v1.4 */
		card->cid.manfid	= unstuff_bits(resp, 104, 24);
		card->cid.prod_name[0]	= unstuff_bits(resp, 96, 8);
		card->cid.prod_name[1]	= unstuff_bits(resp, 88, 8);
		card->cid.prod_name[2]	= unstuff_bits(resp, 80, 8);
		card->cid.prod_name[3]	= unstuff_bits(resp, 72, 8);
		card->cid.prod_name[4]	= unstuff_bits(resp, 64, 8);
		card->cid.prod_name[5]	= unstuff_bits(resp, 56, 8);
		card->cid.prod_name[6]	= unstuff_bits(resp, 48, 8);
		card->cid.hwrev		= unstuff_bits(resp, 44, 4);
		card->cid.fwrev		= unstuff_bits(resp, 40, 4);
		card->cid.serial	= unstuff_bits(resp, 16, 24);
		card->cid.month		= unstuff_bits(resp, 12, 4);
		card->cid.year		= unstuff_bits(resp, 8, 4) + 1997;
		break;

	case 2: /* MMC v2.0 - v2.2 */
	case 3: /* MMC v3.1 - v3.3 */
	case 4: /* MMC v4 */
		card->cid.manfid	= unstuff_bits(resp, 120, 8);
		card->cid.oemid		= unstuff_bits(resp, 104, 16);
		card->cid.prod_name[0]	= unstuff_bits(resp, 96, 8);
		card->cid.prod_name[1]	= unstuff_bits(resp, 88, 8);
		card->cid.prod_name[2]	= unstuff_bits(resp, 80, 8);
		card->cid.prod_name[3]	= unstuff_bits(resp, 72, 8);
		card->cid.prod_name[4]	= unstuff_bits(resp, 64, 8);
		card->cid.prod_name[5]	= unstuff_bits(resp, 56, 8);
		card->cid.prv		= unstuff_bits(resp, 48, 8);
		card->cid.serial	= unstuff_bits(resp, 16, 32);
		card->cid.month		= unstuff_bits(resp, 12, 4);
		card->cid.year		= unstuff_bits(resp, 8, 4) + 1997;
		break;

	default:
		dev_err(&card->dev, "card has unknown MMCA version %d\n", mmca_vsn);
		return -EINVAL;
	}

	return 0;
}

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static void mci_sd_decode_cid(struct mci *card)
{
	u32 *resp = card->raw_cid;

	/*
	 * SD doesn't currently have a version field so we will
	 * have to assume we can parse this.
	 */
	card->cid.manfid		= unstuff_bits(resp, 120, 8);
	card->cid.oemid			= unstuff_bits(resp, 104, 16);
	card->cid.prod_name[0]		= unstuff_bits(resp, 96, 8);
	card->cid.prod_name[1]		= unstuff_bits(resp, 88, 8);
	card->cid.prod_name[2]		= unstuff_bits(resp, 80, 8);
	card->cid.prod_name[3]		= unstuff_bits(resp, 72, 8);
	card->cid.prod_name[4]		= unstuff_bits(resp, 64, 8);
	card->cid.hwrev			= unstuff_bits(resp, 60, 4);
	card->cid.fwrev			= unstuff_bits(resp, 56, 4);
	card->cid.serial		= unstuff_bits(resp, 24, 32);
	card->cid.year			= unstuff_bits(resp, 12, 8);
	card->cid.month			= unstuff_bits(resp, 8, 4);

	card->cid.year += 2000; /* SD cards year offset */
}

/**
 * Output some valuable information when the user runs 'devinfo' on an MCI device
 * @param mci MCI device instance
 */
static void mci_info(struct device *dev)
{
	struct mci *mci = container_of(dev, struct mci, dev);
	struct mci_host *host = mci->host;
	int bw;

	if (!mci->ready_for_use) {
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
	printf("  current timing: %s\n", mci_timing_tostr(host->timing));
	mci_print_caps(host->host_caps);

	printf("Card information:\n");
	printf("  Card type: %s\n", mci->sdio ? "SDIO" : IS_SD(mci) ? "SD" : "MMC");
	if (mci->sdio)
		return;
	printf("  Version: %s\n", mci_version_string(mci));
	printf("  Capacity: %u MiB\n", (unsigned)(mci->capacity >> 20));
	if (IS_ENABLED(CONFIG_MCI_ERASE)) {
		printf("  Erase support:");
		if (mci->can_trim)
			printf(" trim");
		if (mci->erase_grp_size) {
			printf(" erase(%u sector%s%s)", mci->ssr.au ?: mci->erase_grp_size,
			       mci->erase_grp_size > 1 ? "s" : "",
			       mci->ssr.au ? " AU" : "");
		}
		if (mci->can_trim || mci->erase_grp_size)
			printf(", erase value: 0x%02x\n", mci->erased_byte);
		else
			printf(" none\n");
	}

	if (mci->high_capacity)
		printf("  High capacity card\n");
	printf("   CID: %08X-%08X-%08X-%08X\n", mci->raw_cid[0], mci->raw_cid[1],
		mci->raw_cid[2], mci->raw_cid[3]);
	printf("   CSD: %08X-%08X-%08X-%08X\n", mci->csd[0], mci->csd[1],
		mci->csd[2], mci->csd[3]);
	printf("  Max. transfer speed: %u Hz\n", mci->tran_speed);
	mci_print_caps(mci->card_caps);
	printf("  Manufacturer ID: 0x%02x\n", mci->cid.manfid);
	printf("  OEM/Application ID: 0x%04x\n", mci->cid.oemid);
	printf("  Product name: '%s'\n", mci->cid.prod_name);
	printf("  Hardware revision: 0x%02x\n", mci->cid.hwrev);
	printf("  Firmware revision: 0x%02x\n", mci->cid.fwrev);
	printf("  Serial no: %u\n", mci->cid.serial);
	printf("  Manufacturing date: %u.%u\n", mci->cid.year, mci->cid.month);
}

static void mci_parse_cid(struct mci *mci)
{
	struct device *dev = &mci->dev;

	if (IS_SD(mci))
		mci_sd_decode_cid(mci);
	else
		mci_mmc_decode_cid(mci);

	if (mci->ext_csd[EXT_CSD_REV] >= 5) {
		/* Adjust production date as per JEDEC JESD84-B451 */
		if (mci->cid.year < 2010)
			mci->cid.year += 16;
	}

	dev_add_param_uint32_fixed(dev, "cid_mid", mci->cid.manfid, "0x%02X");
	dev_add_param_uint32_fixed(dev, "cid_oid", mci->cid.oemid, "0x%04X");
	dev_add_param_string_fixed(dev, "cid_pnm", mci->cid.prod_name);
	dev_add_param_uint32_fixed(dev, "cid_hwrev", mci->cid.hwrev, "0x%02X");
	dev_add_param_uint32_fixed(dev, "cid_fwrev", mci->cid.fwrev, "0x%02X");
	dev_add_param_uint32_fixed(dev, "cid_psn", mci->cid.serial, "%0u");
	dev_add_param_uint32_fixed(dev, "cid_year", mci->cid.year, "%0u");
	dev_add_param_uint32_fixed(dev, "cid_month", mci->cid.month, "%0u");
}

static struct block_device_ops mci_ops = {
	.read = mci_sd_read,
	.write = IS_ENABLED(CONFIG_MCI_WRITE) ? mci_sd_write : NULL,
	.erase = IS_ENABLED(CONFIG_MCI_ERASE) ? mci_sd_erase : NULL,
};

static int mci_set_boot(struct param_d *param, void *priv)
{
	struct mci *mci = priv;

	mci->ext_csd_part_config &= ~(7 << 3);
	mci->ext_csd_part_config |= mci->bootpart << 3;

	return mci_switch(mci,
			  EXT_CSD_PARTITION_CONFIG, mci->ext_csd_part_config);
}

static int mci_set_boot_ack(struct param_d *param, void *priv)
{
	struct mci *mci = priv;

	mci->ext_csd_part_config &= ~(0x1 << 6);
	mci->ext_csd_part_config |= mci->boot_ack_enable << 6;

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

static struct device_node *mci_get_partition_node(struct device_node *hwnode,
						  unsigned int type,
						  unsigned int index)
{
	struct device_node *np;
	char partnodename[sizeof("bootx-partitions")];

	if (index > 8)
		return NULL;

	switch (type) {
	case MMC_BLK_DATA_AREA_BOOT:
		sprintf(partnodename, "partitions-boot%u", index + 1);
		break;
	case MMC_BLK_DATA_AREA_MAIN:
		return hwnode;
	case MMC_BLK_DATA_AREA_GP:
		sprintf(partnodename, "partitions-gp%u", index + 1);
		break;
	default:
		return NULL;
	}

	np = of_get_child_by_name(hwnode, partnodename);
	if (np)
		return np;

	if (type != MMC_BLK_DATA_AREA_BOOT)
		return NULL;

	/*
	 * barebox historically understands "bootx-partitions" with x starting
	 * at zero.
	 */
	sprintf(partnodename, "boot%u-partitions", index);

	return of_get_child_by_name(hwnode, partnodename);
}

static int mci_register_partition(struct mci_part *part)
{
	struct mci *mci = part->mci;
	struct mci_host *host = mci->host;
	struct device_node *np;
	int rc;

	/*
	 * An MMC/SD card acts like an ordinary disk.
	 * So, re-use the disk driver to gain access to this media
	 */
	part->blk.dev = &mci->dev;
	part->blk.ops = &mci_ops;
	part->blk.type = IS_SD(mci) ? BLK_TYPE_SD : BLK_TYPE_MMC;

	rc = blockdevice_register(&part->blk);
	if (rc != 0) {
		dev_err(&mci->dev, "Failed to register MCI/SD blockdevice\n");
		return rc;
	}
	dev_info(&mci->dev, "registered %s\n", part->blk.cdev.name);

	np = mci_get_partition_node(host->hw_dev->of_node, part->area_type, part->idx);
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

static int of_broken_cd_fixup(struct device_node *root, void *ctx)
{
	struct mci_host *host = ctx;
	struct device *hw_dev = host->hw_dev;
	struct device_node *np;
	char *name;

	if (!host->broken_cd)
		return 0;

	name = of_get_reproducible_name(hw_dev->of_node);
	np = of_find_node_by_reproducible_name(root, name);
	free(name);
	if (!np) {
		dev_warn(hw_dev, "Cannot find nodepath %pOF, cannot fixup\n",
			 hw_dev->of_node);
		return -EINVAL;
	}

	of_property_write_bool(np, "cd-gpios", false);
	of_property_write_bool(np, "broken-cd", true);

	return 0;
}

static int mci_get_partition_setting_completed(struct mci *mci)
{
	u8 *ext_csd;
	int ret;

	ext_csd = mci_get_ext_csd(mci);
	if (IS_ERR(ext_csd))
		return PTR_ERR(ext_csd);

	ret = ext_csd[EXT_CSD_PARTITION_SETTING_COMPLETED];

	dma_free(ext_csd);

	return ret;
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
	bool has_bootpart = false;

	if (host->ops.card_present && !host->ops.card_present(host) && !host->non_removable) {
		if (!host->broken_cd) {
			dev_err(&mci->dev, "no card inserted\n");
			return -ENODEV;
		}

		dev_info(&mci->dev, "no card inserted (ignoring)\n");
	}

	ret = regulator_enable(host->supply);
	if (ret) {
		dev_err(&mci->dev, "failed to enable regulator: %s\n",
			strerror(-ret));
		return ret;
	}

	/* start with a host interface reset */
	rc = (host->ops.init)(host, &mci->dev);
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

	if (!(host->caps2 & MMC_CAP2_NO_SDIO)) {
		rc = sdio_send_op_cond(mci);
		if (!rc) {
			mci->ready_for_use = true;
			mci->sdio = true;
			dev_info(&mci->dev, "SDIO card detected, ignoring\n");
			return 0;
		}
	}

	/* Check if this card can handle the "SD Card Physical Layer Specification 2.0" */
	if (!(host->caps2 & MMC_CAP2_NO_SD)) {
		rc = sd_send_if_cond(mci);
		rc = sd_send_op_cond(mci);
	}
	if ((host->caps2 & MMC_CAP2_NO_SD) || rc == -ETIMEDOUT) {
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

	if (!sector_buf)
		sector_buf = dma_alloc(SECTOR_SIZE);

	/* FIXME we don't check sector_buf against the device dma mask here */

	rc = mci_startup(mci);
	if (rc) {
		dev_warn(&mci->dev, "Card's startup fails with %d\n", rc);
		goto on_error;
	}

	dev_dbg(&mci->dev, "Card is up and running now, registering as a disk\n");
	mci->ready_for_use = true; /* TODO now or later? */

	for (i = 0; i < mci->nr_parts; i++) {
		struct mci_part *part = &mci->part[i];

		rc = mci_register_partition(part);

		if (IS_ENABLED(CONFIG_MCI_MMC_BOOT_PARTITIONS) &&
		    part->area_type == MMC_BLK_DATA_AREA_BOOT)
			has_bootpart = true;
	}

	if (has_bootpart) {
		dev_add_param_enum(&mci->dev, "boot", mci_set_boot,
				   NULL, &mci->bootpart, mci_boot_names,
				   ARRAY_SIZE(mci_boot_names), mci);

		dev_add_param_bool(&mci->dev, "boot_ack",
				   mci_set_boot_ack, NULL,
				   &mci->boot_ack_enable, mci);

		ret = mci_get_partition_setting_completed(mci);
		if (ret < 0)
			dev_dbg(&mci->dev,
				"Failed to determine EXT_CSD_PARTITION_SETTING_COMPLETED\n");
		else
			dev_add_param_bool_fixed(&mci->dev, "partitioning_completed", ret);
	}

	mci_parse_cid(mci);

	dev_dbg(&mci->dev, "Card successfully added\n");

on_error:
	if (rc != 0) {
		host->clock = 0;	/* disable the MCI clock */
		mci_set_ios(mci);
		regulator_disable(host->supply);
		mci->nr_parts = 0;
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

	if (mci->ready_for_use)
		return 0;

	rc = mci_card_probe(mci);
	if (rc != 0)
		return rc;

	return 0;
}

int mci_detect_card(struct mci_host *host)
{
	if (host->mci->ready_for_use)
		return 0;

	return mci_card_probe(host->mci);
}

static int mci_detect(struct device *dev)
{
	struct mci *mci = container_of(dev, struct mci, dev);

	return mci_detect_card(mci->host);
}

static int mci_hw_detect(struct device *dev)
{
	struct mci *mci;

	list_for_each_entry(mci, &mci_list, list) {
		if (dev == mci->host->hw_dev)
			return mci_detect_card(mci->host);
	}

	return -ENODEV;
}

/**
 * Create a new mci device (for convenience)
 * @param host mci_host for this MCI device
 * @return 0 on success
 */
int mci_register(struct mci_host *host)
{
	struct mci *mci;
	struct device *hw_dev;
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

	hw_dev = host->hw_dev;
	mci->dev.platform_data = host;
	mci->dev.parent = hw_dev;
	mci->host = host;
	host->mci = mci;
	mci->dev.detect = mci_detect;
	if (!hw_dev->detect)
		hw_dev->detect = mci_hw_detect;

	host->supply = regulator_get(hw_dev, "vmmc");
	if (IS_ERR(host->supply)) {
		/*
		 * If you know your regulator to be always online on boot, but
		 * can't easily add a barebox driver for it, you may use
		 * barebox,allow-dummy-supply in your board's regulator device
		 * tree node to side step this warning.
		 *
		 * If you run into this warning, because your regulator driver
		 * hasn't probed the device yet, consider enabling deep probe
		 * for your board, to probe dependencies on demand.
		 */
		dev_warn(hw_dev, "Failed to get 'vmmc' regulator (ignored).\n");
		host->supply = NULL;
	}

	ret = register_device(&mci->dev);
	if (ret)
		goto err_free;

	dev_info(hw_dev, "registered as %s\n", dev_name(&mci->dev));

	dev_add_param_bool(&mci->dev, "probe", mci_set_probe, NULL,
			   &mci->probe, mci);

	if (IS_ENABLED(CONFIG_MCI_INFO))
		devinfo_add(&mci->dev, mci_info);

	/* if enabled, probe the attached card immediately */
	if (IS_ENABLED(CONFIG_MCI_STARTUP))
		mci_card_probe(mci);

	if (!(host->caps2 & MMC_CAP2_NO_SD) && dev_of_node(host->hw_dev)) {
		dev_add_param_bool(&mci->dev, "broken_cd", NULL, NULL,
				   &host->broken_cd, mci);

		of_register_fixup(of_broken_cd_fixup, host);
	}

	list_add_tail(&mci->list, &mci_list);

	return 0;

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
			fallthrough;
		case 4:
			host->host_caps |= MMC_CAP_4_BIT_DATA;
			fallthrough;
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

	host->broken_cd = of_property_read_bool(np, "broken-cd");
	host->non_removable = of_property_read_bool(np, "non-removable");
	host->disable_wp = of_property_read_bool(np, "disable-wp");

	if (of_property_read_bool(np, "full-pwr-cycle"))
		host->caps2 |= MMC_CAP2_FULL_PWR_CYCLE;
	if (of_property_read_bool(np, "full-pwr-cycle-in-suspend"))
		host->caps2 |= MMC_CAP2_FULL_PWR_CYCLE_IN_SUSPEND;
	if (of_property_read_bool(np, "no-sdio"))
		host->caps2 |= MMC_CAP2_NO_SDIO;
	if (of_property_read_bool(np, "no-sd"))
		host->caps2 |= MMC_CAP2_NO_SD;
	if (of_property_read_bool(np, "no-mmc"))
		host->caps2 |= MMC_CAP2_NO_MMC;
	if (IS_ENABLED(CONFIG_MCI_TUNING)) {
		if (of_property_read_bool(np, "mmc-hs200-1_8v"))
			host->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
		if (of_property_read_bool(np, "mmc-hs200-1_2v"))
			host->caps2 |= MMC_CAP2_HS200_1_2V_SDR;
		if (of_property_read_bool(np, "mmc-hs400-1_8v"))
			host->caps2 |= MMC_CAP2_HS400_1_8V | MMC_CAP2_HS200_1_8V_SDR;
		if (of_property_read_bool(np, "mmc-hs400-1_2v"))
			host->caps2 |= MMC_CAP2_HS400_1_2V | MMC_CAP2_HS200_1_2V_SDR;
		if (of_property_read_bool(np, "mmc-hs400-enhanced-strobe"))
			host->caps2 |= MMC_CAP2_HS400_ES;
		if (of_property_read_bool(np, "no-mmc-hs400"))
			host->caps2 &= ~(MMC_CAP2_HS400_1_8V | MMC_CAP2_HS400_1_2V |
					 MMC_CAP2_HS400_ES);
	}
}

void mci_of_parse(struct mci_host *host)
{
	return mci_of_parse_node(host, host->hw_dev->of_node);
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
