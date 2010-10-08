/*
 * Copyright (C) 2010 Juergen Beisert <juergen@kreuzholzen.de>
 *
 * This code is partially based on u-boot code:
 *
 * This code is based on various Linux and u-boot sources:
 *  Copyright (C) 2004-2006 maintech GmbH, Thomas Kleffel <tk@maintech.de>
 *  Copyright (C) 2008 Simtec Electronics <ben-linux@fluff.org>
 *  (C) Copyright 2006 by OpenMoko, Inc.
 *  Author: Harald Welte <laforge@openmoko.org>
 *  based on u-boot pxa MMC driver and linux/drivers/mmc/s3c2410mci.c
 *  (C) 2005-2005 Thomas Kleffel
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

/**
 * @file
 * @brief MCI card host interface for S3C2440 CPU
 */

/* #define DEBUG */

#include <common.h>
#include <init.h>
#include <mci.h>
#include <errno.h>
#include <clock.h>
#include <asm/io.h>
#include <mach/mci.h>
#include <mach/s3c24xx-generic.h>
#include <mach/s3c24x0-iomap.h>

#define SDICON 0x0
# define SDICON_SDRESET (1 << 8)
# define SDICON_MMCCLOCK (1 << 5) /* this is a clock type SD or MMC style WTF? */
# define SDICON_BYTEORDER (1 << 4)
# define SDICON_SDIOIRQ (1 << 3)
# define SDICON_RWAITEN (1 << 2)
# define SDICON_FIFORESET (1 << 1) /* reserved bit on 2440 ????? */
# define SDICON_CLKEN (1 << 0) /* enable/disable external clock */

#define SDIPRE 0x4

#define SDICMDARG 0x8

#define SDICMDCON 0xc
# define SDICMDCON_ABORT (1 << 12)
# define SDICMDCON_WITHDATA (1 << 11)
# define SDICMDCON_LONGRSP (1 << 10)
# define SDICMDCON_WAITRSP (1 << 9)
# define SDICMDCON_CMDSTART (1 << 8)
# define SDICMDCON_SENDERHOST (1 << 6)
# define SDICMDCON_INDEX (0x3f)

#define SDICMDSTAT 0x10
# define SDICMDSTAT_CRCFAIL (1 << 12)
# define SDICMDSTAT_CMDSENT (1 << 11)
# define SDICMDSTAT_CMDTIMEOUT (1 << 10)
# define SDICMDSTAT_RSPFIN (1 << 9)
# define SDICMDSTAT_XFERING (1 << 8)
# define SDICMDSTAT_INDEX (0xff)

#define SDIRSP0 0x14
#define SDIRSP1 0x18
#define SDIRSP2 0x1C
#define SDIRSP3 0x20

#define SDITIMER 0x24
#define SDIBSIZE 0x28

#define SDIDCON 0x2c
# define SDIDCON_DS_BYTE (0 << 22)
# define SDIDCON_DS_HALFWORD (1 << 22)
# define SDIDCON_DS_WORD (2 << 22)
# define SDIDCON_IRQPERIOD (1 << 21)
# define SDIDCON_TXAFTERRESP (1 << 20)
# define SDIDCON_RXAFTERCMD (1 << 19)
# define SDIDCON_BUSYAFTERCMD (1 << 18)
# define SDIDCON_BLOCKMODE (1 << 17)
# define SDIDCON_WIDEBUS (1 << 16)
# define SDIDCON_DMAEN (1 << 15)
# define SDIDCON_STOP (0 << 14)
# define SDIDCON_DATSTART (1 << 14)
# define SDIDCON_DATMODE (3 << 12)
# define SDIDCON_BLKNUM (0xfff)
# define SDIDCON_XFER_READY    (0 << 12)
# define SDIDCON_XFER_CHKSTART (1 << 12)
# define SDIDCON_XFER_RXSTART  (2 << 12)
# define SDIDCON_XFER_TXSTART  (3 << 12)

#define SDIDCNT 0x30
# define SDIDCNT_BLKNUM_SHIFT 12

#define SDIDSTA 0x34
# define SDIDSTA_RDYWAITREQ (1 << 10)
# define SDIDSTA_SDIOIRQDETECT (1 << 9)
# define SDIDSTA_FIFOFAIL (1 << 8) /* reserved on 2440 */
# define SDIDSTA_CRCFAIL (1 << 7)
# define SDIDSTA_RXCRCFAIL (1 << 6)
# define SDIDSTA_DATATIMEOUT (1 << 5)
# define SDIDSTA_XFERFINISH (1 << 4)
# define SDIDSTA_BUSYFINISH (1 << 3)
# define SDIDSTA_SBITERR (1 << 2) /* reserved on 2410a/2440 */
# define SDIDSTA_TXDATAON (1 << 1)
# define SDIDSTA_RXDATAON (1 << 0)

#define SDIFSTA 0x38
# define SDIFSTA_FIFORESET (1<<16)
# define SDIFSTA_FIFOFAIL (3<<14)  /* 3 is correct (2 bits) */
# define SDIFSTA_TFDET (1<<13)
# define SDIFSTA_RFDET (1<<12)
# define SDIFSTA_TFHALF (1<<11)
# define SDIFSTA_TFEMPTY (1<<10)
# define SDIFSTA_RFLAST (1<<9)
# define SDIFSTA_RFFULL (1<<8)
# define SDIFSTA_RFHALF (1<<7)
# define SDIFSTA_COUNTMASK (0x7f)

#define SDIIMSK 0x3C
# define SDIIMSK_RESPONSECRC    (1<<17)
# define SDIIMSK_CMDSENT        (1<<16)
# define SDIIMSK_CMDTIMEOUT     (1<<15)
# define SDIIMSK_RESPONSEND     (1<<14)
# define SDIIMSK_READWAIT       (1<<13)
# define SDIIMSK_SDIOIRQ        (1<<12)
# define SDIIMSK_FIFOFAIL       (1<<11)
# define SDIIMSK_CRCSTATUS      (1<<10)
# define SDIIMSK_DATACRC        (1<<9)
# define SDIIMSK_DATATIMEOUT    (1<<8)
# define SDIIMSK_DATAFINISH     (1<<7)
# define SDIIMSK_BUSYFINISH     (1<<6)
# define SDIIMSK_SBITERR        (1<<5) /* reserved 2440/2410a */
# define SDIIMSK_TXFIFOHALF     (1<<4)
# define SDIIMSK_TXFIFOEMPTY    (1<<3)
# define SDIIMSK_RXFIFOLAST     (1<<2)
# define SDIIMSK_RXFIFOFULL     (1<<1)
# define SDIIMSK_RXFIFOHALF     (1<<0)

#define SDIDATA 0x40

struct s3c_mci_host {
	int		bus_width:2; /* 0 = 1 bit, 1 = 4 bit, 2 = 8 bit */
	unsigned	clock;	/* current clock in Hz */
	unsigned	data_size;	/* data transfer in bytes */
};

/*
 * There is only one host MCI hardware instance available.
 * It makes no sense to dynamically allocate this data
 */
static struct s3c_mci_host host_data;

/**
 * Finish a request
 * @param hw_dev Host interface instance
 *
 * Just a little bit paranoia.
 */
static void s3c_finish_request(struct device_d *hw_dev)
{
	/* TODO ensure the engines are stopped */
}

/* TODO GPIO feature is required for this architecture */
static unsigned gpio_get_value(unsigned val)
{
	return 0;
}

/**
 * Detect if a card is plugged in
 * @param hw_dev Host interface instance
 * @return 0 if a card is plugged in
 *
 * Note: If there is no GPIO registered to detect if a card is present, we
 * assume a card _is_ present.
 */
static int s3c_mci_card_present(struct device_d *hw_dev)
{
	struct s3c_mci_platform_data *pd = GET_HOST_PDATA(hw_dev);
	int ret;

	if (pd->gpio_detect == 0)
		return 0;	/* assume the card is present */

	ret = gpio_get_value(pd->gpio_detect) ? 0 : 1;
	return ret ^ pd->detect_invert;
}

/**
 * Setup a new clock frequency on this MCI bus
 * @param hw_dev Host interface instance
 * @param nc New clock value in Hz (can be 0)
 * @return New clock value (may differ from 'nc')
 */
static unsigned s3c_setup_clock_speed(struct device_d *hw_dev, unsigned nc)
{
	unsigned clock;
	uint32_t mci_psc;

	if (nc == 0)
		return 0;

	clock = s3c24xx_get_pclk();
	/* Calculate the required prescaler value to get the requested frequency */
	mci_psc = (clock + (nc >> 2)) / nc;

	if (mci_psc > 256) {
		mci_psc = 256;
		pr_warning("SD/MMC clock might be too high!\n");
	}

	writel(mci_psc - 1, hw_dev->map_base + SDIPRE);

	return clock / mci_psc;
}

/**
 * Reset the MCI engine (the hard way)
 * @param hw_dev Host interface instance
 *
 * This will reset everything in all registers of this unit!
 */
static void s3c_mci_reset(struct device_d *hw_dev)
{
	/* reset the hardware */
	writel(SDICON_SDRESET, hw_dev->map_base + SDICON);
	/* wait until reset it finished */
	while (readl(hw_dev->map_base + SDICON) & SDICON_SDRESET)
		;
}

/**
 * Initialize hard and software
 * @param hw_dev Host interface instance
 * @param mci_dev MCI device instance (might be NULL)
 */
static int s3c_mci_initialize(struct device_d *hw_dev, struct device_d *mci_dev)
{
	struct s3c_mci_host *host_data = GET_HOST_DATA(hw_dev);

	s3c_mci_reset(hw_dev);

	/* restore last settings */
	host_data->clock = s3c_setup_clock_speed(hw_dev, host_data->clock);
	writel(0x007FFFFF, hw_dev->map_base + SDITIMER);
	writel(SDICON_MMCCLOCK, hw_dev->map_base + SDICON);
	writel(512, hw_dev->map_base + SDIBSIZE);

	return 0;
}

/**
 * Prepare engine's bits for the next command transfer
 * @param cmd_flags MCI's command flags
 * @param data_flags MCI's data flags
 * @return Register bits for this transfer
 */
static uint32_t s3c_prepare_command_setup(unsigned cmd_flags, unsigned data_flags)
{
	uint32_t reg;

	/* source (=host) */
	reg = SDICMDCON_SENDERHOST;

	if (cmd_flags & MMC_RSP_PRESENT) {
		reg |= SDICMDCON_WAITRSP;
		pr_debug("Command with response\n");
	}
	if (cmd_flags & MMC_RSP_136) {
		reg |= SDICMDCON_LONGRSP;
		pr_debug("Command with long response\n");
	}
	if (cmd_flags & MMC_RSP_CRC)
		; /* FIXME */
	if (cmd_flags & MMC_RSP_BUSY)
		; /* FIXME */
	if (cmd_flags & MMC_RSP_OPCODE)
		; /* FIXME */
	if (data_flags != 0)
		reg |= SDICMDCON_WITHDATA;

	return reg;
}

/**
 * Prepare engine's bits for the next data transfer
 * @param hw_dev Host interface device instance
 * @param data_flags MCI's data flags
 * @return Register bits for this transfer
 */
static uint32_t s3c_prepare_data_setup(struct device_d *hw_dev, unsigned data_flags)
{
	struct s3c_mci_host *host_data = (struct s3c_mci_host*)GET_HOST_DATA(hw_dev);
	uint32_t reg = SDIDCON_BLOCKMODE;	/* block mode only is supported */

	if (host_data->bus_width == 1)
		reg |= SDIDCON_WIDEBUS;

	/* enable any kind of data transfers on demand only */
	if (data_flags & MMC_DATA_WRITE)
		reg |= SDIDCON_TXAFTERRESP | SDIDCON_XFER_TXSTART;

	if (data_flags & MMC_DATA_READ)
		reg |= SDIDCON_RXAFTERCMD | SDIDCON_XFER_RXSTART;

	/* TODO: Support more than the 2440 CPU */
	reg |= SDIDCON_DS_WORD | SDIDCON_DATSTART;

	return reg;
}

/**
 * Terminate a current running transfer
 * @param hw_dev Host interface device instance
 * @return 0 on success
 *
 * Note: Try to stop a running transfer. This should not happen, as all
 * transfers must complete in this driver. But who knows... ;-)
 */
static int s3c_terminate_transfer(struct device_d *hw_dev)
{
	unsigned stoptries = 3;

	while (readl(hw_dev->map_base + SDIDSTA) & (SDIDSTA_TXDATAON | SDIDSTA_RXDATAON)) {
		pr_debug("Transfer still in progress.\n");

		writel(SDIDCON_STOP, hw_dev->map_base + SDIDCON);
		s3c_mci_initialize(hw_dev, NULL);

		if ((stoptries--) == 0) {
			pr_warning("Cannot stop the engine!\n");
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * Setup registers for data transfer
 * @param hw_dev Host interface device instance
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 */
static int s3c_prepare_data_transfer(struct device_d *hw_dev, struct mci_data *data)
{
	uint32_t reg;

	writel(data->blocksize, hw_dev->map_base + SDIBSIZE);
	reg = s3c_prepare_data_setup(hw_dev, data->flags);
	reg |= data->blocks & SDIDCON_BLKNUM;
	writel(reg, hw_dev->map_base + SDIDCON);
	writel(0x007FFFFF, hw_dev->map_base + SDITIMER);

	return 0;
}

/**
 * Send a command and receive the response
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 */
static int s3c_send_command(struct device_d *hw_dev, struct mci_cmd *cmd,
				struct mci_data *data)
{
	uint32_t reg, t1;
	int rc;

	writel(0x007FFFFF, hw_dev->map_base + SDITIMER);

	/* setup argument */
	writel(cmd->cmdarg, hw_dev->map_base + SDICMDARG);

	/* setup command and transfer characteristic */
	reg = s3c_prepare_command_setup(cmd->resp_type, data != NULL ? data->flags : 0);
	reg |= cmd->cmdidx & SDICMDCON_INDEX;

	/* run the command right now */
	writel(reg | SDICMDCON_CMDSTART, hw_dev->map_base + SDICMDCON);
	t1 = readl(hw_dev->map_base + SDICMDSTAT);
	/* wait until command is done */
	while (1) {
		reg = readl(hw_dev->map_base + SDICMDSTAT);
		/* done? */
		if (cmd->resp_type & MMC_RSP_PRESENT) {
			if (reg & SDICMDSTAT_RSPFIN) {
				writel(SDICMDSTAT_RSPFIN,
					hw_dev->map_base + SDICMDSTAT);
				rc = 0;
				break;
			}
		} else {
			if (reg & SDICMDSTAT_CMDSENT) {
					writel(SDICMDSTAT_CMDSENT,
						hw_dev->map_base + SDICMDSTAT);
					rc = 0;
					break;
			}
		}
		/* timeout? */
		if (reg & SDICMDSTAT_CMDTIMEOUT) {
			writel(SDICMDSTAT_CMDTIMEOUT,
				hw_dev->map_base + SDICMDSTAT);
			rc = -ETIMEDOUT;
			break;
		}
	}

	if ((rc == 0) && (cmd->resp_type & MMC_RSP_PRESENT)) {
		cmd->response[0] = readl(hw_dev->map_base + SDIRSP0);
		cmd->response[1] = readl(hw_dev->map_base + SDIRSP1);
		cmd->response[2] = readl(hw_dev->map_base + SDIRSP2);
		cmd->response[3] = readl(hw_dev->map_base + SDIRSP3);
	}
	/* do not disable the clock! */
	return rc;
}

/**
 * Clear major registers prior a new transaction
 * @param hw_dev Host interface device instance
 * @return 0 on success
 *
 * FIFO clear is only necessary on 2440, but doesn't hurt on 2410
 */
static int s3c_prepare_engine(struct device_d *hw_dev)
{
	int rc;

	rc = s3c_terminate_transfer(hw_dev);
	if (rc != 0)
		return rc;

	writel(-1, hw_dev->map_base + SDICMDSTAT);
	writel(-1, hw_dev->map_base + SDIDSTA);
	writel(-1, hw_dev->map_base + SDIFSTA);

	return 0;
}

/**
 * Handle MCI commands without data
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @return 0 on success
 *
 * This functions handles the following MCI commands:
 * - "broadcast command (BC)" without a response
 * - "broadcast commands with response (BCR)"
 * - "addressed command (AC)" with response, but without data
 */
static int s3c_mci_std_cmds(struct device_d *hw_dev, struct mci_cmd *cmd)
{
	int rc;

	rc = s3c_prepare_engine(hw_dev);
	if (rc != 0)
		return 0;

	return s3c_send_command(hw_dev, cmd, NULL);
}

/**
 * Read one block of data from the FIFO
 * @param hw_dev Host interface device instance
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 */
static int s3c_mci_read_block(struct device_d *hw_dev, struct mci_data *data)
{
	uint32_t *p;
	unsigned cnt, data_size;

#define READ_REASON_TO_FAIL (SDIDSTA_CRCFAIL | SDIDSTA_RXCRCFAIL | SDIDSTA_DATATIMEOUT)

	p = (uint32_t*)data->dest;
	data_size = data->blocksize * data->blocks;

	while (data_size > 0) {

		/* serious error? */
		if (readl(hw_dev->map_base + SDIDSTA) & READ_REASON_TO_FAIL) {
			pr_err("Failed while reading data\n");
			return -EIO;
		}

		/* now check the FIFO status */
		if (readl(hw_dev->map_base + SDIFSTA) & SDIFSTA_FIFOFAIL) {
			pr_err("Data loss due to FIFO overflow when reading\n");
			return -EIO;
		}

		/* we only want to read full words */
		cnt = (readl(hw_dev->map_base + SDIFSTA) & SDIFSTA_COUNTMASK) >> 2;

		/* read one chunk of data from the FIFO */
		while (cnt--) {
			*p = readl(hw_dev->map_base + SDIDATA);
			p++;
			if (data_size >= 4)
				data_size -= 4;
			else {
				data_size = 0;
				break;
			}
		}
	}

	return 0;
}

/**
 * Write one block of data into the FIFO
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 *
 * We must ensure data in the FIFO when the command phase changes into the
 * data phase. To ensure this, the FIFO gets filled first, then the command.
 */
static int s3c_mci_write_block(struct device_d *hw_dev, struct mci_cmd *cmd,
				struct mci_data *data)
{
	const uint32_t *p = (const uint32_t*)data->src;
	unsigned cnt, data_size;
	uint32_t reg;

#define WRITE_REASON_TO_FAIL (SDIDSTA_CRCFAIL | SDIDSTA_DATATIMEOUT)

	data_size = data->blocksize * data->blocks;
	/*
	 * With high clock rates we must fill the FIFO as early as possible
	 * Its size is 16 words. We assume its empty, when this function is
	 * entered.
	 */
	cnt = 16;
	while (cnt--) {
		writel(*p, hw_dev->map_base + SDIDATA);
		p++;
		if (data_size >= 4)
			data_size -= 4;
		else {
			data_size = 0;
			break;
		}
	}

	/* data is now in place and waits for transmitt. Start the command right now */
	s3c_send_command(hw_dev, cmd, data);

	if ((reg = readl(hw_dev->map_base + SDIFSTA)) & SDIFSTA_FIFOFAIL) {
		pr_err("Command fails immediatly due to FIFO underrun when writing %08X\n",
			reg);
		return -EIO;
	}

	while (data_size > 0) {

		if (readl(hw_dev->map_base + SDIDSTA) & WRITE_REASON_TO_FAIL) {
			pr_err("Failed writing data\n");
			return -EIO;
		}

		/* now check the FIFO status */
		if ((reg = readl(hw_dev->map_base + SDIFSTA)) & SDIFSTA_FIFOFAIL) {
			pr_err("Data loss due to FIFO underrun when writing %08X\n",
					reg);
			return -EIO;
		}

		/* we only want to write full words */
		cnt = 16 - (((readl(hw_dev->map_base + SDIFSTA) & SDIFSTA_COUNTMASK) + 3) >> 2);

		/* fill the FIFO if it has free entries */
		while (cnt--) {
			writel(*p, hw_dev->map_base + SDIDATA);
			p++;
			if (data_size >= 4)
				data_size -= 4;
			else {
				data_size = 0;
				break;
			}
		}
	}

	return 0;
}

/**
 * Handle MCI commands with or without data
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
*/
static int s3c_mci_adtc(struct device_d *hw_dev, struct mci_cmd *cmd,
			struct mci_data *data)
{
	int rc;

	rc = s3c_prepare_engine(hw_dev);
	if (rc != 0)
		return rc;

	rc = s3c_prepare_data_transfer(hw_dev, data);
	if (rc != 0)
		return rc;

	if (data->flags & MMC_DATA_READ) {
		s3c_send_command(hw_dev, cmd, data);
		rc = s3c_mci_read_block(hw_dev, data);
		if (rc == 0) {
			while (!(readl(hw_dev->map_base + SDIDSTA) & SDIDSTA_XFERFINISH))
				;
		} else
			s3c_terminate_transfer(hw_dev);
	}

	if (data->flags & MMC_DATA_WRITE) {
		rc = s3c_mci_write_block(hw_dev, cmd, data);
		if (rc == 0) {
			while (!(readl(hw_dev->map_base + SDIDSTA) & SDIDSTA_XFERFINISH))
				;
		} else
			s3c_terminate_transfer(hw_dev);
	}
	writel(0, hw_dev->map_base + SDIDCON);

	return rc;
}

/* ------------------------- MCI API -------------------------------------- */

/**
 * Keep the attached MMC/SD unit in a well know state
 * @param mci_pdata MCI platform data
 * @param mci_dev MCI device instance
 * @return 0 on success, negative value else
 */
static int mci_reset(struct mci_host *mci_pdata, struct device_d *mci_dev)
{
	struct device_d *hw_dev = mci_pdata->hw_dev;

	return s3c_mci_initialize(hw_dev, mci_dev);
}

/**
 * Process one command to the MCI card
 * @param mci_pdata MCI platform data
 * @param cmd The command to process
 * @param data The data to handle in the command (can be NULL)
 * @return 0 on success, negative value else
 */
static int mci_request(struct mci_host *mci_pdata, struct mci_cmd *cmd,
			struct mci_data *data)
{
	struct device_d *hw_dev = mci_pdata->hw_dev;
	int rc;

	/* enable clock */
	writel(readl(hw_dev->map_base + SDICON) | SDICON_CLKEN,
		hw_dev->map_base + SDICON);

	if ((cmd->resp_type == 0) || (data == NULL))
		rc = s3c_mci_std_cmds(hw_dev, cmd);
	else
		rc = s3c_mci_adtc(hw_dev, cmd, data);	/* with response and data */

	s3c_finish_request(hw_dev);

	/* disable clock */
	writel(readl(hw_dev->map_base + SDICON) & ~SDICON_CLKEN,
		hw_dev->map_base + SDICON);
	return rc;
}

/**
 * Setup the bus width and IO speed
 * @param mci_pdata MCI platform data
 * @param mci_dev MCI device instance
 * @param bus_width New bus width value (1, 4 or 8)
 * @param clock New clock in Hz (can be '0' to disable the clock)
 */
static void mci_set_ios(struct mci_host *mci_pdata, struct device_d *mci_dev,
			unsigned bus_width, unsigned clock)
{
	struct device_d *hw_dev = mci_pdata->hw_dev;
	struct s3c_mci_host *host_data = GET_HOST_DATA(hw_dev);
	struct mci_host *host = GET_MCI_PDATA(mci_dev);
	uint32_t reg;

	switch (bus_width) {
	case 8:		/* no 8 bit support, fall back to 4 bit */
	case 4:
		host_data->bus_width = 1;
		host->bus_width = 4;	/* 4 bit is possible */
		break;
	default:
		host_data->bus_width = 0;
		host->bus_width = 1;	/* 1 bit is possible */
		break;
	}

	reg = readl(hw_dev->map_base + SDICON);
	if (clock) {
		/* setup the IO clock frequency and enable it */
		host->clock = host_data->clock = s3c_setup_clock_speed(hw_dev, clock);
		reg |= SDICON_CLKEN;	/* enable the clock */
	} else {
		reg &= ~SDICON_CLKEN;	/* disable the clock */
		host->clock = host_data->clock = 0;
	}
	writel(reg, hw_dev->map_base + SDICON);

	pr_debug("IO settings: bus width=%d, frequency=%u Hz\n",
		host->bus_width, host->clock);
}

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_MCI_INFO
static void s3c_info(struct device_d *hw_dev)
{
	struct s3c_mci_host *host = hw_dev->priv;
	struct s3c_mci_platform_data *pd = hw_dev->platform_data;

	printf("  Bus data width: %d bit\n", host->bus_width == 1 ? 4 : 1);
	printf("  Bus frequency: %u Hz\n", host->clock);
	printf("   Frequency limits: ");
	if (pd->f_min == 0)
		printf("no lower limit ");
	else
		printf("%u Hz lower limit ", pd->f_min);
	if (pd->f_max == 0)
		printf("- no upper limit");
	else
		printf("- %u Hz upper limit", pd->f_max);
	printf("\n  Card detection support: %s\n",
		pd->gpio_detect != 0 ? "yes" : "no");
}
#endif

/*
 * There is only one host MCI hardware instance available.
 * It makes no sense to dynamically allocate this data
 */
static struct mci_host mci_pdata = {
	.send_cmd = mci_request,
	.set_ios = mci_set_ios,
	.init = mci_reset,
};

static int s3c_mci_probe(struct device_d *hw_dev)
{
	struct s3c_mci_platform_data *pd = hw_dev->platform_data;

	/* TODO replace by the global func: enable the SDI unit clock */
	writel(readl(S3C24X0_CLOCK_POWER_BASE + 0x0c) | 0x200,
		S3C24X0_CLOCK_POWER_BASE + 0x0c);

	if (pd == NULL) {
		pr_err("Missing platform data\n");
		return -EINVAL;
	}

	hw_dev->priv = &host_data;
	mci_pdata.hw_dev = hw_dev;

	/* feed forward the platform specific values */
	mci_pdata.voltages = pd->voltages;
	mci_pdata.host_caps = pd->caps;
	mci_pdata.f_min = pd->f_min == 0 ? s3c24xx_get_pclk() / 256 : pd->f_min;
	mci_pdata.f_max = pd->f_max == 0 ? s3c24xx_get_pclk() / 2 : pd->f_max;

	/*
	 * Start the clock to let the engine and the card finishes its startup
	 */
	host_data.clock = s3c_setup_clock_speed(hw_dev, mci_pdata.f_min);
	writel(SDICON_FIFORESET | SDICON_MMCCLOCK, hw_dev->map_base + SDICON);

	return mci_register(&mci_pdata);
}

static struct driver_d s3c_mci_driver = {
        .name  = "s3c_mci",
        .probe = s3c_mci_probe,
#ifdef CONFIG_MCI_INFO
	.info = s3c_info,
#endif
};

static int s3c_mci_init_driver(void)
{
        register_driver(&s3c_mci_driver);
        return 0;
}

device_initcall(s3c_mci_init_driver);
