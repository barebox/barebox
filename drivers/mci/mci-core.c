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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* #define DEBUG */

#include <init.h>
#include <common.h>
#include <mci.h>
#include <malloc.h>
#include <errno.h>
#include <asm-generic/div64.h>
#include <asm/byteorder.h>
#include <ata.h>

#define MAX_BUFFER_NUMBER 0xffffffff

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

/**
 * Call the MMC/SD instance driver to run the command on the MMC/SD card
 * @param mci_dev MCI instance
 * @param cmd The information about the command to run
 * @param data The data according to the command (can be NULL)
 * @return Driver's answer (0 on success)
 */
static int mci_send_cmd(struct device_d *mci_dev, struct mci_cmd *cmd, struct mci_data *data)
{
	struct mci_host *host = GET_MCI_PDATA(mci_dev);

	return host->send_cmd(host, cmd, data);
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
 * Setup SD/MMC card's blocklength to be used for future transmitts
 * @param mci_dev MCI instance
 * @param len Blocklength in bytes
 * @return Transaction status (0 on success)
 */
static int mci_set_blocklen(struct device_d *mci_dev, unsigned len)
{
	struct mci_cmd cmd;

	mci_setup_cmd(&cmd, MMC_CMD_SET_BLOCKLEN, len, MMC_RSP_R1);
	return mci_send_cmd(mci_dev, &cmd, NULL);
}

static void *sector_buf;

/**
 * Write one block of data to the card
 * @param mci_dev MCI instance
 * @param src Where to read from to write to the card
 * @param blocknum Block number to write
 * @return Transaction status (0 on success)
 */
#ifdef CONFIG_MCI_WRITE
static int mci_block_write(struct device_d *mci_dev, const void *src, unsigned blocknum)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	struct mci_cmd cmd;
	struct mci_data data;
	const void *buf;

	if ((unsigned long)src & 0x3) {
		memcpy(sector_buf, src, 512);
		buf = sector_buf;
	} else {
		buf = src;
	}

	mci_setup_cmd(&cmd,
		MMC_CMD_WRITE_SINGLE_BLOCK,
		mci->high_capacity != 0 ? blocknum : blocknum * mci->write_bl_len,
		MMC_RSP_R1);

	data.src = buf;
	data.blocks = 1;
	data.blocksize = mci->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	return mci_send_cmd(mci_dev, &cmd, &data);
}
#endif

/**
 * Read one block of data from the card
 * @param mci_dev MCI instance
 * @param dst Where to store the data read from the card
 * @param blocknum Block number to read
 * @param blocks number of blocks to read
 */
static int mci_read_block(struct device_d *mci_dev, void *dst, unsigned blocknum,
		int blocks)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
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

	ret = mci_send_cmd(mci_dev, &cmd, &data);
	if (ret)
		return ret;

	if (blocks > 1) {
		mci_setup_cmd(&cmd,
			MMC_CMD_STOP_TRANSMISSION,
			0,
			MMC_RSP_R1b);
			ret = mci_send_cmd(mci_dev, &cmd, NULL);
	}
	return ret;
}

/**
 * Reset the attached MMC/SD card
 * @param mci_dev MCI instance
 * @return Transaction status (0 on success)
 */
static int mci_go_idle(struct device_d *mci_dev)
{
	struct mci_cmd cmd;
	int err;

	udelay(1000);

	mci_setup_cmd(&cmd, MMC_CMD_GO_IDLE_STATE, 0, MMC_RSP_NONE);
	err = mci_send_cmd(mci_dev, &cmd, NULL);

	if (err) {
		pr_debug("Activating IDLE state failed: %d\n", err);
		return err;
	}

	udelay(2000);	/* WTF? */

	return 0;
}

/**
 * FIXME
 * @param mci_dev MCI instance
 * @return Transaction status (0 on success)
 */
static int sd_send_op_cond(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	struct mci_host *host = GET_MCI_PDATA(mci_dev);
	struct mci_cmd cmd;
	int timeout = 1000;
	int err;
	unsigned voltages;

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
		err = mci_send_cmd(mci_dev, &cmd, NULL);
		if (err) {
			pr_debug("Preparing SD for operating conditions failed: %d\n", err);
			return err;
		}

		mci_setup_cmd(&cmd, SD_CMD_APP_SEND_OP_COND,
			voltages | (mci->version == SD_VERSION_2 ? OCR_HCS : 0),
			MMC_RSP_R3);
		err = mci_send_cmd(mci_dev, &cmd, NULL);
		if (err) {
			pr_debug("SD operation condition set failed: %d\n", err);
			return err;
		}
		udelay(1000);
	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

	if (timeout <= 0) {
		pr_debug("SD operation condition set timed out\n");
		return -ENODEV;
	}

	if (mci->version != SD_VERSION_2)
		mci->version = SD_VERSION_1_0;

	mci->ocr = cmd.response[0];

	mci->high_capacity = ((mci->ocr & OCR_HCS) == OCR_HCS);
	mci->rca = 0;

	return 0;
}

/**
 * Setup the operation conditions to a MultiMediaCard
 * @param mci_dev MCI instance
 * @return Transaction status (0 on success)
 */
static int mmc_send_op_cond(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	struct mci_host *host = GET_MCI_PDATA(mci_dev);
	struct mci_cmd cmd;
	int timeout = 1000;
	int err;

	/* Some cards seem to need this */
	mci_go_idle(mci_dev);

	do {
		mci_setup_cmd(&cmd, MMC_CMD_SEND_OP_COND, OCR_HCS |
				host->voltages, MMC_RSP_R3);
		err = mci_send_cmd(mci_dev, &cmd, NULL);

		if (err) {
			pr_debug("Preparing MMC for operating conditions failed: %d\n", err);
			return err;
		}

		udelay(1000);
	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);

	if (timeout <= 0) {
		pr_debug("SD operation condition set timed out\n");
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
 * @param mci_dev MCI instance
 * @param ext_csd Buffer for a 512 byte sized extended CSD
 * @return Transaction status (0 on success)
 *
 * Note: Only cards newer than Version 1.1 (Physical Layer Spec) support
 * this command
 */
static int mci_send_ext_csd(struct device_d *mci_dev, char *ext_csd)
{
	struct mci_cmd cmd;
	struct mci_data data;

	/* Get the Card Status Register */
	mci_setup_cmd(&cmd, MMC_CMD_SEND_EXT_CSD, 0, MMC_RSP_R1);

	data.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	return mci_send_cmd(mci_dev, &cmd, &data);
}

/**
 * FIXME
 * @param mci_dev MCI instance
 * @param set FIXME
 * @param index FIXME
 * @param value FIXME
 * @return Transaction status (0 on success)
 */
static int mci_switch(struct device_d *mci_dev, unsigned set, unsigned index,
			unsigned value)
{
	struct mci_cmd cmd;

	mci_setup_cmd(&cmd, MMC_CMD_SWITCH,
		(MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		(index << 16) |
		(value << 8),
		 MMC_RSP_R1b);

	return mci_send_cmd(mci_dev, &cmd, NULL);
}

/**
 * Change transfer frequency for an MMC card
 * @param mci_dev MCI instance
 * @return Transaction status (0 on success)
 */
static int mmc_change_freq(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	char *ext_csd = sector_buf;
	char cardtype;
	int err;

	mci->card_caps = 0;

	/* Only version 4 supports high-speed */
	if (mci->version < MMC_VERSION_4)
		return 0;

	mci->card_caps |= MMC_MODE_4BIT;

	err = mci_send_ext_csd(mci_dev, ext_csd);
	if (err) {
		pr_debug("Preparing for frequency setup failed: %d\n", err);
		return err;
	}

	if (ext_csd[212] || ext_csd[213] || ext_csd[214] || ext_csd[215])
		mci->high_capacity = 1;

	cardtype = ext_csd[196] & 0xf;

	err = mci_switch(mci_dev, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);

	if (err) {
		pr_debug("MMC frequency changing failed: %d\n", err);
		return err;
	}

	/* Now check to see that it worked */
	err = mci_send_ext_csd(mci_dev, ext_csd);

	if (err) {
		pr_debug("Verifying frequency change failed: %d\n", err);
		return err;
	}

	/* No high-speed support */
	if (!ext_csd[185])
		return 0;

	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & MMC_HS_52MHZ)
		mci->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
	else
		mci->card_caps |= MMC_MODE_HS;

	return 0;
}

/**
 * FIXME
 * @param mci_dev MCI instance
 * @param mode FIXME
 * @param group FIXME
 * @param value FIXME
 * @param resp FIXME
 * @return Transaction status (0 on success)
 */
static int sd_switch(struct device_d *mci_dev, unsigned mode, unsigned group,
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

	return mci_send_cmd(mci_dev, &cmd, &data);
}

/**
 * Change transfer frequency for an SD card
 * @param mci_dev MCI instance
 * @return Transaction status (0 on success)
 */
static int sd_change_freq(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	struct mci_cmd cmd;
	struct mci_data data;
	uint32_t *switch_status = sector_buf;
	uint32_t *scr = sector_buf;
	int timeout;
	int err;

	pr_debug("Changing transfer frequency\n");
	mci->card_caps = 0;

	/* Read the SCR to find out if this card supports higher speeds */
	mci_setup_cmd(&cmd, MMC_CMD_APP_CMD, mci->rca << 16, MMC_RSP_R1);
	err = mci_send_cmd(mci_dev, &cmd, NULL);
	if (err) {
		pr_debug("Query SD card capabilities failed: %d\n", err);
		return err;
	}

	mci_setup_cmd(&cmd, SD_CMD_APP_SEND_SCR, 0, MMC_RSP_R1);

	timeout = 3;

retry_scr:
	pr_debug("Trying to read the SCR (try %d of %d)\n", 4 - timeout, 3);
	data.dest = (char *)scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = mci_send_cmd(mci_dev, &cmd, &data);
	if (err) {
		pr_debug(" Catch error (%d)", err);
		if (timeout--) {
			pr_debug("-- retrying\n");
			goto retry_scr;
		}
		pr_debug("-- giving up\n");
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

	/* Version 1.0 doesn't support switching */
	if (mci->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sd_switch(mci_dev, SD_SWITCH_CHECK, 0, 1,
				(uint8_t*)switch_status);
		if (err) {
			pr_debug("Checking SD transfer switch frequency feature failed: %d\n", err);
			return err;
		}

		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	if (mci->scr[0] & SD_DATA_4BIT)
		mci->card_caps |= MMC_MODE_4BIT;

	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	err = sd_switch(mci_dev, SD_SWITCH_SWITCH, 0, 1, (uint8_t*)switch_status);
	if (err) {
		pr_debug("Switching SD transfer frequency failed: %d\n", err);
		return err;
	}

	if ((__be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
		mci->card_caps |= MMC_MODE_HS;

	return 0;
}

/**
 * Setup host's interface bus width and transfer frequency
 * @param mci_dev MCI instance
 */
static void mci_set_ios(struct device_d *mci_dev)
{
	struct mci_host *host = GET_MCI_PDATA(mci_dev);

	host->set_ios(host, mci_dev, host->bus_width, host->clock);
}

/**
 * Setup host's interface transfer frequency
 * @param mci_dev MCI instance
 * @param clock New clock in Hz to set
 */
static void mci_set_clock(struct device_d *mci_dev, unsigned clock)
{
	struct mci_host *host = GET_MCI_PDATA(mci_dev);

	/* check against any given limits */
	if (clock > host->f_max)
		clock = host->f_max;

	if (clock < host->f_min)
		clock = host->f_min;

	host->clock = clock;	/* the new target frequency */
	mci_set_ios(mci_dev);
}

/**
 * Setup host's interface bus width
 * @param mci_dev MCI instance
 * @param width New interface bit width (1, 4 or 8)
 */
static void mci_set_bus_width(struct device_d *mci_dev, unsigned width)
{
	struct mci_host *host = GET_MCI_PDATA(mci_dev);

	host->bus_width = width;	/* the new target bus width */
	mci_set_ios(mci_dev);
}

/**
 * Extract card's version from its CSD
 * @param mci_dev MCI instance
 * @return 0 on success
 */
static void mci_detect_version_from_csd(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	int version;
	char *vstr;

	if (mci->version == MMC_VERSION_UNKNOWN) {
		/* the version is coded in the bits 127:126 (left aligned) */
		version = (mci->csd[0] >> 26) & 0xf;	/* FIXME why other width? */

		switch (version) {
		case 0:
			vstr = "1.2";
			mci->version = MMC_VERSION_1_2;
			break;
		case 1:
			vstr = "1.4";
			mci->version = MMC_VERSION_1_4;
			break;
		case 2:
			vstr = "2.2";
			mci->version = MMC_VERSION_2_2;
			break;
		case 3:
			vstr = "3.0";
			mci->version = MMC_VERSION_3;
			break;
		case 4:
			vstr = "4.0";
			mci->version = MMC_VERSION_4;
			break;
		default:
			vstr = "unknown, fallback to 1.2";
			mci->version = MMC_VERSION_1_2;
			break;
		}
		printf("detected card version %s\n", vstr);
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
 * @param mci_dev MCI instance
 *
 * Encoded in bit 103:96 (103: reserved, 102:99: time, 98:96 unit)
 */
static void mci_extract_max_tran_speed_from_csd(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	unsigned unit, time;

	unit = tran_speed_unit[(mci->csd[0] & 0x7)];
	time = tran_speed_time[((mci->csd[0] >> 3) & 0xf)];
	if ((unit == 0) || (time == 0)) {
		pr_debug("Unsupported 'TRAN_SPEED' unit/time value."
				" Can't calculate card's max. transfer speed\n");
		return;
	}

	mci->tran_speed = time * unit;
	pr_debug("Transfer speed: %u\n", mci->tran_speed);
}

/**
 * Extract max read and write block length from the CSD
 * @param mci_dev MCI instance
 *
 * Encoded in bit 83:80 (read) and 25:22 (write)
 */
static void mci_extract_block_lengths_from_csd(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);

	mci->read_bl_len = 1 << ((mci->csd[1] >> 16) & 0xf);

	if (IS_SD(mci))
		mci->write_bl_len = mci->read_bl_len;	/* FIXME why? */
	else
		mci->write_bl_len = 1 << ((mci->csd[3] >> 22) & 0xf);

	pr_debug("Max. block length are: Write=%u, Read=%u Bytes\n",
		mci->read_bl_len, mci->write_bl_len);
}

/**
 * Extract card's capacitiy from the CSD
 * @param mci_dev MCI instance
 */
static void mci_extract_card_capacity_from_csd(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	uint64_t csize, cmult;

	if (mci->high_capacity) {
		csize = (mci->csd[1] & 0x3f) << 16 | (mci->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (mci->csd[1] & 0x3ff) << 2 | (mci->csd[2] & 0xc0000000) >> 30;
		cmult = (mci->csd[2] & 0x00038000) >> 15;
	}

	mci->capacity = (csize + 1) << (cmult + 2);
	mci->capacity *= mci->read_bl_len;
	pr_debug("Capacity: %u MiB\n", (unsigned)mci->capacity >> 20);
}

/**
 * Scan the given host interfaces and detect connected MMC/SD cards
 * @param mci_dev MCI instance
 * @return 0 on success, negative value else
 */
static int mci_startup(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	struct mci_host *host = GET_MCI_PDATA(mci_dev);
	struct mci_cmd cmd;
	int err;

	pr_debug("Put the Card in Identify Mode\n");

	/* Put the Card in Identify Mode */
	mci_setup_cmd(&cmd, MMC_CMD_ALL_SEND_CID, 0, MMC_RSP_R2);
	err = mci_send_cmd(mci_dev, &cmd, NULL);
	if (err) {
		pr_debug("Can't bring card into identify mode: %d\n", err);
		return err;
	}

	memcpy(mci->cid, cmd.response, 16);

	pr_debug("Card's identification data is: %08X-%08X-%08X-%08X\n",
		mci->cid[0], mci->cid[1], mci->cid[2], mci->cid[3]);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	pr_debug("Get/Set relative address\n");
	mci_setup_cmd(&cmd, SD_CMD_SEND_RELATIVE_ADDR, mci->rca << 16, MMC_RSP_R6);
	err = mci_send_cmd(mci_dev, &cmd, NULL);
	if (err) {
		pr_debug("Get/Set relative address failed: %d\n", err);
		return err;
	}

	if (IS_SD(mci))
		mci->rca = (cmd.response[0] >> 16) & 0xffff;

	pr_debug("Get card's specific data\n");
	/* Get the Card-Specific Data */
	mci_setup_cmd(&cmd, MMC_CMD_SEND_CSD, mci->rca << 16, MMC_RSP_R2);
	err = mci_send_cmd(mci_dev, &cmd, NULL);
	if (err) {
		pr_debug("Getting card's specific data failed: %d\n", err);
		return err;
	}

	/* CSD is of 128 bit */
	memcpy(mci->csd, cmd.response, 16);

	pr_debug("Card's specific data is: %08X-%08X-%08X-%08X\n",
		mci->csd[0], mci->csd[1], mci->csd[2], mci->csd[3]);

	mci_detect_version_from_csd(mci_dev);
	mci_extract_max_tran_speed_from_csd(mci_dev);
	mci_extract_block_lengths_from_csd(mci_dev);
	mci_extract_card_capacity_from_csd(mci_dev);

	/* sanitiy? */
	if (mci->read_bl_len > 512) {
		mci->read_bl_len = 512;
		pr_debug("Limiting max. read block size down to %u\n",
				mci->read_bl_len);
	}

	if (mci->write_bl_len > 512) {
		mci->write_bl_len = 512;
		pr_debug("Limiting max. write block size down to %u\n",
				mci->read_bl_len);
	}
	pr_debug("Read block length: %u, Write block length: %u\n",
		mci->read_bl_len, mci->write_bl_len);

	pr_debug("Select the card, and put it into Transfer Mode\n");
	/* Select the card, and put it into Transfer Mode */
	mci_setup_cmd(&cmd, MMC_CMD_SELECT_CARD, mci->rca << 16, MMC_RSP_R1b);
	err = mci_send_cmd(mci_dev, &cmd, NULL);
	if (err) {
		pr_debug("Putting in transfer mode failed: %d\n", err);
		return err;
	}

	if (IS_SD(mci))
		err = sd_change_freq(mci_dev);
	else
		err = mmc_change_freq(mci_dev);

	if (err)
		return err;

	/* Restrict card's capabilities by what the host can do */
	mci->card_caps &= host->host_caps;

	if (IS_SD(mci)) {
		if (mci->card_caps & MMC_MODE_4BIT) {
			pr_debug("Prepare for bus width change\n");
			mci_setup_cmd(&cmd, MMC_CMD_APP_CMD, mci->rca << 16, MMC_RSP_R1);
			err = mci_send_cmd(mci_dev, &cmd, NULL);
			if (err) {
				pr_debug("Preparing SD for bus width change failed: %d\n", err);
				return err;
			}

			pr_debug("Set SD bus width to 4 bit\n");
			mci_setup_cmd(&cmd, SD_CMD_APP_SET_BUS_WIDTH, 2, MMC_RSP_R1);
			err = mci_send_cmd(mci_dev, &cmd, NULL);
			if (err) {
				pr_debug("Changing SD bus width failed: %d\n", err);
				/* TODO continue with 1 bit? */
				return err;
			}
			mci_set_bus_width(mci_dev, 4);
		}
		/* if possible, speed up the transfer */
		if (mci->card_caps & MMC_MODE_HS)
			mci_set_clock(mci_dev, 50000000);
		else
			mci_set_clock(mci_dev, 25000000);
	} else {
		if (mci->card_caps & MMC_MODE_4BIT) {
			pr_debug("Set MMC bus width to 4 bit\n");
			/* Set the card to use 4 bit*/
			err = mci_switch(mci_dev, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_4);
			if (err) {
				pr_debug("Changing MMC bus width failed: %d\n", err);
				return err;
			}
			mci_set_bus_width(mci_dev, 4);
		} else if (mci->card_caps & MMC_MODE_8BIT) {
			pr_debug("Set MMC bus width to 8 bit\n");
			/* Set the card to use 8 bit*/
			err = mci_switch(mci_dev, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_8);
			if (err) {
				pr_debug("Changing MMC bus width failed: %d\n", err);
				return err;
			}
			mci_set_bus_width(mci_dev, 8);
		}
		/* if possible, speed up the transfer */
		if (mci->card_caps & MMC_MODE_HS) {
			if (mci->card_caps & MMC_MODE_HS_52MHz)
				mci_set_clock(mci_dev, 52000000);
			else
				mci_set_clock(mci_dev, 26000000);
		} else
			mci_set_clock(mci_dev, 20000000);
	}

	/* we setup the blocklength only one times for all accesses to this media  */
	err = mci_set_blocklen(mci_dev, mci->read_bl_len);

	return err;
}

/**
 * Detect a SD 2.0 card and enable its features
 * @param mci_dev MCI instance
 * @return Transfer status (0 on success)
 *
 * By issuing the CMD8 command SDHC/SDXC cards realize that the host supports
 * the Physical Layer Version 2.00 or later and the card can enable
 * corresponding new functions.
 *
 * If this CMD8 command will end with a timeout it is a MultiMediaCard only.
 */
static int sd_send_if_cond(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	struct mci_host *host = GET_MCI_PDATA(mci_dev);
	struct mci_cmd cmd;
	int err;

	mci_setup_cmd(&cmd, SD_CMD_SEND_IF_COND,
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
		((host->voltages & 0x00ff8000) != 0) << 8 | 0xaa,
		MMC_RSP_R7);
	err = mci_send_cmd(mci_dev, &cmd, NULL);
	if (err) {
		pr_debug("Query interface conditions failed: %d\n", err);
		return err;
	}

	if ((cmd.response[0] & 0xff) != 0xaa) {
		pr_debug("Card cannot work with hosts supply voltages\n");
		return -EINVAL;
	} else {
		pr_debug("SD Card Rev. 2.00 or later detected\n");
		mci->version = SD_VERSION_2;
	}

	return 0;
}

/* ------------------ attach to the ATA API --------------------------- */

/**
 * Write a chunk of sectors to media
 * @param disk_dev Disk device instance
 * @param sector_start Sector's number to start write to
 * @param sector_count Sectors to write
 * @param buffer Buffer to write from
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to read all data!
 */
#ifdef CONFIG_MCI_WRITE
static int mci_sd_write(struct device_d *disk_dev, uint64_t sector_start,
			unsigned sector_count, const void *buffer)
{
	struct ata_interface *intf = disk_dev->platform_data;
	struct device_d *mci_dev = intf->priv;
	struct mci *mci = GET_MCI_DATA(mci_dev);
	int rc;

	pr_debug("%s: Write %u block(s), starting at %u\n",
		__func__, sector_count, (unsigned)sector_start);

	if (mci->write_bl_len != 512) {
		pr_debug("MMC/SD block size is not 512 bytes (its %u bytes instead)\n",
				mci->read_bl_len);
		return -EINVAL;
	}

	while (sector_count) {
		/* size of the block number field in the MMC/SD command is 32 bit only */
		if (sector_start > MAX_BUFFER_NUMBER) {
			pr_debug("Cannot handle block number %llu. Too large!\n",
				sector_start);
			return -EINVAL;
		}
		rc = mci_block_write(mci_dev, buffer, sector_start);
		if (rc != 0) {
			pr_debug("Writing block %u failed with %d\n", (unsigned)sector_start, rc);
			return rc;
		}
		sector_count--;
		buffer += mci->write_bl_len;
		sector_start++;
	}

	return 0;
}
#endif

/**
 * Read a chunk of sectors from media
 * @param disk_dev Disk device instance
 * @param sector_start Sector's number to start read from
 * @param sector_count Sectors to read
 * @param buffer Buffer to read into
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to store all data!
 */
static int mci_sd_read(struct device_d *disk_dev, uint64_t sector_start,
			unsigned sector_count, void *buffer)
{
	struct ata_interface *intf = disk_dev->platform_data;
	struct device_d *mci_dev = intf->priv;
	struct mci *mci = GET_MCI_DATA(mci_dev);
	int rc;

	pr_debug("%s: Read %u block(s), starting at %u\n",
		__func__, sector_count, (unsigned)sector_start);

	if (mci->read_bl_len != 512) {
		pr_debug("MMC/SD block size is not 512 bytes (its %u bytes instead)\n",
				mci->read_bl_len);
		return -EINVAL;
	}

	while (sector_count) {
		int now = min(sector_count, 32U);
		if (sector_start > MAX_BUFFER_NUMBER) {
			pr_err("Cannot handle block number %u. Too large!\n",
				(unsigned)sector_start);
			return -EINVAL;
		}
		rc = mci_read_block(mci_dev, buffer, (unsigned)sector_start, now);
		if (rc != 0) {
			pr_debug("Reading block %u failed with %d\n", (unsigned)sector_start, rc);
			return rc;
		}
		sector_count -= now;
		buffer += mci->read_bl_len * now;
		sector_start += now;
	}

	return 0;
}

/* ------------------ attach to the device API --------------------------- */

#ifdef CONFIG_MCI_INFO
/**
 * Extract the Manufacturer ID from the CID
 * @param mci Instance data
 *
 * The 'MID' is encoded in bit 127:120 in the CID
 */
static unsigned extract_mid(struct mci *mci)
{
	return mci->cid[0] >> 24;
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
	return (mci->cid[2] << 8) | (mci->cid[3] >> 24);
}

/**
 * Extract the month of the manufacturing date from the CID
 * @param mci Instance data
 *
 * The 'MTD' is encoded in bit 19:8 in the CID, month in 11:8
 */
static unsigned extract_mtd_month(struct mci *mci)
{
	return (mci->cid[3] >> 8) & 0xf;
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
	return ((mci->cid[3] >> 12) & 0xff) + 2000U;
}

/**
 * Output some valuable information when the user runs 'devinfo' on an MCI device
 * @param mci_dev MCI device instance
 */
static void mci_info(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);

	if (mci->ready_for_use == 0) {
		printf(" No information available:\n  MCI card not probed yet\n");
		return;
	}

	printf(" Card:\n");
	if (mci->version < SD_VERSION_SD) {
		printf("  Attached is a MultiMediaCard (Version: %u.%u)\n",
			(mci->version >> 4) & 0xf, mci->version & 0xf);
	} else {
		printf("  Attached is an SD Card (Version: %u.%u)\n",
			(mci->version >> 4) & 0xf, mci->version & 0xf);
	}
	printf("  Capacity: %u MiB\n", (unsigned)(mci->capacity >> 20));

	if (mci->high_capacity)
		printf("  High capacity card\n");
	printf("   CID: %08X-%08X-%08X-%08X\n", mci->cid[0], mci->cid[1],
		mci->cid[2], mci->cid[3]);
	printf("   CSD: %08X-%08X-%08X-%08X\n", mci->csd[0], mci->csd[1],
		mci->csd[2], mci->csd[3]);
	printf("  Max. transfer speed: %u Hz\n", mci->tran_speed);
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
#endif

/**
 * Check if the MCI card is already probed
 * @param mci_dev MCI device instance
 * @return 0 when not probed yet, -EPERM if already probed
 *
 * @a barebox cannot really cope with hot plugging. So, probing an attached
 * MCI card is a one time only job. If its already done, there is no way to
 * return.
 */
static int mci_check_if_already_initialized(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);

	if (mci->ready_for_use != 0)
		return -EPERM;

	return 0;
}

/**
 * Probe an MCI card at the given host interface
 * @param mci_dev MCI device instance
 * @return 0 on success, negative values else
 */
static int mci_card_probe(struct device_d *mci_dev)
{
	struct mci *mci = GET_MCI_DATA(mci_dev);
	struct mci_host *host = GET_MCI_PDATA(mci_dev);
	struct device_d *disk_dev;
	struct ata_interface *p;
	int rc;

	/* start with a host interface reset */
	rc = (host->init)(host, mci_dev);
	if (rc) {
		pr_err("Cannot reset the SD/MMC interface\n");
		return rc;
	}

	mci_set_bus_width(mci_dev, 1);
	mci_set_clock(mci_dev, 1);	/* set the lowest available clock */

	/* reset the card */
	rc = mci_go_idle(mci_dev);
	if (rc) {
		pr_warn("Cannot reset the SD/MMC card\n");
		goto on_error;
	}

	/* Check if this card can handle the "SD Card Physical Layer Specification 2.0" */
	rc = sd_send_if_cond(mci_dev);
	rc = sd_send_op_cond(mci_dev);
	if (rc && rc == -ETIMEDOUT) {
		/* If the command timed out, we check for an MMC card */
		pr_debug("Card seems to be a MultiMediaCard\n");
		rc = mmc_send_op_cond(mci_dev);
	}

	if (rc)
		goto on_error;

	rc = mci_startup(mci_dev);
	if (rc) {
		pr_debug("Card's startup fails with %d\n", rc);
		goto on_error;
	}

	pr_debug("Card is up and running now, registering as a disk\n");
	mci->ready_for_use = 1;	/* TODO now or later? */

	/*
	 * An MMC/SD card acts like an ordinary disk.
	 * So, re-use the disk driver to gain access to this media
	 */
	disk_dev = xzalloc(sizeof(struct device_d) + sizeof(struct ata_interface));
	p = (struct ata_interface*)&disk_dev[1];

#ifdef CONFIG_MCI_WRITE
	p->write = mci_sd_write;
#endif
	p->read = mci_sd_read;
	p->priv = mci_dev;

	strcpy(disk_dev->name, "disk");
	disk_dev->size = mci->capacity;
	disk_dev->map_base = 0;
	disk_dev->platform_data = p;

	register_device(disk_dev);

	pr_debug("SD Card successfully added\n");

on_error:
	if (rc != 0) {
		host->clock = 0;	/* disable the MCI clock */
		mci_set_ios(mci_dev);
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
static int mci_set_probe(struct device_d *mci_dev, struct param_d *param,
				const char *val)
{
	int rc, probe;

	rc = mci_check_if_already_initialized(mci_dev);
	if (rc != 0)
		return rc;

	probe = simple_strtoul(val, NULL, 0);
	if (probe != 0) {
		rc = mci_card_probe(mci_dev);
		if (rc != 0)
			return rc;
	}

	return dev_param_set_generic(mci_dev, param, val);
}

/**
 * Add parameter to the MCI device on demand
 * @param mci_dev MCI device instance
 * @return 0 on success
 *
 * This parameter is only available (or usefull) if MCI card probing is delayed
 */
static int add_mci_parameter(struct device_d *mci_dev)
{
	int rc;

	/* provide a 'probing right now' parameter for the user */
	rc = dev_add_param(mci_dev, "probe", mci_set_probe, NULL, 0);
	if (rc != 0)
		return rc;

	return dev_set_param(mci_dev, "probe", "0");
}

/**
 * Prepare for MCI card's usage
 * @param mci_dev MCI device instance
 * @return 0 on success
 *
 * This routine will probe an attached MCI card immediately or provide
 * a parameter to do it later on user's demand.
 */
static int mci_probe(struct device_d *mci_dev)
{
	struct mci *mci;
	int rc;

	mci = xzalloc(sizeof(struct mci));
	mci_dev->priv = mci;

#ifdef CONFIG_MCI_STARTUP
	/* if enabled, probe the attached card immediately */
	rc = mci_card_probe(mci_dev);
	if (rc == -ENODEV) {
		/*
		 * If it fails, add the 'probe' parameter to give the user
		 * a chance to insert a card and try again. Note: This may fail
		 * systems that rely on the MCI card for startup (for the
		 * persistant environment for example)
		 */
		rc = add_mci_parameter(mci_dev);
		if (rc != 0) {
			pr_debug("Failed to add 'probe' parameter to the MCI device\n");
			goto on_error;
		}
	}
#endif

#ifndef CONFIG_MCI_STARTUP
	/* add params on demand */
	rc = add_mci_parameter(mci_dev);
	if (rc != 0) {
		pr_debug("Failed to add 'probe' parameter to the MCI device\n");
		goto on_error;
	}
#endif

	return rc;

on_error:
	free(mci);
	return rc;
}

static struct driver_d mci_driver = {
	.name	= "mci",
	.probe	= mci_probe,
#ifdef CONFIG_MCI_INFO
	.info	= mci_info,
#endif
};

static int mci_init(void)
{
	sector_buf = xmemalign(32, 512);
	return register_driver(&mci_driver);
}

device_initcall(mci_init);

/**
 * Create a new mci device (for convenience)
 * @param host mci_host for this MCI device
 * @return 0 on success
 */
int mci_register(struct mci_host *host)
{
	struct device_d *mci_dev;

	mci_dev = xzalloc(sizeof(struct device_d));

	strcpy(mci_dev->name, mci_driver.name);
	mci_dev->platform_data = (void*)host;

	return register_device(mci_dev);
}
