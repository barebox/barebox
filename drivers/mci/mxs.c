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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <asm/io.h>
#include <asm/bitops.h>
#include <mach/imx-regs.h>
#include <mach/mci.h>
#include <mach/clock.h>

#define CLOCKRATE_MIN (1 * 1000 * 1000)
#define CLOCKRATE_MAX (480 * 1000 * 1000)

#define HW_SSP_CTRL0 0x000
# define SSP_CTRL0_SFTRST (1 << 31)
# define SSP_CTRL0_CLKGATE (1 << 30)
# define SSP_CTRL0_RUN (1 << 29)
# define SSP_CTRL0_LOCK_CS (1 << 27)
# define SSP_CTRL0_READ (1 << 25)
# define SSP_CTRL0_IGNORE_CRC (1 << 26)
# define SSP_CTRL0_DATA_XFER (1 << 24)
# define SSP_CTRL0_BUS_WIDTH(x) (((x) & 0x3) << 22)
# define SSP_CTRL0_WAIT_FOR_IRQ (1 << 21)
# define SSP_CTRL0_LONG_RESP (1 << 19)
# define SSP_CTRL0_GET_RESP (1 << 17)
# define SSP_CTRL0_ENABLE (1 << 16)
#ifdef CONFIG_ARCH_IMX23
# define SSP_CTRL0_XFER_COUNT(x) ((x) & 0xffff)
#endif

#define HW_SSP_CMD0 0x010
# define SSP_CMD0_SLOW_CLK (1 << 22)
# define SSP_CMD0_CONT_CLK (1 << 21)
# define SSP_CMD0_APPEND_8CYC (1 << 20)
#ifdef CONFIG_ARCH_IMX23
# define SSP_CMD0_BLOCK_SIZE(x) (((x) & 0xf) << 16)
# define SSP_CMD0_BLOCK_COUNT(x) (((x) & 0xff) << 8)
#endif
# define SSP_CMD0_CMD(x) ((x) & 0xff)

#define HW_SSP_CMD1 0x020

#ifdef CONFIG_ARCH_IMX23
# define HW_SSP_COMPREF 0x030
# define HW_SSP_COMPMASK 0x040
# define HW_SSP_TIMING 0x050
# define HW_SSP_CTRL1 0x060
# define HW_SSP_DATA 0x070
#endif
#ifdef CONFIG_ARCH_IMX28
# define HW_SSP_XFER_COUNT 0x30
# define HW_SSP_BLOCK_SIZE 0x40
#  define SSP_BLOCK_SIZE(x) ((x) & 0xf)
#  define SSP_BLOCK_COUNT(x) (((x) & 0xffffff) << 4)
# define HW_SSP_COMPREF 0x050
# define HW_SSP_COMPMASK 0x060
# define HW_SSP_TIMING 0x070
# define HW_SSP_CTRL1 0x080
# define HW_SSP_DATA 0x090
#endif
/* bit definition for register HW_SSP_TIMING */
# define SSP_TIMING_TIMEOUT_MASK (0xffff0000)
# define SSP_TIMING_TIMEOUT(x) ((x) << 16)
# define SSP_TIMING_CLOCK_DIVIDE(x) (((x) & 0xff) << 8)
# define SSP_TIMING_CLOCK_RATE(x) ((x) & 0xff)

/* bit definition for register HW_SSP_CTRL1 */
# define SSP_CTRL1_POLARITY (1 << 9)
# define SSP_CTRL1_WORD_LENGTH(x) (((x) & 0xf) << 4)
# define SSP_CTRL1_SSP_MODE(x) ((x) & 0xf)

#ifdef CONFIG_ARCH_IMX23
# define HW_SSP_SDRESP0 0x080
# define HW_SSP_SDRESP1 0x090
# define HW_SSP_SDRESP2 0x0A0
# define HW_SSP_SDRESP3 0x0B0
#endif
#ifdef CONFIG_ARCH_IMX28
# define HW_SSP_SDRESP0 0x0A0
# define HW_SSP_SDRESP1 0x0B0
# define HW_SSP_SDRESP2 0x0C0
# define HW_SSP_SDRESP3 0x0D0
#endif

#ifdef CONFIG_ARCH_IMX28
# define HW_SSP_DDR_CTRL 0x0E0
# define HW_SSP_DLL_CTRL 0x0F0
#endif

#ifdef CONFIG_ARCH_IMX23
# define HW_SSP_STATUS 0x0C0
#endif
#ifdef CONFIG_ARCH_IMX28
# define HW_SSP_STATUS 0x100
#endif

/* bit definition for register HW_SSP_STATUS */
# define SSP_STATUS_PRESENT (1 << 31)
# define SSP_STATUS_SD_PRESENT (1 << 29)
# define SSP_STATUS_CARD_DETECT (1 << 28)
# define SSP_STATUS_RESP_CRC_ERR (1 << 16)
# define SSP_STATUS_RESP_ERR (1 << 15)
# define SSP_STATUS_RESP_TIMEOUT (1 << 14)
# define SSP_STATUS_DATA_CRC_ERR (1 << 13)
# define SSP_STATUS_TIMEOUT (1 << 12)
# define SSP_STATUS_FIFO_OVRFLW (1 << 9)
# define SSP_STATUS_FIFO_FULL (1 << 8)
# define SSP_STATUS_FIFO_EMPTY (1 << 5)
# define SSP_STATUS_FIFO_UNDRFLW (1 << 4)
# define SSP_STATUS_CMD_BUSY (1 << 3)
# define SSP_STATUS_DATA_BUSY (1 << 2)
# define SSP_STATUS_BUSY (1 << 0)
# define SSP_STATUS_ERROR (SSP_STATUS_FIFO_OVRFLW | SSP_STATUS_FIFO_UNDRFLW | \
	SSP_STATUS_RESP_CRC_ERR | SSP_STATUS_RESP_ERR | \
	SSP_STATUS_RESP_TIMEOUT | SSP_STATUS_DATA_CRC_ERR | SSP_STATUS_TIMEOUT)

#ifdef CONFIG_ARCH_IMX28
# define HW_SSP_DLL_STS 0x110
#endif

#ifdef CONFIG_ARCH_IMX23
# define HW_SSP_DEBUG 0x100
# define HW_SSP_VERSION 0x110
#endif

#ifdef CONFIG_ARCH_IMX28
# define HW_SSP_DEBUG 0x120
# define HW_SSP_VERSION 0x130
#endif

struct mxs_mci_host {
	unsigned	clock;	/* current clock speed in Hz ("0" if disabled) */
	unsigned	index;
#ifdef CONFIG_MCI_INFO
	unsigned	f_min;
	unsigned	f_max;
#endif
	int		bus_width:2; /* 0 = 1 bit, 1 = 4 bit, 2 = 8 bit */
};

/**
 * Get the SSP clock rate
 * @param hw_dev Host interface device instance
 * @return Unit's clock in [Hz]
 */
static unsigned mxs_mci_get_unit_clock(struct device_d *hw_dev)
{
	struct mxs_mci_host *host_data = GET_HOST_DATA(hw_dev);

	return imx_get_sspclk(host_data->index);
}

/**
 * Get MCI cards response if defined for the type of command
 * @param hw_dev Host interface device instance
 * @param cmd Command description
 * @return Response bytes count, -EINVAL for unsupported response types
 */
static int mxs_mci_get_cards_response(struct device_d *hw_dev, struct mci_cmd *cmd)
{
	switch (cmd->resp_type) {
	case MMC_RSP_NONE:
		return 0;

	case MMC_RSP_R1:
	case MMC_RSP_R1b:
	case MMC_RSP_R3:
		cmd->response[0] = readl(hw_dev->map_base + HW_SSP_SDRESP0);
		return 1;

	case MMC_RSP_R2:
		cmd->response[3] = readl(hw_dev->map_base + HW_SSP_SDRESP0);
		cmd->response[2] = readl(hw_dev->map_base + HW_SSP_SDRESP1);
		cmd->response[1] = readl(hw_dev->map_base + HW_SSP_SDRESP2);
		cmd->response[0] = readl(hw_dev->map_base + HW_SSP_SDRESP3);
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
static void mxs_mci_finish_request(struct device_d *hw_dev)
{
	/* stop the engines (normaly already done) */
	writel(SSP_CTRL0_RUN, hw_dev->map_base + HW_SSP_CTRL0 + 8);
}

/**
 * Check if the last command failed and if, why it failed
 * @param status HW_SSP_STATUS's content
 * @return 0 if no error, negative values else
 */
static int mxs_mci_get_cmd_error(unsigned status)
{
	if (status & SSP_STATUS_ERROR)
		pr_debug("Status Reg reports %08X\n", status);

	if (status & SSP_STATUS_TIMEOUT) {
		pr_debug("CMD timeout\n");
		return -ETIMEDOUT;
	} else if (status & SSP_STATUS_RESP_TIMEOUT) {
		pr_debug("RESP timeout\n");
		return -ETIMEDOUT;
	} else if (status & SSP_STATUS_RESP_CRC_ERR) {
		pr_debug("CMD crc error\n");
		return -EILSEQ;
	} else if (status & SSP_STATUS_RESP_ERR) {
		pr_debug("RESP error\n");
		return -EIO;
	}

	return 0;
}

/**
 * Define the timout for the next command
 * @param hw_dev Host interface device instance
 * @param to Timeout value in MCI card's bus clocks
 */
static void mxs_mci_setup_timeout(struct device_d *hw_dev, unsigned to)
{
	uint32_t reg;

	reg = readl(hw_dev->map_base + HW_SSP_TIMING) & ~SSP_TIMING_TIMEOUT_MASK;
	reg |= SSP_TIMING_TIMEOUT(to);
	writel(reg, hw_dev->map_base + HW_SSP_TIMING);
}

/**
 * Read data from the MCI card
 * @param hw_dev Host interface device instance
 * @param buffer To write data into
 * @param length Count of bytes to read (must be multiples of 4)
 * @return 0 on success, negative values else
 *
 * @note This routine uses PIO to read in the data bytes from the FIFO. This
 * may fail whith high clock speeds. If you receive -EIO errors you can try
 * again with reduced clock speeds.
 */
static int mxs_mci_read_data(struct device_d *hw_dev, void *buffer, unsigned length)
{
	uint32_t *p = buffer;

	if (length & 0x3) {
		pr_debug("Cannot read data sizes not multiple of 4 (request for %u detected)\n",
				length);
		return -EINVAL;
	}

	while ((length != 0) &&
		((readl(hw_dev->map_base + HW_SSP_STATUS) & SSP_STATUS_ERROR) == 0)) {
		/* TODO sort out FIFO overflows and emit -EOI for this case */
		if ((readl(hw_dev->map_base + HW_SSP_STATUS) & SSP_STATUS_FIFO_EMPTY) == 0) {
			*p = readl(hw_dev->map_base + HW_SSP_DATA);
			p++;
			length -= 4;
		}
	}

	if (length == 0)
		return 0;

	return -EINVAL;
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
static int mxs_mci_write_data(struct device_d *hw_dev, const void *buffer, unsigned length)
{
	const uint32_t *p = buffer;

	if (length & 0x3) {
		pr_debug("Cannot write data sizes not multiple of 4 (request for %u detected)\n",
				length);
		return -EINVAL;
	}

	while ((length != 0) &&
		((readl(hw_dev->map_base + HW_SSP_STATUS) & SSP_STATUS_ERROR) == 0)) {
		/* TODO sort out FIFO overflows and emit -EOI for this case */
		if ((readl(hw_dev->map_base + HW_SSP_STATUS) & SSP_STATUS_FIFO_FULL) == 0) {
			writel(*p, hw_dev->map_base + HW_SSP_DATA);
			p++;
			length -= 4;
		}
	}
	if (length == 0)
		return 0;

	return -EINVAL;
}

/**
 * Start the transaction with or without data
 * @param hw_dev Host interface device instance
 * @param data Data transfer description (might be NULL)
 * @return 0 on success
 */
static int mxs_mci_transfer_data(struct device_d *hw_dev, struct mci_data *data)
{
	/*
	 * Everything is ready for the transaction now:
	 * - transfer configuration
	 * - command and its parameters
	 *
	 * Start the transaction right now
	 */
	writel(SSP_CTRL0_RUN, hw_dev->map_base + HW_SSP_CTRL0 + 4);

	if (data != NULL) {
		unsigned length = data->blocks * data->blocksize;

		if (data->flags & MMC_DATA_READ)
			return mxs_mci_read_data(hw_dev, data->dest, length);
		else
			return mxs_mci_write_data(hw_dev, data->src, length);
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
static int mxs_mci_std_cmds(struct device_d *hw_dev, struct mci_cmd *cmd)
{
	/* setup command and transfer parameters */
	writel(mxs_mci_prepare_transfer_setup(cmd->resp_type, 0) |
		SSP_CTRL0_ENABLE, hw_dev->map_base + HW_SSP_CTRL0);

	/* prepare the command, when no response is expected add a few trailing clocks */
	writel(SSP_CMD0_CMD(cmd->cmdidx) |
		(cmd->resp_type & MMC_RSP_PRESENT ? 0 : SSP_CMD0_APPEND_8CYC),
		hw_dev->map_base + HW_SSP_CMD0);

	/* prepare command's arguments */
	writel(cmd->cmdarg, hw_dev->map_base + HW_SSP_CMD1);

	mxs_mci_setup_timeout(hw_dev, 0xffff);

	/* start the transfer */
	writel(SSP_CTRL0_RUN, hw_dev->map_base + HW_SSP_CTRL0 + 4);

	/* wait until finished */
	while (readl(hw_dev->map_base + HW_SSP_CTRL0) & SSP_CTRL0_RUN)
		;

	if (cmd->resp_type & MMC_RSP_PRESENT)
		mxs_mci_get_cards_response(hw_dev, cmd);

	return mxs_mci_get_cmd_error(readl(hw_dev->map_base + HW_SSP_STATUS));
}

/**
 * Handle an "addressed data transfer command " with or without data
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.) May be NULL
 * @return 0 on success
 */
static int mxs_mci_adtc(struct device_d *hw_dev, struct mci_cmd *cmd,
			struct mci_data *data)
{
	struct mxs_mci_host *host_data = (struct mxs_mci_host*)GET_HOST_DATA(hw_dev);
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
		SSP_CTRL0_BUS_WIDTH(host_data->bus_width) |
		(xfer_cnt != 0 ? SSP_CTRL0_DATA_XFER : 0) | /* command plus data */
		SSP_CTRL0_ENABLE |
		SSP_CTRL0_XFER_COUNT(xfer_cnt), /* byte count to be transfered */
		hw_dev->map_base + HW_SSP_CTRL0);

	/* prepare the command and the transfered data count */
	writel(SSP_CMD0_CMD(cmd->cmdidx) |
		SSP_CMD0_BLOCK_SIZE(log2blocksize) |
		SSP_CMD0_BLOCK_COUNT(block_cnt) |
		(cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION ? SSP_CMD0_APPEND_8CYC : 0),
		hw_dev->map_base + HW_SSP_CMD0);
#endif
#ifdef CONFIG_ARCH_IMX28
	writel(mxs_mci_prepare_transfer_setup(cmd->resp_type, data != NULL ? data->flags : 0) |
		SSP_CTRL0_BUS_WIDTH(host_data->bus_width) |
		(xfer_cnt != 0 ? SSP_CTRL0_DATA_XFER : 0) | /* command plus data */
		SSP_CTRL0_ENABLE,
		hw_dev->map_base + HW_SSP_CTRL0);
	writel(xfer_cnt, hw_dev->map_base + HW_SSP_XFER_COUNT);

	/* prepare the command and the transfered data count */
	writel(SSP_CMD0_CMD(cmd->cmdidx) |
		(cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION ? SSP_CMD0_APPEND_8CYC : 0),
		hw_dev->map_base + HW_SSP_CMD0);
	writel(SSP_BLOCK_SIZE(log2blocksize) |
		SSP_BLOCK_COUNT(block_cnt),
		hw_dev->map_base + HW_SSP_BLOCK_SIZE);
#endif

	/* prepare command's arguments */
	writel(cmd->cmdarg, hw_dev->map_base + HW_SSP_CMD1);

	mxs_mci_setup_timeout(hw_dev, 0xffff);

	err = mxs_mci_transfer_data(hw_dev, data);
	if (err != 0) {
		pr_debug(" Transfering data failed\n");
		return err;
	}

	/* wait until finished */
	while (readl(hw_dev->map_base + HW_SSP_CTRL0) & SSP_CTRL0_RUN)
		;

	mxs_mci_get_cards_response(hw_dev, cmd);

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
static unsigned mxs_mci_setup_clock_speed(struct device_d *hw_dev, unsigned nc)
{
	unsigned ssp, div, rate, reg;

	if (nc == 0U) {
		/* TODO stop the clock */
		return 0;
	}

	ssp = mxs_mci_get_unit_clock(hw_dev);

	for (div = 2; div < 255; div += 2) {
		rate = DIV_ROUND_CLOSEST(DIV_ROUND_CLOSEST(ssp, nc), div);
		if (rate <= 0x100)
			break;
	}
	if (div >= 255) {
		pr_warning("Cannot set clock to %d Hz\n", nc);
		return 0;
	}

	reg = readl(hw_dev->map_base + HW_SSP_TIMING) & SSP_TIMING_TIMEOUT_MASK;
	reg |= SSP_TIMING_CLOCK_DIVIDE(div) | SSP_TIMING_CLOCK_RATE(rate - 1);
	writel(reg, hw_dev->map_base + HW_SSP_TIMING);

	return ssp / div / rate;
}

/**
 * Reset the MCI engine (the hard way)
 * @param hw_dev Host interface instance
 *
 * This will reset everything in all registers of this unit! (FIXME)
 */
static void mxs_mci_reset(struct device_d *hw_dev)
{
	writel(SSP_CTRL0_SFTRST, hw_dev->map_base + HW_SSP_CTRL0 + 8);
	while (readl(hw_dev->map_base + HW_SSP_CTRL0) & SSP_CTRL0_SFTRST)
		;
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
	struct device_d *hw_dev = host->hw_dev;
	struct mxs_mci_host *host_data = (struct mxs_mci_host*)GET_HOST_DATA(hw_dev);

	/* enable the clock to this unit to be able to reset it */
	writel(SSP_CTRL0_CLKGATE, hw_dev->map_base + HW_SSP_CTRL0 + 8);

	/* reset the unit */
	mxs_mci_reset(hw_dev);

	/* restore the last settings */
	host->clock = host_data->clock = mxs_mci_setup_clock_speed(hw_dev, host->clock);
	mxs_mci_setup_timeout(hw_dev, 0xffff);
	writel(SSP_CTRL0_IGNORE_CRC |
		SSP_CTRL0_BUS_WIDTH(host_data->bus_width),
		hw_dev->map_base + HW_SSP_CTRL0);
	writel(SSP_CTRL1_POLARITY |
		SSP_CTRL1_SSP_MODE(3) |
		SSP_CTRL1_WORD_LENGTH(7), hw_dev->map_base + HW_SSP_CTRL1);

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
	struct device_d *hw_dev = host->hw_dev;
	int rc;

	if ((cmd->resp_type == 0) || (data == NULL))
		rc = mxs_mci_std_cmds(hw_dev, cmd);
	else
		rc = mxs_mci_adtc(hw_dev, cmd, data);	/* with response and data */

	mxs_mci_finish_request(hw_dev);	/* TODO */
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
static void mxs_mci_set_ios(struct mci_host *host, struct device_d *mci_dev,
			unsigned bus_width, unsigned clock)
{
	struct device_d *hw_dev = host->hw_dev;
	struct mxs_mci_host *host_data = (struct mxs_mci_host*)GET_HOST_DATA(hw_dev);

	switch (bus_width) {
	case 8:
		host_data->bus_width = 2;
		host->bus_width = 8;	/* 8 bit is possible */
		break;
	case 4:
		host_data->bus_width = 1;
		host->bus_width = 4;	/* 4 bit is possible */
		break;
	default:
		host_data->bus_width = 0;
		host->bus_width = 1;	/* 1 bit is possible */
		break;
	}

	host->clock = host_data->clock = mxs_mci_setup_clock_speed(hw_dev, clock);
	pr_debug("IO settings: bus width=%d, frequency=%u Hz\n", host->bus_width,
			host->clock);
}

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_MCI_INFO
const unsigned char bus_width[3] = { 1, 4, 8 };

static void mxs_mci_info(struct device_d *hw_dev)
{
	struct mxs_mci_host *host_data = GET_HOST_DATA(hw_dev);

	printf(" Interface\n");
	printf("  Min. bus clock: %u Hz\n", host_data->f_min);
	printf("  Max. bus clock: %u Hz\n", host_data->f_max);
	printf("  Current bus clock: %u Hz\n", host_data->clock);
	printf("  Bus width: %u bit\n", bus_width[host_data->bus_width]);
	printf("\n");
}
#endif

static int mxs_mci_probe(struct device_d *hw_dev)
{
	struct mxs_mci_platform_data *pd = hw_dev->platform_data;
	struct mxs_mci_host *host_data;
	struct mci_host *host;

	if (hw_dev->platform_data == NULL) {
		pr_err("Missing platform data\n");
		return -EINVAL;
	}

	host = xzalloc(sizeof(struct mxs_mci_host) + sizeof(struct mci_host));
	host_data = (struct mxs_mci_host*)&host[1];

	hw_dev->priv = host_data;
	host->hw_dev = hw_dev;
	host->send_cmd = mxs_mci_request,
	host->set_ios = mxs_mci_set_ios,
	host->init = mxs_mci_initialize,

	/* feed forward the platform specific values */
	host->voltages = pd->voltages;
	host->host_caps = pd->caps;

#ifdef CONFIG_ARCH_IMX23
	host_data->index = 0;	/* there is only one clock for all */
#endif
#ifdef CONFIG_ARCH_IMX28
	/* one dedicated clock per unit */
	switch (hw_dev->map_base) {
	case IMX_SSP0_BASE:
		host_data->index = 0;
		break;
	case IMX_SSP1_BASE:
		host_data->index = 1;
		break;
	case IMX_SSP2_BASE:
		host_data->index = 2;
		break;
	case IMX_SSP3_BASE:
		host_data->index = 3;
		break;
	default:
		pr_debug("Unknown SSP unit at address 0x%08x\n", hw_dev->map_base);
		return 0;
	}
#endif
	if (pd->f_min == 0) {
		host->f_min = mxs_mci_get_unit_clock(hw_dev) / 254 / 256;
		pr_debug("Min. frequency is %u Hz\n", host->f_min);
	} else {
		host->f_min = pd->f_min;
		pr_debug("Min. frequency is %u Hz, could be %u Hz\n",
			host->f_min, mxs_mci_get_unit_clock(hw_dev) / 254 / 256);
	}
	if (pd->f_max == 0) {
		host->f_max = mxs_mci_get_unit_clock(hw_dev) / 2 / 1;
		pr_debug("Max. frequency is %u Hz\n", host->f_max);
	} else {
		host->f_max =  pd->f_max;
		pr_debug("Max. frequency is %u Hz, could be %u Hz\n",
			host->f_max, mxs_mci_get_unit_clock(hw_dev) / 2 / 1);
	}

#ifdef CONFIG_MCI_INFO
	host_data->f_min = host->f_min;
	host_data->f_max = host->f_max;
#endif

	return mci_register(host);
}

static struct driver_d mxs_mci_driver = {
        .name  = "mxs_mci",
        .probe = mxs_mci_probe,
#ifdef CONFIG_MCI_INFO
	.info = mxs_mci_info,
#endif
};

static int mxs_mci_init_driver(void)
{
        register_driver(&mxs_mci_driver);
        return 0;
}

device_initcall(mxs_mci_init_driver);
