/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
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

#include <linux/compiler.h>
#include "mach/tegra20-car.h"
#include "mach/lowlevel.h"

static __always_inline
void tegra_dvc_init(void)
{
	int div;
	u32 reg;

	/* reset DVC controller and enable clock */
	writel(CRC_RST_DEV_H_DVC, TEGRA_CLK_RESET_BASE + CRC_RST_DEV_H_SET);
	reg = readl(TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_H);
	reg |= CRC_CLK_OUT_ENB_H_DVC;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_H);

	/* set DVC I2C clock source to CLK_M and aim for 100kHz I2C clock */
	div = ((tegra_get_osc_clock() * 3) >> 22) - 1;
	writel((div) | (3 << 30),
	       TEGRA_CLK_RESET_BASE + CRC_CLK_SOURCE_DVC);

	/* clear DVC reset */
	tegra_ll_delay_usec(3);
	writel(CRC_RST_DEV_H_DVC, TEGRA_CLK_RESET_BASE + CRC_RST_DEV_H_CLR);
}

#define TEGRA_I2C_CNFG		0x00
#define TEGRA_I2C_CMD_ADDR0	0x04
#define TEGRA_I2C_CMD_DATA1	0x0c
#define TEGRA_I2C_SEND_2_BYTES	0x0a02

static __always_inline
void tegra_dvc_write_addr(u32 addr, u32 config)
{
	writel(addr, TEGRA_DVC_BASE + TEGRA_I2C_CMD_ADDR0);
	writel(config, TEGRA_DVC_BASE + TEGRA_I2C_CNFG);
}

static __always_inline
void tegra_dvc_write_data(u32 data, u32 config)
{
	writel(data, TEGRA_DVC_BASE + TEGRA_I2C_CMD_DATA1);
	writel(config, TEGRA_DVC_BASE + TEGRA_I2C_CNFG);
}

static inline __attribute__((always_inline))
void tegra30_tps65911_cpu_rail_enable(void)
{
	tegra_dvc_write_addr(0x5a, 2);
	/* reg 28, 600mV + (35-3) * 12,5mV = 1,0V */
	tegra_dvc_write_data(0x2328, TEGRA_I2C_SEND_2_BYTES);
	tegra_ll_delay_usec(1000);
	/* reg 27, VDDctrl enable */
	tegra_dvc_write_data(0x0127, TEGRA_I2C_SEND_2_BYTES);
	tegra_ll_delay_usec(10 * 1000);
}
