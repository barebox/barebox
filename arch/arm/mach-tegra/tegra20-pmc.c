/*
 * Copyright (C) 2013-2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Device driver for the Tegra 20 power management controller.
 */

#include <command.h>
#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <mach/tegra/lowlevel.h>
#include <mach/tegra/tegra-powergate.h>
#include <reset_source.h>

#include <mach/tegra/tegra20-pmc.h>

static void __iomem *pmc_base;
static int tegra_num_powerdomains;

/* main SoC reset trigger */
static void __noreturn tegra20_restart_soc(struct restart_handler *rst)
{
	writel(PMC_CNTRL_MAIN_RST, pmc_base + PMC_CNTRL);

	hang();
}

static int tegra_powergate_set(int id, bool new_state)
{
	bool status;

	status = readl(pmc_base + PMC_PWRGATE_STATUS) & (1 << id);

	if (status == new_state) {
		return 0;
	}

	writel(PMC_PWRGATE_TOGGLE_START | id, pmc_base + PMC_PWRGATE_TOGGLE);
	/* I don't know exactly why this is needed, seems to flush the write */
	readl(pmc_base + PMC_PWRGATE_TOGGLE);

	return 0;
}

int tegra_powergate_power_on(int id)
{
	if (id < 0 || id >= tegra_num_powerdomains)
		return -EINVAL;

	return tegra_powergate_set(id, true);
}

int tegra_powergate_power_off(int id)
{
	if (id < 0 || id >= tegra_num_powerdomains)
		return -EINVAL;

	return tegra_powergate_set(id, false);
}
EXPORT_SYMBOL(tegra_powergate_power_off);

int tegra_powergate_is_powered(int id)
{
	u32 status;

	if (id < 0 || id >= tegra_num_powerdomains)
		return -EINVAL;

	status = readl(pmc_base + PMC_PWRGATE_STATUS) & (1 << id);
	return !!status;
}

int tegra_powergate_remove_clamping(int id)
{
	u32 mask;

	if (id < 0 || id >= tegra_num_powerdomains)
		return -EINVAL;

	/*
	 * Tegra 2 has a bug where PCIE and VDE clamping masks are
	 * swapped relatively to the partition ids
	 */
	if (id == TEGRA_POWERGATE_VDEC)
		mask = (1 << TEGRA_POWERGATE_PCIE);
	else if (id == TEGRA_POWERGATE_PCIE)
		mask = (1 << TEGRA_POWERGATE_VDEC);
	else
		mask = (1 << id);

	writel(mask, pmc_base + PMC_REMOVE_CLAMPING_CMD);

	return 0;
}
EXPORT_SYMBOL(tegra_powergate_remove_clamping);

/* Must be called with clk disabled, and returns with clk enabled */
int tegra_powergate_sequence_power_up(int id, struct clk *clk,
					struct reset_control *rst)
{
	int ret;

	reset_control_assert(rst);

	ret = tegra_powergate_power_on(id);
	if (ret)
		goto err_power;

	ret = clk_enable(clk);
	if (ret)
		goto err_clk;

	udelay(10);

	ret = tegra_powergate_remove_clamping(id);
	if (ret)
		goto err_clamp;

	udelay(10);
	reset_control_deassert(rst);

	return 0;

err_clamp:
	clk_disable(clk);
err_clk:
	tegra_powergate_power_off(id);
err_power:
	return ret;
}
EXPORT_SYMBOL(tegra_powergate_sequence_power_up);

static int tegra_powergate_init(void)
{
	switch (tegra_get_chiptype()) {
	case TEGRA20:
		tegra_num_powerdomains = 7;
		break;
	case TEGRA30:
		tegra_num_powerdomains = 14;
		break;
	case TEGRA114:
		tegra_num_powerdomains = 23;
		break;
	case TEGRA124:
		tegra_num_powerdomains = 25;
		break;
	default:
		/* Unknown Tegra variant. Disable powergating */
		tegra_num_powerdomains = 0;
		break;
	}

	return 0;
}

static void tegra20_pmc_detect_reset_cause(void)
{
	u32 reg = readl(pmc_base + PMC_RST_STATUS);

	switch ((reg & PMC_RST_STATUS_RST_SRC_MASK) >>
	         PMC_RST_STATUS_RST_SRC_SHIFT) {
	case PMC_RST_STATUS_RST_SRC_POR:
		reset_source_set(RESET_POR);
		break;
	case PMC_RST_STATUS_RST_SRC_WATCHDOG:
		reset_source_set(RESET_WDG);
		break;
	case PMC_RST_STATUS_RST_SRC_LP0:
		reset_source_set(RESET_WKE);
		break;
	case PMC_RST_STATUS_RST_SRC_SW_MAIN:
		reset_source_set(RESET_RST);
		break;
	case PMC_RST_STATUS_RST_SRC_SENSOR:
		reset_source_set(RESET_THERM);
		break;
	default:
		reset_source_set(RESET_UKWN);
		break;
	}
}

static int tegra20_pmc_probe(struct device *dev)
{
	struct resource *iores;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	pmc_base = IOMEM(iores->start);

	tegra_powergate_init();

	if (IS_ENABLED(CONFIG_RESET_SOURCE))
		tegra20_pmc_detect_reset_cause();

	return 0;
}

static int do_tegrarcm(int argc, char *argv[])
{
	writel(2, pmc_base + PMC_SCRATCH(0));
	restart_machine();

	return 0;
}

static __maybe_unused struct of_device_id tegra20_pmc_dt_ids[] = {
	{
		.compatible = "nvidia,tegra20-pmc",
	}, {
		.compatible = "nvidia,tegra30-pmc",
	}, {
		.compatible = "nvidia,tegra124-pmc",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, tegra20_pmc_dt_ids);

static struct driver tegra20_pmc_driver = {
	.probe	= tegra20_pmc_probe,
	.name	= "tegra20-pmc",
	.of_compatible = DRV_OF_COMPAT(tegra20_pmc_dt_ids),
};

static int tegra20_pmc_init(void)
{
	restart_handler_register_fn("soc", tegra20_restart_soc);
	return platform_driver_register(&tegra20_pmc_driver);
}
coredevice_initcall(tegra20_pmc_init);

BAREBOX_CMD_HELP_START(tegrarcm)
BAREBOX_CMD_HELP_TEXT("Get into recovery mode without using a physical switch\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(tegrarcm)
	.cmd		= do_tegrarcm,
	BAREBOX_CMD_DESC("Usage: tegrarcm")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_tegrarcm_help)
BAREBOX_CMD_END
