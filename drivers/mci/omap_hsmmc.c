// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2008 Texas Instruments (http://www.ti.com/, Sukumar Ghorai <s-ghorai@ti.com>)

/* #define DEBUG */
#include <common.h>
#include <init.h>
#include <driver.h>
#include <mci.h>
#include <clock.h>
#include <errno.h>
#include <io.h>
#include <linux/err.h>

#include "omap_hsmmc.h"

#include <mach/omap/omap_hsmmc.h>

#if defined(CONFIG_MFD_TWL6030) && \
	defined(CONFIG_MCI_OMAP_HSMMC) && \
	defined(CONFIG_ARCH_OMAP4)
#include <mach/omap/omap4_twl6030_mmc.h>
#endif

struct omap_mmc_driver_data {
	unsigned long reg_ofs;
};

static struct omap_mmc_driver_data omap3_data = {
	.reg_ofs = 0,
};

static struct omap_mmc_driver_data omap4_data = {
	.reg_ofs = 0x100,
};

#define to_hsmmc(mci)	container_of(mci, struct omap_hsmmc, mci)

static int mmc_init_setup(struct mci_host *mci, struct device *dev)
{
	struct omap_hsmmc *hsmmc = to_hsmmc(mci);

/*
 * Fix voltage for mmc, if booting from nand.
 * It's necessary to do this here, because
 * you need to set up this at probetime.
 */
#if defined(CONFIG_MFD_TWL6030) && \
	defined(CONFIG_MCI_OMAP_HSMMC) && \
	defined(CONFIG_ARCH_OMAP4)
	set_up_mmc_voltage_omap4();
#endif

	return omap_hsmmc_init(hsmmc);
}

static int mmc_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
		struct mci_data *data)
{
	struct omap_hsmmc *hsmmc = to_hsmmc(mci);

	return omap_hsmmc_send_cmd(hsmmc, cmd, data);
}

static void mmc_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct omap_hsmmc *hsmmc = to_hsmmc(mci);

	return omap_hsmmc_set_ios(hsmmc, ios);
}

static const struct mci_ops omap_mmc_ops = {
	.send_cmd = mmc_send_cmd,
	.set_ios = mmc_set_ios,
	.init = mmc_init_setup,
};

static int omap_mmc_probe(struct device *dev)
{
	struct resource *iores;
	struct omap_hsmmc *hsmmc;
	struct omap_hsmmc_platform_data *pdata;
	const struct omap_mmc_driver_data *drvdata;
	unsigned long reg_ofs = 0;

	drvdata = device_get_match_data(dev);
	if (drvdata)
		reg_ofs = drvdata->reg_ofs;

	hsmmc = xzalloc(sizeof(*hsmmc));

	hsmmc->dev = dev;
	hsmmc->mci.ops = omap_mmc_ops;
	hsmmc->mci.host_caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED |
		MMC_CAP_MMC_HIGHSPEED | MMC_CAP_8_BIT_DATA;
	hsmmc->mci.hw_dev = dev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	hsmmc->iobase = IOMEM(iores->start);
	hsmmc->base = hsmmc->iobase + reg_ofs;

	hsmmc->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	hsmmc->mci.f_min = 400000;
	hsmmc->mci.f_max = 52000000;

	pdata = (struct omap_hsmmc_platform_data *)dev->platform_data;
	if (pdata) {
		if (pdata->f_max)
			hsmmc->mci.f_max = pdata->f_max;

		if (pdata->devname)
			hsmmc->mci.devname = pdata->devname;
	}

	mci_of_parse(&hsmmc->mci);

	mci_register(&hsmmc->mci);

	return 0;
}

static struct platform_device_id omap_mmc_ids[] = {
	{
		.name = "omap3-hsmmc",
		.driver_data = (unsigned long)&omap3_data,
	}, {
		.name = "omap4-hsmmc",
		.driver_data = (unsigned long)&omap4_data,
	}, {
		/* sentinel */
	},
};

static __maybe_unused struct of_device_id omap_mmc_dt_ids[] = {
	{
		.compatible = "ti,omap3-hsmmc",
		.data = &omap3_data,
	}, {
		.compatible = "ti,omap4-hsmmc",
		.data = &omap4_data,
	}, {
		.compatible = "ti,am335-sdhci",
		.data = &omap4_data,
	}, {
		.compatible = "ti,am437-sdhci",
		.data = &omap4_data,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, omap_mmc_dt_ids);

static struct driver omap_mmc_driver = {
	.name  = "omap-hsmmc",
	.probe = omap_mmc_probe,
	.id_table = omap_mmc_ids,
	.of_compatible = DRV_OF_COMPAT(omap_mmc_dt_ids),
};
device_platform_driver(omap_mmc_driver);
