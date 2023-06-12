// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2011 Hubert Feurstein <h.feurstein@gmail.com>
// SPDX-FileCopyrightText: 2009 Ilya Yanok <yanok@emcraft.com>
// SPDX-FileCopyrightText: 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2006 Pavel Pisa <ppisa@pikron.com>, PiKRON

/* Atmel MCI driver */

#include <common.h>
#include <gpio.h>
#include <linux/clk.h>
#include <mci.h>
#include <of_gpio.h>
#include <platform_data/atmel-mci.h>

#include "atmel-mci-regs.h"

/** change host interface settings */
static void atmci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct atmel_mci *host = to_mci_host(mci);

	atmci_common_set_ios(host, ios);
}

static int atmci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
		  struct mci_data *data)
{
	struct atmel_mci *host = to_mci_host(mci);

	return atmci_common_request(host, cmd, data);
}

static void atmci_info(struct device *mci_dev)
{
	struct atmel_mci *host = mci_dev->priv;

	printf("  Bus data width: %d bit\n", host->mci.bus_width);

	printf("  Bus frequency: %u Hz\n", host->mci.clock);
	printf("  Frequency limits: ");
	if (host->mci.f_min == 0)
		printf("no lower limit ");
	else
		printf("%u Hz lower limit ", host->mci.f_min);
	if (host->mci.f_max == 0)
		printf("- no upper limit");
	else
		printf("- %u Hz upper limit", host->mci.f_max);

	printf("\n  Card detection support: %s\n",
		gpio_is_valid(host->detect_pin) ? "yes" : "no");

}

static int atmci_card_present(struct mci_host *mci)
{
	struct atmel_mci *host = to_mci_host(mci);
	int ret;

	/* No gpio, assume card is present */
	if (!gpio_is_valid(host->detect_pin))
		return 1;

	ret = gpio_get_value(host->detect_pin);

	return ret == 0 ? 1 : 0;
}

static int atmci_probe(struct device *hw_dev)
{
	struct resource *iores;
	struct atmel_mci *host;
	struct device_node *np = hw_dev->of_node;
	struct atmel_mci_platform_data *pd = hw_dev->platform_data;
	int ret;

	host = xzalloc(sizeof(*host));
	host->mci.send_cmd = atmci_send_cmd;
	host->mci.set_ios = atmci_set_ios;
	host->mci.init = atmci_reset;
	host->mci.card_present = atmci_card_present;
	host->mci.hw_dev = hw_dev;
	host->detect_pin = -EINVAL;

	if (pd) {
		host->detect_pin  = pd->detect_pin;
		host->mci.devname = pd->devname;

		if (pd->bus_width >= 4)
			host->mci.host_caps |= MMC_CAP_4_BIT_DATA;
		if (pd->bus_width == 8)
			host->mci.host_caps |= MMC_CAP_8_BIT_DATA;

		host->slot_b = pd->slot_b;
	} else if (np) {
		u32 slot_id;
		struct device_node *cnp;
		const char *alias = of_alias_get(np);

		if (alias)
			host->mci.devname = xstrdup(alias);

		host->detect_pin = of_get_named_gpio(np, "cd-gpios", 0);

		for_each_child_of_node(np, cnp) {
			if (of_property_read_u32(cnp, "reg", &slot_id)) {
				dev_warn(hw_dev, "reg property is missing for %s\n",
					 cnp->full_name);
				continue;
			}

			host->slot_b = slot_id;
			mci_of_parse_node(&host->mci, cnp);
			break;
		}
	} else {
		dev_err(hw_dev, "Missing device information\n");
		ret = -EINVAL;
		goto error_out;
	}

	if (gpio_is_valid(host->detect_pin)) {
		ret = gpio_request(host->detect_pin, "mci_cd");
		if (ret) {
			dev_err(hw_dev, "Impossible to request CD gpio %d (%d)\n",
				ret, host->detect_pin);
			goto error_out;
		}

		ret = gpio_direction_input(host->detect_pin);
		if (ret) {
			dev_err(hw_dev, "Impossible to configure CD gpio %d as input (%d)\n",
				ret, host->detect_pin);
			goto error_out;
		}
	}

	iores = dev_request_mem_resource(hw_dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	host->regs = IOMEM(iores->start);
	host->hw_dev = hw_dev;
	hw_dev->priv = host;
	host->clk = clk_get(hw_dev, "mci_clk");
	if (IS_ERR(host->clk)) {
		dev_err(hw_dev, "no mci_clk\n");
		ret = PTR_ERR(host->clk);
		goto error_out;
	}

	clk_enable(host->clk);
	atmci_writel(host, ATMCI_CR, ATMCI_CR_SWRST);
	atmci_writel(host, ATMCI_IDR, ~0UL);
	host->bus_hz = clk_get_rate(host->clk);
	clk_disable(host->clk);

	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	host->mci.f_min = DIV_ROUND_UP(host->bus_hz, 512);
	host->mci.f_max = host->bus_hz >> 1;

	atmci_get_cap(host);

	if (host->caps.has_highspeed)
		host->mci.host_caps |= MMC_CAP_SD_HIGHSPEED;

	if (host->slot_b)
		host->sdc_reg = ATMCI_SDCSEL_SLOT_B;
	else
		host->sdc_reg = ATMCI_SDCSEL_SLOT_A;

	if (IS_ENABLED(CONFIG_MCI_INFO))
		hw_dev->info = atmci_info;

	mci_register(&host->mci);

	return 0;

error_out:
	free(host);

	if (gpio_is_valid(host->detect_pin))
		gpio_free(host->detect_pin);

	return ret;
}

static __maybe_unused struct of_device_id atmci_compatible[] = {
	{
		.compatible = "atmel,hsmci",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, atmci_compatible);

static struct driver atmci_driver = {
	.name	= "atmel_mci",
	.probe	= atmci_probe,
	.of_compatible = DRV_OF_COMPAT(atmci_compatible),
};
device_platform_driver(atmci_driver);
