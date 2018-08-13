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
#include <io.h>
#include <linux/err.h>
#include <mach/s3c-mci.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-iomap.h>

#define GET_HOST_DATA(x) (x->priv)
#define GET_MCI_PDATA(x) (x->platform_data)

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
	struct mci_host	host;
	void __iomem	*base;
	int		bus_width:2; /* 0 = 1 bit, 1 = 4 bit, 2 = 8 bit */
	unsigned	clock;	/* current clock in Hz */
	unsigned	data_size;	/* data transfer in bytes */
};

#define to_s3c_host(h)	container_of(h, struct s3c_mci_host, host)

/**
 * Finish a request
 * @param hw_dev Host interface instance
 *
 * Just a little bit paranoia.
 */
static void s3c_finish_request(struct s3c_mci_host *host_data)
{
	/* TODO ensure the engines are stopped */
}

/**
 * Setup a new clock frequency on this MCI bus
 * @param hw_dev Host interface instance
 * @param nc New clock value in Hz (can be 0)
 * @return New clock value (may differ from 'nc')
 */
static unsigned s3c_setup_clock_speed(struct s3c_mci_host *host_data, unsigned nc)
{
	unsigned clock;
	uint32_t mci_psc;

	if (nc == 0)
		return 0;

	clock = s3c_get_pclk();
	/* Calculate the required prescaler value to get the requested frequency */
	mci_psc = (clock + (nc >> 2)) / nc;

	if (mci_psc > 256) {
		mci_psc = 256;
		pr_warning("SD/MMC clock might be too high!\n");
	}

	writel(mci_psc - 1, host_data->base + SDIPRE);

	return clock / mci_psc;
}

/**
 * Reset the MCI engine (the hard way)
 * @param hw_dev Host interface instance
 *
 * This will reset everything in all registers of this unit!
 */
static void s3c_mci_reset(struct s3c_mci_host *host_data)
{
	/* reset the hardware */
	writel(SDICON_SDRESET, host_data->base + SDICON);
	/* wait until reset it finished */
	while (readl(host_data->base + SDICON) & SDICON_SDRESET)
		;
}

/**
 * Initialize hard and software
 * @param hw_dev Host interface instance
 * @param mci_dev MCI device instance (might be NULL)
 */
static int s3c_mci_initialize(struct s3c_mci_host *host_data, struct device_d *mci_dev)
{
	s3c_mci_reset(host_data);

	/* restore last settings */
	host_data->clock = s3c_setup_clock_speed(host_data, host_data->clock);
	writel(0x007FFFFF, host_data->base + SDITIMER);
	writel(SDICON_MMCCLOCK, host_data->base + SDICON);
	writel(512, host_data->base + SDIBSIZE);

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
static uint32_t s3c_prepare_data_setup(struct s3c_mci_host *host_data, unsigned data_flags)
{
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
static int s3c_terminate_transfer(struct s3c_mci_host *host_data)
{
	unsigned stoptries = 3;

	while (readl(host_data->base + SDIDSTA) & (SDIDSTA_TXDATAON | SDIDSTA_RXDATAON)) {
		pr_debug("Transfer still in progress.\n");

		writel(SDIDCON_STOP, host_data->base + SDIDCON);
		s3c_mci_initialize(host_data, NULL);

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
static int s3c_prepare_data_transfer(struct s3c_mci_host *host_data, struct mci_data *data)
{
	uint32_t reg;

	writel(data->blocksize, host_data->base + SDIBSIZE);
	reg = s3c_prepare_data_setup(host_data, data->flags);
	reg |= data->blocks & SDIDCON_BLKNUM;
	writel(reg, host_data->base + SDIDCON);
	writel(0x007FFFFF, host_data->base + SDITIMER);

	return 0;
}

/**
 * Send a command and receive the response
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 */
static int s3c_send_command(struct s3c_mci_host *host_data, struct mci_cmd *cmd,
				struct mci_data *data)
{
	uint32_t reg, t1;
	int rc;

	writel(0x007FFFFF, host_data->base + SDITIMER);

	/* setup argument */
	writel(cmd->cmdarg, host_data->base + SDICMDARG);

	/* setup command and transfer characteristic */
	reg = s3c_prepare_command_setup(cmd->resp_type, data != NULL ? data->flags : 0);
	reg |= cmd->cmdidx & SDICMDCON_INDEX;

	/* run the command right now */
	writel(reg | SDICMDCON_CMDSTART, host_data->base + SDICMDCON);
	t1 = readl(host_data->base + SDICMDSTAT);
	/* wait until command is done */
	while (1) {
		reg = readl(host_data->base + SDICMDSTAT);
		/* done? */
		if (cmd->resp_type & MMC_RSP_PRESENT) {
			if (reg & SDICMDSTAT_RSPFIN) {
				writel(SDICMDSTAT_RSPFIN,
					host_data->base + SDICMDSTAT);
				rc = 0;
				break;
			}
		} else {
			if (reg & SDICMDSTAT_CMDSENT) {
					writel(SDICMDSTAT_CMDSENT,
						host_data->base + SDICMDSTAT);
					rc = 0;
					break;
			}
		}
		/* timeout? */
		if (reg & SDICMDSTAT_CMDTIMEOUT) {
			writel(SDICMDSTAT_CMDTIMEOUT,
				host_data->base + SDICMDSTAT);
			rc = -ETIMEDOUT;
			break;
		}
	}

	if ((rc == 0) && (cmd->resp_type & MMC_RSP_PRESENT)) {
		cmd->response[0] = readl(host_data->base + SDIRSP0);
		cmd->response[1] = readl(host_data->base + SDIRSP1);
		cmd->response[2] = readl(host_data->base + SDIRSP2);
		cmd->response[3] = readl(host_data->base + SDIRSP3);
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
static int s3c_prepare_engine(struct s3c_mci_host *host_data)
{
	int rc;

	rc = s3c_terminate_transfer(host_data);
	if (rc != 0)
		return rc;

	writel(-1, host_data->base + SDICMDSTAT);
	writel(-1, host_data->base + SDIDSTA);
	writel(-1, host_data->base + SDIFSTA);

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
static int s3c_mci_std_cmds(struct s3c_mci_host *host_data, struct mci_cmd *cmd)
{
	int rc;

	rc = s3c_prepare_engine(host_data);
	if (rc != 0)
		return 0;

	return s3c_send_command(host_data, cmd, NULL);
}

/**
 * Read one block of data from the FIFO
 * @param hw_dev Host interface device instance
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 */
static int s3c_mci_read_block(struct s3c_mci_host *host_data, struct mci_data *data)
{
	uint32_t *p;
	unsigned cnt, data_size;

#define READ_REASON_TO_FAIL (SDIDSTA_CRCFAIL | SDIDSTA_RXCRCFAIL | SDIDSTA_DATATIMEOUT)

	p = (uint32_t*)data->dest;
	data_size = data->blocksize * data->blocks;

	while (data_size > 0) {

		/* serious error? */
		if (readl(host_data->base + SDIDSTA) & READ_REASON_TO_FAIL) {
			pr_err("Failed while reading data\n");
			return -EIO;
		}

		/* now check the FIFO status */
		if (readl(host_data->base + SDIFSTA) & SDIFSTA_FIFOFAIL) {
			pr_err("Data loss due to FIFO overflow when reading\n");
			return -EIO;
		}

		/* we only want to read full words */
		cnt = (readl(host_data->base + SDIFSTA) & SDIFSTA_COUNTMASK) >> 2;

		/* read one chunk of data from the FIFO */
		while (cnt--) {
			*p = readl(host_data->base + SDIDATA);
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
static int s3c_mci_write_block(struct s3c_mci_host *host_data, struct mci_cmd *cmd,
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
		writel(*p, host_data->base + SDIDATA);
		p++;
		if (data_size >= 4)
			data_size -= 4;
		else {
			data_size = 0;
			break;
		}
	}

	/* data is now in place and waits for transmitt. Start the command right now */
	s3c_send_command(host_data, cmd, data);

	if ((reg = readl(host_data->base + SDIFSTA)) & SDIFSTA_FIFOFAIL) {
		pr_err("Command fails immediatly due to FIFO underrun when writing %08X\n",
			reg);
		return -EIO;
	}

	while (data_size > 0) {

		if (readl(host_data->base + SDIDSTA) & WRITE_REASON_TO_FAIL) {
			pr_err("Failed writing data\n");
			return -EIO;
		}

		/* now check the FIFO status */
		if ((reg = readl(host_data->base + SDIFSTA)) & SDIFSTA_FIFOFAIL) {
			pr_err("Data loss due to FIFO underrun when writing %08X\n",
					reg);
			return -EIO;
		}

		/* we only want to write full words */
		cnt = 16 - (((readl(host_data->base + SDIFSTA) & SDIFSTA_COUNTMASK) + 3) >> 2);

		/* fill the FIFO if it has free entries */
		while (cnt--) {
			writel(*p, host_data->base + SDIDATA);
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
static int s3c_mci_adtc(struct s3c_mci_host *host_data, struct mci_cmd *cmd,
			struct mci_data *data)
{
	int rc;

	rc = s3c_prepare_engine(host_data);
	if (rc != 0)
		return rc;

	rc = s3c_prepare_data_transfer(host_data, data);
	if (rc != 0)
		return rc;

	if (data->flags & MMC_DATA_READ) {
		s3c_send_command(host_data, cmd, data);
		rc = s3c_mci_read_block(host_data, data);
		if (rc == 0) {
			while (!(readl(host_data->base + SDIDSTA) & SDIDSTA_XFERFINISH))
				;
		} else
			s3c_terminate_transfer(host_data);
	}

	if (data->flags & MMC_DATA_WRITE) {
		rc = s3c_mci_write_block(host_data, cmd, data);
		if (rc == 0) {
			while (!(readl(host_data->base + SDIDSTA) & SDIDSTA_XFERFINISH))
				;
		} else
			s3c_terminate_transfer(host_data);
	}
	writel(0, host_data->base + SDIDCON);

	return rc;
}

/* ------------------------- MCI API -------------------------------------- */

/**
 * Keep the attached MMC/SD unit in a well know state
 * @param host MCI host
 * @param mci_dev MCI device instance
 * @return 0 on success, negative value else
 */
static int mci_reset(struct mci_host *host, struct device_d *mci_dev)
{
	struct s3c_mci_host *host_data = to_s3c_host(host);

	return s3c_mci_initialize(host_data, mci_dev);
}

/**
 * Process one command to the MCI card
 * @param host MCI host
 * @param cmd The command to process
 * @param data The data to handle in the command (can be NULL)
 * @return 0 on success, negative value else
 */
static int mci_request(struct mci_host *host, struct mci_cmd *cmd,
			struct mci_data *data)
{
	struct s3c_mci_host *host_data = to_s3c_host(host);
	int rc;

	/* enable clock */
	writel(readl(host_data->base + SDICON) | SDICON_CLKEN,
		host_data->base + SDICON);

	if ((cmd->resp_type == 0) || (data == NULL))
		rc = s3c_mci_std_cmds(host_data, cmd);
	else
		rc = s3c_mci_adtc(host_data, cmd, data);	/* with response and data */

	s3c_finish_request(host_data);

	/* disable clock */
	writel(readl(host_data->base + SDICON) & ~SDICON_CLKEN,
		host_data->base + SDICON);
	return rc;
}

/**
 * Setup the bus width and IO speed
 * @param host MCI host
 * @param bus_width New bus width value (1, 4 or 8)
 * @param clock New clock in Hz (can be '0' to disable the clock)
 */
static void mci_set_ios(struct mci_host *host, struct mci_ios *ios)
{
	struct s3c_mci_host *host_data = to_s3c_host(host);
	uint32_t reg;

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		host_data->bus_width = 1;
		break;
	case MMC_BUS_WIDTH_1:
		host_data->bus_width = 0;
		break;
	default:
		return;
	}

	reg = readl(host_data->base + SDICON);
	if (ios->clock) {
		/* setup the IO clock frequency and enable it */
		host_data->clock = s3c_setup_clock_speed(host_data, ios->clock);
		reg |= SDICON_CLKEN;	/* enable the clock */
	} else {
		reg &= ~SDICON_CLKEN;	/* disable the clock */
		host_data->clock = 0;
	}
	writel(reg, host_data->base + SDICON);

	pr_debug("IO settings: bus width=%d, frequency=%u Hz\n",
		host_data->bus_width, host_data->clock);
}

/* ----------------------------------------------------------------------- */

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

static int s3c_mci_probe(struct device_d *hw_dev)
{
	struct resource *iores;
	struct s3c_mci_host *s3c_host;
	struct s3c_mci_platform_data *pd = hw_dev->platform_data;

	s3c_host = xzalloc(sizeof(*s3c_host));
	s3c_host->host.send_cmd = mci_request;
	s3c_host->host.set_ios = mci_set_ios;
	s3c_host->host.init = mci_reset;

	/* TODO replace by the global func: enable the SDI unit clock */
	writel(readl(S3C_CLOCK_POWER_BASE + 0x0c) | 0x200,
		S3C_CLOCK_POWER_BASE + 0x0c);

	if (pd == NULL) {
		pr_err("Missing platform data\n");
		return -EINVAL;
	}

	hw_dev->priv = s3c_host;
	iores = dev_request_mem_resource(hw_dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	s3c_host->base = IOMEM(iores->start);

	s3c_host->host.hw_dev = hw_dev;

	/* feed forward the platform specific values */
	s3c_host->host.voltages = pd->voltages;
	s3c_host->host.host_caps = pd->caps;
	s3c_host->host.f_min = pd->f_min == 0 ? s3c_get_pclk() / 256 : pd->f_min;
	s3c_host->host.f_max = pd->f_max == 0 ? s3c_get_pclk() / 2 : pd->f_max;

	if (IS_ENABLED(CONFIG_MCI_INFO))
		hw_dev->info = s3c_info;

	/*
	 * Start the clock to let the engine and the card finishes its startup
	 */
	s3c_host->clock = s3c_setup_clock_speed(s3c_host, pd->f_min);
	writel(SDICON_FIFORESET | SDICON_MMCCLOCK, s3c_host->base + SDICON);

	return mci_register(&s3c_host->host);
}

static struct driver_d s3c_mci_driver = {
        .name  = "s3c_mci",
        .probe = s3c_mci_probe,
};
device_platform_driver(s3c_mci_driver);
