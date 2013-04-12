/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
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
 * @brief Device driver for the Tegra 20 clock and reset (CAR) controller
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/iomap.h>

#include <mach/tegra20-car.h>

static void __iomem *car_base;

enum tegra20_clks {
	cpu, ac97 = 3, rtc, timer, uarta, gpio = 8, sdmmc2, i2s1 = 11, i2c1,
	ndflash, sdmmc1, sdmmc4, twc, pwm, i2s2, epp, gr2d = 21, usbd, isp,
	gr3d, ide, disp2, disp1, host1x, vcp, cache2 = 31, mem, ahbdma, apbdma,
	kbc = 36, stat_mon, pmc, fuse, kfuse, sbc1, nor, spi, sbc2, xio, sbc3,
	dvc, dsi, mipi = 50, hdmi, csi, tvdac, i2c2, uartc, emc = 57, usb2,
	usb3, mpe, vde, bsea, bsev, speedo, uartd, uarte, i2c3, sbc4, sdmmc3,
	pex, owr, afi, csite, pcie_xclk, avpucq = 75, la, irama = 84, iramb,
	iramc, iramd, cram2, audio_2x, clk_d, csus = 92, cdev1, cdev2,
	uartb = 96, vfir, spdif_in, spdif_out, vi, vi_sensor, tvo, cve,
	osc, clk_32k, clk_m, sclk, cclk, hclk, pclk, blink, pll_a, pll_a_out0,
	pll_c, pll_c_out1, pll_d, pll_d_out0, pll_e, pll_m, pll_m_out1,
	pll_p, pll_p_out1, pll_p_out2, pll_p_out3, pll_p_out4, pll_u,
	pll_x, audio, pll_ref, twd, clk_max,
};

static struct clk *clks[clk_max];

static unsigned long get_osc_frequency(void)
{
	u32 osc_ctrl = readl(car_base + CRC_OSC_CTRL);

	switch ((osc_ctrl & CRC_OSC_CTRL_OSC_FREQ_MASK) >>
		CRC_OSC_CTRL_OSC_FREQ_SHIFT) {
	case 0:
		return 13000000;
	case 1:
		return 19200000;
	case 2:
		return 12000000;
	case 3:
		return 26000000;
	default:
		return 0;
	}
}

static unsigned int get_pll_ref_div(void)
{
	u32 osc_ctrl = readl(car_base + CRC_OSC_CTRL);

	return 1U << ((osc_ctrl & CRC_OSC_CTRL_PLL_REF_DIV_MASK) >>
		      CRC_OSC_CTRL_PLL_REF_DIV_SHIFT);
}

static int tegra20_car_probe(struct device_d *dev)
{
	car_base = dev_request_mem_region(dev, 0);
	if (!car_base)
		return -EBUSY;

	/* primary clocks */
	clks[clk_m] = clk_fixed("clk_m", get_osc_frequency());
	clks[clk_32k] = clk_fixed("clk_32k", 32768);

	clks[pll_ref] = clk_fixed_factor("pll_ref", "clk_m", 1,
					 get_pll_ref_div());

	/* derived clocks */
	/* timer is a gate, but as it's enabled by BOOTROM we needn't worry */
	clks[timer] = clk_fixed_factor("timer", "clk_m", 1, 1);

	return 0;
}

static __maybe_unused struct of_device_id tegra20_car_dt_ids[] = {
	{
		.compatible = "nvidia,tegra20-car",
	}, {
		/* sentinel */
	}
};

static struct driver_d tegra20_car_driver = {
	.probe	= tegra20_car_probe,
	.name	= "tegra20-car",
	.of_compatible = DRV_OF_COMPAT(tegra20_car_dt_ids),
};

static int tegra20_car_init(void)
{
	return platform_driver_register(&tegra20_car_driver);
}
postcore_initcall(tegra20_car_init);
