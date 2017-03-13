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
 * The Linux kernel code has the following (c) and license, which is hence
 * propagated to here:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	struct device_d *hw_dev;
	int bus_width;
	u32 clock;
	u32 max_clock;
	u32 version;
	uint64_t last_write;
};

void bcm2835_mci_write(struct bcm2835_mci_host *host, u32 reg, u32 val)
{
	/*
	 * The Arasan has a bugette whereby it may lose the content of
	 * successive writes to registers that are within two SD-card clock
	 * cycles of each other (a clock domain crossing problem).
	 * It seems, however, that the data register does not have this problem.
	 * (Which is just as well - otherwise we'd have to nobble the DMA engine
	 * too)
	 */

	if (host->last_write != 0)
		while ((get_time_ns() - host->last_write) < twoticks_delay)
			;
	host->last_write = get_time_ns();
	writel(val, host->regs + reg);
}

u32 bcm2835_mci_read(struct bcm2835_mci_host *host, u32 reg)
{
	return readl(host->regs + reg);
}

/* Create special write data function since the data
 * register is not affected by the twoticks_delay bug
 * and we can thus get better speed here
 */
void bcm2835_mci_write_data(struct bcm2835_mci_host *host, u32 *p)
{
	writel(*p, host->regs + SDHCI_BUFFER);
}

/* Make a read data functions as well just to keep structure */
void bcm2835_mci_read_data(struct bcm2835_mci_host *host, u32 *p)
{
	*p = readl(host->regs + SDHCI_BUFFER);
}

static int bcm2835_mci_transfer_data(struct bcm2835_mci_host *host,
		struct mci_cmd *cmd, struct mci_data *data) {
	u32 *p;
	u32 data_size, status, intr_status = 0;
	u32 data_ready_intr_mask;
	u32 data_ready_status_mask;
	int i = 0;
	void (*read_write_func)(struct bcm2835_mci_host*, u32*);

	data_size = data->blocksize * data->blocks;

	if (data->flags & MMC_DATA_READ) {
		p = (u32 *) data->dest;
		data_ready_intr_mask = IRQSTAT_BRR;
		data_ready_status_mask = PRSSTAT_BREN;
		read_write_func = &bcm2835_mci_read_data;
	} else {
		p = (u32 *) data->src;
		data_ready_intr_mask = IRQSTAT_BWR;
		data_ready_status_mask = PRSSTAT_BWEN;
		read_write_func = &bcm2835_mci_write_data;
	}
	do {
		intr_status = bcm2835_mci_read(host, SDHCI_INT_STATUS);
		if (intr_status & IRQSTAT_CIE) {
			dev_err(host->hw_dev,
					"Error occured while transferring data: 0x%X\n",
					intr_status);
			return -EPERM;
		}
		if (intr_status & data_ready_intr_mask) {
			status = bcm2835_mci_read(host, SDHCI_PRESENT_STATE);
			if ((status & data_ready_status_mask) == 0)
				continue;
			/*Clear latest int and transfer one block size of data*/
			bcm2835_mci_write(host, SDHCI_INT_STATUS,
					data_ready_intr_mask);
			for (i = 0; i < data->blocksize; i += 4) {
				read_write_func(host, p);
				p++;
				data_size -= 4;
			}
		}
	} while ((intr_status & IRQSTAT_TC) == 0);

	if (data_size != 0) {
		if (data->flags & MMC_DATA_READ)
			dev_err(host->hw_dev, "Error while reading:\n");
		else
			dev_err(host->hw_dev, "Error while writing:\n");

		dev_err(host->hw_dev, "Transferred %d bytes of data, wanted %d\n",
				(data->blocksize * data->blocks) - data_size,
				data->blocksize * data->blocks);

		dev_err(host->hw_dev, "Status: 0x%X, Interrupt: 0x%X\n",
				bcm2835_mci_read(host, SDHCI_PRESENT_STATE),
				bcm2835_mci_read(host, SDHCI_INT_STATUS));

		return -EPERM;
	}
	return 0;
}

static u32 bcm2835_mci_wait_command_done(struct bcm2835_mci_host *host)
{
	u32 interrupt = 0;

	while (true) {
		interrupt = bcm2835_mci_read(
				host, SDHCI_INT_STATUS);
		if (interrupt & IRQSTAT_CIE)
			return -EPERM;
		if (interrupt & IRQSTAT_CC)
			break;
	}
	return 0;
}

static void bcm2835_mci_reset_emmc(struct bcm2835_mci_host *host, u32 reset,
		u32 wait_for)
{
	u32 ret;
	u32 current = bcm2835_mci_read(host,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);

	bcm2835_mci_write(host,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			current | reset);

	while (true) {
		ret = bcm2835_mci_read(host,
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
	u32 command, block_data = 0, ret = 0;
	u32 wait_inhibit_mask = PRSSTAT_CICHB | PRSSTAT_CIDHB;
	struct bcm2835_mci_host *host = to_bcm2835_host(mci);

	command = COMMAND_CMD(cmd->cmdidx);

	if (cmd->resp_type != MMC_RSP_NONE) {
		if (cmd->resp_type & MMC_RSP_136)
			command |= COMMAND_RSPTYP_136;
		else if (cmd->resp_type & MMC_RSP_BUSY)
			command |= COMMAND_RSPTYP_48_BUSY;
		else
			command |= COMMAND_RSPTYP_48;
		if (cmd->resp_type & MMC_RSP_CRC)
			command |= COMMAND_CCCEN;
	}
	if (data != NULL) {
		command |= COMMAND_DPSEL | TRANSFER_MODE_BCEN;

		if (data->blocks > 1)
			command |= TRANSFER_MODE_MSBSEL;

		if (data->flags & MMC_DATA_READ)
			command |= TRANSFER_MODE_DTDSEL;

		block_data = (data->blocks << BLOCK_SHIFT);
		block_data |= data->blocksize;
	}

	/* We shouldn't wait for data inihibit for stop commands, even
	though they might use busy signaling */
	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		wait_inhibit_mask = PRSSTAT_CICHB;

	/*Wait for old command*/
	while (bcm2835_mci_read(host, SDHCI_PRESENT_STATE)
			& wait_inhibit_mask)
		;

	bcm2835_mci_write(host, SDHCI_INT_ENABLE, 0xFFFFFFFF);
	bcm2835_mci_write(host, SDHCI_INT_STATUS, 0xFFFFFFFF);

	bcm2835_mci_write(host, SDHCI_BLOCK_SIZE__BLOCK_COUNT, block_data);
	bcm2835_mci_write(host, SDHCI_ARGUMENT, cmd->cmdarg);
	bcm2835_mci_write(host, SDHCI_TRANSFER_MODE__COMMAND, command);

	ret = bcm2835_mci_wait_command_done(host);
	if (ret) {
		dev_err(host->hw_dev, "Error while executing command %d\n",
				cmd->cmdidx);
		dev_err(host->hw_dev, "Status: 0x%X, Interrupt: 0x%X\n",
				bcm2835_mci_read(host, SDHCI_PRESENT_STATE),
				bcm2835_mci_read(host, SDHCI_INT_STATUS));
	}
	if (cmd->resp_type != 0 && ret != -1) {
		int i = 0;

		/* CRC is stripped so we need to do some shifting. */
		if (cmd->resp_type & MMC_RSP_136) {
			for (i = 0; i < 4; i++) {
				cmd->response[i] = bcm2835_mci_read(
					host,
					SDHCI_RESPONSE_0 + (3 - i) * 4) << 8;
				if (i != 3)
					cmd->response[i] |=
						readb((u32) (host->regs) +
						SDHCI_RESPONSE_0 +
						(3 - i) * 4 - 1);
			}
		} else {
			cmd->response[0] = bcm2835_mci_read(
					host, SDHCI_RESPONSE_0);
		}
		bcm2835_mci_write(host, SDHCI_INT_STATUS,
				IRQSTAT_CC);
	}

	if (!ret && data)
		ret = bcm2835_mci_transfer_data(host, cmd, data);

	bcm2835_mci_write(host, SDHCI_INT_STATUS, 0xFFFFFFFF);
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

	current_val = bcm2835_mci_read(host,
			SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		bcm2835_mci_write(host,
				SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
				current_val | CONTROL0_4DATA);
		host->bus_width = 1;
		dev_dbg(host->hw_dev, "Changing bus width to 4\n");
		break;
	case MMC_BUS_WIDTH_1:
		bcm2835_mci_write(host,
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
		bcm2835_mci_write(host,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
				0x00);

		if (ios->clock > 26000000) {
			enable = bcm2835_mci_read(host,
					SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL);
			enable |= CONTROL0_HISPEED;
			bcm2835_mci_write(host,
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

		bcm2835_mci_write(host,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
				enable);
		while (true) {
			u32 ret = bcm2835_mci_read(host,
					SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
			if (ret & CONTROL1_CLK_STABLE)
				break;
		}

		enable |= CONTROL1_CLKENA;
		bcm2835_mci_write(host,
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

int bcm2835_mci_reset(struct mci_host *mci, struct device_d *mci_dev)
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

	bcm2835_mci_write(host,
			SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
			0x00);
	bcm2835_mci_write(host, SDHCI_ACMD12_ERR__HOST_CONTROL2,
			0x00);
	bcm2835_mci_write(host,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			enable);
	while (true) {
		ret = bcm2835_mci_read(host,
				SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
		if (ret & CONTROL1_CLK_STABLE)
			break;
	}

	enable |= CONTROL1_CLKENA;
	bcm2835_mci_write(host,
			SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			enable);

	/*Delay atelast 74 clk cycles for card init*/
	mdelay(100);

	bcm2835_mci_write(host, SDHCI_INT_ENABLE,
			0xFFFFFFFF);
	bcm2835_mci_write(host, SDHCI_INT_STATUS,
			0xFFFFFFFF);

	/*Now write command 0 and see if we get response*/
	bcm2835_mci_write(host, SDHCI_ARGUMENT, 0x0);
	bcm2835_mci_write(host, SDHCI_TRANSFER_MODE__COMMAND, 0x0);
	return bcm2835_mci_wait_command_done(host);
}

static int bcm2835_mci_detect(struct device_d *dev)
{
	struct bcm2835_mci_host *host = dev->priv;

	return mci_detect_card(&host->mci);
}

static int bcm2835_mci_probe(struct device_d *hw_dev)
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

	hw_dev->priv = host;
	hw_dev->detect = bcm2835_mci_detect,

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

	host->version = bcm2835_mci_read(
			host, BCM2835_MCI_SLOTISR_VER);
	host->version = (host->version >> 16) & 0xFF;
	return mci_register(&host->mci);
}

static __maybe_unused struct of_device_id bcm2835_mci_compatible[] = {
	{
		.compatible = "brcm,bcm2835-sdhci",
	}, {
		/* sentinel */
	}
};

static struct driver_d bcm2835_mci_driver = {
	.name = "bcm2835_mci",
	.probe = bcm2835_mci_probe,
	.of_compatible = DRV_OF_COMPAT(bcm2835_mci_compatible),
};

static int bcm2835_mci_add(void)
{
	return platform_driver_register(&bcm2835_mci_driver);
}
coredevice_initcall(bcm2835_mci_add);
