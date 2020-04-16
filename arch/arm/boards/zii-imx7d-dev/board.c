// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2018 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <gpio.h>
#include <mach/imx7-regs.h>
#include <mfd/imx7-iomuxc-gpr.h>
#include <environment.h>
#include <envfs.h>
#include <mach/bbu.h>

static void zii_imx7d_rpu2_init_fec(void)
{
	void __iomem *gpr = IOMEM(MX7_IOMUXC_GPR_BASE_ADDR);
	uint32_t gpr1;

	/*
	 * Make sure we do not drive ENETn_TX_CLK signal
	 */
	gpr1 = readl(gpr + IOMUXC_GPR1);
	gpr1 &= ~(IMX7D_GPR1_ENET1_TX_CLK_SEL_MASK |
		  IMX7D_GPR1_ENET1_CLK_DIR_MASK |
		  IMX7D_GPR1_ENET2_TX_CLK_SEL_MASK |
		  IMX7D_GPR1_ENET2_CLK_DIR_MASK);
	writel(gpr1, gpr + IOMUXC_GPR1);
}

static int zii_imx7d_rpu2_coredevices_init(void)
{
	if (!of_machine_is_compatible("zii,imx7d-rpu2") &&
	    !of_machine_is_compatible("zii,imx7d-rmu2"))
		return 0;

	zii_imx7d_rpu2_init_fec();

	imx7_bbu_internal_spi_i2c_register_handler("SPI", "/dev/m25p0.barebox",
						   BBU_HANDLER_FLAG_DEFAULT);

	imx7_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", 0);

	return 0;
}
coredevice_initcall(zii_imx7d_rpu2_coredevices_init);

static int zii_imx7d_dev_init(void)
{
	if (of_machine_is_compatible("zii,imx7d-rpu2"))
		barebox_set_hostname("rpu2");
	else if (of_machine_is_compatible("zii,imx7d-rmu2"))
		barebox_set_hostname("rmu2");

	defaultenv_append_directory(defaultenv_zii_common);

	return 0;
}
late_initcall(zii_imx7d_dev_init);
