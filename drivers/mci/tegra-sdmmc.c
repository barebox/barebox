// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2013 Lucas Stach <l.stach@pengutronix.de>
// SPDX-FileCopyrightText: 2010-2013 NVIDIA Corporation (http://www.nvidia.com/)

#include <common.h>
#include <clock.h>
#include <driver.h>
#include <dma.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <mci.h>
#include <of_gpio.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/err.h>

#include "sdhci.h"

#define TEGRA_SDMMC_PRESENT_STATE			0x024
#define  TEGRA_SDMMC_PRESENT_STATE_CMD_INHIBIT_CMD	(1 << 0)
#define  TEGRA_SDMMC_PRESENT_STATE_CMD_INHIBIT_DAT	(1 << 1)

#define TEGRA_SDMMC_PWR_CNTL				0x028
#define  TEGRA_SDMMC_PWR_CNTL_SD_BUS			(1 << 8)
#define  TEGRA_SDMMC_PWR_CNTL_33_V			(7 << 9)

#define TEGRA_SDMMC_CLK_CNTL				0x02c
#define  TEGRA_SDMMC_CLK_CNTL_SW_RESET_FOR_ALL		(1 << 24)
#define  TEGRA_SDMMC_CLK_CNTL_SD_CLOCK_EN		(1 << 2)
#define  TEGRA_SDMMC_CLK_INTERNAL_CLOCK_STABLE		(1 << 1)
#define  TEGRA_SDMMC_CLK_CNTL_INTERNAL_CLOCK_EN		(1 << 0)

#define  TEGRA_SDMMC_INTERRUPT_STATUS_ERR_INTERRUPT	(1 << 15)
#define  TEGRA_SDMMC_INTERRUPT_STATUS_CMD_TIMEOUT	(1 << 16)

#define TEGRA_SDMMC_INT_STAT_EN				0x034
#define  TEGRA_SDMMC_INT_STAT_EN_CMD_COMPLETE		(1 << 0)
#define  TEGRA_SDMMC_INT_STAT_EN_XFER_COMPLETE		(1 << 1)
#define  TEGRA_SDMMC_INT_STAT_EN_DMA_INTERRUPT		(1 << 3)
#define  TEGRA_SDMMC_INT_STAT_EN_BUFFER_WRITE_READY	(1 << 4)
#define  TEGRA_SDMMC_INT_STAT_EN_BUFFER_READ_READY	(1 << 5)

#define TEGRA_SDMMC_INT_SIG_EN				0x038
#define  TEGRA_SDMMC_INT_SIG_EN_XFER_COMPLETE		(1 << 1)

#define TEGRA_SDMMC_SDMEMCOMPPADCTRL			0x1e0
#define  TEGRA_SDMMC_SDMEMCOMPPADCTRL_VREF_SEL_SHIFT	0

#define TEGRA_SDMMC_AUTO_CAL_CONFIG			0x1e4
#define  TEGRA_SDMMC_AUTO_CAL_CONFIG_PU_OFFSET_SHIFT	0
#define  TEGRA_SDMMC_AUTO_CAL_CONFIG_PD_OFFSET_SHIFT	8
#define  TEGRA_SDMMC_AUTO_CAL_CONFIG_ENABLE		(1 << 29)

struct tegra_sdmmc_host {
	struct mci_host		mci;
	void __iomem		*regs;
	struct clk		*clk;
	struct reset_control	*reset;
	int			gpio_cd, gpio_pwr;
	struct sdhci		sdhci;
};
#define to_tegra_sdmmc_host(mci) container_of(mci, struct tegra_sdmmc_host, mci)

static u32 tegra_sdmmc_read32(struct sdhci *sdhci, int reg)
{
	struct tegra_sdmmc_host *host = container_of(sdhci, struct tegra_sdmmc_host, sdhci);

	return readl(host->regs + reg);
}

static void tegra_sdmmc_write32(struct sdhci *sdhci, int reg, u32 val)
{
	struct tegra_sdmmc_host *host = container_of(sdhci, struct tegra_sdmmc_host, sdhci);

	writel(val, host->regs + reg);
}

static int tegra_sdmmc_wait_inhibit(struct tegra_sdmmc_host *host,
				    struct mci_cmd *cmd, struct mci_data *data,
				    unsigned int timeout)
{
	u32 val = TEGRA_SDMMC_PRESENT_STATE_CMD_INHIBIT_CMD;

	/*
	 * We shouldn't wait for data inhibit for stop commands, even
	 * though they might use busy signaling
	 */
	if ((data == NULL) && (cmd->resp_type & MMC_RSP_BUSY))
		val |= TEGRA_SDMMC_PRESENT_STATE_CMD_INHIBIT_DAT;

	wait_on_timeout(timeout * MSECOND,
	                !(sdhci_read32(&host->sdhci, TEGRA_SDMMC_PRESENT_STATE) & val));

	return 0;
}

static int tegra_sdmmc_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				struct mci_data *data)
{
	struct tegra_sdmmc_host *host = to_tegra_sdmmc_host(mci);
	unsigned int num_bytes = 0;
	u32 val = 0, command, xfer;
	int ret;

	ret = tegra_sdmmc_wait_inhibit(host, cmd, data, 10);
	if (ret < 0)
		return ret;

	/* Set up for a data transfer if we have one */
	if (data) {
		num_bytes = data->blocks * data->blocksize;

		if (data->flags & MMC_DATA_WRITE) {
			dma_sync_single_for_device(mci->hw_dev, (unsigned long)data->src,
						   num_bytes, DMA_TO_DEVICE);
			sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS,
				      lower_32_bits(virt_to_phys(data->src)));
		} else {
			dma_sync_single_for_device(mci->hw_dev, (unsigned long)data->dest,
						   num_bytes, DMA_FROM_DEVICE);
			sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS,
				      lower_32_bits(virt_to_phys(data->dest)));
		}

		sdhci_write32(&host->sdhci, SDHCI_BLOCK_SIZE__BLOCK_COUNT,
			      (7 << 12) | data->blocks << 16 | data->blocksize);
	}

	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
		return -1;

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data, true, &command, &xfer);

	sdhci_write32(&host->sdhci, SDHCI_TRANSFER_MODE__COMMAND,
		      command << 16 | xfer);

	ret = wait_on_timeout(100 * MSECOND,
			(val = sdhci_read32(&host->sdhci, SDHCI_INT_STATUS))
			& SDHCI_INT_CMD_COMPLETE);

	if (ret) {
		sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);
		return ret;
	}

	if ((val & SDHCI_INT_CMD_COMPLETE) && !data)
		sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);

	if (val & TEGRA_SDMMC_INTERRUPT_STATUS_CMD_TIMEOUT) {
		/* Timeout Error */
		dev_dbg(mci->hw_dev, "timeout: %08x cmd %d\n", val, cmd->cmdidx);
		sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);
		return -ETIMEDOUT;
	} else if (val & TEGRA_SDMMC_INTERRUPT_STATUS_ERR_INTERRUPT) {
		/* Error Interrupt */
		dev_dbg(mci->hw_dev, "error: %08x cmd %d\n", val, cmd->cmdidx);
		sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);
		return -EIO;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		sdhci_read_response(&host->sdhci, cmd);

		if (cmd->resp_type & MMC_RSP_BUSY) {
			ret = wait_on_timeout(100 * MSECOND,
				sdhci_read32(&host->sdhci, TEGRA_SDMMC_PRESENT_STATE)
				& (1 << 20));

			if (ret) {
				dev_err(mci->hw_dev, "card is still busy\n");
				sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);
				return ret;
			}

			cmd->response[0] = sdhci_read32(&host->sdhci, SDHCI_RESPONSE_0);
		}
	}

	if (data) {
		uint64_t start = get_time_ns();

		while (1) {
			val = sdhci_read32(&host->sdhci, SDHCI_INT_STATUS);

			if (val & TEGRA_SDMMC_INTERRUPT_STATUS_ERR_INTERRUPT) {
				/* Error Interrupt */
				sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);
				dev_err(mci->hw_dev,
					"error during transfer: 0x%08x\n", val);
				return -EIO;
			} else if (val & SDHCI_INT_DMA) {
				/*
				 * DMA Interrupt, restart the transfer where
				 * it was interrupted.
				 */
				u32 address = readl(host->regs +
						    SDHCI_DMA_ADDRESS);

				sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, SDHCI_INT_DMA);
				sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS, address);
			} else if (val & SDHCI_INT_XFER_COMPLETE) {
				/* Transfer Complete */;
				break;
			} else if (is_timeout(start, 2 * SECOND)) {
				sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);
				dev_err(mci->hw_dev, "MMC Timeout\n"
					"    Interrupt status        0x%08x\n"
					"    Interrupt status enable 0x%08x\n"
					"    Interrupt signal enable 0x%08x\n"
					"    Present status          0x%08x\n",
					val,
					sdhci_read32(&host->sdhci, SDHCI_INT_ENABLE),
					sdhci_read32(&host->sdhci, SDHCI_SIGNAL_ENABLE),
					sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE));
				return -ETIMEDOUT;
			}
		}
		sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, val);

		if (data->flags & MMC_DATA_WRITE)
			dma_sync_single_for_cpu(mci->hw_dev, (unsigned long)data->src,
						num_bytes, DMA_TO_DEVICE);
		else
			dma_sync_single_for_cpu(mci->hw_dev, (unsigned long)data->dest,
						num_bytes, DMA_FROM_DEVICE);
	}

	return 0;
}

static void tegra_sdmmc_set_clock(struct tegra_sdmmc_host *host, u32 clock)
{
	u32 prediv = 1, adjusted_clock = clock, val;

	while (adjusted_clock < 3200000) {
		prediv *= 2;
		adjusted_clock = clock * prediv * 2;
	}

	/* clear clock related bits */
	val = sdhci_read32(&host->sdhci, TEGRA_SDMMC_CLK_CNTL);
	val &= 0xffff0000;
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_CLK_CNTL, val);

	/* set new frequency */
	val |= prediv << 8;
	val |= TEGRA_SDMMC_CLK_CNTL_INTERNAL_CLOCK_EN;
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_CLK_CNTL, val);

	clk_set_rate(host->clk, adjusted_clock);

	/* wait for controller to settle */
	wait_on_timeout(10 * MSECOND,
			!(sdhci_read32(&host->sdhci, TEGRA_SDMMC_CLK_CNTL) &
			  TEGRA_SDMMC_CLK_INTERNAL_CLOCK_STABLE));

	/* enable card clock */
	val |= TEGRA_SDMMC_CLK_CNTL_SD_CLOCK_EN;
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_CLK_CNTL, val);
}

static void tegra_sdmmc_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct tegra_sdmmc_host *host = to_tegra_sdmmc_host(mci);
	u32 val;

	/* set clock */
	if (ios->clock)
		tegra_sdmmc_set_clock(host, ios->clock);

	/* set bus width */
	val = sdhci_read32(&host->sdhci, TEGRA_SDMMC_PWR_CNTL);
	val &= ~(0x21);

	if (ios->bus_width == MMC_BUS_WIDTH_8)
		val |= (1 << 5);
	else if (ios->bus_width == MMC_BUS_WIDTH_4)
		val |= (1 << 1);

	sdhci_write32(&host->sdhci, TEGRA_SDMMC_PWR_CNTL, val);
}

static int tegra_sdmmc_init(struct mci_host *mci, struct device *dev)
{
	struct tegra_sdmmc_host *host = to_tegra_sdmmc_host(mci);
	void __iomem *regs = host->regs;
	u32 val;
	int ret;

	/* reset controller */
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_CLK_CNTL,
		      TEGRA_SDMMC_CLK_CNTL_SW_RESET_FOR_ALL);

	ret = wait_on_timeout(100 * MSECOND,
			!(readl(regs + TEGRA_SDMMC_CLK_CNTL) &
			 TEGRA_SDMMC_CLK_CNTL_SW_RESET_FOR_ALL));
	if (ret) {
		dev_err(dev, "timeout while reset\n");
		return ret;
	}

	/* set power */
	val = readl(regs + TEGRA_SDMMC_PWR_CNTL);
	val &= ~(0xff << 8);
	val |= TEGRA_SDMMC_PWR_CNTL_33_V | TEGRA_SDMMC_PWR_CNTL_SD_BUS;
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_PWR_CNTL, val);

	/* sdmmc1 and sdmmc3 on T30 need a bit of padctrl init */
	if (of_device_is_compatible(mci->hw_dev->of_node, "nvidia,tegra30-sdhci") &&
	    (regs == IOMEM(0x78000000) || regs == IOMEM(0x78000400))) {
		val = readl(regs + TEGRA_SDMMC_SDMEMCOMPPADCTRL);
		val &= 0xfffffff0;
		val |= 0x7 << TEGRA_SDMMC_SDMEMCOMPPADCTRL_VREF_SEL_SHIFT;
		sdhci_write32(&host->sdhci, TEGRA_SDMMC_SDMEMCOMPPADCTRL, val);

		val = readl(regs + TEGRA_SDMMC_AUTO_CAL_CONFIG);
		val &= 0xffff0000;
		val |= (0x62 << TEGRA_SDMMC_AUTO_CAL_CONFIG_PU_OFFSET_SHIFT) |
		       (0x70 << TEGRA_SDMMC_AUTO_CAL_CONFIG_PD_OFFSET_SHIFT) |
		       TEGRA_SDMMC_AUTO_CAL_CONFIG_ENABLE;
		sdhci_write32(&host->sdhci, TEGRA_SDMMC_AUTO_CAL_CONFIG, val);
	}

	/* setup signaling */
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_INT_STAT_EN, 0xffffffff);
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_INT_SIG_EN, 0xffffffff);

	sdhci_write32(&host->sdhci, TEGRA_SDMMC_CLK_CNTL, 0xe << 16);

	val = readl(regs + TEGRA_SDMMC_INT_STAT_EN);
	val &= ~(0xffff);
	val |= (TEGRA_SDMMC_INT_STAT_EN_CMD_COMPLETE |
		TEGRA_SDMMC_INT_STAT_EN_XFER_COMPLETE |
		TEGRA_SDMMC_INT_STAT_EN_DMA_INTERRUPT |
		TEGRA_SDMMC_INT_STAT_EN_BUFFER_WRITE_READY |
		TEGRA_SDMMC_INT_STAT_EN_BUFFER_READ_READY);
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_INT_STAT_EN, val);

	val = readl(regs + TEGRA_SDMMC_INT_SIG_EN);
	val &= ~(0xffff);
	val |= TEGRA_SDMMC_INT_SIG_EN_XFER_COMPLETE;
	sdhci_write32(&host->sdhci, TEGRA_SDMMC_INT_SIG_EN, val);

	tegra_sdmmc_set_clock(host, 400000);

	return 0;
}

static int tegra_sdmmc_card_present(struct mci_host *mci)
{
	struct tegra_sdmmc_host *host = to_tegra_sdmmc_host(mci);
	int ret;

	if (gpio_is_valid(host->gpio_cd)) {
		ret = gpio_direction_input(host->gpio_cd);
		if (ret)
			return 0;
		return gpio_get_value(host->gpio_cd) ? 0 : 1;
	}

	return !(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) & SDHCI_WRITE_PROTECT);
}

static void tegra_sdmmc_parse_dt(struct tegra_sdmmc_host *host)
{
	struct device_node *np = host->mci.hw_dev->of_node;

	host->gpio_cd = of_get_named_gpio(np, "cd-gpios", 0);
	host->gpio_pwr = of_get_named_gpio(np, "power-gpios", 0);
	mci_of_parse(&host->mci);
}

static const struct mci_ops tegra_sdmmc_ops = {
	.init = tegra_sdmmc_init,
	.card_present = tegra_sdmmc_card_present,
	.set_ios = tegra_sdmmc_set_ios,
	.send_cmd = tegra_sdmmc_send_cmd,
};

static int tegra_sdmmc_probe(struct device *dev)
{
	struct resource *iores;
	struct tegra_sdmmc_host *host;
	struct mci_host *mci;
	int ret;

	host = xzalloc(sizeof(*host));
	mci = &host->mci;

	host->clk = clk_get(dev, NULL);
	if (IS_ERR(host->clk))
		return PTR_ERR(host->clk);

	host->reset = reset_control_get(dev, NULL);
	if (IS_ERR(host->reset))
		return PTR_ERR(host->reset);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get iomem region\n");
		return PTR_ERR(iores);
	}
	host->regs = IOMEM(iores->start);

	host->sdhci.read32 = tegra_sdmmc_read32;
	host->sdhci.write32 = tegra_sdmmc_write32;

	mci->hw_dev = dev;
	mci->f_max = 48000000;
	mci->f_min = 375000;
	tegra_sdmmc_parse_dt(host);

	if (gpio_is_valid(host->gpio_pwr)) {
		ret = gpio_request(host->gpio_pwr, "tegra_sdmmc_power");
		if (ret) {
			dev_err(dev, "failed to allocate power gpio\n");
			return -ENODEV;
		}
		gpio_direction_output(host->gpio_pwr, 1);
	}

	if (gpio_is_valid(host->gpio_cd)) {
		ret = gpio_request(host->gpio_cd, "tegra_sdmmc_cd");
		if (ret) {
			dev_err(dev, "failed to allocate cd gpio\n");
			return -ENODEV;
		}
		gpio_direction_input(host->gpio_cd);
	}

	clk_enable(host->clk);
	reset_control_assert(host->reset);
	udelay(2);
	reset_control_deassert(host->reset);

	mci->ops = tegra_sdmmc_ops;
	mci->voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	mci->host_caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED_52MHZ |
	                  MMC_CAP_SD_HIGHSPEED;

	return mci_register(&host->mci);
}

static __maybe_unused struct of_device_id tegra_sdmmc_compatible[] = {
	{
		.compatible = "nvidia,tegra124-sdhci",
	}, {
		.compatible = "nvidia,tegra30-sdhci",
	}, {
		.compatible = "nvidia,tegra20-sdhci",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, tegra_sdmmc_compatible);

static struct driver tegra_sdmmc_driver = {
	.name  = "tegra-sdmmc",
	.probe = tegra_sdmmc_probe,
	.of_compatible = DRV_OF_COMPAT(tegra_sdmmc_compatible),
};
device_platform_driver(tegra_sdmmc_driver);
