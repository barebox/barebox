// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Jules Maselbas
// derived from: linux/drivers/mmc/host/sunxi-mmc.c
#define pr_fmt(fmt) "sunxi-mmc: " fmt

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <mci.h>

#include <gpio.h>
#include <of_gpio.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <errno.h>
#include <dma.h>

#include "sunxi-mmc.h"
#include "sunxi-mmc-common.c"

static int sdxc_send_cmd(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	struct sunxi_mmc_host *host = to_sunxi_mmc_host(mci);
	struct device *dev = mci->hw_dev;
	const char *why;
	int ret;

	ret = sunxi_mmc_send_cmd(host, cmd, data, &why);
	if (ret && ret != -ETIMEDOUT)
		dev_err(dev, "error %s CMD%d (%d)\n", why, cmd->cmdidx, ret);
	if (ret == -ETIMEDOUT)
		mdelay(1);

	return ret;
}

static void sdxc_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct sunxi_mmc_host *host = to_sunxi_mmc_host(mci);
	struct device *dev = mci->hw_dev;

	dev_dbg(dev, "buswidth: %d clock: %d\n", 1 << ios->bus_width, ios->clock);
	sunxi_mmc_set_ios(host, ios);
}

static int sdxc_card_present(struct mci_host *mci)
{
	struct sunxi_mmc_host *host = to_sunxi_mmc_host(mci);

	/* No gpio, assume card is present */
	if (IS_ERR_OR_NULL(host->gpio_cd))
		return 1;

	return gpiod_get_value(host->gpio_cd);
}

static int sdxc_init(struct mci_host *mci, struct device *dev)
{
	struct sunxi_mmc_host *host = to_sunxi_mmc_host(mci);

	sunxi_mmc_init(host);

	return 0;
}

static const struct mci_ops sdxc_mmc_ops = {
	.send_cmd = sdxc_send_cmd,
	.set_ios = sdxc_set_ios,
	.init = sdxc_init,
	.card_present = sdxc_card_present,
};

static int sunxi_mmc_probe(struct device *dev)
{
	struct resource *iores;
	struct sunxi_mmc_host *host;
	unsigned int f_min, f_max;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	host = xzalloc(sizeof(*host));
	host->base = IOMEM(iores->start);
	dma_set_mask(dev, DMA_BIT_MASK(32));
	host->cfg = device_get_match_data(dev);

	host->gpio_cd = gpiod_get_optional(dev, "cd", GPIOD_IN);

	host->clk_ahb = clk_get(dev, "ahb");
	if (IS_ERR(host->clk_ahb)) {
		ret = PTR_ERR(host->clk_ahb);
		goto err;
	}

	host->clk_mmc = clk_get(dev, "mmc");
	if (IS_ERR(host->clk_mmc)) {
		ret = PTR_ERR(host->clk_mmc);
		goto err;
	}

	clk_enable(host->clk_ahb);
	clk_enable(host->clk_mmc);

	host->mci.hw_dev = dev;
	host->mci.ops = sdxc_mmc_ops;
	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	host->mci.host_caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA
		| MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
		| MMC_CAP_MMC_HIGHSPEED_52MHZ;

	host->clkrate = clk_get_rate(host->clk_mmc);
	f_min = host->clkrate / 510;
	f_max = host->clkrate;
	/* clock must at least support freq as low as 400K, and reach 52M */
	if (400000 < f_min || f_max < 52000000) {
		/* if not, try to get a better clock, clk_set_rate never sets
		 * a frequence above the requested one, thus we try to set a
		 * clk rate close or below 2 * 52MHz (260 * 400KHz).
		 */
		clk_set_rate(host->clk_mmc, 260 * 400000);
		host->clkrate = clk_get_rate(host->clk_mmc);
		f_min = host->clkrate / 510;
		f_max = host->clkrate;
	}
	dev_dbg(dev, "freq: min %d max %d\n", f_min, f_max);
	mci_of_parse(&host->mci);
	host->mci.host_caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA
		| MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
		| MMC_CAP_MMC_HIGHSPEED_52MHZ;

	f_min = max_t(unsigned int,   400000, f_min);
	f_max = min_t(unsigned int, 52000000, f_max);
	host->mci.f_min = max_t(unsigned int, host->mci.f_min, f_min);
	host->mci.f_max = min_t(unsigned int, host->mci.f_max, f_max);

	return mci_register(&host->mci);
err:
	if (host->clk_mmc)
		clk_put(host->clk_mmc);
	if (host->clk_ahb)
		clk_put(host->clk_ahb);
	free(host);
	release_region(iores);
	return ret;
}

static const struct sunxi_mmc_cfg sun50i_a64_cfg = {
	.idma_des_size_bits = 16,
	.clk_delays = NULL,
	.can_calibrate = true,
	.mask_data0 = true,
	.needs_new_timings = true,
};

static const struct sunxi_mmc_cfg sun50i_a64_emmc_cfg = {
	.idma_des_size_bits = 13,
	.clk_delays = NULL,
	.can_calibrate = true,
	.needs_new_timings = true,
};

static const struct of_device_id sunxi_mmc_compatible[] = {
	{ .compatible = "allwinner,sun50i-a64-mmc", .data = &sun50i_a64_cfg },
	{ .compatible = "allwinner,sun50i-a64-emmc", .data = &sun50i_a64_emmc_cfg },
	{ /* sentinel */ }
};

static struct driver sunxi_mmc_driver = {
	.name  = "sunxi-mmc",
	.probe = sunxi_mmc_probe,
	.of_compatible = DRV_OF_COMPAT(sunxi_mmc_compatible),
};
device_platform_driver(sunxi_mmc_driver);
