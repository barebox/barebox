// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Marvell Dove SDHCI MCI driver
 *
 * Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 */

#include <clock.h>
#include <common.h>
#include <init.h>
#include <io.h>
#include <dma.h>
#include <malloc.h>
#include <mci.h>
#include <linux/err.h>

#include "sdhci.h"

struct dove_sdhci {
	struct mci_host mci;
	struct sdhci sdhci;
};

#define priv_from_mci_host(h)	\
	container_of(h, struct dove_sdhci, mci);

static int dove_sdhci_wait_for_done(struct dove_sdhci *host, u16 mask)
{
	u16 status;
	u64 start;

	start = get_time_ns();
	while (1) {
		status = sdhci_read16(&host->sdhci, SDHCI_INT_NORMAL_STATUS);
		if (status & SDHCI_INT_ERROR)
			return -EPERM;
		/* this special quirk is necessary, as the dma
		 * engine stops on dma boundary and will only
		 * restart after acknowledging it this way.
		 */
		if (status & SDHCI_INT_DMA) {
			u32 addr = sdhci_read32(&host->sdhci, SDHCI_DMA_ADDRESS);
			sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS, addr);
		}
		if (status & mask)
			break;
		if (is_timeout(start, 1000 * MSECOND)) {
			dev_err(host->mci.hw_dev, "SDHCI timeout while waiting for done\n");
			return -ETIMEDOUT;
		}
	}
	return 0;
}

static int dove_sdhci_mci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				struct mci_data *data)
{
	u32 command, xfer;
	int ret;
	unsigned int num_bytes = 0;
	struct dove_sdhci *host = priv_from_mci_host(mci);

	ret = sdhci_wait_idle(&host->sdhci, cmd);
	if (ret)
		return ret;

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	/* setup transfer data */
	if (data) {
		num_bytes = data->blocks * data->blocksize;
		if (data->flags & MMC_DATA_READ)
			sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS,
				      lower_32_bits(virt_to_phys(data->dest)));
		else
			sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS,
				      lower_32_bits(virt_to_phys(data->src)));
		sdhci_write16(&host->sdhci, SDHCI_BLOCK_SIZE, SDHCI_DMA_BOUNDARY_512K |
				SDHCI_TRANSFER_BLOCK_SIZE(data->blocksize));
		sdhci_write16(&host->sdhci, SDHCI_BLOCK_COUNT, data->blocks);
		sdhci_write8(&host->sdhci, SDHCI_TIMEOUT_CONTROL, 0xe);


		if (data->flags & MMC_DATA_WRITE)
			dma_sync_single_for_device(host->mci.hw_dev,
						   lower_32_bits(virt_to_phys(data->src)),
						    num_bytes, DMA_TO_DEVICE);
		else
			dma_sync_single_for_device(host->mci.hw_dev,
						   lower_32_bits(virt_to_phys(data->dest)),
						    num_bytes, DMA_FROM_DEVICE);
	}

	/* setup transfer mode */
	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data, true, &command, &xfer);

	sdhci_write16(&host->sdhci, SDHCI_TRANSFER_MODE, xfer);
	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write16(&host->sdhci, SDHCI_COMMAND, command);

	ret = dove_sdhci_wait_for_done(host, SDHCI_INT_CMD_COMPLETE);
	if (ret) {
		dev_err(host->mci.hw_dev, "error on command %d\n", cmd->cmdidx);
		dev_err(host->mci.hw_dev, "state = %04x %04x, interrupt = %04x %04x\n",
			sdhci_read16(&host->sdhci, SDHCI_PRESENT_STATE),
			sdhci_read16(&host->sdhci, SDHCI_PRESENT_STATE1),
			sdhci_read16(&host->sdhci, SDHCI_INT_NORMAL_STATUS),
			sdhci_read16(&host->sdhci, SDHCI_INT_ERROR_STATUS));
		goto cmd_error;
	}

	sdhci_read_response(&host->sdhci, cmd);

	if (data) {
		if (data->flags & MMC_DATA_WRITE)
			dma_sync_single_for_cpu(host->mci.hw_dev,
						lower_32_bits(virt_to_phys(data->src)),
						num_bytes, DMA_TO_DEVICE);
		else
			dma_sync_single_for_cpu(host->mci.hw_dev,
						lower_32_bits(virt_to_phys(data->dest)),
						num_bytes, DMA_FROM_DEVICE);

		ret = dove_sdhci_wait_for_done(host, SDHCI_INT_XFER_COMPLETE);
		if (ret) {
			dev_err(host->mci.hw_dev, "error while transfering data for command %d\n",
				cmd->cmdidx);
			dev_err(host->mci.hw_dev, "state = %04x %04x, interrupt = %04x %04x\n",
				sdhci_read16(&host->sdhci, SDHCI_PRESENT_STATE),
				sdhci_read16(&host->sdhci, SDHCI_PRESENT_STATE1),
				sdhci_read16(&host->sdhci, SDHCI_INT_NORMAL_STATUS),
				sdhci_read16(&host->sdhci, SDHCI_INT_ERROR_STATUS));
			goto cmd_error;
		}
	}

cmd_error:
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);
	return ret;
}

static u16 dove_sdhci_get_clock_divider(struct dove_sdhci *host, u32 reqclk)
{
	u16 div;

	for (div = 1; div < SDHCI_MAX_DIV_SPEC_200; div *= 2)
		if ((host->mci.f_max / div) <= reqclk)
			break;
	div /= 2;

	return div;
}

static void dove_sdhci_mci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	u16 val;
	u64 start;
	struct dove_sdhci *host = priv_from_mci_host(mci);

	debug("%s: clock = %u, bus-width = %d, timing = %02x\n", __func__, ios->clock, ios->bus_width, ios->timing);

	/* disable on zero clock */
	if (!ios->clock)
		return;

	/* enable bus power */
	val = SDHCI_BUS_VOLTAGE_330;
	sdhci_write8(&host->sdhci, SDHCI_POWER_CONTROL, val | SDHCI_BUS_POWER_EN);
	udelay(400);

	/* set bus width */
	val = sdhci_read8(&host->sdhci, SDHCI_HOST_CONTROL) &
		~(SDHCI_CTRL_4BITBUS | SDHCI_CTRL_8BITBUS);
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		val |= SDHCI_CTRL_8BITBUS;
		break;
	case MMC_BUS_WIDTH_4:
		val |= SDHCI_CTRL_4BITBUS;
		break;
	case MMC_BUS_WIDTH_1:
		break;
	}

	if (ios->clock > 26000000)
		val |= SDHCI_CTRL_HISPD;
	else
		val &= ~SDHCI_CTRL_HISPD;

	sdhci_write8(&host->sdhci, SDHCI_HOST_CONTROL, val);

	/* set bus clock */
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, 0);
	val = dove_sdhci_get_clock_divider(host, ios->clock);
	val = SDHCI_CLOCK_INT_EN | SDHCI_FREQ_SEL(val);
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, val);

	/* wait for internal clock stable */
	start = get_time_ns();
	while (!(sdhci_read16(&host->sdhci, SDHCI_CLOCK_CONTROL) &
			SDHCI_CLOCK_INT_STABLE)) {
		if (is_timeout(start, 20 * MSECOND)) {
			dev_err(host->mci.hw_dev, "SDHCI clock stable timeout\n");
			return;
		}
	}

	/* enable bus clock */
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, val | SDHCI_CLOCK_CARD_EN);
}

static int dove_sdhci_mci_init(struct mci_host *mci, struct device *dev)
{
	u64 start;
	struct dove_sdhci *host = priv_from_mci_host(mci);

	/* reset sdhci controller */
	sdhci_write8(&host->sdhci, SDHCI_SOFTWARE_RESET, SDHCI_RESET_ALL);

	/* wait for reset completion */
	start = get_time_ns();
	while (1) {
		if ((sdhci_read8(&host->sdhci, SDHCI_SOFTWARE_RESET) &
				SDHCI_RESET_ALL) == 0)
			break;
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(dev, "SDHCI reset timeout\n");
			return -ETIMEDOUT;
		}
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);
	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE, ~0);
	sdhci_write32(&host->sdhci, SDHCI_SIGNAL_ENABLE, ~0);

	return 0;
}

static void dove_sdhci_set_mci_caps(struct dove_sdhci *host)
{
	u32 caps;

	caps = sdhci_read32(&host->sdhci, SDHCI_CAPABILITIES);

	if (caps & SDHCI_CAN_VDD_180)
		host->mci.voltages |= MMC_VDD_165_195;
	if (caps & SDHCI_CAN_VDD_300)
		host->mci.voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
	if (caps & SDHCI_CAN_VDD_330)
		host->mci.voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;

	if (caps & SDHCI_CAN_DO_HISPD)
		host->mci.host_caps |= (MMC_CAP_MMC_HIGHSPEED_52MHZ |
					MMC_CAP_MMC_HIGHSPEED |
					MMC_CAP_SD_HIGHSPEED);

	/* parse board supported bus width capabilities */
	mci_of_parse(&host->mci);

	/* limit bus widths to controller capabilities */
	if ((caps & SDHCI_CAN_DO_8BIT) == 0)
		host->mci.host_caps &= ~MMC_CAP_8_BIT_DATA;
}

static int dove_sdhci_probe(struct device *dev)
{
	struct dove_sdhci *host;
	int ret;

	host = xzalloc(sizeof(*host));
	host->sdhci.base = dev_request_mem_region(dev, 0);
	host->mci.max_req_size = 0x8000;
	host->mci.hw_dev = dev;
	host->mci.send_cmd = dove_sdhci_mci_send_cmd;
	host->mci.set_ios = dove_sdhci_mci_set_ios;
	host->mci.init = dove_sdhci_mci_init;
	host->mci.f_max = 50000000;
	host->mci.f_min = host->mci.f_max / 256;

	dove_sdhci_set_mci_caps(host);

	ret = mci_register(&host->mci);
	if (ret)
		free(host);
	return ret;
}

static struct of_device_id dove_sdhci_dt_ids[] = {
	{ .compatible = "marvell,dove-sdhci", },
	{ }
};
MODULE_DEVICE_TABLE(of, dove_sdhci_dt_ids);

static struct driver dove_sdhci_driver = {
	.name = "dove-sdhci",
	.probe = dove_sdhci_probe,
	.of_compatible = DRV_OF_COMPAT(dove_sdhci_dt_ids),
};
device_platform_driver(dove_sdhci_driver);
