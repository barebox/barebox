// SPDX-License-Identifier: GPL-2.0-only
/*
 * Raspberry PI MCI driver
 *
 * Support for SDHCI device on bcm2835
 * Based on sdhci-bcm2708.c (c) 2010 Broadcom
 * Inspired by bcm2835_sdhci.c from git://github.com/gonzoua/u-boot-pi.git
 *
 * Portions (e.g. read/write macros, concepts for back-to-back register write
 * timing workarounds) obviously extracted from the Linux kernel at:
 * https://github.com/raspberrypi/linux.git rpi-3.6.y
 *
 * Author: Wilhelm Lundgren <wilhelm.lundgren@cybercom.com>
 */

#include <common.h>
#include <init.h>
#include <mci.h>
#include <io.h>
#include <malloc.h>
#include <clock.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "mci-bcm2835.h"
#include "sdhci.h"

#define to_bcm2835_host(h)	container_of(h, struct bcm2835_mci_host, mci)
static int twoticks_delay;
struct bcm2835_mci_host {
	struct mci_host mci;
	void __iomem *regs;
	struct device *hw_dev;
	int bus_width;
	u32 clock;
	u32 max_clock;
	u32 version;
	uint64_t last_write;
	struct sdhci sdhci;
};

static void bcm2835_sdhci_write32(struct sdhci *sdhci, int reg, u32 val)
{
	struct bcm2835_mci_host *host = container_of(sdhci, struct bcm2835_mci_host, sdhci);

	/*
	 * The Arasan has a bugette whereby it may lose the content of
	 * successive writes to registers that are within two SD-card clock
	 * cycles of each other (a clock domain crossing problem).
	 * It seems, however, that the data register does not have this problem.
	 * (Which is just as well - otherwise we'd have to nobble the DMA engine
	 * too)
	 */

	if (reg != SDHCI_BUFFER) {
		if (host->last_write != 0)
			while ((get_time_ns() - host->last_write) < twoticks_delay)
				;
		host->last_write = get_time_ns();
	}

	writel(val, host->regs + reg);
}

static u32 bcm2835_sdhci_read32(struct sdhci *sdhci, int reg)
{
	struct bcm2835_mci_host *host = container_of(sdhci, struct bcm2835_mci_host, sdhci);

	return readl(host->regs + reg);
}

static int bcm2835_mci_wait_command_done(struct bcm2835_mci_host *host)
{
	u32 interrupt = 0;
	uint64_t start;

	start = get_time_ns();
	while (true) {
		interrupt = sdhci_read32(&host->sdhci, SDHCI_INT_STATUS);
		if (interrupt & SDHCI_INT_INDEX)
			return -EPERM;
		if (interrupt & SDHCI_INT_CMD_COMPLETE)
			break;
		if (is_timeout(start, SECOND))
			return -ETIMEDOUT;
	}
	return 0;
}

static void bcm2835_mci_reset_emmc(struct bcm2835_mci_host *host, u32 reset,
		u32 wait_for)
{
	u32 ret;
	u32 current = sdhci_read32(&host->sdhci,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);

	sdhci_write32(&host->sdhci,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			current | reset);

	while (true) {
		ret = sdhci_read32(&host->sdhci,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
		if (ret & wait_for)
			continue;
		break;
	}
}

/**
 * Process one command to the MCI card
 * @param host MCI host
 * @param cmd The command to process
 * @param data The data to handle in the command (can be NULL)
 * @return 0 on success, negative value else
 */
static int bcm2835_mci_request(struct mci_host *mci, struct mci_cmd *cmd,
		struct mci_data *data) {
	u32 command, block_data = 0, transfer_mode = 0;
	int ret;
	struct bcm2835_mci_host *host = to_bcm2835_host(mci);

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data, false,
				&command, &transfer_mode);

	if (data != NULL) {
		block_data = (data->blocks << BLOCK_SHIFT);
		block_data |= data->blocksize;
	}

	ret = sdhci_wait_idle(&host->sdhci, cmd);
	if (ret)
		return ret;

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE, 0xFFFFFFFF);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, 0xFFFFFFFF);

	sdhci_write32(&host->sdhci, SDHCI_BLOCK_SIZE__BLOCK_COUNT, block_data);
	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write32(&host->sdhci, SDHCI_TRANSFER_MODE__COMMAND,
		      command << 16 | transfer_mode);

	ret = bcm2835_mci_wait_command_done(host);
	if (ret && ret != -ETIMEDOUT) {
		dev_err(host->hw_dev, "Error while executing command %d\n",
				cmd->cmdidx);
		dev_err(host->hw_dev, "Status: 0x%X, Interrupt: 0x%X\n",
				sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE),
				sdhci_read32(&host->sdhci, SDHCI_INT_STATUS));
	}
	if (cmd->resp_type != 0 && ret != -1) {
		sdhci_read_response(&host->sdhci, cmd);

		sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, SDHCI_INT_CMD_COMPLETE);
	}

	if (!ret && data)
		ret = sdhci_transfer_data_pio(&host->sdhci, data);

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, 0xFFFFFFFF);
	if (ret) {
		bcm2835_mci_reset_emmc(host, CONTROL1_CMDRST,
				CONTROL1_CMDRST);
		bcm2835_mci_reset_emmc(host, CONTROL1_DATARST,
				CONTROL1_DATARST);
	}

	return ret;
}

static u32 bcm2835_mci_get_clock_divider(struct bcm2835_mci_host *host,
		u32 desired_hz)
{
	u32 div;
	u32 clk_hz;

	if (host->version >= SDHCI_SPEC_300) {
		/* Version 3.00 divisors must be a multiple of 2. */
		if (host->max_clock <= desired_hz)
			div = 1;
		else {
			for (div = 2; div < MAX_CLK_DIVIDER_V3; div += 2) {
				clk_hz = host->max_clock / div;
				if (clk_hz <= desired_hz)
					break;
			}
		}
	} else {
		/* Version 2.00 divisors must be a power of 2. */
		for (div = 1; div < MAX_CLK_DIVIDER_V2; div *= 2) {
			clk_hz = host->max_clock / div;
			if (clk_hz <= desired_hz)
				break;
		}

	}

	/*Since setting lowest bit means divide by two, shift down*/
	dev_dbg(host->hw_dev,
			"Wanted %d hz, returning divider %d (%d) which yields %d hz\n",
			desired_hz, div >> 1, div, host->max_clock / div);
	twoticks_delay = ((2 * 1000000000) / (host->max_clock / div)) + 1;

	div = div >> 1;
	host->clock = desired_hz;
	return div;
}

/**
 * Setup the bus width and IO speed
 * @param host MCI host
 * @param bus_width New bus width value (1, 4 or 8)
 * @param clock New clock in Hz (can be '0' to disable the clock)
 */
static void bcm2835_mci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	u32 divider;
	u32 divider_msb, divider_lsb;
	u32 enable;
	u32 current_val;

	struct bcm2835_mci_host *host = to_bcm2835_host(mci);

	current_val = sdhci_read32(&host->sdhci,
			SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		sdhci_write32(&host->sdhci,
				SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
				current_val | CONTROL0_4DATA);
		host->bus_width = 1;
		dev_dbg(host->hw_dev, "Changing bus width to 4\n");
		break;
	case MMC_BUS_WIDTH_1:
		sdhci_write32(&host->sdhci,
				SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
				current_val & ~CONTROL0_4DATA);
		host->bus_width = 0;
		dev_dbg(host->hw_dev, "Changing bus width to 1\n");
		break;
	default:
		dev_warn(host->hw_dev, "Unsupported width received: %d\n",
				ios->bus_width);
		return;
	}
	if (ios->clock != host->clock && ios->clock != 0) {
		sdhci_write32(&host->sdhci,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
				0x00);

		if (ios->clock > 26000000) {
			enable = sdhci_read32(&host->sdhci,
					SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL);
			enable |= CONTROL0_HISPEED;
			sdhci_write32(&host->sdhci,
					SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
					enable);
		}

		divider = bcm2835_mci_get_clock_divider(host, ios->clock);
		divider_msb = divider & 0x300;
		divider_msb >>= CONTROL1_CLKLSB;
		divider_lsb = divider & 0xFF;
		enable = (divider_lsb << CONTROL1_CLKLSB);
		enable |= (divider_msb << CONTROL1_CLKMSB);
		enable |= CONTROL1_INTCLKENA | CONTROL1_TIMEOUT;

		sdhci_write32(&host->sdhci,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
				enable);
		while (true) {
			u32 ret = sdhci_read32(&host->sdhci,
					SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
			if (ret & CONTROL1_CLK_STABLE)
				break;
		}

		enable |= CONTROL1_CLKENA;
		sdhci_write32(&host->sdhci,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
				enable);

		mdelay(100);

		bcm2835_mci_reset_emmc(host, CONTROL1_CMDRST,
				CONTROL1_CMDRST);
		bcm2835_mci_reset_emmc(host, CONTROL1_DATARST,
				CONTROL1_DATARST);

		host->clock = ios->clock;
	}
	dev_dbg(host->hw_dev, "IO settings: bus width=%d, frequency=%u Hz\n",
			host->bus_width, host->clock);
}

static int bcm2835_mci_reset(struct mci_host *mci, struct device *mci_dev)
{
	struct bcm2835_mci_host *host;
	u32 ret = 0;
	u32 reset = CONTROL1_HOSTRST | CONTROL1_CMDRST | CONTROL1_DATARST;
	u32 enable = 0;
	u32 divider;
	u32 divider_msb, divider_lsb;

	host = to_bcm2835_host(mci);
	divider = bcm2835_mci_get_clock_divider(host, MIN_FREQ);
	divider_msb = divider & 0x300;
	divider_msb = divider_msb >> CONTROL1_CLKLSB;
	divider_lsb = divider & 0xFF;

	enable = (divider_lsb << CONTROL1_CLKLSB);
	enable |= (divider_msb << CONTROL1_CLKMSB);
	enable |= CONTROL1_INTCLKENA | CONTROL1_TIMEOUT;

	bcm2835_mci_reset_emmc(host, enable | reset, CONTROL1_HOSTRST);

	sdhci_write32(&host->sdhci,
			SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
			(SDHCI_BUS_VOLTAGE_330 | SDHCI_BUS_POWER_EN) << 8);
	sdhci_write32(&host->sdhci, SDHCI_ACMD12_ERR__HOST_CONTROL2,
			0x00);
	sdhci_write32(&host->sdhci,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			enable);
	while (true) {
		ret = sdhci_read32(&host->sdhci,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
		if (ret & CONTROL1_CLK_STABLE)
			break;
	}

	enable |= CONTROL1_CLKENA;
	sdhci_write32(&host->sdhci,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			enable);

	/*Delay atelast 74 clk cycles for card init*/
	mdelay(100);

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE,
			0xFFFFFFFF);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS,
			0xFFFFFFFF);

	return 0;
}

static int bcm2835_mci_probe(struct device *hw_dev)
{
	struct resource *iores;
	struct bcm2835_mci_host *host;
	static struct clk *clk;
	int ret;

	clk = clk_get(hw_dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		dev_err(hw_dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(clk);
	if (ret) {
		dev_err(hw_dev, "clock failed to enable: %d\n", ret);
		clk_put(clk);
		return ret;
	}

	host = xzalloc(sizeof(*host));
	host->mci.send_cmd = bcm2835_mci_request;
	host->mci.set_ios = bcm2835_mci_set_ios;
	host->mci.init = bcm2835_mci_reset;
	host->mci.hw_dev = hw_dev;
	host->hw_dev = hw_dev;
	host->max_clock = clk_get_rate(clk);

	host->sdhci.read32 = bcm2835_sdhci_read32;
	host->sdhci.write32 = bcm2835_sdhci_write32;

	mci_of_parse(&host->mci);

	iores = dev_request_mem_resource(hw_dev, 0);
	if (IS_ERR(iores)) {
		dev_err(host->hw_dev, "Failed request mem region, aborting...\n");
		return PTR_ERR(iores);
	}
	host->regs = IOMEM(iores->start);

	host->mci.host_caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED |
		MMC_CAP_MMC_HIGHSPEED;

	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	host->mci.f_min = MIN_FREQ;
	host->mci.f_max = host->max_clock;

	/*
	 * The Arasan has a bugette whereby it may lose the content of
	 * successive writes to registers that are within two SD-card clock
	 * cycles of each other (a clock domain crossing problem).
	 *
	 * 1/MIN_FREQ is (max) time per tick of eMMC clock.
	 * 2/MIN_FREQ is time for two ticks.
	 * Multiply by 1000000000 to get nS per two ticks.
	 * +1 for hack rounding.
	 */

	twoticks_delay = ((2 * 1000000000) / MIN_FREQ) + 1;

	host->version = sdhci_read32(&host->sdhci, BCM2835_MCI_SLOTISR_VER);
	host->version = (host->version >> 16) & 0xFF;
	return mci_register(&host->mci);
}

static __maybe_unused struct of_device_id bcm2835_mci_compatible[] = {
	{
		.compatible = "brcm,bcm2835-sdhci",
	}, {
		.compatible = "brcm,bcm2711-emmc2",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, bcm2835_mci_compatible);

static struct driver bcm2835_mci_driver = {
	.name = "bcm2835_mci",
	.probe = bcm2835_mci_probe,
	.of_compatible = DRV_OF_COMPAT(bcm2835_mci_compatible),
};

device_platform_driver(bcm2835_mci_driver);
