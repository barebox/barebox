// SPDX-License-Identifier: GPL-2.0-or-later

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/clk.h>
#include <mci.h>

#include "sdhci.h"

#define SDHCI_ARASAN_HCAP_CLK_FREQ_MASK		0xFF00
#define SDHCI_ARASAN_HCAP_CLK_FREQ_SHIFT	8
#define SDHCI_INT_ADMAE				BIT(29)
#define SDHCI_ARASAN_INT_DATA_MASK		(SDHCI_INT_XFER_COMPLETE | \
						SDHCI_INT_DMA | \
						SDHCI_INT_SPACE_AVAIL | \
						SDHCI_INT_DATA_AVAIL | \
						SDHCI_INT_DATA_TIMEOUT | \
						SDHCI_INT_DATA_CRC | \
						SDHCI_INT_DATA_END_BIT | \
						SDHCI_INT_ADMAE)

#define SDHCI_ARASAN_INT_CMD_MASK		(SDHCI_INT_CMD_COMPLETE | \
						SDHCI_INT_TIMEOUT | \
						SDHCI_INT_CRC | \
						SDHCI_INT_END_BIT | \
						SDHCI_INT_INDEX)

#define SDHCI_ARASAN_BUS_WIDTH			4
#define TIMEOUT_VAL				0xE

struct arasan_sdhci_host {
	struct mci_host		mci;
	struct sdhci		sdhci;
	unsigned int		quirks; /* Arasan deviations from spec */
/* Controller does not have CD wired and will not function normally without */
#define SDHCI_ARASAN_QUIRK_FORCE_CDTEST		BIT(0)
#define SDHCI_ARASAN_QUIRK_NO_1_8_V		BIT(1)
};

static inline
struct arasan_sdhci_host *to_arasan_sdhci_host(struct mci_host *mci)
{
	return container_of(mci, struct arasan_sdhci_host, mci);
}

static inline
struct arasan_sdhci_host *sdhci_to_arasan(struct sdhci *sdhci)
{
	return container_of(sdhci, struct arasan_sdhci_host, sdhci);
}

static int arasan_sdhci_card_present(struct mci_host *mci)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	u32 val;

	val = sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE);

	return !!(val & SDHCI_CARD_DETECT);
}

static int arasan_sdhci_card_write_protected(struct mci_host *mci)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	u32 val;

	val = sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE);

	return !(val & SDHCI_WRITE_PROTECT);
}

static int arasan_sdhci_reset(struct arasan_sdhci_host *host, u8 mask)
{
	int ret;

	ret = sdhci_reset(&host->sdhci, mask);
	if (ret)
		return ret;

	if (host->quirks & SDHCI_ARASAN_QUIRK_FORCE_CDTEST) {
		u8 ctrl;

		ctrl = sdhci_read8(&host->sdhci, SDHCI_HOST_CONTROL);
		ctrl |= SDHCI_CTRL_CDTEST_INS | SDHCI_CTRL_CDTEST_INS;
		sdhci_write8(&host->sdhci, ctrl, SDHCI_HOST_CONTROL);
	}

	return 0;
}

static int arasan_sdhci_init(struct mci_host *mci, struct device *dev)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	int ret;

	ret = arasan_sdhci_reset(host, SDHCI_RESET_ALL);
	if (ret)
		return ret;

	sdhci_write8(&host->sdhci, SDHCI_POWER_CONTROL,
		     SDHCI_BUS_VOLTAGE_330 | SDHCI_BUS_POWER_EN);
	udelay(400);

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE,
		      SDHCI_ARASAN_INT_DATA_MASK | SDHCI_ARASAN_INT_CMD_MASK);
	sdhci_write32(&host->sdhci, SDHCI_SIGNAL_ENABLE, 0);

	return 0;
}

static void arasan_sdhci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	u16 val;

	if (ios->clock)
		sdhci_set_clock(&host->sdhci, ios->clock, host->sdhci.max_clk);

	sdhci_set_bus_width(&host->sdhci, ios->bus_width);

	val = sdhci_read8(&host->sdhci, SDHCI_HOST_CONTROL);

	if (ios->clock > 26000000)
		val |= SDHCI_CTRL_HISPD;
	else
		val &= ~SDHCI_CTRL_HISPD;

	sdhci_write8(&host->sdhci, SDHCI_HOST_CONTROL, val);
}

static void print_error(struct arasan_sdhci_host *host, int cmdidx, int ret)
{
	if (ret == -ETIMEDOUT)
		return;

	dev_err(host->mci.hw_dev,
		"error while transferring data for command %d\n", cmdidx);
	dev_err(host->mci.hw_dev, "state = 0x%08x , interrupt = 0x%08x\n",
		sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE),
		sdhci_read32(&host->sdhci, SDHCI_INT_NORMAL_STATUS));
}

static int arasan_sdhci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				 struct mci_data *data)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	u32 mask, command, xfer;
	int ret;

	/* Wait for idle before next command */
	mask = SDHCI_CMD_INHIBIT_CMD;
	if (cmd->cmdidx != MMC_CMD_STOP_TRANSMISSION)
		mask |= SDHCI_CMD_INHIBIT_DATA;

	ret = wait_on_timeout(10 * MSECOND,
			      !(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) & mask));
	if (ret) {
		dev_err(host->mci.hw_dev,
			"SDHCI timeout while waiting for idle\n");
		return ret;
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	mask = SDHCI_INT_CMD_COMPLETE;
	if (data && data->flags == MMC_DATA_READ)
		mask |= SDHCI_INT_DATA_AVAIL;
	if (cmd->resp_type & MMC_RSP_BUSY)
		mask |= SDHCI_INT_XFER_COMPLETE;

	sdhci_set_cmd_xfer_mode(&host->sdhci,
				cmd, data, false, &command, &xfer);

	sdhci_write8(&host->sdhci, SDHCI_TIMEOUT_CONTROL, TIMEOUT_VAL);
	if (data) {
		sdhci_write16(&host->sdhci, SDHCI_TRANSFER_MODE, xfer);
		sdhci_write16(&host->sdhci, SDHCI_BLOCK_SIZE,
			      SDHCI_DMA_BOUNDARY_512K | SDHCI_TRANSFER_BLOCK_SIZE(data->blocksize));
		sdhci_write16(&host->sdhci, SDHCI_BLOCK_COUNT, data->blocks);
	}
	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write16(&host->sdhci, SDHCI_COMMAND, command);

	ret = sdhci_wait_for_done(&host->sdhci, mask);
	if (ret)
		goto error;

	sdhci_read_response(&host->sdhci, cmd);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, mask);

	if (data)
		ret = sdhci_transfer_data_pio(&host->sdhci, data);

error:
	if (ret) {
		print_error(host, cmd->cmdidx, ret);
		arasan_sdhci_reset(host, SDHCI_RESET_CMD);
		arasan_sdhci_reset(host, SDHCI_RESET_DATA);
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	return ret;
}

static int arasan_sdhci_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct arasan_sdhci_host *arasan_sdhci;
	struct clk *clk_xin, *clk_ahb;
	struct resource *iores;
	struct mci_host *mci;
	int ret;

	arasan_sdhci = xzalloc(sizeof(*arasan_sdhci));

	mci = &arasan_sdhci->mci;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	clk_ahb = clk_get(dev, "clk_ahb");
	if (IS_ERR(clk_ahb)) {
		dev_err(dev, "clk_ahb clock not found.\n");
		return PTR_ERR(clk_ahb);
	}

	clk_xin = clk_get(dev, "clk_xin");
	if (IS_ERR(clk_xin)) {
		dev_err(dev, "clk_xin clock not found.\n");
		return PTR_ERR(clk_xin);
	}
	ret = clk_enable(clk_ahb);
	if (ret) {
		dev_err(dev, "Failed to enable AHB clock: %s\n",
			strerror(ret));
		return ret;
	}

	ret = clk_enable(clk_xin);
	if (ret) {
		dev_err(dev, "Failed to enable SD clock: %s\n", strerror(ret));
		return ret;
	}

	if (of_property_read_bool(np, "xlnx,fails-without-test-cd"))
		arasan_sdhci->quirks |= SDHCI_ARASAN_QUIRK_FORCE_CDTEST;

	if (of_property_read_bool(np, "no-1-8-v"))
		arasan_sdhci->quirks |= SDHCI_ARASAN_QUIRK_NO_1_8_V;

	arasan_sdhci->sdhci.base = IOMEM(iores->start);
	arasan_sdhci->sdhci.mci = mci;
	mci->send_cmd = arasan_sdhci_send_cmd;
	mci->set_ios = arasan_sdhci_set_ios;
	mci->init = arasan_sdhci_init;
	mci->card_present = arasan_sdhci_card_present;
	mci->card_write_protected = arasan_sdhci_card_write_protected;
	mci->hw_dev = dev;

	mci->f_max = clk_get_rate(clk_xin);
	mci->f_min = 50000000 / 256;

	/* parse board supported bus width capabilities */
	mci_of_parse(&arasan_sdhci->mci);

	sdhci_setup_host(&arasan_sdhci->sdhci);

	dev->priv = arasan_sdhci;

	return mci_register(&arasan_sdhci->mci);
}

static __maybe_unused struct of_device_id arasan_sdhci_compatible[] = {
	{ .compatible = "arasan,sdhci-8.9a" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, arasan_sdhci_compatible);

static struct driver arasan_sdhci_driver = {
	.name = "arasan-sdhci",
	.probe = arasan_sdhci_probe,
	.of_compatible = DRV_OF_COMPAT(arasan_sdhci_compatible),
};
device_platform_driver(arasan_sdhci_driver);
