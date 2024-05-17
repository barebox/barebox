// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2019 Yann Sionneau, Kalray Inc.
// SPDX-FileCopyrightText: 2023 Jules Maselbas, Kalray Inc.

#include <clock.h>
#include <common.h>
#include <init.h>
#include <io.h>
#include <dma.h>
#include <malloc.h>
#include <mci.h>
#include <of_device.h>
#include <linux/err.h>
#include <linux/clk.h>

#include "sdhci.h"

#define tx_delay_static_cfg(delay)      (delay << 5)
#define tx_tuning_clk_sel(delay)        (delay)

#define DWCMSHC_GPIO_OUT  0x34 /* offset from vendor specific area */
#define CARD_STATUS_MASK (0x1e00)
#define CARD_STATUS_TRAN (4 << 9)

static int do_send_cmd(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data);

struct dwcmshc_host {
	struct mci_host mci;
	struct sdhci sdhci;
	int vendor_specific_area;
	const struct dwcmshc_callbacks *cb;
};

struct dwcmshc_callbacks {
	void (*init)(struct mci_host *mci, struct device *dev);
};

static inline struct dwcmshc_host *priv_from_mci_host(struct mci_host *h)
{
	return container_of(h, struct dwcmshc_host, mci);
}

static void mci_setup_cmd(struct mci_cmd *p, unsigned int cmd, unsigned int arg,
			  unsigned int response)
{
	p->cmdidx = cmd;
	p->cmdarg = arg;
	p->resp_type = response;
}

static int do_abort_sequence(struct mci_host *mci, struct mci_cmd *current_cmd)
{
	int ret = 0;
	struct dwcmshc_host *host = priv_from_mci_host(mci);
	struct mci_cmd cmd;
	u64 start;

	mci_setup_cmd(&cmd, MMC_CMD_STOP_TRANSMISSION, 0, MMC_RSP_R1b);
	ret = do_send_cmd(mci, &cmd, NULL);
	if (ret) {
		dev_err(host->mci.hw_dev, "Abort failed at first cmd12!\n");
		goto out;
	}
	mci_setup_cmd(&cmd, MMC_CMD_SEND_STATUS, mci->mci->rca << 16,
		      MMC_RSP_R1);
	ret = do_send_cmd(mci, &cmd, NULL);
	if (ret) {
		dev_err(host->mci.hw_dev, "Abort failed at first cmd13!\n");
		goto out;
	}

	if ((cmd.response[0] & CARD_STATUS_MASK) == CARD_STATUS_TRAN)
		goto out; /* All is OK! */

	mci_setup_cmd(&cmd, MMC_CMD_STOP_TRANSMISSION, 0, MMC_RSP_R1b);
	ret = do_send_cmd(mci, &cmd, NULL);
	if (ret) {
		dev_err(host->mci.hw_dev, "Abort failed at second cmd12!\n");
		goto out;
	}

	mci_setup_cmd(&cmd, MMC_CMD_SEND_STATUS, mci->mci->rca << 16,
		      MMC_RSP_R1);
	ret = do_send_cmd(mci, &cmd, NULL);
	if (ret) {
		dev_err(host->mci.hw_dev, "Abort failed at second cmd13!\n");
		goto out;
	}

	if ((cmd.response[0] & CARD_STATUS_MASK) != CARD_STATUS_TRAN) {
		dev_err(host->mci.hw_dev,
			"Abort sequence failed to put card in TRAN state!\n");
		ret = -EIO;
		goto out;
	}

out:
	/* Perform SW reset if in abort sequence */
	sdhci_write8(&host->sdhci, SDHCI_SOFTWARE_RESET,
		       SDHCI_RESET_DATA | SDHCI_RESET_CMD);
	start = get_time_ns();
	while (sdhci_read8(&host->sdhci, SDHCI_SOFTWARE_RESET) != 0) {
		if (is_timeout(start, 50 * MSECOND)) {
			dev_err(host->mci.hw_dev,
				"SDHCI data reset timeout\n");
			break;
		}
	}

	return ret;
}

static int dwcmshc_mci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				struct mci_data *data)
{
	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		return do_abort_sequence(mci, cmd);
	return do_send_cmd(mci, cmd, data);
}

static int do_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
		       struct mci_data *data)
{
	struct dwcmshc_host *host = priv_from_mci_host(mci);
	dma_addr_t dma = SDHCI_NO_DMA;
	u32 mask, command, xfer;
	int ret;

	/* Do not wait for CMD_INHIBIT_DAT on stop commands */
	mask = SDHCI_CMD_INHIBIT_CMD;
	if (cmd->cmdidx != MMC_CMD_STOP_TRANSMISSION)
		mask |= SDHCI_CMD_INHIBIT_DATA;

	/* Wait for bus idle */
	ret = wait_on_timeout(50 * MSECOND,
			      !(sdhci_read16(&host->sdhci, SDHCI_PRESENT_STATE) & mask));
	if (ret) {
		dev_err(host->mci.hw_dev, "SDHCI timeout while waiting for idle\n");
		return -ETIMEDOUT;
	}

	if (data)
		dev_dbg(host->mci.hw_dev, "cmd %d arg %d bcnt %d bsize %d\n",
			cmd->cmdidx, cmd->cmdarg, data->blocks, data->blocksize);
	else
		dev_dbg(host->mci.hw_dev, "cmd %d arg %d\n", cmd->cmdidx, cmd->cmdarg);

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	sdhci_setup_data_dma(&host->sdhci, data, &dma);

	sdhci_write8(&host->sdhci, SDHCI_TIMEOUT_CONTROL, 0xe);

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data,
				dma == SDHCI_NO_DMA ? false : true,
				&command, &xfer);

	sdhci_write16(&host->sdhci, SDHCI_TRANSFER_MODE, xfer);

	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write16(&host->sdhci, SDHCI_COMMAND, command);

	ret = sdhci_wait_for_done(&host->sdhci, SDHCI_INT_CMD_COMPLETE);
	if (ret)
		goto error;

	sdhci_read_response(&host->sdhci, cmd);

	ret = sdhci_transfer_data(&host->sdhci, data, dma);
error:
	if (ret) {
		sdhci_reset(&host->sdhci, SDHCI_RESET_CMD);
		sdhci_reset(&host->sdhci, SDHCI_RESET_DATA);
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);
	return ret;
}

static u16 dwcmshc_get_clock_divider(struct dwcmshc_host *host, u32 reqclk)
{
	u16 div;

	for (div = 1; div < SDHCI_MAX_DIV_SPEC_300; div += 2)
		if ((host->sdhci.max_clk / div) <= reqclk)
			break;
	div /= 2;

	return div;
}

static void dwcmshc_mci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct dwcmshc_host *host = priv_from_mci_host(mci);
	u16 val;

	dev_dbg(host->mci.hw_dev,
		"%s: clock = %u, bus-width = %d, timing = %02x\n",
		__func__, ios->clock, ios->bus_width, ios->timing);

	/* stop clock */
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, 0);

	if (!ios->clock)
		return;

	/* enable bus power */
	val = SDHCI_BUS_VOLTAGE_330;
	sdhci_write8(&host->sdhci, SDHCI_POWER_CONTROL, val | SDHCI_BUS_POWER_EN);
	udelay(400);

	/* set bus width */
	sdhci_set_bus_width(&host->sdhci, ios->bus_width);

	val = sdhci_read8(&host->sdhci, SDHCI_HOST_CONTROL);
	if (ios->clock > 26000000)
		val |= SDHCI_CTRL_HISPD;
	else
		val &= ~SDHCI_CTRL_HISPD;
	sdhci_write8(&host->sdhci, SDHCI_HOST_CONTROL, val);

	/* set bus clock */
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, 0);
	val = dwcmshc_get_clock_divider(host, ios->clock);
	val = SDHCI_CLOCK_INT_EN | SDHCI_FREQ_SEL(val) | ((val & 0x300) >> 2);
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, val);

	/* wait for internal clock stable */
	if (wait_on_timeout(20 * MSECOND,
			sdhci_read16(&host->sdhci, SDHCI_CLOCK_CONTROL)
			& SDHCI_CLOCK_INT_STABLE)) {
		dev_err(host->mci.hw_dev, "SDHCI clock stable timeout\n");
		return;
	}

	/* enable bus clock */
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, val | SDHCI_CLOCK_CARD_EN);
}

static int dwcmshc_mci_init(struct mci_host *mci, struct device *dev)
{
	struct dwcmshc_host *host = priv_from_mci_host(mci);
	u16 ctrl2;

	/* reset mshci controller */
	sdhci_write8(&host->sdhci, SDHCI_SOFTWARE_RESET, SDHCI_RESET_ALL);

	/* wait for reset completion */
	if (wait_on_timeout(100 * MSECOND,
			    (sdhci_read8(&host->sdhci, SDHCI_SOFTWARE_RESET)
			     & SDHCI_RESET_ALL) == 0)) {
		dev_err(dev, "SDHCI reset timeout\n");
		return -ETIMEDOUT;
	}

	sdhci_write16(&host->sdhci, SDHCI_INT_ERROR_ENABLE, ~0);
	sdhci_write16(&host->sdhci, SDHCI_INT_ENABLE, ~0);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);
	sdhci_write32(&host->sdhci, SDHCI_SIGNAL_ENABLE, ~0);

	sdhci_enable_v4_mode(&host->sdhci);

	dev_dbg(host->mci.hw_dev, "host version4: %s\n",
		ctrl2 & SDHCI_CTRL_V4_MODE ? "enabled" : "disabled");

	if (host->cb && host->cb->init)
		host->cb->init(mci, dev);

	return 0;
}

static int dwcmshc_detect(struct device *dev)
{
	struct dwcmshc_host *host = dev->priv;

	return mci_detect_card(&host->mci);
}

static int dwcmshc_mci_card_present(struct mci_host *mci)
{
	struct dwcmshc_host *host = priv_from_mci_host(mci);
	u32 pstate;

	pstate = sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE);
	return pstate & SDHCI_CARD_PRESENT;
}

static void dwcmshc_set_dma_mask(struct device *dev)
{
	struct dwcmshc_host *host = dev->priv;

	if (host->sdhci.caps & SDHCI_CAN_64BIT_V4)
		dma_set_mask(dev, DMA_BIT_MASK(64));
	else
		dma_set_mask(dev, DMA_BIT_MASK(32));
}

static const struct mci_ops dwcmshc_ops = {
	.init = dwcmshc_mci_init,
	.set_ios = dwcmshc_mci_set_ios,
	.send_cmd = dwcmshc_mci_send_cmd,
	.card_present = dwcmshc_mci_card_present,
};

static int dwcmshc_probe(struct device *dev)
{
	const struct dwcmshc_callbacks *dwcmshc_cb =
			of_device_get_match_data(dev);
	struct dwcmshc_host *host;
	struct resource *iores;
	struct mci_host *mci;
	struct clk *clk;
	int ret;

	host = xzalloc(sizeof(*host));
	mci = &host->mci;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_mem_req;
	}

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto err_clk_get;
	}
	clk_enable(clk);

	host->sdhci.base = IOMEM(iores->start);
	host->sdhci.mci = mci;
	host->sdhci.max_clk = clk_get_rate(clk);
	host->cb = dwcmshc_cb;

	mci->hw_dev = dev;
	mci->ops = dwcmshc_ops;

	sdhci_setup_host(&host->sdhci);

	mci->max_req_size = 0x8000;
	/*
	 * Let's first initialize f_max to the DT clock freq
	 * Then mci_of_parse can override if with the content
	 * of the 'max-frequency' DT property if it is present.
	 * Then we can finish by computing the f_min.
	 */
	mci->f_max = clk_get_rate(clk);
	mci_of_parse(&host->mci);
	mci->f_min = mci->f_max / SDHCI_MAX_DIV_SPEC_300;

	dev->priv = host;
	dev->detect = dwcmshc_detect;

	dwcmshc_set_dma_mask(dev);

	dev_dbg(host->mci.hw_dev, "host controller version: %u\n",
		host->sdhci.version);

	host->vendor_specific_area = sdhci_read32(&host->sdhci,
						   SDHCI_P_VENDOR_SPEC_AREA);
	host->vendor_specific_area &= SDHCI_P_VENDOR_SPEC_AREA_MASK;

	ret = mci_register(&host->mci);
	if (ret)
		goto err_register;

	return ret;

err_register:
	clk_disable(clk);
	clk_put(clk);
err_clk_get:
	release_region(iores);
err_mem_req:
	free(host);

	return ret;
}

static void dwcmshc_coolidgev2_init(struct mci_host *mci, struct device *dev)
{
	struct dwcmshc_host *host = priv_from_mci_host(mci);

	/* configure TX delay to set correct setup/hold for Coolidge V2 */
	sdhci_write32(&host->sdhci,
		      host->vendor_specific_area + DWCMSHC_GPIO_OUT,
		      tx_delay_static_cfg(0xf) | tx_tuning_clk_sel(4));
}

struct dwcmshc_callbacks kalray_coolidgev2_callbacks = {
	.init = dwcmshc_coolidgev2_init,
};

static struct of_device_id dwcmshc_dt_ids[] = {
	{ .compatible = "snps,dwcmshc-sdhci", },
	{ .compatible = "kalray,coolidge-v2-dwcmshc-sdhci", .data = &kalray_coolidgev2_callbacks },
	{ }
};

static struct driver dwcmshc_driver = {
	.name = "dwcmshc-sdhci",
	.probe = dwcmshc_probe,
	.of_compatible = DRV_OF_COMPAT(dwcmshc_dt_ids),
};
device_platform_driver(dwcmshc_driver);
