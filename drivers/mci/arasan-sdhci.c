// SPDX-License-Identifier: GPL-2.0-or-later

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/clk.h>
#include <mci.h>
#include <mach/zynqmp/firmware-zynqmp.h>

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

#define ZYNQMP_CLK_PHASES			10
#define ZYNQMP_CLK_PHASE_UHS_SDR104		6
#define ZYNQMP_CLK_PHASE_HS200			9
/* Default settings for ZynqMP Clock Phases */
#define ZYNQMP_ICLK_PHASE {0, 63, 63, 0, 63,  0,   0, 183, 54,  0, 0}
#define ZYNQMP_OCLK_PHASE {0, 72, 60, 0, 60, 72, 135, 48, 72, 135, 0}

/**
 * struct sdhci_arasan_clk_data - Arasan Controller Clock Data.
 *
 * @sdcardclk_hw:	Struct for the clock we might provide to a PHY.
 * @sdcardclk:		Pointer to normal 'struct clock' for sdcardclk_hw.
 * @sampleclk_hw:	Struct for the clock we might provide to a PHY.
 * @sampleclk:		Pointer to normal 'struct clock' for sampleclk_hw.
 * @clk_phase_in:	Array of Input Clock Phase Delays for all speed modes
 * @clk_phase_out:	Array of Output Clock Phase Delays for all speed modes
 * @set_clk_delays:	Function pointer for setting Clock Delays
 * @clk_of_data:	Platform specific runtime clock data storage pointer
 */
struct sdhci_arasan_clk_data {
	struct clk_hw	sdcardclk_hw;
	struct clk      *sdcardclk;
	struct clk_hw	sampleclk_hw;
	struct clk      *sampleclk;
	int		clk_phase_in[ZYNQMP_CLK_PHASES + 1];
	int		clk_phase_out[ZYNQMP_CLK_PHASES + 1];
	void		(*set_clk_delays)(struct sdhci *host);
	void		*clk_of_data;
};

struct arasan_sdhci_host {
	struct mci_host		mci;
	struct sdhci		sdhci;
	unsigned int		quirks; /* Arasan deviations from spec */
	const struct clk_ops	*sdcardclk_ops;
	const struct clk_ops	*sampleclk_ops;
	struct sdhci_arasan_clk_data clk_data;
/* Controller does not have CD wired and will not function normally without */
#define SDHCI_ARASAN_QUIRK_FORCE_CDTEST		BIT(0)
#define SDHCI_ARASAN_QUIRK_NO_1_8_V		BIT(1)
/*
 * Some of the Arasan variations might not have timing requirements
 * met at 25MHz for Default Speed mode, those controllers work at
 * 19MHz instead
 */
#define SDHCI_ARASAN_QUIRK_CLOCK_25_BROKEN	BIT(2)
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
		sdhci_write8(&host->sdhci, SDHCI_HOST_CONTROL, ctrl);
	}

	return 0;
}

static void arasan_zynqmp_dll_reset(struct arasan_sdhci_host *host, u32 deviceid)
{
	u16 clk;

	clk = sdhci_read16(&host->sdhci, SDHCI_CLOCK_CONTROL);
	clk &= ~(SDHCI_CLOCK_CARD_EN | SDHCI_CLOCK_INT_EN);
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, clk);

	/* Issue DLL Reset */
	zynqmp_pm_sd_dll_reset(deviceid, PM_DLL_RESET_PULSE);

	clk = sdhci_read16(&host->sdhci, SDHCI_CLOCK_CONTROL);

	sdhci_enable_clk(&host->sdhci, clk);
}

static int arasan_zynqmp_execute_tuning(struct mci_host *mci, u32 opcode)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	struct clk_hw *hw = &host->clk_data.sdcardclk_hw;
	const char *clk_name = clk_hw_get_name(hw);
	u32 device_id = !strcmp(clk_name, "clk_out_sd0") ? NODE_SD_0 :
							   NODE_SD_1;
	int err;

	/* ZynqMP SD controller does not perform auto tuning in DDR50 mode */
	if (mci->timing == MMC_TIMING_UHS_DDR50)
		return 0;

	arasan_zynqmp_dll_reset(host, device_id);

	err = sdhci_execute_tuning(&host->sdhci, opcode);
	if (err)
		return err;

	arasan_zynqmp_dll_reset(host, device_id);

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

static void arasan_sdhci_set_clock(struct mci_host *mci, unsigned int clock)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	struct sdhci_arasan_clk_data *clk_data = &host->clk_data;

	if (host->quirks & SDHCI_ARASAN_QUIRK_CLOCK_25_BROKEN) {
		/*
		 * Some of the Arasan variations might not have timing
		 * requirements met at 25MHz for Default Speed mode,
		 * those controllers work at 19MHz instead.
		 */
		if (clock == 25000000)
			clock = (25000000 * 19) / 25;
	}

	clk_set_phase(clk_data->sampleclk,
		      clk_data->clk_phase_in[mci->mci->host->timing]);
	clk_set_phase(clk_data->sdcardclk,
		      clk_data->clk_phase_out[mci->mci->host->timing]);

	sdhci_set_clock(&host->sdhci, clock, host->sdhci.max_clk);
}

static void arasan_sdhci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct arasan_sdhci_host *host = to_arasan_sdhci_host(mci);
	u16 val;

	if (ios->clock)
		arasan_sdhci_set_clock(mci, ios->clock);

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
	dma_addr_t dma;
	int ret;

	ret = sdhci_wait_idle(&host->sdhci, cmd, data);
	if (ret)
		return ret;

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	mask = SDHCI_INT_CMD_COMPLETE;
	if (cmd->resp_type & MMC_RSP_BUSY)
		mask |= SDHCI_INT_XFER_COMPLETE;

	sdhci_setup_data_dma(&host->sdhci, data, &dma);

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data,
				dma == SDHCI_NO_DMA ? false : true,
				&command, &xfer);

	sdhci_write8(&host->sdhci, SDHCI_TIMEOUT_CONTROL, TIMEOUT_VAL);
	if (xfer)
		sdhci_write16(&host->sdhci, SDHCI_TRANSFER_MODE, xfer);
	if (data) {
		sdhci_write16(&host->sdhci, SDHCI_BLOCK_SIZE,
			      SDHCI_DMA_BOUNDARY_512K | SDHCI_TRANSFER_BLOCK_SIZE(data->blocksize));
		sdhci_write16(&host->sdhci, SDHCI_BLOCK_COUNT, data->blocks);
	}
	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write16(&host->sdhci, SDHCI_COMMAND, command);

	/* CMD19/21 generate _only_ Buffer Read Ready interrupt */
	if (cmd->cmdidx == MMC_SEND_TUNING_BLOCK || cmd->cmdidx == MMC_SEND_TUNING_BLOCK_HS200)
		mask = SDHCI_INT_DATA_AVAIL;

	ret = sdhci_wait_for_done(&host->sdhci, mask);
	if (ret)
		goto error;

	sdhci_read_response(&host->sdhci, cmd);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, SDHCI_INT_CMD_COMPLETE);

	if (data)
		ret = sdhci_transfer_data_dma(&host->sdhci, data, dma);

error:
	if (ret) {
		print_error(host, cmd->cmdidx, ret);
		arasan_sdhci_reset(host, SDHCI_RESET_CMD);
		arasan_sdhci_reset(host, SDHCI_RESET_DATA);
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	return ret;
}

static void sdhci_arasan_set_clk_delays(struct sdhci *host)
{
	struct arasan_sdhci_host *arasan_sdhci = sdhci_to_arasan(host);
	struct mci_host *mci = &arasan_sdhci->mci;
	struct sdhci_arasan_clk_data *clk_data = &arasan_sdhci->clk_data;

	clk_set_phase(clk_data->sampleclk,
		      clk_data->clk_phase_in[mci->timing]);
	clk_set_phase(clk_data->sdcardclk,
		      clk_data->clk_phase_out[mci->timing]);
}

static void arasan_dt_read_clk_phase(struct device *dev,
				     struct sdhci_arasan_clk_data *clk_data,
				     unsigned int timing, const char *prop)
{
	struct device_node *np = dev->of_node;

	u32 clk_phase[2] = {0};
	int ret;

	/*
	 * Read Tap Delay values from DT, if the DT does not contain the
	 * Tap Values then use the pre-defined values.
	 */
	ret = of_property_read_u32_array(np, prop, &clk_phase[0], 2);
	if (ret < 0) {
		dev_dbg(dev, "Using predefined clock phase for %s = %d %d\n",
			prop, clk_data->clk_phase_in[timing],
			clk_data->clk_phase_out[timing]);
		return;
	}

	/* The values read are Input and Output Clock Delays in order */
	clk_data->clk_phase_in[timing] = clk_phase[0];
	clk_data->clk_phase_out[timing] = clk_phase[1];
}

/**
 * sdhci_zynqmp_sampleclk_set_phase - Set the SD Input Clock Tap Delays
 *
 * @hw:			Pointer to the hardware clock structure.
 * @degrees:		The clock phase shift between 0 - 359.
 *
 * Set the SD Input Clock Tap Delays for Input path
 *
 * Return: 0 on success and error value on error
 */
static int arasan_zynqmp_sampleclk_set_phase(struct clk_hw *hw, int degrees)
{
	struct sdhci_arasan_clk_data *clk_data =
		container_of(hw, struct sdhci_arasan_clk_data, sampleclk_hw);
	struct arasan_sdhci_host *sdhci_arasan =
		container_of(clk_data, struct arasan_sdhci_host, clk_data);
	struct mci_host *host = &sdhci_arasan->mci;
	const char *clk_name = clk_hw_get_name(hw);
	u32 node_id = !strcmp(clk_name, "clk_in_sd0") ? NODE_SD_0 : NODE_SD_1;
	u8 tap_delay, tap_max = 0;
	int ret;

	/* This is applicable for SDHCI_SPEC_300 and above */
	if (sdhci_arasan->sdhci.version < SDHCI_SPEC_300)
		return 0;

	/* Assert DLL Reset */
	zynqmp_pm_sd_dll_reset(node_id, PM_DLL_RESET_ASSERT);

	switch (host->timing) {
	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_DDR52:
		/* For 50MHz clock, 120 Taps are available */
		tap_max = 120;
		break;
	case MMC_TIMING_UHS_SDR50:
		/* For 100MHz clock, 60 Taps are available */
		tap_max = 60;
		break;
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_MMC_HS200:
		/* For 200MHz clock, 30 Taps are available */
		tap_max = 30;
		break;
	default:
		break;
	}

	tap_delay = (degrees * tap_max) / 360;

	/* Set the Clock Phase */
	ret = zynqmp_pm_set_sd_tapdelay(node_id, PM_TAPDELAY_INPUT, tap_delay);
	if (ret)
		pr_err("Error setting Input Tap Delay\n");

	return ret;
}

static unsigned long arasan_sampleclk_recalc_rate(struct clk_hw *hw,
						  unsigned long parent_rate)
{
	struct sdhci_arasan_clk_data *clk_data =
		container_of(hw, struct sdhci_arasan_clk_data, sdcardclk_hw);
	struct arasan_sdhci_host *sdhci_arasan =
		container_of(clk_data, struct arasan_sdhci_host, clk_data);
	struct mci_host *host = &sdhci_arasan->mci;

	return host->actual_clock;
};

/**
 * sdhci_zynqmp_sdcardclk_set_phase - Set the SD Output Clock Tap Delays
 *
 * @hw:			Pointer to the hardware clock structure.
 * @degrees:		The clock phase shift between 0 - 359.
 *
 * Set the SD Output Clock Tap Delays for Output path
 *
 * Return: 0 on success and error value on error
 */
static int arasan_zynqmp_sdcardclk_set_phase(struct clk_hw *hw, int degrees)
{
	struct sdhci_arasan_clk_data *clk_data =
		container_of(hw, struct sdhci_arasan_clk_data, sdcardclk_hw);
	struct arasan_sdhci_host *sdhci_arasan =
		container_of(clk_data, struct arasan_sdhci_host, clk_data);
	struct mci_host *host = &sdhci_arasan->mci;
	const char *clk_name = clk_hw_get_name(hw);
	u32 node_id = !strcmp(clk_name, "clk_out_sd0") ? NODE_SD_0 : NODE_SD_1;
	u8 tap_delay, tap_max = 0;
	int ret;

	/* This is applicable for SDHCI_SPEC_300 and above */
	if (sdhci_arasan->sdhci.version < SDHCI_SPEC_300)
		return 0;

	switch (host->timing) {
	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_DDR52:
		/* For 50MHz clock, 30 Taps are available */
		tap_max = 30;
		break;
	case MMC_TIMING_UHS_SDR50:
		/* For 100MHz clock, 15 Taps are available */
		tap_max = 15;
		break;
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_MMC_HS200:
		/* For 200MHz clock, 8 Taps are available */
		tap_max = 8;
		break;
	default:
		break;
	}

	tap_delay = (degrees * tap_max) / 360;

	/* Set the Clock Phase */
	ret = zynqmp_pm_set_sd_tapdelay(node_id, PM_TAPDELAY_OUTPUT, tap_delay);
	if (ret)
		pr_err("Error setting Output Tap Delay\n");

	/* Release DLL Reset */
	zynqmp_pm_sd_dll_reset(node_id, PM_DLL_RESET_RELEASE);

	return ret;
}

static unsigned long arasan_sdcardclk_recalc_rate(struct clk_hw *hw,
						  unsigned long parent_rate)
{
	struct sdhci_arasan_clk_data *clk_data =
		container_of(hw, struct sdhci_arasan_clk_data, sdcardclk_hw);
	struct arasan_sdhci_host *sdhci_arasan =
		container_of(clk_data, struct arasan_sdhci_host, clk_data);
	struct mci_host *host = &sdhci_arasan->mci;

	return host->actual_clock;
};

static const struct clk_ops arasan_sampleclk_ops = {
	.recalc_rate = arasan_sampleclk_recalc_rate,
};

static const struct clk_ops arasan_sdcardclk_ops = {
	.recalc_rate = arasan_sdcardclk_recalc_rate,
};

static const struct clk_ops zynqmp_sampleclk_ops = {
	.recalc_rate = arasan_sampleclk_recalc_rate,
	.set_phase = arasan_zynqmp_sampleclk_set_phase,
};

static const struct clk_ops zynqmp_sdcardclk_ops = {
	.recalc_rate = arasan_sdcardclk_recalc_rate,
	.set_phase = arasan_zynqmp_sdcardclk_set_phase,
};

/**
 * arasan_sdhci_register_sampleclk - Register the sampleclk for a PHY to use
 *
 * @sdhci_arasan:	Our private data structure.
 * @clk_xin:		Pointer to the functional clock
 * @dev:		Pointer to our struct device.
 *
 * Some PHY devices need to know what the actual card clock is.  In order for
 * them to find out, we'll provide a clock through the common clock framework
 * for them to query.
 *
 * Return: 0 on success and error value on error
 */
static int
arasan_sdhci_register_sampleclk(struct arasan_sdhci_host *sdhci_arasan,
				struct clk *clk_xin,
				struct device *dev)
{
	struct sdhci_arasan_clk_data *clk_data = &sdhci_arasan->clk_data;
	struct device_node *np = dev->of_node;
	struct clk_init_data sampleclk_init = {};
	const char *clk_name;
	int ret;

	ret = of_property_read_string_index(np, "clock-output-names", 1,
					    &sampleclk_init.name);
	if (ret) {
		dev_err(dev, "DT has #clock-cells but no clock-output-names\n");
		return ret;
	}

	clk_name = __clk_get_name(clk_xin);
	sampleclk_init.parent_names = &clk_name;
	sampleclk_init.num_parents = 1;
	sampleclk_init.ops = sdhci_arasan->sampleclk_ops;

	clk_data->sampleclk_hw.init = &sampleclk_init;
	clk_data->sampleclk = clk_register(dev, &clk_data->sampleclk_hw);
	if (IS_ERR(clk_data->sampleclk))
		return PTR_ERR(clk_data->sampleclk);
	clk_data->sampleclk_hw.init = NULL;

	ret = of_clk_add_provider(np, of_clk_src_simple_get,
				  clk_data->sampleclk);
	if (ret)
		dev_err(dev, "Failed to add sample clock provider\n");

	return ret;
}

/**
 * sdhci_arasan_register_sdcardclk - Register the sdcardclk for a PHY to use
 *
 * @sdhci_arasan:	Our private data structure.
 * @clk_xin:		Pointer to the functional clock
 * @dev:		Pointer to our struct device.
 *
 * Some PHY devices need to know what the actual card clock is.  In order for
 * them to find out, we'll provide a clock through the common clock framework
 * for them to query.
 *
 * Return: 0 on success and error value on error
 */
static int
arasan_sdhci_register_sdcardclk(struct arasan_sdhci_host *sdhci_arasan,
				struct clk *clk_xin,
				struct device *dev)
{
	struct sdhci_arasan_clk_data *clk_data = &sdhci_arasan->clk_data;
	struct device_node *np = dev->of_node;
	struct clk_init_data sdcardclk_init = {};
	const char *clk_name;
	int ret;

	ret = of_property_read_string_index(np, "clock-output-names", 0,
					    &sdcardclk_init.name);
	if (ret) {
		dev_err(dev, "DT has #clock-cells but no clock-output-names\n");
		return ret;
	}

	clk_name = __clk_get_name(clk_xin);
	sdcardclk_init.parent_names = &clk_name;
	sdcardclk_init.ops = sdhci_arasan->sdcardclk_ops;
	sdcardclk_init.num_parents = 1;

	clk_data->sdcardclk_hw.init = &sdcardclk_init;
	clk_data->sdcardclk = clk_register(dev, &clk_data->sdcardclk_hw);
	if (IS_ERR(clk_data->sdcardclk))
		return PTR_ERR(clk_data->sdcardclk);
	clk_data->sdcardclk_hw.init = NULL;

	ret = of_clk_add_provider(np, of_clk_src_simple_get,
				  clk_data->sdcardclk);
	if (ret)
		dev_err(dev, "Failed to add sdcard clock provider\n");

	return ret;
}

/**
 * arasan_sdhci_register_sdclk - Register the sdcardclk for a PHY to use
 *
 * @sdhci_arasan:	Our private data structure.
 * @clk_xin:		Pointer to the functional clock
 * @dev:		Pointer to our struct device.
 *
 * Some PHY devices need to know what the actual card clock is.  In order for
 * them to find out, we'll provide a clock through the common clock framework
 * for them to query.
 *
 * Note: without seriously re-architecting SDHCI's clock code and testing on
 * all platforms, there's no way to create a totally beautiful clock here
 * with all clock ops implemented.  Instead, we'll just create a clock that can
 * be queried and set the CLK_GET_RATE_NOCACHE attribute to tell common clock
 * framework that we're doing things behind its back.  This should be sufficient
 * to create nice clean device tree bindings and later (if needed) we can try
 * re-architecting SDHCI if we see some benefit to it.
 *
 * Return: 0 on success and error value on error
 */
static int arasan_sdhci_register_sdclk(struct arasan_sdhci_host *sdhci_arasan,
				       struct clk *clk_xin,
				       struct device *dev)
{
	struct device_node *np = dev->of_node;
	u32 num_clks = 0;
	int ret;

	/* Providing a clock to the PHY is optional; no error if missing */
	if (of_property_read_u32(np, "#clock-cells", &num_clks) < 0)
		return 0;

	ret = arasan_sdhci_register_sdcardclk(sdhci_arasan, clk_xin, dev);
	if (ret)
		return ret;

	if (num_clks)
		return arasan_sdhci_register_sampleclk(sdhci_arasan, clk_xin,
						       dev);

	return 0;
}

/**
 * arasan_dt_parse_clk_phases - Read Clock Delay values from DT
 *
 * @dev:		Pointer to our struct device.
 * @clk_data:		Pointer to the Clock Data structure
 *
 * Called at initialization to parse the values of Clock Delays.
 */
static void arasan_dt_parse_clk_phases(struct device *dev,
				       struct sdhci_arasan_clk_data *clk_data)
{
	u32 mio_bank = 0;
	int i;

	/*
	 * This has been kept as a pointer and is assigned a function here.
	 * So that different controller variants can assign their own handling
	 * function.
	 */
	clk_data->set_clk_delays = sdhci_arasan_set_clk_delays;

	if (of_device_is_compatible(dev->of_node, "xlnx,zynqmp-8.9a")) {
		u32 zynqmp_iclk_phase[ZYNQMP_CLK_PHASES + 1] = ZYNQMP_ICLK_PHASE;
		u32 zynqmp_oclk_phase[ZYNQMP_CLK_PHASES + 1] = ZYNQMP_OCLK_PHASE;

		of_property_read_u32(dev->of_node, "xlnx,mio-bank", &mio_bank);
		if (mio_bank == 2) {
			zynqmp_oclk_phase[ZYNQMP_CLK_PHASE_UHS_SDR104] = 90;
			zynqmp_oclk_phase[ZYNQMP_CLK_PHASE_HS200] = 90;
		}

		for (i = 0; i <= ZYNQMP_CLK_PHASES; i++) {
			clk_data->clk_phase_in[i] = zynqmp_iclk_phase[i];
			clk_data->clk_phase_out[i] = zynqmp_oclk_phase[i];
		}
	}

	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_LEGACY,
				 "clk-phase-legacy");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_MMC_HS,
				 "clk-phase-mmc-hs");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_SD_HS,
				 "clk-phase-sd-hs");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_UHS_SDR12,
				 "clk-phase-uhs-sdr12");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_UHS_SDR25,
				 "clk-phase-uhs-sdr25");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_UHS_SDR50,
				 "clk-phase-uhs-sdr50");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_UHS_SDR104,
				 "clk-phase-uhs-sdr104");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_UHS_DDR50,
				 "clk-phase-uhs-ddr50");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_MMC_DDR52,
				 "clk-phase-mmc-ddr52");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_MMC_HS200,
				 "clk-phase-mmc-hs200");
	arasan_dt_read_clk_phase(dev, clk_data, MMC_TIMING_MMC_HS400,
				 "clk-phase-mmc-hs400");
}

static const struct mci_ops arasan_sdhci_ops = {
	.send_cmd = arasan_sdhci_send_cmd,
	.set_ios = arasan_sdhci_set_ios,
	.init = arasan_sdhci_init,
	.card_present = arasan_sdhci_card_present,
	.card_write_protected = arasan_sdhci_card_write_protected,
};

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

	arasan_sdhci->sdhci.base = IOMEM(iores->start);
	arasan_sdhci->sdhci.mci = mci;
	mci->ops = arasan_sdhci_ops;
	mci->hw_dev = dev;

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

	if (of_device_is_compatible(np, "xlnx,zynqmp-8.9a")) {
		if (IS_ENABLED(CONFIG_MCI_TUNING))
			mci->ops.execute_tuning = arasan_zynqmp_execute_tuning;
		arasan_sdhci->quirks |= SDHCI_ARASAN_QUIRK_CLOCK_25_BROKEN;
	}

	/*
	 * clk_rates on ZynqMP are rounded wrong. For HS200 clk_get_rate retunrs
	 * 199999998 instead of 200000000
	 */
	if (of_device_is_compatible(np, "xlnx,zynqmp-8.9a"))
		mci->f_max = 200000000;
	else
		mci->f_max = clk_get_rate(clk_xin);

	mci->f_min = 50000000 / 256;

	if (of_device_is_compatible(np, "xlnx,zynqmp-8.9a")) {
		arasan_sdhci->sdcardclk_ops = &zynqmp_sdcardclk_ops;
		arasan_sdhci->sampleclk_ops = &zynqmp_sampleclk_ops;
	} else {
		arasan_sdhci->sdcardclk_ops = &arasan_sdcardclk_ops;
		arasan_sdhci->sampleclk_ops = &arasan_sampleclk_ops;
	}

	arasan_sdhci_register_sdclk(arasan_sdhci, clk_xin, dev);

	arasan_dt_parse_clk_phases(dev, &arasan_sdhci->clk_data);

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
