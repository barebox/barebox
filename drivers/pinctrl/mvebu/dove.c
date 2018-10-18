/*
 * Marvell Dove pinctrl driver based on mvebu pinctrl core
 *
 * Author: Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <malloc.h>
#include <of.h>
#include <of_address.h>
#include <linux/sizes.h>

#include "common.h"

/* Internal registers can be configured at any 1 MiB aligned address */
#define INT_REGS_MASK		(SZ_1M - 1)
#define PMU_REGS_OFFS		0xd802c

/* MPP Base registers */
#define PMU_MPP_GENERAL_CTRL	0x10
#define  AU0_AC97_SEL		BIT(16)

/* MPP Control 4 register */
#define SPI_GPIO_SEL		BIT(5)
#define UART1_GPIO_SEL		BIT(4)
#define AU1_GPIO_SEL		BIT(3)
#define CAM_GPIO_SEL		BIT(2)
#define SD1_GPIO_SEL		BIT(1)
#define SD0_GPIO_SEL		BIT(0)

/* PMU Signal Select registers */
#define PMU_SIGNAL_SELECT_0	0x00
#define PMU_SIGNAL_SELECT_1	0x04

/* Global Config regmap registers */
#define GLOBAL_CONFIG_1		0x00
#define  TWSI_ENABLE_OPTION1	BIT(7)
#define GLOBAL_CONFIG_2		0x04
#define  TWSI_ENABLE_OPTION2	BIT(20)
#define  TWSI_ENABLE_OPTION3	BIT(21)
#define  TWSI_OPTION3_GPIO	BIT(22)
#define SSP_CTRL_STATUS_1	0x08
#define  SSP_ON_AU1		BIT(0)
#define MPP_GENERAL_CONFIG	0x10
#define  AU1_SPDIFO_GPIO_EN	BIT(1)
#define  NAND_GPIO_EN		BIT(0)

#define CONFIG_PMU	BIT(4)

static void __iomem *mpp_base;
static void __iomem *mpp4_base;
static void __iomem *pmu_base;
static void __iomem *gconf_base;

static int dove_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base, pid, config);
}

static int dove_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(mpp_base, pid, config);
}

static int dove_pmu_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	unsigned off = (pid / MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;
	unsigned shift = (pid % MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;
	unsigned long pmu = readl(mpp_base + PMU_MPP_GENERAL_CTRL);
	unsigned long func;

	if ((pmu & BIT(pid)) == 0)
		return default_mpp_ctrl_get(mpp_base, pid, config);

	func = readl(pmu_base + PMU_SIGNAL_SELECT_0 + off);
	*config = (func >> shift) & MVEBU_MPP_MASK;
	*config |= CONFIG_PMU;

	return 0;
}

static int dove_pmu_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	unsigned off = (pid / MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;
	unsigned shift = (pid % MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;
	unsigned long pmu = readl(mpp_base + PMU_MPP_GENERAL_CTRL);
	unsigned long func;

	if ((config & CONFIG_PMU) == 0) {
		writel(pmu & ~BIT(pid), mpp_base + PMU_MPP_GENERAL_CTRL);
		return default_mpp_ctrl_set(mpp_base, pid, config);
	}

	writel(pmu | BIT(pid), mpp_base + PMU_MPP_GENERAL_CTRL);
	func = readl(pmu_base + PMU_SIGNAL_SELECT_0 + off);
	func &= ~(MVEBU_MPP_MASK << shift);
	func |= (config & MVEBU_MPP_MASK) << shift;
	writel(func, pmu_base + PMU_SIGNAL_SELECT_0 + off);

	return 0;
}

static int dove_mpp4_ctrl_get(unsigned pid, unsigned long *config)
{
	unsigned long mpp4 = readl(mpp4_base);
	unsigned long mask;

	switch (pid) {
	case 24: /* mpp_camera */
		mask = CAM_GPIO_SEL;
		break;
	case 40: /* mpp_sdio0 */
		mask = SD0_GPIO_SEL;
		break;
	case 46: /* mpp_sdio1 */
		mask = SD1_GPIO_SEL;
		break;
	case 58: /* mpp_spi0 */
		mask = SPI_GPIO_SEL;
		break;
	case 62: /* mpp_uart1 */
		mask = UART1_GPIO_SEL;
		break;
	default:
		return -EINVAL;
	}

	*config = ((mpp4 & mask) != 0);

	return 0;
}

static int dove_mpp4_ctrl_set(unsigned pid, unsigned long config)
{
	unsigned long mpp4 = readl(mpp4_base);
	unsigned long mask;

	switch (pid) {
	case 24: /* mpp_camera */
		mask = CAM_GPIO_SEL;
		break;
	case 40: /* mpp_sdio0 */
		mask = SD0_GPIO_SEL;
		break;
	case 46: /* mpp_sdio1 */
		mask = SD1_GPIO_SEL;
		break;
	case 58: /* mpp_spi0 */
		mask = SPI_GPIO_SEL;
		break;
	case 62: /* mpp_uart1 */
		mask = UART1_GPIO_SEL;
		break;
	default:
		return -EINVAL;
	}

	mpp4 &= ~mask;
	if (config)
		mpp4 |= mask;

	writel(mpp4, mpp4_base);

	return 0;
}

static int dove_nand_ctrl_get(unsigned pid, unsigned long *config)
{
	unsigned int gmpp;

	gmpp = readl(gconf_base + MPP_GENERAL_CONFIG);
	*config = ((gmpp & NAND_GPIO_EN) != 0);

	return 0;
}

static int dove_nand_ctrl_set(unsigned pid, unsigned long config)
{
	unsigned int gmpp;

	gmpp = readl(gconf_base + MPP_GENERAL_CONFIG);
	gmpp &= ~NAND_GPIO_EN;
	if (config)
		gmpp |= NAND_GPIO_EN;
	writel(gmpp, gconf_base + MPP_GENERAL_CONFIG);

	return 0;
}

static int dove_audio0_ctrl_get(unsigned pid, unsigned long *config)
{
	unsigned long pmu = readl(mpp_base + PMU_MPP_GENERAL_CTRL);

	*config = ((pmu & AU0_AC97_SEL) != 0);

	return 0;
}

static int dove_audio0_ctrl_set(unsigned pid, unsigned long config)
{
	unsigned long pmu = readl(mpp_base + PMU_MPP_GENERAL_CTRL);

	pmu &= ~AU0_AC97_SEL;
	if (config)
		pmu |= AU0_AC97_SEL;
	writel(pmu, mpp_base + PMU_MPP_GENERAL_CTRL);

	return 0;
}

static int dove_audio1_ctrl_get(unsigned pid, unsigned long *config)
{
	unsigned int mpp4 = readl(mpp4_base);
	unsigned int sspc1;
	unsigned int gmpp;
	unsigned int gcfg2;

	sspc1 = readl(gconf_base + SSP_CTRL_STATUS_1);
	gmpp = readl(gconf_base + MPP_GENERAL_CONFIG);
	gcfg2 = readl(gconf_base + GLOBAL_CONFIG_2);

	*config = 0;
	if (mpp4 & AU1_GPIO_SEL)
		*config |= BIT(3);
	if (sspc1 & SSP_ON_AU1)
		*config |= BIT(2);
	if (gmpp & AU1_SPDIFO_GPIO_EN)
		*config |= BIT(1);
	if (gcfg2 & TWSI_OPTION3_GPIO)
		*config |= BIT(0);

	/* SSP/TWSI only if I2S1 not set*/
	if ((*config & BIT(3)) == 0)
		*config &= ~(BIT(2) | BIT(0));
	/* TWSI only if SPDIFO not set*/
	if ((*config & BIT(1)) == 0)
		*config &= ~BIT(0);

	return 0;
}

static int dove_audio1_ctrl_set(unsigned pid, unsigned long config)
{
	unsigned int reg = readl(mpp4_base);

	reg &= ~AU1_GPIO_SEL;
	if (config & BIT(3))
		reg |= AU1_GPIO_SEL;
	writel(reg, mpp4_base);

	reg = readl(gconf_base + SSP_CTRL_STATUS_1);
	reg &= ~SSP_ON_AU1;
	if (config & BIT(2))
		reg |= SSP_ON_AU1;

	reg = readl(gconf_base + MPP_GENERAL_CONFIG);
	reg &= ~AU1_SPDIFO_GPIO_EN;
	if (config & BIT(1))
		reg |= AU1_SPDIFO_GPIO_EN;

	reg = readl(gconf_base + GLOBAL_CONFIG_2);
	reg &= ~TWSI_OPTION3_GPIO;
	if (config & BIT(0))
		reg |= TWSI_OPTION3_GPIO;

	return 0;
}

static int dove_twsi_ctrl_get(unsigned pid, unsigned long *config)
{
	unsigned int gcfg1 = readl(gconf_base + GLOBAL_CONFIG_1);
	unsigned int gcfg2 = readl(gconf_base + GLOBAL_CONFIG_2);

	*config = 0;
	if (gcfg1 & TWSI_ENABLE_OPTION1)
		*config = 1;
	else if (gcfg2 & TWSI_ENABLE_OPTION2)
		*config = 2;
	else if (gcfg2 & TWSI_ENABLE_OPTION3)
		*config = 3;

	return 0;
}

static int dove_twsi_ctrl_set(unsigned pid, unsigned long config)
{
	unsigned int gcfg1 = readl(gconf_base + GLOBAL_CONFIG_1);
	unsigned int gcfg2 = readl(gconf_base + GLOBAL_CONFIG_2);

	gcfg1 &= ~TWSI_ENABLE_OPTION1;
	gcfg2 &= ~(TWSI_ENABLE_OPTION2 | TWSI_ENABLE_OPTION3);

	switch (config) {
	case 1:
		gcfg1 = TWSI_ENABLE_OPTION1;
		break;
	case 2:
		gcfg2 = TWSI_ENABLE_OPTION2;
		break;
	case 3:
		gcfg2 = TWSI_ENABLE_OPTION3;
		break;
	}

	writel(gcfg1, gconf_base + GLOBAL_CONFIG_1);
	writel(gcfg2, gconf_base + GLOBAL_CONFIG_2);

	return 0;
}

static struct mvebu_mpp_mode dove_mpp_modes[] = {
	MPP_MODE(0, "mpp0", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart2", "rts"),
		MPP_FUNCTION(0x03, "sdio0", "cd"),
		MPP_FUNCTION(0x0f, "lcd0", "pwm"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(1, "mpp1", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart2", "cts"),
		MPP_FUNCTION(0x03, "sdio0", "wp"),
		MPP_FUNCTION(0x0f, "lcd1", "pwm"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(2, "mpp2", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "sata", "prsnt"),
		MPP_FUNCTION(0x02, "uart2", "txd"),
		MPP_FUNCTION(0x03, "sdio0", "buspwr"),
		MPP_FUNCTION(0x04, "uart1", "rts"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(3, "mpp3", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "sata", "act"),
		MPP_FUNCTION(0x02, "uart2", "rxd"),
		MPP_FUNCTION(0x03, "sdio0", "ledctrl"),
		MPP_FUNCTION(0x04, "uart1", "cts"),
		MPP_FUNCTION(0x0f, "lcd-spi", "cs1"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(4, "mpp4", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart3", "rts"),
		MPP_FUNCTION(0x03, "sdio1", "cd"),
		MPP_FUNCTION(0x04, "spi1", "miso"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(5, "mpp5", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart3", "cts"),
		MPP_FUNCTION(0x03, "sdio1", "wp"),
		MPP_FUNCTION(0x04, "spi1", "cs"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(6, "mpp6", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart3", "txd"),
		MPP_FUNCTION(0x03, "sdio1", "buspwr"),
		MPP_FUNCTION(0x04, "spi1", "mosi"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(7, "mpp7", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart3", "rxd"),
		MPP_FUNCTION(0x03, "sdio1", "ledctrl"),
		MPP_FUNCTION(0x04, "spi1", "sck"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "core-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(8, "mpp8", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "watchdog", "rstout"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(9, "mpp9", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x05, "pex1", "clkreq"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(10, "mpp10", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x05, "ssp", "sclk"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(11, "mpp11", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "sata", "prsnt"),
		MPP_FUNCTION(0x02, "sata-1", "act"),
		MPP_FUNCTION(0x03, "sdio0", "ledctrl"),
		MPP_FUNCTION(0x04, "sdio1", "ledctrl"),
		MPP_FUNCTION(0x05, "pex0", "clkreq"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(12, "mpp12", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "sata", "act"),
		MPP_FUNCTION(0x02, "uart2", "rts"),
		MPP_FUNCTION(0x03, "audio0", "extclk"),
		MPP_FUNCTION(0x04, "sdio1", "cd"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(13, "mpp13", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart2", "cts"),
		MPP_FUNCTION(0x03, "audio1", "extclk"),
		MPP_FUNCTION(0x04, "sdio1", "wp"),
		MPP_FUNCTION(0x05, "ssp", "extclk"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(14, "mpp14", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart2", "txd"),
		MPP_FUNCTION(0x04, "sdio1", "buspwr"),
		MPP_FUNCTION(0x05, "ssp", "rxd"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(15, "mpp15", dove_pmu_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart2", "rxd"),
		MPP_FUNCTION(0x04, "sdio1", "ledctrl"),
		MPP_FUNCTION(0x05, "ssp", "sfrm"),
		MPP_FUNCTION(CONFIG_PMU | 0x0, "pmu-nc", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x1, "pmu-low", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x2, "pmu-high", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x3, "pmic", "sdi"),
		MPP_FUNCTION(CONFIG_PMU | 0x4, "cpu-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x5, "standby-pwr-down", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0x8, "cpu-pwr-good", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xa, "bat-fault", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xb, "ext0-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xc, "ext1-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xd, "ext2-wakeup", NULL),
		MPP_FUNCTION(CONFIG_PMU | 0xe, "pmu-blink", NULL)),
	MPP_MODE(16, "mpp16", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart3", "rts"),
		MPP_FUNCTION(0x03, "sdio0", "cd"),
		MPP_FUNCTION(0x04, "lcd-spi", "cs1"),
		MPP_FUNCTION(0x05, "ac97", "sdi1")),
	MPP_MODE(17, "mpp17", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "ac97-1", "sysclko"),
		MPP_FUNCTION(0x02, "uart3", "cts"),
		MPP_FUNCTION(0x03, "sdio0", "wp"),
		MPP_FUNCTION(0x04, "twsi", "sda"),
		MPP_FUNCTION(0x05, "ac97", "sdi2")),
	MPP_MODE(18, "mpp18", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart3", "txd"),
		MPP_FUNCTION(0x03, "sdio0", "buspwr"),
		MPP_FUNCTION(0x04, "lcd0", "pwm"),
		MPP_FUNCTION(0x05, "ac97", "sdi3")),
	MPP_MODE(19, "mpp19", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "uart3", "rxd"),
		MPP_FUNCTION(0x03, "sdio0", "ledctrl"),
		MPP_FUNCTION(0x04, "twsi", "sck")),
	MPP_MODE(20, "mpp20", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "ac97", "sysclko"),
		MPP_FUNCTION(0x02, "lcd-spi", "miso"),
		MPP_FUNCTION(0x03, "sdio1", "cd"),
		MPP_FUNCTION(0x05, "sdio0", "cd"),
		MPP_FUNCTION(0x06, "spi1", "miso")),
	MPP_MODE(21, "mpp21", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "uart1", "rts"),
		MPP_FUNCTION(0x02, "lcd-spi", "cs0"),
		MPP_FUNCTION(0x03, "sdio1", "wp"),
		MPP_FUNCTION(0x04, "ssp", "sfrm"),
		MPP_FUNCTION(0x05, "sdio0", "wp"),
		MPP_FUNCTION(0x06, "spi1", "cs")),
	MPP_MODE(22, "mpp22", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x01, "uart1", "cts"),
		MPP_FUNCTION(0x02, "lcd-spi", "mosi"),
		MPP_FUNCTION(0x03, "sdio1", "buspwr"),
		MPP_FUNCTION(0x04, "ssp", "txd"),
		MPP_FUNCTION(0x05, "sdio0", "buspwr"),
		MPP_FUNCTION(0x06, "spi1", "mosi")),
	MPP_MODE(23, "mpp23", dove_mpp_ctrl,
		MPP_FUNCTION(0x00, "gpio", NULL),
		MPP_FUNCTION(0x02, "lcd-spi", "sck"),
		MPP_FUNCTION(0x03, "sdio1", "ledctrl"),
		MPP_FUNCTION(0x04, "ssp", "sclk"),
		MPP_FUNCTION(0x05, "sdio0", "ledctrl"),
		MPP_FUNCTION(0x06, "spi1", "sck")),
	MPP_MODE(24, "mpp_camera", dove_mpp4_ctrl,
		MPP_FUNCTION(0x00, "camera", NULL),
		MPP_FUNCTION(0x01, "gpio", NULL)),
	MPP_MODE(40, "mpp_sdio0", dove_mpp4_ctrl,
		MPP_FUNCTION(0x00, "sdio0", NULL),
		MPP_FUNCTION(0x01, "gpio", NULL)),
	MPP_MODE(46, "mpp_sdio1", dove_mpp4_ctrl,
		MPP_FUNCTION(0x00, "sdio1", NULL),
		MPP_FUNCTION(0x01, "gpio", NULL)),
	MPP_MODE(52, "mpp_audio1", dove_audio1_ctrl,
		MPP_FUNCTION(0x00, "i2s1/spdifo", NULL),
		MPP_FUNCTION(0x02, "i2s1", NULL),
		MPP_FUNCTION(0x08, "spdifo", NULL),
		MPP_FUNCTION(0x0a, "gpio", NULL),
		MPP_FUNCTION(0x0b, "twsi", NULL),
		MPP_FUNCTION(0x0c, "ssp/spdifo", NULL),
		MPP_FUNCTION(0x0e, "ssp", NULL),
		MPP_FUNCTION(0x0f, "ssp/twsi", NULL)),
	MPP_MODE(58, "mpp_spi0", dove_mpp4_ctrl,
		MPP_FUNCTION(0x00, "spi0", NULL),
		MPP_FUNCTION(0x01, "gpio", NULL)),
	MPP_MODE(62, "mpp_uart1", dove_mpp4_ctrl,
		MPP_FUNCTION(0x00, "uart1", NULL),
		MPP_FUNCTION(0x01, "gpio", NULL)),
	MPP_MODE(64, "mpp_nand", dove_nand_ctrl,
		MPP_FUNCTION(0x00, "nand", NULL),
		MPP_FUNCTION(0x01, "gpo", NULL)),
	MPP_MODE(72, "audio0", dove_audio0_ctrl,
		MPP_FUNCTION(0x00, "i2s", NULL),
		MPP_FUNCTION(0x01, "ac97", NULL)),
	MPP_MODE(73, "twsi", dove_twsi_ctrl,
		MPP_FUNCTION(0x00, "twsi-none", NULL),
		MPP_FUNCTION(0x01, "twsi-opt1", NULL),
		MPP_FUNCTION(0x02, "twsi-opt2", NULL),
		MPP_FUNCTION(0x03, "twsi-opt3", NULL)),
};

static struct mvebu_pinctrl_soc_info dove_pinctrl_info = {
	.modes = dove_mpp_modes,
	.nmodes = ARRAY_SIZE(dove_mpp_modes),
	.variant = 0,
};

static struct of_device_id dove_pinctrl_of_match[] = {
	{
		.compatible = "marvell,dove-pinctrl",
		.data = &dove_pinctrl_info
	},
	{ }
};

static int dove_pinctrl_probe(struct device_d *dev)
{
	struct resource *iores;
	const struct of_device_id *match =
		of_match_node(dove_pinctrl_of_match, dev->device_node);
	struct mvebu_pinctrl_soc_info *soc =
		(struct mvebu_pinctrl_soc_info *)match->data;
	struct device_node *np;
	struct clk *clk;

	clk = clk_get(dev, NULL);
	clk_enable(clk);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	mpp_base = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	mpp4_base = IOMEM(iores->start);

	/*
	 * Dove PMU does not have a stable binding, yet.
	 * Derive pmu_base from mpp_base until proper binding is
	 * available.
	 */
	pmu_base = (void *)((u32)mpp_base & ~INT_REGS_MASK) + PMU_REGS_OFFS;

	np = of_find_compatible_node(NULL, NULL, "marvell,dove-global-config");
	if (!np)
		return -ENODEV;
	gconf_base = of_iomap(np, 0);

	return mvebu_pinctrl_probe(dev, soc);
}

static struct driver_d dove_pinctrl_driver = {
	.name		= "pinctrl-dove",
	.probe		= dove_pinctrl_probe,
	.of_compatible	= dove_pinctrl_of_match,
};

static int dove_pinctrl_init(void)
{
	return platform_driver_register(&dove_pinctrl_driver);
}
core_initcall(dove_pinctrl_init);
