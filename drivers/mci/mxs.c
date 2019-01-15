/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <jbe@pengutronix.de>
 *
 * This code is based on:
 *
 * Copyright (C) 2007 SigmaTel, Inc., Ioannis Kappas <ikappas@sigmatel.com>
 *
 * Portions copyright (C) 2003 Russell King, PXA MMCI Driver
 * Portions copyright (C) 2004-2005 Pierre Ossman, W83L51xD SD/MMC driver
 *
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
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
 * @brief MCI card host interface for i.MX23 CPU
 */

/* #define DEBUG */

#include <common.h>
#include <init.h>
#include <mci.h>
#include <errno.h>
#include <clock.h>
#include <io.h>
#include <stmp-device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm/bitops.h>
#include <mach/mci.h>
#include <mach/clock.h>
#include <mach/ssp.h>

#define CLOCKRATE_MIN (1 * 1000 * 1000)
#define CLOCKRATE_MAX (480 * 1000 * 1000)

struct mxs_mci_host {
	struct mci_host	host;
	void __iomem	*regs;
	struct clk	*clk;
	unsigned	clock;	/* current clock speed in Hz ("0" if disabled) */
	unsigned	f_min;
	unsigned	f_max;
	unsigned	bus_width:2; /* 0 = 1 bit, 1 = 4 bit, 2 = 8 bit */
};

#define to_mxs_mci(mxs) container_of(mxs, struct mxs_mci_host, host)

/**
 * Get MCI cards response if defined for the type of command
 * @param hw_dev Host interface device instance
 * @param cmd Command description
 * @return Response bytes count, -EINVAL for unsupported response types
 */
static int mxs_mci_get_cards_response(struct mxs_mci_host *mxs_mci, struct mci_cmd *cmd)
{
	switch (cmd->resp_type) {
	case MMC_RSP_NONE:
		return 0;

	case MMC_RSP_R1:
	case MMC_RSP_R1b:
	case MMC_RSP_R3:
		cmd->response[0] = readl(mxs_mci->regs + HW_SSP_SDRESP0);
		return 1;

	case MMC_RSP_R2:
		cmd->response[3] = readl(mxs_mci->regs + HW_SSP_SDRESP0);
		cmd->response[2] = readl(mxs_mci->regs + HW_SSP_SDRESP1);
		cmd->response[1] = readl(mxs_mci->regs + HW_SSP_SDRESP2);
		cmd->response[0] = readl(mxs_mci->regs + HW_SSP_SDRESP3);
		return 4;
	}

	return -EINVAL;
}

/**
 * Finish a request to the MCI card
 * @param hw_dev Host interface device instance
 *
 * Can also stop the clock to save power
 */
static void mxs_mci_finish_request(struct mxs_mci_host *mxs_mci)
{
	/* stop the engines (normaly already done) */
	writel(SSP_CTRL0_RUN, mxs_mci->regs + HW_SSP_CTRL0 + 8);
}

/**
 * Check if the last command failed and if, why it failed
 * @param status HW_SSP_STATUS's content
 * @return 0 if no error, negative values else
 */
static int mxs_mci_get_cmd_error(struct mxs_mci_host *mxs_mci, unsigned status)
{
	if (status & SSP_STATUS_ERROR)
		dev_dbg(mxs_mci->host.hw_dev, "Status Reg reports %08X\n", status);

	if (status & SSP_STATUS_TIMEOUT) {
		dev_dbg(mxs_mci->host.hw_dev, "CMD timeout\n");
		return -ETIMEDOUT;
	} else if (status & SSP_STATUS_RESP_TIMEOUT) {
		dev_dbg(mxs_mci->host.hw_dev, "RESP timeout\n");
		return -ETIMEDOUT;
	} else if (status & SSP_STATUS_RESP_CRC_ERR) {
		dev_dbg(mxs_mci->host.hw_dev, "CMD crc error\n");
		return -EILSEQ;
	} else if (status & SSP_STATUS_RESP_ERR) {
		dev_dbg(mxs_mci->host.hw_dev, "RESP error\n");
		return -EIO;
	}

	return 0;
}

/**
 * Define the timout for the next command
 * @param hw_dev Host interface device instance
 * @param to Timeout value in MCI card's bus clocks
 */
static void mxs_mci_setup_timeout(struct mxs_mci_host *mxs_mci, unsigned to)
{
	uint32_t reg;

	reg = readl(mxs_mci->regs + HW_SSP_TIMING) & ~SSP_TIMING_TIMEOUT_MASK;
	reg |= SSP_TIMING_TIMEOUT(to);
	writel(reg, mxs_mci->regs + HW_SSP_TIMING);
}

/**
 * Read data from the MCI card
 * @param hw_dev Host interface device instance
 * @param buffer To write data into
 * @param length Count of bytes to read (must be multiples of 4)
 * @return 0 on success, negative values else
 *
 * @note This routine uses PIO to read in the data bytes from the FIFO. This
 * may fail with high clock speeds. If you receive -EIO errors you can try
 * again with reduced clock speeds.
 */
static int mxs_mci_read_data(struct mxs_mci_host *mxs_mci, void *buffer, unsigned length)
{
	uint32_t *p = buffer;

	if (length & 0x3) {
		dev_dbg(mxs_mci->host.hw_dev,
				"Cannot read data sizes not multiple of 4 (request for %u detected)\n",
				length);
		return -EINVAL;
	}

	while ((length != 0) &&
		((readl(mxs_mci->regs + HW_SSP_STATUS) & SSP_STATUS_ERROR) == 0)) {
		/* TODO sort out FIFO overflows and emit -EOI for this case */
		if ((readl(mxs_mci->regs + HW_SSP_STATUS) & SSP_STATUS_FIFO_EMPTY) == 0) {
			*p = readl(mxs_mci->regs + HW_SSP_DATA);
			p++;
			length -= 4;
		}
	}

	if (length == 0)
		return 0;

	return -EIO;
}


/**
 * Write data into the MCI card
 * @param hw_dev Host interface device instance
 * @param buffer To read the data from
 * @param length Count of bytes to write (must be multiples of 4)
 * @return 0 on success, negative values else
 *
 * @note This routine uses PIO to write the data bytes into the FIFO. This
 * may fail with high clock speeds. If you receive -EIO errors you can try
 * again with reduced clock speeds.
 */
static int mxs_mci_write_data(struct mxs_mci_host *mxs_mci, const void *buffer, unsigned length)
{
	const uint32_t *p = buffer;

	if (length & 0x3) {
		dev_dbg(mxs_mci->host.hw_dev,
				"Cannot write data sizes not multiple of 4 (request for %u detected)\n",
				length);
		return -EINVAL;
	}

	while ((length != 0) &&
		((readl(mxs_mci->regs + HW_SSP_STATUS) & SSP_STATUS_ERROR) == 0)) {
		/* TODO sort out FIFO overflows and emit -EOI for this case */
		if ((readl(mxs_mci->regs + HW_SSP_STATUS) & SSP_STATUS_FIFO_FULL) == 0) {
			writel(*p, mxs_mci->regs + HW_SSP_DATA);
			p++;
			length -= 4;
		}
	}
	if (length == 0)
		return 0;

	return -EIO;
}

/**
 * Start the transaction with or without data
 * @param hw_dev Host interface device instance
 * @param data Data transfer description (might be NULL)
 * @return 0 on success
 */
static int mxs_mci_transfer_data(struct mxs_mci_host *mxs_mci, struct mci_data *data)
{
	/*
	 * Everything is ready for the transaction now:
	 * - transfer configuration
	 * - command and its parameters
	 *
	 * Start the transaction right now
	 */
	writel(SSP_CTRL0_RUN, mxs_mci->regs + HW_SSP_CTRL0 + 4);

	if (data != NULL) {
		unsigned length = data->blocks * data->blocksize;

		if (data->flags & MMC_DATA_READ)
			return mxs_mci_read_data(mxs_mci, data->dest, length);
		else
			return mxs_mci_write_data(mxs_mci, data->src, length);
	}

	return 0;
}

/**
 * Configure the MCI hardware for the next transaction
 * @param cmd_flags Command information
 * @param data_flags Data information (may be 0)
 * @return Corresponding setting for the SSP_CTRL0 register
 */
static uint32_t mxs_mci_prepare_transfer_setup(unsigned cmd_flags, unsigned data_flags)
{
	uint32_t reg = 0;

	if (cmd_flags & MMC_RSP_PRESENT)
		reg |= SSP_CTRL0_GET_RESP;
	if ((cmd_flags & MMC_RSP_CRC) == 0)
		reg |= SSP_CTRL0_IGNORE_CRC;
	if (cmd_flags & MMC_RSP_136)
		reg |= SSP_CTRL0_LONG_RESP;
	if (cmd_flags & MMC_RSP_BUSY)
		reg |= SSP_CTRL0_WAIT_FOR_IRQ;	/* FIXME correct? */
#if 0
	if (cmd_flags & MMC_RSP_OPCODE)
		/* TODO */
#endif
	if (data_flags & MMC_DATA_READ)
		reg |= SSP_CTRL0_READ;

	return reg;
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
static int mxs_mci_std_cmds(struct mxs_mci_host *mxs_mci, struct mci_cmd *cmd)
{
	/* setup command and transfer parameters */
	writel(mxs_mci_prepare_transfer_setup(cmd->resp_type, 0) |
		SSP_CTRL0_ENABLE, mxs_mci->regs + HW_SSP_CTRL0);

	/* prepare the command, when no response is expected add a few trailing clocks */
	writel(SSP_CMD0_CMD(cmd->cmdidx) |
		(cmd->resp_type & MMC_RSP_PRESENT ? 0 : SSP_CMD0_APPEND_8CYC),
		mxs_mci->regs + HW_SSP_CMD0);

	/* prepare command's arguments */
	writel(cmd->cmdarg, mxs_mci->regs + HW_SSP_CMD1);

	mxs_mci_setup_timeout(mxs_mci, 0xffff);

	/* start the transfer */
	writel(SSP_CTRL0_RUN, mxs_mci->regs + HW_SSP_CTRL0 + 4);

	/* wait until finished */
	while (readl(mxs_mci->regs + HW_SSP_CTRL0) & SSP_CTRL0_RUN)
		;

	if (cmd->resp_type & MMC_RSP_PRESENT)
		mxs_mci_get_cards_response(mxs_mci, cmd);

	return mxs_mci_get_cmd_error(mxs_mci, readl(mxs_mci->regs + HW_SSP_STATUS));
}

/**
 * Handle an "addressed data transfer command " with or without data
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.) May be NULL
 * @return 0 on success
 */
static int mxs_mci_adtc(struct mxs_mci_host *mxs_mci, struct mci_cmd *cmd,
			struct mci_data *data)
{
	uint32_t xfer_cnt, log2blocksize, block_cnt;
	int err;

	/* Note: 'data' can be NULL! */
	if (data != NULL) {
		xfer_cnt = data->blocks * data->blocksize;
		block_cnt = data->blocks - 1;	/* can be 0 */
		log2blocksize = find_first_bit((const unsigned long*)&data->blocksize,
						32);
	} else
		xfer_cnt = log2blocksize = block_cnt = 0;

	/* setup command and transfer parameters */
#ifdef CONFIG_ARCH_IMX23
	writel(mxs_mci_prepare_transfer_setup(cmd->resp_type, data != NULL ? data->flags : 0) |
		SSP_CTRL0_BUS_WIDTH(mxs_mci->bus_width) |
		(xfer_cnt != 0 ? SSP_CTRL0_DATA_XFER : 0) | /* command plus data */
		SSP_CTRL0_ENABLE |
		SSP_CTRL0_XFER_COUNT(xfer_cnt), /* byte count to be transfered */
		mxs_mci->regs + HW_SSP_CTRL0);

	/* prepare the command and the transfered data count */
	writel(SSP_CMD0_CMD(cmd->cmdidx) |
		SSP_CMD0_BLOCK_SIZE(log2blocksize) |
		SSP_CMD0_BLOCK_COUNT(block_cnt) |
		(cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION ? SSP_CMD0_APPEND_8CYC : 0),
		mxs_mci->regs + HW_SSP_CMD0);
#endif
#ifdef CONFIG_ARCH_IMX28
	writel(mxs_mci_prepare_transfer_setup(cmd->resp_type, data != NULL ? data->flags : 0) |
		SSP_CTRL0_BUS_WIDTH(mxs_mci->bus_width) |
		(xfer_cnt != 0 ? SSP_CTRL0_DATA_XFER : 0) | /* command plus data */
		SSP_CTRL0_ENABLE,
		mxs_mci->regs + HW_SSP_CTRL0);
	writel(xfer_cnt, mxs_mci->regs + HW_SSP_XFER_COUNT);

	/* prepare the command and the transfered data count */
	writel(SSP_CMD0_CMD(cmd->cmdidx) |
		(cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION ? SSP_CMD0_APPEND_8CYC : 0),
		mxs_mci->regs + HW_SSP_CMD0);
	writel(SSP_BLOCK_SIZE(log2blocksize) |
		SSP_BLOCK_COUNT(block_cnt),
		mxs_mci->regs + HW_SSP_BLOCK_SIZE);
#endif

	/* prepare command's arguments */
	writel(cmd->cmdarg, mxs_mci->regs + HW_SSP_CMD1);

	mxs_mci_setup_timeout(mxs_mci, 0xffff);

	err = mxs_mci_transfer_data(mxs_mci, data);
	if (err != 0) {
		dev_dbg(mxs_mci->host.hw_dev, "Transfering data failed with %d\n", err);
		return err;
	}

	/* wait until finished */
	while (readl(mxs_mci->regs + HW_SSP_CTRL0) & SSP_CTRL0_RUN)
		;

	mxs_mci_get_cards_response(mxs_mci, cmd);

	return 0;
}


/**
 * @param hw_dev Host interface device instance
 * @param nc New Clock in [Hz] (may be 0 to disable the clock)
 * @return The real clock frequency
 *
 * The SSP unit clock can base on the external 24 MHz or the internal 480 MHz
 * Its unit clock value is derived from the io clock, from the SSP divider
 * and at least the SSP bus clock itself is derived from the SSP unit's divider
 *
 * @code
 * |------------------- generic -------------|-peripheral specific-|-----all SSPs-----|-per SSP unit-|
 *  24 MHz ----------------------------
 *          \                          \
 *           \                          |----| FRAC |----IO CLK----| SSP unit DIV |---| SSP DIV |--- SSP output clock
 *            \- | PLL |--- 480 MHz ---/
 * @endcode
 *
 * @note Up to "SSP unit DIV" the outer world must care. This routine only
 * handles the "SSP DIV".
 */
static unsigned mxs_mci_setup_clock_speed(struct mxs_mci_host *mxs_mci, unsigned nc)
{
	unsigned ssp, div, rate, reg;

	if (nc == 0U) {
		/* TODO stop the clock */
		return 0;
	}

	ssp = clk_get_rate(mxs_mci->clk);

	for (div = 2; div < 255; div += 2) {
		rate = DIV_ROUND_CLOSEST(DIV_ROUND_CLOSEST(ssp, nc), div);
		if (rate <= 0x100)
			break;
	}
	if (div >= 255) {
		dev_warn(mxs_mci->host.hw_dev, "Cannot set clock to %d Hz\n", nc);
		return 0;
	}

	reg = readl(mxs_mci->regs + HW_SSP_TIMING) & SSP_TIMING_TIMEOUT_MASK;
	reg |= SSP_TIMING_CLOCK_DIVIDE(div) | SSP_TIMING_CLOCK_RATE(rate - 1);
	writel(reg, mxs_mci->regs + HW_SSP_TIMING);

	return ssp / div / rate;
}

/* ------------------------- MCI API -------------------------------------- */

/**
 * Keep the attached MMC/SD unit in a well know state
 * @param mci_pdata MCI platform data
 * @param mci_dev MCI device instance
 * @return 0 on success, negative value else
 */
static int mxs_mci_initialize(struct mci_host *host, struct device_d *mci_dev)
{
	struct mxs_mci_host *mxs_mci = to_mxs_mci(host);

	/* enable the clock to this unit to be able to reset it */
	writel(SSP_CTRL0_CLKGATE, mxs_mci->regs + HW_SSP_CTRL0 + 8);

	/* reset the unit */
	stmp_reset_block(mxs_mci->regs + HW_SSP_CTRL0, 0);

	/* restore the last settings */
	mxs_mci_setup_timeout(mxs_mci, 0xffff);
	writel(SSP_CTRL0_IGNORE_CRC |
		SSP_CTRL0_BUS_WIDTH(mxs_mci->bus_width),
		mxs_mci->regs + HW_SSP_CTRL0);
	writel(SSP_CTRL1_POLARITY |
		SSP_CTRL1_SSP_MODE(3) |
		SSP_CTRL1_WORD_LENGTH(7), mxs_mci->regs + HW_SSP_CTRL1);

	return 0;
}

/**
 * Process one command to the MCI card
 * @param mci_pdata MCI platform data
 * @param cmd The command to process
 * @param data The data to handle in the command (can be NULL)
 * @return 0 on success, negative value else
 */
static int mxs_mci_request(struct mci_host *host, struct mci_cmd *cmd,
			struct mci_data *data)
{
	struct mxs_mci_host *mxs_mci = to_mxs_mci(host);
	int rc;

	if ((cmd->resp_type == 0) || (data == NULL))
		rc = mxs_mci_std_cmds(mxs_mci, cmd);
	else
		rc = mxs_mci_adtc(mxs_mci, cmd, data);	/* with response and data */

	mxs_mci_finish_request(mxs_mci);	/* TODO */
	return rc;
}

/**
 * Setup the bus width and IO speed
 * @param mci_pdata MCI platform data
 * @param mci_dev MCI device instance
 * @param bus_width New bus width value (1, 4 or 8)
 * @param clock New clock in Hz (can be '0' to disable the clock)
 *
 * Drivers currently realized values are stored in MCI's platformdata
 */
static void mxs_mci_set_ios(struct mci_host *host, struct mci_ios *ios)
{
	struct mxs_mci_host *mxs_mci = to_mxs_mci(host);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		mxs_mci->bus_width = 2;
		pr_debug("IO settings: changing bus width to 8 bits\n");
		break;
	case MMC_BUS_WIDTH_4:
		mxs_mci->bus_width = 1;
		pr_debug("IO settings: changing bus width to 4 bits\n");
		break;
	case MMC_BUS_WIDTH_1:
		mxs_mci->bus_width = 0;
		pr_debug("IO settings: changing bus width to 1 bit\n");
		break;
	default:
		pr_debug("IO settings: unsupported bus width!\n");
		return;
	}

	mxs_mci->clock = mxs_mci_setup_clock_speed(mxs_mci, ios->clock);

	dev_dbg(host->hw_dev, "IO settings: frequency=%u Hz\n", mxs_mci->clock);
}

/* ----------------------------------------------------------------------- */

const unsigned char bus_width[3] = { 1, 4, 8 };

static void mxs_mci_info(struct device_d *hw_dev)
{
	struct mxs_mci_host *mxs_mci = hw_dev->priv;

	printf(" Interface\n");
	printf("  Min. bus clock: %u Hz\n", mxs_mci->f_min);
	printf("  Max. bus clock: %u Hz\n", mxs_mci->f_max);
	printf("  Current bus clock: %u Hz\n", mxs_mci->clock);
	printf("  Bus width: %u bit\n", bus_width[mxs_mci->bus_width]);
	printf("\n");
}

static int mxs_mci_probe(struct device_d *hw_dev)
{
	struct resource *iores;
	struct mxs_mci_platform_data *pd = hw_dev->platform_data;
	struct mxs_mci_host *mxs_mci;
	struct mci_host *host;
	unsigned long rate;

	mxs_mci = xzalloc(sizeof(*mxs_mci));
	host = &mxs_mci->host;

	hw_dev->priv = mxs_mci;
	host->hw_dev = hw_dev;
	host->send_cmd = mxs_mci_request;
	host->set_ios = mxs_mci_set_ios;
	host->init = mxs_mci_initialize;
	iores = dev_request_mem_resource(hw_dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	mxs_mci->regs = IOMEM(iores->start);

	/* feed forward the platform specific values */
	if (pd) {
		host->voltages = pd->voltages;
		host->host_caps = pd->caps;
		host->devname = pd->devname;
		host->f_min = pd->f_min;
		host->f_max =  pd->f_max;
	} else {
		/* fixed to 3.3 V */
		host->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

		mci_of_parse(host);
	}

	mxs_mci->clk = clk_get(hw_dev, NULL);
	if (IS_ERR(mxs_mci->clk))
		return PTR_ERR(mxs_mci->clk);

	clk_enable(mxs_mci->clk);

	rate = clk_get_rate(mxs_mci->clk);

	if (host->f_min < 400000)
		host->f_min = 400000;
	if (host->f_max == 0)
		host->f_max = rate / 2 / 1;

	dev_dbg(hw_dev, "Max. frequency is %u Hz\n", host->f_max);
	dev_dbg(hw_dev, "Min. frequency is %u Hz\n", host->f_min);

	if (IS_ENABLED(CONFIG_MCI_INFO)) {
		mxs_mci->f_min = host->f_min;
		mxs_mci->f_max = host->f_max;
		hw_dev->info = mxs_mci_info;
	}

	return mci_register(host);
}

static __maybe_unused struct of_device_id mxs_mmc_compatible[] = {
	{
		.compatible = "fsl,imx23-mmc",
	}, {
		.compatible = "fsl,imx28-mmc",
	}, {
		/* sentinel */
	}
};

static struct driver_d mxs_mci_driver = {
        .name  = "mxs_mci",
        .probe = mxs_mci_probe,
	.of_compatible = DRV_OF_COMPAT(mxs_mmc_compatible),
};
device_platform_driver(mxs_mci_driver);
