// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2007,2010 Freescale Semiconductor, Inc
// SPDX-FileCopyrightText: 2003 Kyle Harris <kharris@nexus-tech.net>, Nexus Technologies

/*
 * Andy Fleming
 */

#include <config.h>
#include <common.h>
#include <dma.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <malloc.h>
#include <mci.h>
#include <linux/pinctrl/consumer.h>
#include <clock.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <gpio.h>
#include <of_device.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx53-regs.h>

#include "sdhci.h"
#include "imx-esdhc.h"


#define PRSSTAT_SDSTB 0x00000008
#define ESDHC_BURST_LEN_EN_INCR		(1 << 27)

/* Bits 3 and 6 are not SDHCI standard definitions */
#define  ESDHC_MIX_CTRL_SDHCI_MASK	0xb7
/* Tuning bits */
#define  ESDHC_MIX_CTRL_TUNING_MASK	0x03c00000

/* dll control register */
#define ESDHC_DLL_CTRL			0x60
#define ESDHC_DLL_OVERRIDE_VAL_SHIFT	9
#define ESDHC_DLL_OVERRIDE_EN_SHIFT	8

/* tune control register */
#define ESDHC_TUNE_CTRL_STATUS		0x68
#define  ESDHC_TUNE_CTRL_STEP		1
#define  ESDHC_TUNE_CTRL_MIN		0
#define  ESDHC_TUNE_CTRL_MAX		((1 << 7) - 1)

#define ESDHC_TUNING_CTRL		0xcc
#define ESDHC_STD_TUNING_EN		(1 << 24)
/* NOTE: the minimum valid tuning start tap for mx6sl is 1 */
#define ESDHC_TUNING_START_TAP_DEFAULT	0x1
#define ESDHC_TUNING_START_TAP_MASK	0x7f
#define ESDHC_TUNING_CMD_CRC_CHECK_DISABLE	(1 << 7)
#define ESDHC_TUNING_STEP_DEFAULT	0x1
#define ESDHC_TUNING_STEP_MASK		0x00070000
#define ESDHC_TUNING_STEP_SHIFT		16

/* pinctrl state */
#define ESDHC_PINCTRL_STATE_100MHZ	"state_100mhz"
#define ESDHC_PINCTRL_STATE_200MHZ	"state_200mhz"

#define to_fsl_esdhc(mci)	container_of(mci, struct fsl_esdhc_host, mci)

/*
 * Sends a command out on the bus.  Takes the mci pointer,
 * a command pointer, and an optional data pointer.
 */
static int
esdhc_send_cmd(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);

	return  __esdhc_send_cmd(host, cmd, data);
}

static void set_sysctl(struct mci_host *mci, u32 clock, bool ddr)
{
	int div, pre_div, ddr_pre_div = 1;
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	unsigned sdhc_clk = clk_get_rate(host->clk);
	u32 val, clk;
	unsigned long  cur_clock;

	if (esdhc_is_usdhc(host) && ddr)
		ddr_pre_div = 2;

	if (esdhc_is_layerscape(host))
		sdhc_clk >>= 1;

	/* For i.MX53 eSDHCv3, SYSCTL.SDCLKFS may not be set to 0. */
	if (cpu_is_mx53() && host->sdhci.base == (void *)MX53_ESDHC3_BASE_ADDR)
		pre_div = 2;
	else
		pre_div = 1;

	if (sdhc_clk == clock)
		pre_div = 1;
	else if (sdhc_clk / 16 > clock)
		for (; pre_div < 256; pre_div *= 2)
			if ((sdhc_clk / (pre_div * ddr_pre_div)) <= (clock * 16))
				break;

	for (div = 1; div <= 16; div++)
		if ((sdhc_clk / (div * pre_div * ddr_pre_div)) <= clock)
			break;

	cur_clock = sdhc_clk / pre_div / div;

	dev_dbg(host->dev, "set clock: wanted: %d got: %ld\n", clock, cur_clock);
	dev_dbg(host->dev, "pre_div: %d div: %d\n", pre_div, div);

	/* the register values start with 0 */
	div -= 1;
	pre_div >>= 1;

	clk = (pre_div << 8) | (div << 4);

	esdhc_clrbits32(host, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			SYSCTL_CKEN);

	esdhc_clrsetbits32(host, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			SYSCTL_CLOCK_MASK, clk);

	esdhc_poll(host, SDHCI_PRESENT_STATE, val,
		   val & PRSSTAT_SDSTB,
		   10 * MSECOND);

	clk = SYSCTL_PEREN | SYSCTL_CKEN | SYSCTL_INITA;

	esdhc_setbits32(host, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			clk);

	esdhc_poll(host, SDHCI_CLOCK_CONTROL, val,
		   val & SYSCTL_INITA,
		   10 * MSECOND);
}

static int esdhc_change_pinstate(struct fsl_esdhc_host *host,
				 unsigned int uhs)
{
	struct pinctrl_state *pinctrl;

	dev_dbg(host->dev, "change pinctrl state for uhs %d\n", uhs);

	if (IS_ERR(host->pinctrl) ||
		IS_ERR(host->pins_100mhz) ||
		IS_ERR(host->pins_200mhz))
		return -EINVAL;

	switch (uhs) {
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_UHS_DDR50:
		pinctrl = host->pins_100mhz;
		break;
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_MMC_HS200:
	case MMC_TIMING_MMC_HS400:
		pinctrl = host->pins_200mhz;
		break;
	default:
		/* back to default state for other legacy timing */
		return pinctrl_select_state_default(host->dev);
	}

	return pinctrl_select_state(host->pinctrl, pinctrl);
}


static void usdhc_set_timing(struct fsl_esdhc_host *host, enum mci_timing timing)
{
	u32 mixctrl;

	mixctrl = sdhci_read32(&host->sdhci, IMX_SDHCI_MIXCTRL);
	mixctrl &= ~MIX_CTRL_DDREN;

	switch (timing) {
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_DDR52:
		mixctrl |= MIX_CTRL_DDREN;
		sdhci_write32(&host->sdhci, IMX_SDHCI_MIXCTRL, mixctrl);
		if (host->boarddata.delay_line) {
			u32 v;
			v = host->boarddata.delay_line <<
				IMX_SDHCI_DLL_CTRL_OVERRIDE_VAL_SHIFT |
				(1 << IMX_SDHCI_DLL_CTRL_OVERRIDE_EN_SHIFT);
			if (cpu_is_mx53())
				v <<= 1;
			sdhci_write32(&host->sdhci, IMX_SDHCI_DLL_CTRL, v);
		}
		break;
	default:
		sdhci_write32(&host->sdhci, IMX_SDHCI_MIXCTRL, mixctrl);
	}

	esdhc_change_pinstate(host, timing);

	host->sdhci.timing = timing;
}

static void layerscape_set_timing(struct fsl_esdhc_host *host, enum mci_timing timing)
{
	esdhc_clrbits32(host, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			SYSCTL_CKEN);

	switch (timing) {
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_DDR52:
		esdhc_clrsetbits32(host, SDHCI_ACMD12_ERR__HOST_CONTROL2,
				   SDHCI_ACMD12_ERR__HOST_CONTROL2_UHSM,
				   FIELD_PREP(SDHCI_ACMD12_ERR__HOST_CONTROL2_UHSM, 4));
		break;
	default:
		esdhc_clrbits32(host, SDHCI_ACMD12_ERR__HOST_CONTROL2,
				SDHCI_ACMD12_ERR__HOST_CONTROL2_UHSM);
		break;
	}

	esdhc_setbits32(host, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			SYSCTL_CKEN);

	host->sdhci.timing = timing;
}

static void esdhc_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);

	/*
	 * call esdhc_set_timing() before update the clock rate,
	 * This is because current we support DDR and SDR timing,
	 * Once the DDR_EN bit is set, the card clock will be
	 * divide by 2 automatically. So need to do this before
	 * setting clock rate.
	 */
	if (host->sdhci.timing != ios->timing) {
		if (esdhc_is_usdhc(host))
			usdhc_set_timing(host, ios->timing);
		else if (esdhc_is_layerscape(host))
			layerscape_set_timing(host, ios->timing);
	}

	/* Set the clock speed */
	set_sysctl(mci, ios->clock, mci_timing_is_ddr(ios->timing));

	sdhci_set_drv_type(&host->sdhci, ios->drv_type);

	/* Set the bus width */
	esdhc_clrbits32(host, SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
			PROCTL_DTW_4 | PROCTL_DTW_8);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		esdhc_setbits32(host, SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
				PROCTL_DTW_4);
		break;
	case MMC_BUS_WIDTH_8:
		esdhc_setbits32(host, SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
				PROCTL_DTW_8);
		break;
	case MMC_BUS_WIDTH_1:
		break;
	default:
		return;
	}

}

static int esdhc_card_present(struct mci_host *mci)
{
	return 1;
}

static int esdhc_reset(struct fsl_esdhc_host *host)
{
	u32 val;

	/* reset the controller */
	sdhci_write32(&host->sdhci, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			SYSCTL_RSTA);

	/* extra register reset for i.MX6 Solo/DualLite */
	if (esdhc_is_usdhc(host)) {
		/* reset bit FBCLK_SEL */
		val = sdhci_read32(&host->sdhci, IMX_SDHCI_MIXCTRL);
		val &= ~IMX_SDHCI_MIX_CTRL_FBCLK_SEL;
		sdhci_write32(&host->sdhci, IMX_SDHCI_MIXCTRL, val);

		/* reset delay line settings in IMX_SDHCI_DLL_CTRL */
		sdhci_write32(&host->sdhci, IMX_SDHCI_DLL_CTRL, 0x0);
	}

	/* hardware clears the bit when it is done */
	if (esdhc_poll(host, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
		       val, (val & SYSCTL_RSTA) == 0, 100 * MSECOND)) {
		dev_err(host->dev, "Reset never completed.\n");
		return -EIO;
	}

	return 0;
}

static int esdhc_init(struct mci_host *mci, struct device *dev)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	int ret;

	ret = esdhc_reset(host);
	if (ret)
		return ret;

	sdhci_write32(&host->sdhci, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			SYSCTL_HCKEN | SYSCTL_IPGEN);

	/* RSTA doesn't reset MMC_BOOT register, so manually reset it */
	sdhci_write32(&host->sdhci, SDHCI_MMC_BOOT, 0);

	if (esdhc_is_layerscape(host))
		esdhc_setbits32(host, ESDHC_DMA_SYSCTL,
				ESDHC_SYSCTL_DMA_SNOOP | /* Enable cache snooping */
				ESDHC_SYSCTL_PERIPHERAL_CLK_SEL);

	/* Set the initial clock speed */
	set_sysctl(mci, 400000, false);

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE, SDHCI_INT_CMD_COMPLETE |
			SDHCI_INT_XFER_COMPLETE | SDHCI_INT_CARD_INT |
			SDHCI_INT_TIMEOUT | SDHCI_INT_CRC | SDHCI_INT_END_BIT |
			SDHCI_INT_INDEX | SDHCI_INT_DATA_TIMEOUT |
			SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_END_BIT | SDHCI_INT_DMA);

	/* Put the PROCTL reg back to the default */
	sdhci_write32(&host->sdhci, SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL,
			PROCTL_INIT);

	/* Set timout to the maximum value */
	esdhc_clrsetbits32(host, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET,
			SYSCTL_TIMEOUT_MASK, 14 << 16);

	if (IS_ENABLED(CONFIG_MCI_TUNING) && esdhc_is_usdhc(host) &&
	    (host->socdata->flags & ESDHC_FLAG_STD_TUNING)) {
		u32 tmp;

		/* disable DLL_CTRL delay line settings */
		sdhci_write32(&host->sdhci, ESDHC_DLL_CTRL, 0x0);

		tmp = sdhci_read32(&host->sdhci, ESDHC_TUNING_CTRL);
		tmp |= ESDHC_STD_TUNING_EN;

		tmp &= ~(ESDHC_TUNING_START_TAP_MASK | ESDHC_TUNING_STEP_MASK);
		tmp |= host->boarddata.tuning_start_tap;

		tmp |= host->boarddata.tuning_step << ESDHC_TUNING_STEP_SHIFT;

		/* Disable the CMD CRC check for tuning, if not, need to
		 * add some delay after every tuning command, because
		 * hardware standard tuning logic will directly go to next
		 * step once it detect the CMD CRC error, will not wait for
		 * the card side to finally send out the tuning data, trigger
		 * the buffer read ready interrupt immediately. If usdhc send
		 * the next tuning command some eMMC card will stuck, can't
		 * response, block the tuning procedure or the first command
		 * after the whole tuning procedure always can't get any response.
		 */
		tmp |= ESDHC_TUNING_CMD_CRC_CHECK_DISABLE;
		sdhci_write32(&host->sdhci, ESDHC_TUNING_CTRL, tmp);
	}

	return 0;
}

static const struct mci_ops fsl_esdhc_ops = {
	.send_cmd = esdhc_send_cmd,
	.set_ios = esdhc_set_ios,
	.init = esdhc_init,
	.card_present = esdhc_card_present,
};

static void fsl_esdhc_probe_dt(struct device *dev, struct fsl_esdhc_host *host)
{
	struct device_node *np = dev->of_node;
	struct esdhc_platform_data *boarddata = &host->boarddata;

	if (!IS_ENABLED(CONFIG_MCI_TUNING))
		return;

	if (of_property_read_u32(np, "fsl,tuning-step", &boarddata->tuning_step))
		boarddata->tuning_step = ESDHC_TUNING_STEP_DEFAULT;
	if (of_property_read_u32(np, "fsl,tuning-start-tap",
			     &boarddata->tuning_start_tap))
		boarddata->tuning_start_tap = ESDHC_TUNING_START_TAP_DEFAULT;
	if (of_property_read_u32(np, "fsl,delay-line", &boarddata->delay_line))
		boarddata->delay_line = 0;

	if (esdhc_is_usdhc(host) && !IS_ERR(host->pinctrl)) {
		host->pins_100mhz = pinctrl_lookup_state(host->pinctrl,
						ESDHC_PINCTRL_STATE_100MHZ);
		host->pins_200mhz = pinctrl_lookup_state(host->pinctrl,
						ESDHC_PINCTRL_STATE_200MHZ);
	}
}

static int fsl_esdhc_probe(struct device *dev)
{
	struct resource *iores;
	struct fsl_esdhc_host *host;
	struct mci_host *mci;
	int ret;
	unsigned long rate;
	const struct esdhc_soc_data *socdata;

	ret = dev_get_drvdata(dev, (const void **)&socdata);
	if (ret)
		return ret;

	host = xzalloc(sizeof(*host));
	host->socdata = socdata;
	mci = &host->mci;

	dma_set_mask(dev, DMA_BIT_MASK(32));

	host->clk = clk_get(dev, socdata->clkidx);
	if (IS_ERR(host->clk)) {
		ret = PTR_ERR(host->clk);
		goto err_free;
	}

	ret = clk_enable(host->clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock: %pe\n", ERR_PTR(ret));
		goto err_clk_put;
	}

	host->dev = dev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_clk_disable;
	}
	host->sdhci.base = IOMEM(iores->start);

	esdhc_populate_sdhci(host);

	host->mci.ops = fsl_esdhc_ops;
	host->mci.hw_dev = dev;
	host->sdhci.mci = &host->mci;

	ret = sdhci_setup_host(&host->sdhci);
	if (ret)
		goto err_clk_disable;

	if (esdhc_is_usdhc(host) || esdhc_is_layerscape(host))
		mci->host_caps |= MMC_CAP_3_3V_DDR | MMC_CAP_1_8V_DDR;

	rate = clk_get_rate(host->clk);
	host->mci.f_min = rate >> 12;
	if (host->mci.f_min < 200000)
		host->mci.f_min = 200000;
	host->mci.f_max = rate;

	mci_of_parse(&host->mci);

	host->pinctrl = pinctrl_get(dev);
	if (IS_ERR(host->pinctrl))
		dev_warn(host->dev, "could not get pinctrl\n");

	fsl_esdhc_probe_dt(dev, host);

	ret = mci_register(&host->mci);
	if (ret)
		goto err_release_res;

	return 0;

err_release_res:
	release_region(iores);
err_clk_disable:
	clk_disable(host->clk);
err_clk_put:
	clk_put(host->clk);
err_free:
	free(host);
	return ret;
}

static struct esdhc_soc_data esdhc_imx25_data = {
	.flags = ESDHC_FLAG_ENGCM07207,
	.clkidx = "per",
};

static struct esdhc_soc_data esdhc_imx53_data = {
	.flags = ESDHC_FLAG_MULTIBLK_NO_INT,
	.clkidx = "per",
};

static struct esdhc_soc_data usdhc_imx6q_data = {
	.flags = ESDHC_FLAG_USDHC | ESDHC_FLAG_MAN_TUNING,
	.clkidx = "per",
};

static struct esdhc_soc_data usdhc_imx6sl_data = {
	.flags = ESDHC_FLAG_USDHC | ESDHC_FLAG_STD_TUNING
	       | ESDHC_FLAG_HAVE_CAP1 | ESDHC_FLAG_ERR004536
	       | ESDHC_FLAG_HS200,
	.clkidx = "per",
};

static struct esdhc_soc_data usdhc_imx6sx_data = {
	.flags = ESDHC_FLAG_USDHC | ESDHC_FLAG_STD_TUNING
	       | ESDHC_FLAG_HAVE_CAP1 | ESDHC_FLAG_HS200,
	.clkidx = "per",
};

static struct esdhc_soc_data esdhc_ls_be_data = {
	.flags = ESDHC_FLAG_MULTIBLK_NO_INT | ESDHC_FLAG_BIGENDIAN |
		 ESDHC_FLAG_LAYERSCAPE,
};

static struct esdhc_soc_data esdhc_ls_le_data = {
	.flags = ESDHC_FLAG_MULTIBLK_NO_INT | ESDHC_FLAG_LAYERSCAPE,
};

static __maybe_unused struct of_device_id fsl_esdhc_compatible[] = {
	{ .compatible = "fsl,imx25-esdhc",  .data = &esdhc_imx25_data  },
	{ .compatible = "fsl,imx50-esdhc",  .data = &esdhc_imx53_data  },
	{ .compatible = "fsl,imx51-esdhc",  .data = &esdhc_imx53_data  },
	{ .compatible = "fsl,imx53-esdhc",  .data = &esdhc_imx53_data  },
	{ .compatible = "fsl,imx6q-usdhc",  .data = &usdhc_imx6q_data  },
	{ .compatible = "fsl,imx6sl-usdhc", .data = &usdhc_imx6sl_data },
	{ .compatible = "fsl,imx6sx-usdhc", .data = &usdhc_imx6sx_data },
	{ .compatible = "fsl,imx8mq-usdhc", .data = &usdhc_imx6sx_data },
	{ .compatible = "fsl,imx8mm-usdhc", .data = &usdhc_imx6sx_data },
	{ .compatible = "fsl,imx8mn-usdhc", .data = &usdhc_imx6sx_data },
	{ .compatible = "fsl,imx8mp-usdhc", .data = &usdhc_imx6sx_data },
	{ .compatible = "fsl,ls1028a-esdhc",.data = &esdhc_ls_le_data  },
	{ .compatible = "fsl,ls1046a-esdhc",.data = &esdhc_ls_be_data  },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fsl_esdhc_compatible);

static struct platform_device_id imx_esdhc_ids[] = {
	{
		.name = "imx25-esdhc",
		.driver_data = (unsigned long)&esdhc_imx25_data,
	}, {
		.name = "imx5-esdhc",
		.driver_data = (unsigned long)&esdhc_imx53_data,
	}, {
		/* sentinel */
	}
};

static struct driver fsl_esdhc_driver = {
	.name  = "imx-esdhc",
	.probe = fsl_esdhc_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_esdhc_compatible),
	.id_table = imx_esdhc_ids,
};
device_platform_driver(fsl_esdhc_driver);
